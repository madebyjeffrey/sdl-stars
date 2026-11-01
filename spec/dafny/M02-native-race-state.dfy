include "M02-library.dfy"
include "S04-utf8-prefix.dfy"

module M02NativeRaceState {
  import S = S01
  import C = S01Codecs
  import L = M02Library
  import U = S04Utf8Prefix
  const PlayerBytes: nat := 192
  const NameAt: nat := 128
  const PluralAt: nat := 160
  const FlagAt: nat := 84
  const DetailAt: nat := 6
  datatype State = State(race: S.Bytes,password: S.Bytes,cache: S.Bytes,path: S.Bytes,
    newFlag: S.I16,suggestion: S.Bytes)
  predicate Shape(s: State) {
    |s.race|==PlayerBytes && |s.password|==16 && |s.cache|==16 && |s.path|==512 && |s.suggestion|==256
  }
  function Text(bytes: S.Bytes): S.Bytes { bytes[..C.FirstNul(bytes)] }
  predicate Comparable(race: S.Bytes) {
    |race|==PlayerBytes && C.FirstNul(race[NameAt..PluralAt])<32 && C.FirstNul(race[PluralAt..])<32
  }
  function ClearLearned(byte: S.U8): S.U8 { byte-8*((byte/8)%2) }
  function ComparedPrefix(race: S.Bytes): S.Bytes
    requires |race|==PlayerBytes
    ensures |ComparedPrefix(race)|==NameAt
  { race[..NameAt][FlagAt:=ClearLearned(race[FlagAt])] }
  function Project(race: S.Bytes): L.Design
    requires Comparable(race)
  { L.Design(ComparedPrefix(race),Text(race[NameAt..PluralAt]),Text(race[PluralAt..])) }
  predicate Equal(a: S.Bytes,b: S.Bytes)
    requires Comparable(a) && Comparable(b)
  { L.FSame(Project(a),Project(b)) }
  // Actual WriteRtPlr mutates this one input field before copying/encoding.
  function BeforeWrite(race: S.Bytes): S.Bytes
    requires |race|==PlayerBytes
    ensures |BeforeWrite(race)|==PlayerBytes
    ensures BeforeWrite(race)[NameAt..]==race[NameAt..]
    ensures forall i: nat | i<PlayerBytes && i!=DetailAt :: BeforeWrite(race)[i]==race[i]
  { if race[FlagAt]%2==0 then race else race[DetailAt:=race[DetailAt]-race[DetailAt]%8+7] }
  datatype Snapshot = Snapshot(race: S.Bytes,password: S.Bytes,cache: S.Bytes,path: S.Bytes,newFlag: S.I16)
  function Capture(s: State): Snapshot { Snapshot(s.race,s.password,s.cache,s.path,s.newFlag) }
  function Restore(saved: Snapshot,current: State,race: bool,path: bool): State {
    State(if race then saved.race else current.race,if race then saved.password else current.password,
      if race then saved.cache else current.cache,if path then saved.path else current.path,
      if path then saved.newFlag else current.newFlag,current.suggestion)
  }
  function CopyString(buffer: S.Bytes,text: S.Bytes): S.Bytes
    requires |text|<|buffer|
    ensures |CopyString(buffer,text)|==|buffer|
  { text+[0]+buffer[|text|+1..] }
  function SetFile(s: State,name: S.Bytes): State
    requires Shape(s)
    ensures Shape(SetFile(s,name))
  { s.(cache:=CopyString(s.cache,U.Prefix(name,15)),suggestion:=CopyString(s.suggestion,U.Prefix(name,255))) }
  function Bind(s: State,path: S.Bytes,name: S.Bytes): State
    requires Shape(s) && |path|<512
    ensures Shape(Bind(s,path,name))
  { SetFile(s,name).(path:=CopyString(s.path,path),newFlag:=0) }
  function Logical(s: State): L.RaceState
    requires Shape(s) && Comparable(s.race)
  { L.RaceState(Project(s.race),Text(s.password),Text(s.cache),Text(s.path),s.newFlag!=0) }
  function Suggested(s: State,aliasesCache: bool,requested: S.Bytes): S.Bytes
    requires Shape(s) && C.FirstNul(s.cache)<16 && C.FirstNul(s.suggestion)<256
    requires !aliasesCache || requested==Text(s.cache)
  {
    var cache:=Text(s.cache);var full:=Text(s.suggestion);
    if aliasesCache && |cache|>0 && |full|>0 && |cache|<=|full| && full[..|cache|]==cache then full else requested
  }
  lemma UsesFullSuggestion(s: State,requested: S.Bytes)
    requires requested==Text(s.cache)
    requires Shape(s) && C.FirstNul(s.cache)<16 && C.FirstNul(s.suggestion)<256
    requires 0<|Text(s.cache)|<=|Text(s.suggestion)|
    requires Text(s.suggestion)[..|Text(s.cache)|]==Text(s.cache)
    ensures Suggested(s,true,requested)==Text(s.suggestion)
  {}
  lemma RestoreShape(saved: State,current: State,race: bool,path: bool)
    requires Shape(saved) && Shape(current)
    ensures Shape(Restore(Capture(saved),current,race,path))
    ensures Restore(Capture(saved),current,race,path).suggestion==current.suggestion
  {}
  lemma RestoreOwned(saved: State,current: State)
    ensures Restore(Capture(saved),current,true,true)==saved.(suggestion:=current.suggestion)
  {}
  lemma ComparedFlag(byte: S.U8)
    ensures ClearLearned(ClearLearned(byte))==ClearLearned(byte)
    ensures (ClearLearned(byte)/8)%2==0
  {}
  lemma WrittenDetail(race: S.Bytes)
    requires |race|==PlayerBytes
    ensures race[FlagAt]%2==1 ==> BeforeWrite(race)[DetailAt]%8==7
    ensures race[FlagAt]%2==0 ==> BeforeWrite(race)==race
  {}
  lemma Vectors() {
    var bytes:=seq(192,i=>0 as S.U8);
    assert Comparable(bytes) && Comparable(bytes[FlagAt:=8]);
    assert Equal(bytes,bytes[FlagAt:=8]);
    var other:=bytes[12:=1];
    assert ComparedPrefix(bytes)[12]==0 && ComparedPrefix(other)[12]==1;
    assert !Equal(bytes,other); // credential bytes participate
    assert Equal(bytes,bytes[129:=99]); // after the first NUL
    assert BeforeWrite(bytes[FlagAt:=1])[DetailAt]==7;
  }
}
