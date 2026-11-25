include "S04-integer-format.dfy"

// util.c PszPlayerName; current generated lexicon is an explicit versioned input.
module S03PlayerLabels {
  import S = S01
  import C = S01Codecs
  import U = S04Utf8Prefix
  import F = S04IntegerFormat
  function B(s: string): S.Bytes
    requires U.ByteString(s)
    ensures |B(s)|==|s|
    ensures forall i | 0<=i<|s| :: B(s)[i]==s[i] as int
  { U.BytesOf(s) }
  lemma BConcat(a: string,b: string)
    requires U.ByteString(a) && U.ByteString(b)
    ensures B(a+b)==B(a)+B(b)
    ensures U.ByteString(a+b)
  {
    assert forall i | 0<=i<|a+b| :: B(a+b)[i]==(B(a)+B(b))[i];
  }
  lemma Excludes(s: string,b: S.U8)
    requires U.ByteString(s) && b as char !in s
    ensures b !in B(s)
  {
    if b in B(s) {
      var i :| 0<=i<|B(s)| && B(s)[i]==b;
      assert s[i] as int==b;
      assert s[i]==b as char;
    }
  }
  datatype Lookup = Absent | Text(bytes: S.Bytes)
  function Catalog(id: S.U16): Lookup {
    if id==270 then Text(B("Custom Race Wizard - Step %d of 6")) else
    if id==271 then Text(B("View Race - Page %d of 6")) else
    if id==1369 then Text(B("No Name")) else
    if id==1370 then Text(B(" have")) else
    if id==1371 then Text(B(" are")) else
    if id==1372 then Text(B(" has")) else
    if id==1373 then Text(B(" is")) else
    if id==1374 then Text(B("Player %d")) else
    if 1383<=id<=1389 then Text(Defaults()[id-1383]) else Absent
  }
  function Defaults(): seq<S.Bytes> {
    [B("Humanoid"),B("Rabbitoid"),B("Insectoid"),B("Nucleotid"),B("Silicanoid"),B("Antetheral"),B("Random")]
  }
  function Zero(field: S.Bytes): nat
    ensures Zero(field)<=|field|
    ensures Zero(field)<|field| ==> field[Zero(field)]==0
    ensures forall i | 0<=i<Zero(field) :: field[i]!=0
    decreases |field|
  { if |field|==0 || field[0]==0 then 0 else 1+Zero(field[1..]) }
  opaque function Trim(s: S.Bytes): S.Bytes
    ensures |Trim(s)|<=|s|
    ensures |Trim(s)|>0 ==> Trim(s)[|Trim(s)|-1]!=32
    ensures Trim(s)==s[..|Trim(s)|]
    ensures forall i | |Trim(s)|<=i<|s| :: s[i]==32
    ensures |s|==0 || s[|s|-1]!=32 ==> Trim(s)==s
    decreases |s|
  { if |s|==0 || s[|s|-1]!=32 then s else Trim(s[..|s|-1]) }
  datatype Names = Names(singular: S.Bytes,plural: S.Bytes)
  predicate Shape(row: Names) { |row.singular|==32 && |row.plural|==32 }
  datatype Status = Label(text: S.Bytes,lookups: seq<S.U16>)
    | UndefinedNative(reason: nat) | InvalidEvidence
  // Reason 1: reached unterminated field. 2: trim's before-buffer read.
  // Reason 3: automatic suffix's before-buffer read. 4: local capacity.
  function Grammar(named: bool,word: S.I16): S.Bytes {
    if word==1 then B(if named then " have" else " has") else
    if word==2 then B(if named then " are" else " is") else []
  }
  function GrammarCalls(named: bool,word: S.I16): seq<S.U16> {
    if word==1 then [if named then 1370 else 1372] else
    if word==2 then [if named then 1371 else 1373] else []
  }
  function Finish(text: S.Bytes,calls: seq<S.U16>): Status {
    if |text|>=50 then UndefinedNative(4) else Label(text,calls)
  }
  function Name(row: Names,index: S.I16,capital: S.I16,plural: S.I16,article: S.I16,word: S.I16): Status {
    if !Shape(row) then InvalidEvidence else
    if row.singular[0]==0 then
      Finish(B("Player ")+F.Decimal(index+1)+(if plural==0 then B("'s") else [])+Grammar(false,word),
        [1374]+GrammarCalls(false,word)) else
    var field:=if plural!=0 && row.plural[0]!=0 then row.plural else row.singular;
    var end:=Zero(field);
    if end==32 then UndefinedNative(1) else
    var prefix:=if article==0 then [] else B(if capital==0 then "the " else "The ");
    var joined:=prefix+field[..end];
    if |joined|>=50 then UndefinedNative(4) else
    var trimmed:=Trim(joined);
    if |trimmed|==0 then UndefinedNative(2) else
    var auto:=plural!=0 && row.plural[0]==0;
    var last:=trimmed[|trimmed|-1];
    if auto && last==101 && |trimmed|==1 then UndefinedNative(3) else
    var suffix:=auto && last!=115 && (last!=101 || trimmed[|trimmed|-2]!=115);
    var inflected:=trimmed+(if suffix then [115] else []);
    Finish(inflected+Grammar(true,word),GrammarCalls(true,word))
  }
  function Select(rows: seq<Names>,explicit: bool,row: Names,index: S.I16,
    capital: S.I16,plural: S.I16,article: S.I16,word: S.I16): Status {
    if explicit then Name(row,index,capital,plural,article,word) else
    if |rows|!=16 then InvalidEvidence else
    if index<0 || index>=16 then UndefinedNative(5) else Name(rows[index],index,capital,plural,article,word)
  }
  function Work(work: S.Bytes,text: S.Bytes): S.Bytes
    requires |work|==360 && |text|<50 && 0 !in text
    ensures |Work(work,text)|==360
    ensures Work(work,text)[..|text|+1]==text+[0]
    ensures Work(work,text)[|text|+1..]==work[|text|+1..]
  { text+[0]+work[|text|+1..] }
  function LastScratch(before: S.Bytes,text: S.Bytes): S.Bytes
    requires |before|==256 && |text|<256 && 0 !in text
    ensures |LastScratch(before,text)|==256
    ensures LastScratch(before,text)[..|text|+1]==text+[0]
    ensures LastScratch(before,text)[|text|+1..]==before[|text|+1..]
  { text+[0]+before[|text|+1..] }
  function Field(text: S.Bytes): S.Bytes
    requires |text|<32 && 0 !in text
    ensures |Field(text)|==32
  { text+C.Zeros(32-|text|) }
  function Row(name: string,plural: string): Names
    requires U.ByteString(name) && U.ByteString(plural) && |name|<32 && |plural|<32
    requires 0 !in B(name) && 0 !in B(plural)
  { Names(Field(B(name)),Field(B(plural))) }
  lemma ZeroPrefix(prefix: S.Bytes,tail: S.Bytes)
    requires 0 !in prefix
    ensures Zero(prefix+tail)==|prefix|+Zero(tail)
    decreases |prefix|
  {
    if |prefix|>0 {
      ZeroPrefix(prefix[1..],tail);
      assert (prefix+tail)[0]==prefix[0] && prefix[0]!=0;
      assert (prefix+tail)[1..]==prefix[1..]+tail;
      calc {
        Zero(prefix+tail);
        == 1+Zero(prefix[1..]+tail);
        == 1+|prefix[1..]|+Zero(tail);
        == |prefix|+Zero(tail);
      }
    } else { assert prefix==[] && prefix+tail==tail; }
  }
  lemma FieldTerminator(text: S.Bytes)
    requires |text|<32 && 0 !in text
    ensures Zero(Field(text))==|text|
    ensures Field(text)[..Zero(Field(text))]==text
  { ZeroPrefix(text,C.Zeros(32-|text|)); }
  lemma TrimSpaces(text: S.Bytes,count: nat)
    requires |text|==0 || text[|text|-1]!=32
    ensures Trim(text+seq(count,i=>32 as S.U8))==text
    decreases count
  {
    reveal Trim();
    if count>0 {
      TrimSpaces(text,count-1);
      assert (text+seq(count,i=>32 as S.U8))[..|text|+count-1]==text+seq(count-1,i=>32 as S.U8);
    }
  }
  lemma TrimIdempotent(text: S.Bytes)
    ensures Trim(Trim(text))==Trim(text)
    decreases |text|
  { reveal Trim(); if |text|>0 && text[|text|-1]==32 { TrimIdempotent(text[..|text|-1]); } }
  lemma EmptyPrefix(text: S.Bytes)
    ensures []+text==text
    ensures Trim([]+text)==Trim(text)
  { assert []+text==text; }
  lemma EmptyNameIgnoresUnusedPlural(row: Names,index: S.I16,capital: S.I16,plural: S.I16,article: S.I16,word: S.I16)
    requires Shape(row) && row.singular[0]==0
    ensures Name(row,index,capital,plural,article,word)==
      Name(Names(row.singular,C.Zeros(32)),index,capital,plural,article,word)
  {}
  lemma AutomaticPlain(text: S.Bytes)
    requires 0<|text|<32 && 0 !in text && text[|text|-1]!=32 && text[|text|-1]!=115
    requires text[|text|-1]!=101 || (|text|>1 && text[|text|-2]!=115)
    ensures Name(Names(Field(text),Field([])),0,1,1,0,0)==Label(text+[115],[])
  { FieldTerminator(text);TrimSpaces(text,0); }
  lemma {:isolate_assertions} LabelVectors() {
    F.WideDecimalVectors();
    var empty:=Field([]);
    var humanoid: S.Bytes:=[72,117,109,97,110,111,105,100];
    var upper: S.Bytes:=[82,65,67,69,32];
    var mouse: S.Bytes:=[77,111,117,115,101];
    var spaces: S.Bytes:=[32,32,32];
    var people: S.Bytes:=[80,101,111,112,108,101,32];
    var lower: S.Bytes:=[108,111,119,101,114];
    FieldTerminator(humanoid);FieldTerminator(upper);FieldTerminator(mouse);
    FieldTerminator([101]);FieldTerminator(spaces);FieldTerminator(people);FieldTerminator(lower);
    TrimSpaces([82,65,67,69],1);TrimSpaces([],3);
    TrimSpaces(humanoid,0);TrimSpaces(mouse,0);TrimSpaces([101],0);TrimSpaces(lower,0);
    TrimSpaces([84,104,101],4);TrimSpaces([116,104,101,32,80,101,111,112,108,101],1);
    assert upper==[82,65,67,69]+seq(1,i=>32 as S.U8);
    assert spaces==[]+seq(3,i=>32 as S.U8);
    assert Trim(upper)==[82,65,67,69];
    assert Trim(spaces)==[];
    EmptyPrefix(upper);EmptyPrefix(spaces);
    assert Trim([]+upper)==[82,65,67,69];
    assert Trim([]+spaces)==[];
    assert B(" have")==[32,104,97,118,101];
    assert B(" are")==[32,97,114,101];
    assert B(" has")==[32,104,97,115];
    assert B(" is")==[32,105,115];
    assert B("The ")==[84,104,101,32] && B("the ")==[116,104,101,32];
    assert B("The ")+spaces==[84,104,101]+seq(4,i=>32 as S.U8);
    assert B("the ")+people==[116,104,101,32,80,101,111,112,108,101]+seq(1,i=>32 as S.U8);
    assert Trim(B("The ")+spaces)==[84,104,101];
    assert Trim(B("the ")+people)==[116,104,101,32,80,101,111,112,108,101];
    assert B("Player ")==[80,108,97,121,101,114,32] && B("'s")==[39,115];
    // Humanoids; RACEs have; Mouse (the literal lowercase se suffix).
    assert Name(Names(Field(humanoid),empty),0,1,1,0,0)==Label(humanoid+[115],[]);
    assert Name(Names(Field(upper),empty),0,1,1,0,1)==Label([82,65,67,69,115,32,104,97,118,101],[1370]);
    assert Name(Names(Field(mouse),empty),0,1,1,0,0)==Label(mouse,[]);
    assert Name(Names(Field([101]),empty),0,1,1,0,0)==UndefinedNative(3);
    assert Name(Names(Field(spaces),empty),0,1,1,0,0)==UndefinedNative(2);
    assert Name(Names(Field(spaces),empty),0,1,1,1,0)==Label([84,104,101,115],[]);
    assert Name(Names(Field([120]),Field(people)),0,0,-7,1,2)==
      Label([116,104,101,32,80,101,111,112,108,101,32,97,114,101],[1371]);
    // Promoted fallback numbers 32768 and -32767, with source grammar.
    assert Name(Names(empty,empty),32767,1,0,1,1)==
      Label([80,108,97,121,101,114,32,51,50,55,54,56,39,115,32,104,97,115],[1374,1372]);
    assert Name(Names(empty,empty),-32768,0,1,0,2)==
      Label([80,108,97,121,101,114,32,45,51,50,55,54,55,32,105,115],[1374,1373]);
    assert Name(Names(Field(lower),empty),0,1,0,0,0)==Label(lower,[]);
  }
}
