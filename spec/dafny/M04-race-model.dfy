include "S04-arithmetic.dfy"

// M04 logical race schema, observed correction and audited top-level workflow.
module M04Race {
  import S = S01
  import C = S01Codecs
  import opened S04Arithmetic
  datatype Axis = Axis(minimum: int,preferred: int,maximum: int)
  function Immune(): Axis { Axis(-1,-1,-1) }
  predicate FAxis(a: Axis,fSafe: bool) {
    a==Immune() || (0<=a.minimum<=a.maximum<=100 &&
      (!fSafe || a.minimum<a.maximum) && a.preferred==a.minimum+(a.maximum-a.minimum)/2)
  }
  function NormalizeAxis(a: Axis): Axis {
    if a.minimum == -1 then Immune() else
    var lo := Clamp(a.minimum,0,100);
    var hi := Max(lo,Min(a.maximum,100));
    Axis(lo,lo+(hi-lo)/2,hi)
  }
  function MinAttr(): seq<int> { [7,5,5,5,5,2,5,0,0,0,0,0,0,0,0,0] }
  function MaxAttr(): seq<int> { [25,15,25,25,25,15,25,6,2,2,2,2,2,2,9,0] }
  datatype Race = Race(singular: S.Bytes,plural: S.Bytes,portrait: int,credential: int,
    habitat: seq<Axis>,growth: int,attributes: seq<int>,traits: S.U32,fHacker: bool)
  predicate FShape(r: Race) { |r.habitat|==3 && |r.attributes|==16 }
  predicate FIdentity(r: Race) {
    S.FName(r.singular) && S.FName(r.plural) && 0 !in r.singular && 0 !in r.plural &&
    0<=r.portrait<32 && -2147483648<=r.credential<=2147483647
  }
  predicate FPricing(r: Race,fSafe: bool) {
    FShape(r) && (forall a <- r.habitat :: FAxis(a,fSafe)) && 1<=r.growth<=20 &&
    forall i | 0<=i<16 :: MinAttr()[i]<=r.attributes[i]<=MaxAttr()[i]
  }
  predicate FRace(r: Race) { FIdentity(r) && FPricing(r,true) }
  function Bounds(r: Race,fPricing: bool): Race
    requires FShape(r)
  {
    var axes := seq(3,i requires 0<=i<3 => NormalizeAxis(r.habitat[i]));
    var attrs := seq(16,i requires 0<=i<16 => Clamp(r.attributes[i],MinAttr()[i],MaxAttr()[i]));
    var growth := if fPricing then Clamp(r.growth,1,20) else Min(r.growth,20);
    r.(habitat:=axes,attributes:=attrs,growth:=growth,
       fHacker:=r.fHacker || axes!=r.habitat || attrs!=r.attributes || growth!=r.growth)
  }
  function NormalizeInnateAxis(a: Axis): Axis {
    if (a.minimum<0 || a.preferred<0 || a.maximum<0 || a.minimum>100 || a.preferred>100 || a.maximum>100)
       && a!=Immune() then Immune() else a
  }
  function Trait(r: Race,ibit: nat): bool { C.Bits(r.traits,ibit,1)==1 }
  function Primary(r: Race): int requires FShape(r) { r.attributes[14] }
  function Temperature(x: int): int { 4*x-200 }
  function GravityHundredths(x: int): int {
    var d := Abs(x-50);
    var v := if d<=25 then 100+4*d else 200+24*(d-25);
    if x<50 then 10000/v else v
  }
  function Custom(r: Race): Race {
    r.(traits:=((r.traits as bv32) & 0xbfffffff) as int)
  }
  predicate FCredentialInput(rgb: S.Bytes) { |rgb|<=15 && S.FUtf8(rgb) && 0 !in rgb }
  function Credential(rgb: S.Bytes): int requires 0 !in rgb { C.Signed(C.Password(rgb),32) }
  // Schema proposals, not a SQL implementation. Child count is transactional.
  datatype RaceVersion = RaceVersion(id: nat,race: Race,calculator: S.Bytes,
    points: int,innate: int,fCached: bool)
  predicate FVersion(v: RaceVersion) {
    FRace(v.race) && (v.fCached ==> (|v.calculator|>0 && -32768<=v.points<=32767 &&
      -2147483648<=v.innate<=2147483647))
  }
  datatype HabitatRow = HabitatRow(version: nat,axis: int,value: Axis)
  predicate FHabitatRows(id: nat,rows: seq<HabitatRow>,r: Race) {
    |rows|==3 && FShape(r) && forall i | 0<=i<3 ::
      rows[i].version==id && rows[i].axis==i && rows[i].value==r.habitat[i]
  }
  predicate FSamePriceInputs(a: Race,b: Race) {
    FShape(a) && FShape(b) && a.habitat==b.habitat && a.growth==b.growth &&
    (forall i | 0<=i<16 && i!=7 :: a.attributes[i]==b.attributes[i]) &&
    a.traits%16384==b.traits%16384 && Trait(a,29)==Trait(b,29) && Trait(a,31)==Trait(b,31)
  }
  predicate FRepriceRequired(previous: RaceVersion,current: Race,calculator: S.Bytes) {
    !previous.fCached || previous.calculator!=calculator || !FSamePriceInputs(previous.race,current)
  }
  function LabelsOnly(r: Race,singular: S.Bytes,plural: S.Bytes,portrait: int,credential: int): Race {
    r.(singular:=singular,plural:=plural,portrait:=portrait,credential:=credential)
  }
  lemma MetadataIndependent(r: Race,singular: S.Bytes,plural: S.Bytes,portrait: int,credential: int)
    requires FShape(r)
    ensures FSamePriceInputs(r,LabelsOnly(r,singular,plural,portrait,credential))
  {}
  function DialogOrder(): seq<int> { [1,4,5,2,3,6] }
  datatype Exit = Stay(page: int) | Success(fSaved: bool) | Failure
  function Finish(page: int,fReadOnly: bool,fDontWrite: bool,points: int,
    fCredentials: bool,fSave: bool): Exit
    requires 0<=page<6
  {
    if fReadOnly then Stay(page) else if points<0 then Stay(page)
    else if !fCredentials then Stay(0) else if fDontWrite then Success(false)
    else if !fSave then Stay(0) else Success(true)
  }
  function Navigate(page: int,fNext: bool): int
    requires 0<=page<6
  { Clamp(page+(if fNext then 1 else -1),0,5) }
  function Cancel(): Exit { Failure }
  lemma AxisNormalization(a: Axis)
    ensures FAxis(NormalizeAxis(a),false)
    ensures NormalizeAxis(NormalizeAxis(a))==NormalizeAxis(a)
  {}
  lemma HackerSticky(r: Race,f: bool)
    requires FShape(r) && r.fHacker
    ensures Bounds(r,f).fHacker
  {}
  lemma {:isolate_assertions} NormalizationWidthVectors() {
    assert Clamp(10,0,100)==10;
    assert Min(10,100)==10 && Max(10,10)==10;
    assert NormalizeAxis(Axis(10,99,10))==Axis(10,10,10);
    assert FAxis(NormalizeAxis(Axis(10,99,10)),false);
    assert !FAxis(NormalizeAxis(Axis(10,99,10)),true);
    assert Min(12,100)==12 && Max(10,12)==12;
    assert NormalizeAxis(Axis(10,99,12))==Axis(10,11,12);
    assert FAxis(NormalizeAxis(Axis(10,99,12)),true);
  }
  lemma GravityVectors() {
    assert GravityHundredths(0)==12;
    assert GravityHundredths(50)==100;
    assert GravityHundredths(100)==800;
  }
  lemma {:isolate_assertions} RaceVectors() {
    assert NormalizeAxis(Axis(-1,20,100))==Immune();
    assert NormalizeAxis(Axis(10,99,11))==Axis(10,10,11);
    assert FAxis(Axis(10,10,11),true);
    assert FAxis(Axis(10,10,10),false) && !FAxis(Axis(10,10,10),true);
    assert Finish(4,false,true,0,true,false)==Success(false);
    assert Finish(4,false,false,-1,true,true)==Stay(4);
    assert Finish(4,false,false,0,false,true)==Stay(0);
  }
}
