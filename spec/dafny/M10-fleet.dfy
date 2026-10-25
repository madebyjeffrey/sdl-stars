include "S06-shared-state.dfy"
include "S01-orders.dfy"
include "S04-arithmetic.dfy"

module M10Fleet {
  import opened S04Arithmetic
  // Valid current game domain, not a decoder accepting corrupt negative cargo.
  predicate CanSplit(boats: nat,fleets: nat) requires fleets<=512 { boats>1 && fleets!=512 }
  predicate CanSplitAll(boats: nat,fleets: nat) requires fleets<=512 { boats>1 && boats-1+fleets<=512 }
  datatype Stack = Stack(destination: nat,source: nat)
  function Merge(dst: nat,src: nat): Stack
    requires dst<=32767 && src<=32767
  { var moved := Min(src,32767-dst); Stack(dst+moved,src-moved) }
  function NoHeal(destination: bool,source: bool): bool { destination || source }
  function Transfer(request: nat,stock: nat,free: nat,fPermitted: bool): int {
    if !fPermitted then 0 else Min(request,Min(stock,free))
  }
  datatype Balance = Balance(source: nat,destination: nat)
  function Move(b: Balance,request: nat,free: nat,fPermitted: bool): Balance {
    var amount := Transfer(request,b.source,free,fPermitted);
    Balance(b.source-amount,b.destination+amount)
  }
  function Finish(prior: Balance,working: Balance,fAccept: bool): Balance { if fAccept then working else prior }
  datatype Waypoint = Waypoint(x: int,y: int,task: int)
  predicate SamePoint(a: Waypoint,b: Waypoint) { a.x==b.x && a.y==b.y }
  function Delete(q: seq<Waypoint>,i: nat): seq<Waypoint>
    requires 0<i<|q|
  { var reduced := q[..i]+q[i+1..];
    if i<|reduced| && SamePoint(reduced[i-1],reduced[i]) then reduced[..i]+reduced[i+1..] else reduced }
  function Recycle(q: seq<Waypoint>,i: nat,fRepeat: bool): seq<Waypoint>
    requires i<|q|
  { var tail := q[..i]+q[i+1..];
    if fRepeat && i!=86 && |q|!=2 && !SamePoint(q[|q|-1],q[i]) then tail+[q[i]] else tail }
  // Per-design nonnegative contribution in the integer branch of EstFuelUse.
  function Efficiency(base: nat,fIFE: bool): nat { base-(if fIFE then base*15/100 else 0) }
  function FuelTerm(mass: nat,efficiency: nat,distance: nat): nat { mass*efficiency*distance/2000 }
  function FuelTotal(terms: nat,fRange: bool): nat { (terms+(if fRange then 0 else 9))/10 }
  function Range(fuel: nat,rate: nat): nat {
    if rate==0 then 1000000000 else if rate>100000 then
      (assert rate/1000>0; fuel/(rate/1000)) else fuel*1000/rate
  }
  lemma MergeConserves(dst: nat,src: nat)
    requires dst<=32767 && src<=32767
    ensures Merge(dst,src).destination+Merge(dst,src).source==dst+src
    ensures Merge(dst,src).destination<=32767
  {}
  lemma TransferConserves(b: Balance,q: nat,free: nat,f: bool)
    ensures Move(b,q,free,f).source+Move(b,q,free,f).destination==b.source+b.destination
    ensures 0<=Move(b,q,free,f).destination-b.destination<=free
  {}
  lemma OriginSurvives(q: seq<Waypoint>,i: nat)
    requires 0<i<|q|
    ensures |Delete(q,i)|>=1 && Delete(q,i)[0]==q[0]
  {}
  lemma {:isolate_assertions} AcceptanceVectors()
    ensures CanSplit(2,511) && !CanSplit(2,512)
    ensures CanSplitAll(3,510) && !CanSplitAll(4,510)
    ensures Merge(32000,1000)==Stack(32767,233)
    ensures Move(Balance(100,20),90,30,true)==Balance(70,50)
    ensures Move(Balance(100,20),90,30,false)==Balance(100,20)
    ensures Delete([Waypoint(1,1,0),Waypoint(2,2,0),Waypoint(1,1,1)],1)==[Waypoint(1,1,0)]
    ensures Efficiency(100,true)==85 && FuelTotal(1,false)==1 && Range(100,0)==1000000000
  {
    assert Delete([Waypoint(1,1,0),Waypoint(2,2,0),Waypoint(1,1,1)],1)==[Waypoint(1,1,0)];
    assert Efficiency(100,true)==85;
    assert FuelTotal(1,false)==1;
    assert Range(100,0)==1000000000;
  }
}
