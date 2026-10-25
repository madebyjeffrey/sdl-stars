include "M03-scoring.dfy"
include "S06-shared-state.dfy"

module M11Design {
  import opened S04Arithmetic
  datatype Availability = Invalid | Restricted | Available | NextTech | Later(levels: nat) | OtherTech
  function Missing(have: seq<nat>,need: seq<nat>): set<int>
    requires |have|==|need|
  { set i: int | 0<=i<|have| && have[i]<need[i] }
  function TechStatus(have: seq<nat>,need: seq<nat>,field: nat): Availability
    requires |have|==6 && |need|==6 && field<6
  {
    var missing := Missing(have,need);
    if missing=={} then Available
    else if missing=={field} then (if need[field]==have[field]+1 then NextTech else Later(need[field]-have[field]+1))
    else OtherTech
  }
  function Lookup(fValid: bool,fAllowed: bool,status: Availability): Availability {
    if !fValid then Invalid else if !fAllowed then Restricted else status
  }
  predicate CanBuild(hull: Availability,parts: seq<Availability>) {
    hull==Available && forall p <- parts :: p==Available
  }
  predicate CanEdit(existing: nat,fProgress: bool,fProtectedARBase: bool) { existing==0 && !fProgress && !fProtectedARBase }
  predicate CanAccept(fBase: bool,engines: nat) { fBase || engines!=0 }
  function Limit(fBase: bool): nat { if fBase then 10 else 16 }
  function FirstFree(free: seq<bool>,i: nat): nat
    requires i<=|free|
    decreases |free|-i
  { if i==|free| || free[i] then i else FirstFree(free,i+1) }
  predicate SlotAccepts(count: nat,capacity: nat,fCompatible: bool,fSamePart: bool) {
    count<capacity && (if count==0 then fCompatible else fSamePart)
  }
  function Filled(count: nat,capacity: nat,request: nat): int { Min(capacity,count+request) }
  function Armor(count: nat,dp: nat,fRS: bool): nat { (count*dp)/(if fRS then 2 else 1) }
  function Stored(total: nat): nat { total%65536 }
  function Discount(excess: nat,fBET: bool): int { Min(if fBET then 80 else 75,Min(excess,19)*(if fBET then 5 else 4)) }
  function Miniaturized(cost: nat,excess: nat,fBET: bool): int {
    if cost==0 || excess==0 then cost else Max(1,cost-LMulDiv(cost,Discount(excess,fBET),100))
  }
  function QuarterDiscount(cost: nat): nat { cost-cost/4 }
  function QuarterSurcharge(cost: nat): nat { cost+cost/4 }
  function CheapEngine(cost: nat): nat { cost-cost/2 }
  // The canonical aggregate consumes already looked-up/priced component rows.
  datatype Component = Component(count: nat,cost: nat,mass: nat)
  function TotalCost(base: nat,parts: seq<Component>): nat decreases |parts| {
    if |parts|==0 then base else TotalCost(base+parts[0].count*parts[0].cost,parts[1..])
  }
  function TotalMass(base: nat,parts: seq<Component>): nat decreases |parts| {
    if |parts|==0 then base else TotalMass(base+parts[0].count*parts[0].mass,parts[1..])
  }
  datatype EditResult = KeepPrior | FreeNewSlot | CommitWorking | RejectNoEngines
  function Finish(fNew: bool,fAccept: bool,fBase: bool,engines: nat): EditResult {
    if !fAccept then (if fNew then FreeNewSlot else KeepPrior)
    else if CanAccept(fBase,engines) then CommitWorking else RejectNoEngines
  }
  lemma BuildNeedsEveryPart(h: Availability,ps: seq<Availability>,p: Availability)
    requires CanBuild(h,ps) && p in ps
    ensures p==Available
  {}
  lemma FillBound(c: nat,cap: nat,q: nat) requires c<=cap ensures c<=Filled(c,cap,q)<=cap {}
  lemma StoredWidth(n: nat) ensures 0<=Stored(n)<65536 {}
  lemma {:isolate_assertions} AcceptanceVectors()
    ensures TechStatus([1,0,0,0,0,0],[2,0,0,0,0,0],0)==NextTech
    ensures TechStatus([1,0,0,0,0,0],[2,1,0,0,0,0],0)==OtherTech
    ensures !CanBuild(Available,[Available,Restricted])
    ensures Finish(true,false,false,1)==FreeNewSlot && Finish(false,true,false,0)==RejectNoEngines
    ensures FirstFree([false,true,false],0)==1
    ensures Armor(24,1500,true)==18000 && Stored(65540)==4
    ensures Discount(19,false)==75 && Discount(19,true)==80
    ensures Miniaturized(100,1,false)==96 && CheapEngine(5)==3
    ensures TotalCost(10,[Component(2,3,5)])==16 && TotalMass(10,[Component(2,3,5)])==20
  {
    assert Missing([1,0,0,0,0,0],[2,0,0,0,0,0])=={0};
    assert Missing([1,0,0,0,0,0],[2,1,0,0,0,0])=={0,1};
    assert Restricted in [Available,Restricted];
  }
}
