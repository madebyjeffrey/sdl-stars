include "M08-colony.dfy"

module M09Production {
  import opened S04Arithmetic
  datatype Row = Row(item: int,count: nat,percent: nat)
  predicate Fits(r: Row) { r.count<=1020 && r.percent<=100 }
  function Quantum(shift: bool,ctrl: bool): nat { if ctrl then (if shift then 1020 else 100) else if shift then 10 else 1 }
  function AddCount(priorValue: nat,requested: nat,fAutoAlchemy: bool): int {
    Min(if fAutoAlchemy then 1 else 1020,priorValue+requested)
  }
  // Scalar valid-selection projection. M09Editor models actual command order,
  // native wrap and inactive-catalog access for supplied invalid selections.
  function Availability(priorValue: nat,removed: nat): nat { if priorValue==1023 then priorValue else if removed<=priorValue then priorValue-removed else 0 }
  predicate CanInsert(rows: nat) { rows<40 }
  function Remove(q: seq<Row>,i: nat): seq<Row> requires i<|q| { q[..i]+q[i+1..] }
  function Swap(q: seq<Row>,i: nat): seq<Row> requires i+1<|q| { q[i:=q[i+1]][i+1:=q[i]] }
  datatype Editor = Editor(queue: seq<Row>,fNoResearch: bool)
  // Per-colony value projection, conditional on callback behavior. Native
  // ownership, ignored lookup outcomes and recovery live in M09Lifecycle.
  function Finish(prior: Editor,working: Editor,accept: bool): Editor { if accept then working else prior }
  function Research(gross: nat,pct: nat,fNoResearch: bool): nat
    requires pct<=100
  { if fNoResearch then 0 else gross*pct/100 }
  function Paid(cost: nat,pct: nat): nat { cost*pct/100 }
  function Charge(cost: nat,oldPct: nat,newPct: nat): int { Paid(cost,newPct)-Paid(cost,oldPct) }
  function DimensionPercent(cost: nat,available: nat,paid: nat): int {
    if cost==0 || available>=cost then 100 else Max((available+paid)*100/cost,(available+paid+1)*100/cost-1)
  }
  function Progress(cost: seq<nat>,available: seq<nat>,pct: nat): int
    requires |cost|==|available|
    decreases |cost|
  { if |cost|==0 then 100 else Min(DimensionPercent(cost[0],available[0],Paid(cost[0],pct)),Progress(cost[1..],available[1..],pct)) }
  predicate CanComplete(cost: seq<nat>,available: seq<nat>,pct: nat) {
    |cost|==|available| && forall i | 0<=i<|cost| :: cost[i]-Paid(cost[i],pct)<=available[i]
  }
  function AlchemyCost(fTrait: bool): nat { if fTrait then 25 else 100 }
  function TerraformCost(fTT: bool,fCA: bool): nat { (if fTT then 70 else 100)/(if fCA then 2 else 1) }
  function ChangedHullCost(newCost: nat,oldCost: nat): int { Max(newCost/2,newCost-oldCost/2) }
  function ChangedSlotCost(newCost: nat,oldCost: nat,sameCategory: bool,sameItem: bool): int {
    if !sameCategory then Max(3*newCost/10,newCost-7*oldCost/10)
    else if !sameItem then Max(2*newCost/10,newCost-8*oldCost/10)
    else Max(0,newCost-oldCost)
  }
  function BaseCost(cost: nat,fDiscount: bool): nat { ((if fDiscount then cost-cost/5 else cost)+1)/2 }
  datatype Status = NoneAuto | SomeAuto | SkippedAuto | CompleteAuto | BlockedSame | BlockedDiff | Complete | Some
  function Result(auto: nat,built: nat,left: nat,oldPct: nat,newPct: nat): Status {
    if auto==2 then (if built==0 then NoneAuto else SomeAuto)
    else if auto!=0 && left==0 then (if built==0 then SkippedAuto else CompleteAuto)
    else if built==0 then (if oldPct==newPct then BlockedSame else BlockedDiff)
    else if left==0 then Complete else Some
  }
  lemma CancelRestores(p: Editor,w: Editor) ensures Finish(p,w,false)==p {}
  lemma SwapKeepsRows(q: seq<Row>,i: nat)
    requires i+1<|q|
    ensures multiset(Swap(q,i))==multiset(q)
  { assert q==q[..i]+[q[i],q[i+1]]+q[i+2..];
    assert Swap(q,i)==q[..i]+[q[i+1],q[i]]+q[i+2..]; }
  lemma ResearchBound(r: nat,p: nat,f: bool) requires p<=100 ensures 0<=Research(r,p,f)<=r {}
  lemma ChargeTelescopes(c: nat,a: nat,b: nat,d: nat)
    ensures Charge(c,a,b)+Charge(c,b,d)==Charge(c,a,d)
  {}
  lemma {:isolate_assertions} AcceptanceVectors()
    ensures Quantum(true,true)==1020 && !CanInsert(40)
    ensures Availability(1023,10)==1023 && Availability(20,10)==10
    ensures Progress([10,100],[5,40],0)==40
    ensures Charge(3,33,66)==1 && Charge(3,66,100)==2
    ensures ChangedHullCost(100,80)==60 && BaseCost(101,true)==41
    ensures TerraformCost(true,true)==35 && AlchemyCost(true)==25
    ensures Result(2,0,1,0,0)==NoneAuto && Result(0,0,1,0,20)==BlockedDiff
  {}
}
