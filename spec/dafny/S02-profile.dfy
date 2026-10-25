include "S04-arithmetic.dfy"

// profile.c pure parsing/lookup. Strings here are decoded C byte strings:
// ASCII syntax is modeled; non-ASCII locale classification remains an adapter.
module S02Profile {
  predicate Space(c: char) { c in " \t\r\n" || c as int == 11 || c as int == 12 }
  function Left(s: string): string decreases |s| {
    if |s|>0 && Space(s[0]) then Left(s[1..]) else s
  }
  function Right(s: string): string decreases |s| {
    if |s|>0 && Space(s[|s|-1]) then Right(s[..|s|-1]) else s
  }
  function Trim(s: string): string { Right(Left(s)) }
  function Find(s: string,c: char): nat
    ensures Find(s,c)<=|s|
    decreases |s|
  { if |s|==0 || s[0]==c then 0 else 1+Find(s[1..],c) }
  lemma FindWhole(s: string,c: char)
    requires c !in s
    ensures Find(s,c)==|s|
    decreases |s|
  { if |s|>0 { FindWhole(s[1..],c); } }
  lemma FindJoined(s: string,c: char,tail: string)
    requires c !in s
    ensures Find(s+[c]+tail,c)==|s|
    decreases |s|
  {
    if |s|>0 {
      FindJoined(s[1..],c,tail);
      assert (s+[c]+tail)[1..]==s[1..]+[c]+tail;
      assert (s+[c]+tail)[0]==s[0];
    }
  }
  function Fold(s: string): string
    ensures |Fold(s)|==|s|
    decreases |s| {
    if |s|==0 then "" else
    [if 'A'<=s[0]<='Z' then ((s[0] as int)+32) as char else s[0]]+Fold(s[1..])
  }
  datatype Line = Other(raw: string) | Section(raw: string,name: string) |
    Key(raw: string,name: string,value: string)
  function Parse(raw: string): Line {
    var s := Trim(raw);
    if |s|==0 then Other(raw)
    else if s[0]=='[' && Find(s[1..],']')<|s|-1 then
      Section(raw,Trim(s[1..1+Find(s[1..],']')]))
    else if s[0]!=';' && Find(s,'=')<|s| then
      Key(raw,Trim(s[..Find(s,'=')]),Trim(s[Find(s,'=')+1..]))
    else Other(raw)
  }
  function Unquote(s: string): string {
    if |s|>=2 && s[0] in "\"'" && s[|s|-1]==s[0] then s[1..|s|-1] else s
  }
  datatype Value = Missing | Found(text: string)
  function KeyInBlock(lines: seq<Line>,key: string): Value decreases |lines| {
    if |lines|==0 || lines[0].Section? then Missing
    else if lines[0].Key? && Fold(lines[0].name)==Fold(key) then Found(Unquote(lines[0].value))
    else KeyInBlock(lines[1..],key)
  }
  function Lookup(lines: seq<Line>,section: string,key: string): Value decreases |lines| {
    if |lines|==0 then Missing
    else if lines[0].Section? && Fold(lines[0].name)==Fold(section) then KeyInBlock(lines[1..],key)
    else Lookup(lines[1..],section,key)
  }
  function DefaultText(s: string): string decreases |s| {
    if |s|>0 && s[|s|-1]==' ' then DefaultText(s[..|s|-1]) else s
  }
  function ReadValue(lines: seq<Line>,section: string,key: string,fallback: string): string {
    var v := Lookup(lines,section,key);
    if v.Found? then v.text else DefaultText(fallback)
  }
  function Digit(c: char): int {
    if '0'<=c<='9' then (c as int)-48 else if 'a'<=c<='f' then (c as int)-87
    else if 'A'<=c<='F' then (c as int)-55 else -1
  }
  function Digits(s: string,radix: nat,value: nat): nat
    requires radix==10 || radix==16
    ensures Digits(s,radix,value)<4294967296
    decreases |s|
  {
    if |s|==0 || Digit(s[0])<0 || Digit(s[0])>=radix then value%4294967296
    else Digits(s[1..],radix,(value*radix+Digit(s[0]))%4294967296)
  }
  // Input is the returned, UTF-8 bounded string (at most 127 bytes), not a raw line.
  function Integer(s: string,fallback: int): nat
    ensures Integer(s,fallback)<4294967296
  {
    if |s|==0 then fallback%4294967296 else
    var trimmed := Left(s);
    var negative := |trimmed|>0 && trimmed[0]=='-';
    var rest := if |trimmed|>0 && trimmed[0] in "+-" then trimmed[1..] else trimmed;
    var hex := |rest|>=2 && rest[0]=='0' && rest[1] in "xX";
    var u := Digits(if hex then rest[2..] else rest,if hex then 16 else 10,0);
    (if negative then -(u as int) else u as int)%4294967296
  }
  // Ordered list edits preserve unrelated raw lines and affect the first match.
  function EndBlock(lines: seq<Line>): nat
    ensures EndBlock(lines)<=|lines|
    decreases |lines|
  { if |lines|==0 || lines[0].Section? then 0 else 1+EndBlock(lines[1..]) }
  function ChangeKey(lines: seq<Line>,key: string,value: Value): seq<Line> decreases |lines| {
    var row := if value.Found? then [Parse(key+"="+value.text+"\n")] else [];
    if |lines|==0 || lines[0].Section? then row+lines
    else if lines[0].Key? && Fold(lines[0].name)==Fold(key) then row+lines[1..]
    else [lines[0]]+ChangeKey(lines[1..],key,value)
  }
  function Change(lines: seq<Line>,section: string,key: string,value: Value,deleteSection: bool): seq<Line>
    decreases |lines|
  {
    if |lines|==0 then
      (if deleteSection || value.Missing? then [] else
        [Parse("["+section+"]\n"),Parse(key+"="+value.text+"\n")])
    else if lines[0].Section? && Fold(lines[0].name)==Fold(section) then
      (if deleteSection then lines[1+EndBlock(lines[1..])..]
       else [lines[0]]+ChangeKey(lines[1..],key,value))
    else [lines[0]]+Change(lines[1..],section,key,value,deleteSection)
  }
  lemma {:isolate_assertions} {:fuel Left,20} {:fuel Right,20} {:fuel Find,20} ParsingVectors() {
    assert Parse(" [ A ] extra\n")==Section(" [ A ] extra\n","A");
    assert Parse(" ;key=ignored\n").Other?;
    assert Parse("#key=value")==Key("#key=value","#key","value");
    assert Parse("[bad=x")==Key("[bad=x","[bad","x");
    assert Unquote("' a '")==" a ";

  }
  lemma {:isolate_assertions} IntegerVectors() {
    assert Left("nonnumeric")=="nonnumeric";
    assert Digit('n')==-1;
    assert Digits("nonnumeric",10,0)==0;
    assert Integer("",7)==7;
    assert Integer("nonnumeric",7)==0;
    assert Left(" ")=="";
    assert Integer(" ",7)==0;
    assert Left("-0x10tail")=="-0x10tail";
    assert Digit('t')==-1;
    assert Digits("tail",16,16)==16;
    assert Digits("0tail",16,1)==16;
    assert Digits("10tail",16,0)==16;
    assert Integer("-0x10tail",7)==4294967280;
    assert Digits("7",10,7)==77;
    assert Digits("77",10,0)==77;
    assert Digits("077",10,0)==77;
    assert Integer("077",0)==77;
    assert Integer("0x",7)==0;
    assert Digits("6",10,429496729)==0;
  }

  lemma {:isolate_assertions} LookupVectors() {
    assert Fold("A")=="a" && Fold("K")=="k";
    assert Fold("a")=="a" && Fold("k")=="k";
    assert Unquote("'1'")=="1";
    assert KeyInBlock([Key("","k","'1'"),Key("","k","2")],"K")==Found("1");
    assert Lookup([Section("","A"),Section("","A"),Key("","k","2")],"A","k")==Missing;
    assert Lookup([Section("","A"),Key("","k","'1'"),Key("","k","2")],"a","K")==Found("1");
    assert ChangeKey([Key("","k","1"),Key("","k","2")],"k",Missing)==[Key("","k","2")];
    assert Change([Section("","A"),Key("","k","1"),Section("","B")],"A","",Missing,true)==[Section("","B")];
  }

}
