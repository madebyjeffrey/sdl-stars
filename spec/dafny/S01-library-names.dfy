include "S01-external.dfy"

// Byte/path rules shared by M01/M02. OS enumeration, native comparison and
// lossless UTF-8 path conversion remain explicit adapter obligations.
module S01LibraryNames {
  import opened S01
  import X = S01External
  datatype RootSource = LibraryOverride | SettingsOverride | PreferencePath
  function Root(fLibrary: bool,fSettings: bool): RootSource {
    if fLibrary then LibraryOverride else if fSettings then SettingsOverride else PreferencePath
  }
  // limit+1 means exhausted. Used for race suggestions (1..999) and
  // conflict backups (0..999); native occupancy checks supply the set.
  function FirstFree(occupied: set<nat>,i: nat,limit: nat): nat
    requires i<=limit+1
    ensures i<=FirstFree(occupied,i,limit)<=limit+1
    ensures FirstFree(occupied,i,limit)<=limit ==> FirstFree(occupied,i,limit) !in occupied
    ensures forall j | i<=j<FirstFree(occupied,i,limit) :: j in occupied
    decreases limit+1-i
  {
    if i>limit || i !in occupied then i else FirstFree(occupied,i+1,limit)
  }
  function FirstStem(b: Bytes): Bytes
    decreases |b|
  { if |b|==0 || b[0]==46 then [] else [b[0]]+FirstStem(b[1..]) }
  function LastDot(b: Bytes): int
    ensures -1<=LastDot(b)<|b|
    decreases |b|
  { if |b|==0 then -1 else if b[|b|-1]==46 then |b|-1 else LastDot(b[..|b|-1]) }
  function DropSuffix(b: Bytes): Bytes {
    var i := LastDot(b); if i<0 then b else b[..i]
  }
  predicate FForbidden(c: U8) { c<32 || c==127 || c in {60,62,58,34,47,92,124,63,42} }
  predicate FReserved(b: Bytes) {
    var s := X.LowerBytes(FirstStem(b));
    s in {[99,111,110],[112,114,110],[97,117,120],[110,117,108],[99,108,111,99,107,36]} ||
    (|s|==4 && s[..3] in {[99,111,109],[108,112,116]} && 49<=s[3]<=57)
  }
  predicate FComponent(b: Bytes) {
    0<|b| && b[0] !in {32,46} && b[|b|-1] !in {32,46} &&
    (forall c <- b :: !FForbidden(c)) && !FReserved(b)
  }
  predicate FGameName(b: Bytes,cbRoot: nat,cbBase: nat) {
    FComponent(b) && |b|<256 && cbRoot+2*|b|+18<cbBase
  }
  predicate FRaceName(stem: Bytes,digits: Bytes,cbRoot: nat,cbBase: nat) {
    var b := stem+[46,114]+digits;
    |stem|>0 && FComponent(b) && |b|<240 && 1<=|digits|<=3 &&
    X.FDigits(digits) && digits[0]!=48 && cbRoot+|b|+2<cbBase
  }
  // Native equality keys are supplied by the case-comparison adapter.
  // Only ASCII folding is modeled here; no Unicode normalization is invented.
  predicate FAscii(b: Bytes) { forall c <- b :: c<128 }
  predicate FComparisonKey(raw: Bytes,key: Bytes) {
    FAscii(raw) ==> key==X.LowerBytes(raw)
  }
  predicate FAvailable(key: Bytes,occupied: set<Bytes>,fEnumerated: bool) {
    fEnumerated && key !in occupied
  }
  function ReplaceBad(b: Bytes,fDots: bool): Bytes
    decreases |b|
  { if |b|==0 then [] else
      [if FForbidden(b[0]) || (fDots && b[0]==46) then 95 else b[0]]+ReplaceBad(b[1..],fDots) }
  function Trim(b: Bytes): Bytes
    decreases |b|
  { if |b|==0 then [] else if b[0] in {32,46} then Trim(b[1..])
    else if b[|b|-1] in {32,46} then Trim(b[..|b|-1]) else b }
  // Input is already cut at the native UTF-8 byte budget. Fallback/prefix
  // and final path-capacity checks are supplied by the caller.
  function CleanStem(b: Bytes,fGame: bool): Bytes { ReplaceBad(Trim(ReplaceBad(b,false)),fGame) }
  predicate FUtf8Cut(full: Bytes,cut: Bytes,limit: nat) {
    |cut|<=limit && |cut|<=|full| && cut==full[..|cut|] && FUtf8(cut)
  }
  lemma {:isolate_assertions} {:fuel FirstStem,8} {:fuel LastDot,8} {:fuel X.LowerBytes,8} {:fuel ReplaceBad,8} {:fuel Trim,8} NameVectors() {
    assert FirstStem([67,79,78,46,120])==[67,79,78];
    assert X.LowerBytes([67,79,78])==[99,111,110];
    assert FReserved([67,79,78,46,120]);
    assert Root(true,true)==LibraryOverride;
    assert FirstFree({1,3},1,999)==2;
    assert FirstFree({0,1},0,1)==2;
    assert !FGameName([67,79,78],10,256);
    assert FGameName([65,46,66],10,256);
    assert !FGameName([65],236,256);
    assert FRaceName([65],[57,57,57],10,256);
    assert !FRaceName([65],[48,49],10,256);
    assert !FAvailable([97],{[97]},true);
    assert ReplaceBad([32,65,58,66,46,32],false)==[32,65,95,66,46,32];
    assert Trim([32,65,95,66,46,32])==[65,95,66];
    assert ReplaceBad([65,95,66],false)==[65,95,66];
    assert CleanStem([32,65,58,66,46,32],false)==[65,95,66];
    assert DropSuffix(DropSuffix([65,46,66,46,114,49]))==[65];
  }
}
