include "S02-profile.dfy"
include "S04-utf8-prefix.dfy"
module S02HostProfile {
  import S = S01
  import P = S02Profile
  import T = S04Utf8Prefix
  import C = S01Codecs
  function Right(s: string): string
    ensures T.ByteString(s) ==> T.ByteString(Right(s))
    decreases |s|
  { if |s|>0 && s[|s|-1] in "\r \t" then Right(s[..|s|-1]) else s }
  function KeyRight(s: string): string
    ensures T.ByteString(s) ==> T.ByteString(KeyRight(s))
    decreases |s|
  { if |s|>0 && s[|s|-1] in " \t" then KeyRight(s[..|s|-1]) else s }
  function ValueLeft(s: string): string
    ensures T.ByteString(s) ==> T.ByteString(ValueLeft(s))
    decreases |s|
  { if |s|>0 && s[0] in " \t" then ValueLeft(s[1..]) else s }
  datatype LineResult = Next(inSection: bool) | Match(value: string)
  opaque function ReadLine(line: string,section: string,key: string,capacity: nat,inSection: bool): LineResult
    requires T.ByteString(line) && capacity>0
  {
    if |line|>0 && line[0]=='[' then
      var close := P.Find(line,']');
      Next(if close<|line| then P.Fold(line[1..close])==P.Fold(section) else inSection)
    else
      var equal := P.Find(line,'=');
      if inSection && equal<|line| && P.Fold(KeyRight(line[..equal]))==P.Fold(key)
      then Match(T.Bound(ValueLeft(line[equal+1..]),capacity-1)) else Next(inSection)
  }
  opaque function Lookup(text: string,section: string,key: string,capacity: nat,inSection: bool): P.Value
    requires T.ByteString(text) && capacity>0
    decreases |text|
  {
    if |text|==0 then P.Missing else
    var end := P.Find(text,'\n');
    var next := if end<|text| then end+1 else end;
    var line := Right(T.Bound(text[..end],255));
    var result := ReadLine(line,section,key,capacity,inSection);
    if result.Match? then P.Found(result.value) else if next==|text| then P.Missing
    else Lookup(text[next..],section,key,capacity,result.inSection)
  }
  lemma {:fuel Lookup,4} LastLine(line: string,section: string,key: string,capacity: nat,active: bool)
    requires T.ByteString(line) && capacity>0 && '\n' !in line
    ensures Lookup(line,section,key,capacity,active)==
      (if |line|==0 then P.Missing else
       var r := ReadLine(Right(T.Bound(line,255)),section,key,capacity,active);
       if r.Match? then P.Found(r.value) else P.Missing)
  {
    reveal Lookup();
    reveal ReadLine();
    reveal T.Bound();
    P.FindWhole(line,'\n');
    assert P.Find(line,'\n')==|line|;
    assert Lookup("",section,key,capacity,true)==P.Missing;
    assert Lookup("",section,key,capacity,false)==P.Missing;
    if |line|>0 {
      var r := ReadLine(Right(T.Bound(line,255)),section,key,capacity,active);
      var end := P.Find(line,'\n');
      assert line[..end]==line;
      var parsed := Right(T.Bound(line[..end],255));
      var step := ReadLine(parsed,section,key,capacity,active);
      var actual := Lookup(line,section,key,capacity,active);
      assert parsed==Right(T.Bound(line,255));
      assert step==r;
      assert actual==(if r.Match? then P.Found(r.value) else P.Missing);
    }
  }
  lemma HeadLine(line: string,tail: string,section: string,key: string,capacity: nat,active: bool)
    requires T.ByteString(line) && T.ByteString(tail) && capacity>0 && '\n' !in line
    ensures Lookup(line+"\n"+tail,section,key,capacity,active)==
      (var r := ReadLine(Right(T.Bound(line,255)),section,key,capacity,active);
       if r.Match? then P.Found(r.value) else Lookup(tail,section,key,capacity,r.inSection))
  {
    reveal Lookup();
    P.FindJoined(line,'\n',tail);
    var text := line+"\n"+tail;
    var end := P.Find(text,'\n');
    assert end==|line|;
    assert text[..end]==line;
    assert text[end+1..]==tail;
    assert Lookup("",section,key,capacity,true)==P.Missing;
    assert Lookup("",section,key,capacity,false)==P.Missing;
  }
  function DecimalDigits(s: string,value: nat): nat decreases |s| {
    if |s|==0 || !('0'<=s[0]<='9') then value
    else DecimalDigits(s[1..],value*10+(s[0] as int)-48)
  }
  function Decimal(s: string): int {
    var trimmed := P.Left(s);
    var negative := |trimmed|>0 && trimmed[0]=='-';
    var rest := if |trimmed|>0 && trimmed[0] in "+-" then trimmed[1..] else trimmed;
    var value := DecimalDigits(rest,0);
    if negative then -(value as int) else value as int
  }
  datatype IntegerResult = OutsideLongDomain | Number(value: int)
  function Integer(value: P.Value,fallback: int,long64: bool): IntegerResult {
    if value.Missing? then Number(fallback) else
    var n := Decimal(value.text);
    var bound := if long64 then 9223372036854775808 else 2147483648;
    if -bound<=n<bound then Number(C.Signed(n,32)) else OutsideLongDomain
  }
  // FReadHostIni strips exactly one leading BOM after whole-file validation.
  function RemoveBom(bytes: S.Bytes): S.Bytes {
    if |bytes|>=3 && bytes[..3]==[239,187,191] then bytes[3..] else bytes
  }
  lemma {:isolate_assertions} {:fuel ReadLine,4} HostDuplicateVector()
    ensures Lookup("[A]\nx=1\n[A]\nk=2","A","k",32,false)==P.Found("2")
  {
    reveal ReadLine();
    P.FindJoined("[A",']',"");
    T.AsciiBound("[A]",255); T.AsciiBound("x=1",255);
    T.AsciiBound("k=2",255); T.AsciiBound("2",31);
    P.FindJoined("k",'=',"2"); P.FindJoined("x",'=',"1");
    assert P.Fold("k")=="k" && P.Fold("x")=="x";
    assert ReadLine("k=2","A","k",32,true)==Match("2");
    assert ReadLine("x=1","A","k",32,true)==Next(true);
    assert ReadLine("[A]","A","k",32,false)==Next(true);
    assert ReadLine("[A]","A","k",32,true)==Next(true);
    LastLine("k=2","A","k",32,true);
    LastLine("k=2","A","k",32,false);
    LastLine("k=2","A","k",3,true);
    LastLine("k=2","A","k",3,false);
    assert Lookup("k=2","A","k",32,true)==P.Found("2");
    assert "[A]"+"\n"+"k=2"=="[A]\nk=2";
    HeadLine("[A]","k=2","A","k",32,true);
    assert "[A]"+"\n"+"k=2"=="[A]\nk=2";
    HeadLine("[A]","k=2","A","k",32,false);
    assert "[A]"+"\n"+"k=2"=="[A]\nk=2";
    HeadLine("[A]","k=2","A","k",3,true);
    assert "[A]"+"\n"+"k=2"=="[A]\nk=2";
    HeadLine("[A]","k=2","A","k",3,false);
    assert Lookup("[A]\nk=2","A","k",32,true)==P.Found("2");
    assert "x=1"+"\n"+"[A]\nk=2"=="x=1\n[A]\nk=2";
    HeadLine("x=1","[A]\nk=2","A","k",32,true);
    assert "x=1"+"\n"+"[A]\nk=2"=="x=1\n[A]\nk=2";
    HeadLine("x=1","[A]\nk=2","A","k",32,false);
    assert "x=1"+"\n"+"[A]\nk=2"=="x=1\n[A]\nk=2";
    HeadLine("x=1","[A]\nk=2","A","k",3,true);
    assert "x=1"+"\n"+"[A]\nk=2"=="x=1\n[A]\nk=2";
    HeadLine("x=1","[A]\nk=2","A","k",3,false);
    assert Lookup("x=1\n[A]\nk=2","A","k",32,true)==P.Found("2");
    assert "[A]"+"\n"+"x=1\n[A]\nk=2"=="[A]\nx=1\n[A]\nk=2";
    HeadLine("[A]","x=1\n[A]\nk=2","A","k",32,true);
    assert "[A]"+"\n"+"x=1\n[A]\nk=2"=="[A]\nx=1\n[A]\nk=2";
    HeadLine("[A]","x=1\n[A]\nk=2","A","k",32,false);
    assert "[A]"+"\n"+"x=1\n[A]\nk=2"=="[A]\nx=1\n[A]\nk=2";
    HeadLine("[A]","x=1\n[A]\nk=2","A","k",3,true);
    assert "[A]"+"\n"+"x=1\n[A]\nk=2"=="[A]\nx=1\n[A]\nk=2";
    HeadLine("[A]","x=1\n[A]\nk=2","A","k",3,false);
  }
  lemma {:isolate_assertions} {:fuel ReadLine,4} HostSpaceVector()
    ensures Lookup(" [A]\nk=1","A","k",32,false)==P.Missing
    ensures Lookup("[ A ]\nk=1","A","k",32,false)==P.Missing
    ensures Lookup("[A]\n k=1","A","k",32,false)==P.Missing
  {
    reveal ReadLine();
    P.FindJoined("[ A ",']',"");
    T.AsciiBound(" [A]",255); T.AsciiBound("[ A ]",255);
    P.FindJoined("[A",']',"");
    T.AsciiBound("[A]",255); T.AsciiBound("k=1",255); T.AsciiBound(" k=1",255);
    P.FindWhole(" [A]",'=');
    P.FindJoined(" k",'=',"1");
    assert KeyRight(" k")==" k";
    assert ReadLine(" [A]","A","k",32,false)==Next(false);
    assert ReadLine("[ A ]","A","k",32,false)==Next(false);
    assert ReadLine(" k=1","A","k",32,true)==Next(true);
    assert ReadLine("[A]","A","k",32,false)==Next(true);
    LastLine("k=1","A","k",32,true);
    LastLine("k=1","A","k",32,false);
    LastLine("k=1","A","k",3,true);
    LastLine("k=1","A","k",3,false);
    assert Lookup("k=1","A","k",32,false)==P.Missing;
    LastLine(" k=1","A","k",32,true);
    LastLine(" k=1","A","k",32,false);
    LastLine(" k=1","A","k",3,true);
    LastLine(" k=1","A","k",3,false);
    assert Lookup(" k=1","A","k",32,true)==P.Missing;
    assert " [A]"+"\n"+"k=1"==" [A]\nk=1";
    HeadLine(" [A]","k=1","A","k",32,true);
    assert " [A]"+"\n"+"k=1"==" [A]\nk=1";
    HeadLine(" [A]","k=1","A","k",32,false);
    assert " [A]"+"\n"+"k=1"==" [A]\nk=1";
    HeadLine(" [A]","k=1","A","k",3,true);
    assert " [A]"+"\n"+"k=1"==" [A]\nk=1";
    HeadLine(" [A]","k=1","A","k",3,false);
    assert "[ A ]"+"\n"+"k=1"=="[ A ]\nk=1";
    HeadLine("[ A ]","k=1","A","k",32,true);
    assert "[ A ]"+"\n"+"k=1"=="[ A ]\nk=1";
    HeadLine("[ A ]","k=1","A","k",32,false);
    assert "[ A ]"+"\n"+"k=1"=="[ A ]\nk=1";
    HeadLine("[ A ]","k=1","A","k",3,true);
    assert "[ A ]"+"\n"+"k=1"=="[ A ]\nk=1";
    HeadLine("[ A ]","k=1","A","k",3,false);
    assert "[A]"+"\n"+" k=1"=="[A]\n k=1";
    HeadLine("[A]"," k=1","A","k",32,true);
    assert "[A]"+"\n"+" k=1"=="[A]\n k=1";
    HeadLine("[A]"," k=1","A","k",32,false);
    assert "[A]"+"\n"+" k=1"=="[A]\n k=1";
    HeadLine("[A]"," k=1","A","k",3,true);
    assert "[A]"+"\n"+" k=1"=="[A]\n k=1";
    HeadLine("[A]"," k=1","A","k",3,false);
  }
  lemma {:isolate_assertions} {:fuel ReadLine,4} HostBrokenVector()
    ensures Lookup("[A]\n[broken\nk=1","A","k",32,false)==P.Found("1")
  {
    reveal ReadLine();
    P.FindJoined("[A",']',"");
    T.AsciiBound("[A]",255); T.AsciiBound("[broken",255);
    P.FindWhole("[broken",']');
    assert ReadLine("[broken","A","k",32,true)==Next(true);
    assert ReadLine("[A]","A","k",32,false)==Next(true);
    T.AsciiBound("k=1",255); T.AsciiBound("1",31);
    P.FindJoined("k",'=',"1");
    assert KeyRight("k")=="k" && ValueLeft("1")=="1";
    assert P.Fold("k")=="k";
    assert ReadLine("k=1","A","k",32,true)==Match("1");
    assert Right("k=1")=="k=1";
    LastLine("k=1","A","k",32,true);
    LastLine("k=1","A","k",32,false);
    LastLine("k=1","A","k",3,true);
    LastLine("k=1","A","k",3,false);
    assert T.Bound("k=1",255)=="k=1";
    assert Right(T.Bound("k=1",255))=="k=1";
    assert ReadLine(Right(T.Bound("k=1",255)),"A","k",32,true)==Match("1");
    assert Lookup("k=1","A","k",32,true)==P.Found("1");
    assert "[broken"+"\n"+"k=1"=="[broken\nk=1";
    HeadLine("[broken","k=1","A","k",32,true);
    assert "[broken"+"\n"+"k=1"=="[broken\nk=1";
    HeadLine("[broken","k=1","A","k",32,false);
    assert "[broken"+"\n"+"k=1"=="[broken\nk=1";
    HeadLine("[broken","k=1","A","k",3,true);
    assert "[broken"+"\n"+"k=1"=="[broken\nk=1";
    HeadLine("[broken","k=1","A","k",3,false);
    assert Lookup("[broken\nk=1","A","k",32,true)==P.Found("1");
    assert "[A]"+"\n"+"[broken\nk=1"=="[A]\n[broken\nk=1";
    HeadLine("[A]","[broken\nk=1","A","k",32,true);
    assert "[A]"+"\n"+"[broken\nk=1"=="[A]\n[broken\nk=1";
    HeadLine("[A]","[broken\nk=1","A","k",32,false);
    assert "[A]"+"\n"+"[broken\nk=1"=="[A]\n[broken\nk=1";
    HeadLine("[A]","[broken\nk=1","A","k",3,true);
    assert "[A]"+"\n"+"[broken\nk=1"=="[A]\n[broken\nk=1";
    HeadLine("[A]","[broken\nk=1","A","k",3,false);
  }
  lemma {:isolate_assertions} {:fuel ReadLine,4} {:fuel Right,4} HostValueVector()
    ensures Lookup("[A]\nk= '2'  ","A","k",32,false)==P.Found("'2'")
    ensures Lookup("[A]\nk=abcd","A","k",3,false)==P.Found("ab")
  {
    reveal ReadLine();
    P.FindJoined("[A",']',"");
    T.AsciiBound("[A]",255); T.AsciiBound("k= '2'  ",255);
    T.AsciiBound("'2'",31); T.AsciiBound("k=abcd",255); T.AsciiBound("abcd",2);
    P.FindJoined("k",'='," '2'"); P.FindJoined("k",'=',"abcd");
    assert ReadLine("k= '2'","A","k",32,true)==Match("'2'");
    assert ReadLine("k=abcd","A","k",3,true)==Match("ab");
    assert ReadLine("[A]","A","k",32,false)==Next(true);
    assert ReadLine("[A]","A","k",3,false)==Next(true);
    LastLine("k= '2'  ","A","k",32,true);
    LastLine("k= '2'  ","A","k",32,false);
    LastLine("k= '2'  ","A","k",3,true);
    LastLine("k= '2'  ","A","k",3,false);
    assert Right("k= '2'  ")=="k= '2'";
    assert Lookup("k= '2'  ","A","k",32,true)==P.Found("'2'");
    LastLine("k=abcd","A","k",32,true);
    LastLine("k=abcd","A","k",32,false);
    LastLine("k=abcd","A","k",3,true);
    LastLine("k=abcd","A","k",3,false);
    assert Lookup("k=abcd","A","k",3,true)==P.Found("ab");
    assert "[A]"+"\n"+"k= '2'  "=="[A]\nk= '2'  ";
    HeadLine("[A]","k= '2'  ","A","k",32,true);
    assert "[A]"+"\n"+"k= '2'  "=="[A]\nk= '2'  ";
    HeadLine("[A]","k= '2'  ","A","k",32,false);
    assert "[A]"+"\n"+"k= '2'  "=="[A]\nk= '2'  ";
    HeadLine("[A]","k= '2'  ","A","k",3,true);
    assert "[A]"+"\n"+"k= '2'  "=="[A]\nk= '2'  ";
    HeadLine("[A]","k= '2'  ","A","k",3,false);
    assert "[A]"+"\n"+"k=abcd"=="[A]\nk=abcd";
    HeadLine("[A]","k=abcd","A","k",32,true);
    assert "[A]"+"\n"+"k=abcd"=="[A]\nk=abcd";
    HeadLine("[A]","k=abcd","A","k",32,false);
    assert "[A]"+"\n"+"k=abcd"=="[A]\nk=abcd";
    HeadLine("[A]","k=abcd","A","k",3,true);
    assert "[A]"+"\n"+"k=abcd"=="[A]\nk=abcd";
    HeadLine("[A]","k=abcd","A","k",3,false);
  }
  lemma {:isolate_assertions} {:fuel ReadLine,4} HostVectors() {
    HostDuplicateVector(); HostSpaceVector(); HostBrokenVector(); HostValueVector();
    assert Decimal("0x2")==0 && Decimal("'2'")==0;
    assert Decimal("")==0;
    assert Integer(P.Missing,7,false)==Number(7);
    assert RemoveBom([239,187,191,65])==[65];
  }
}
