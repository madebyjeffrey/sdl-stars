include "M03-scoring.dfy"

module M08Colony {
  import opened S04Arithmetic
  import C = S01Codecs
  // Capacity, habitat and resource arithmetic remain in M03Scoring/M04Pricing.
  function CapacityPercent(pop: nat,capacity: int): int {
    if capacity<=0 then 0 else Min(999,(pop*100+capacity/2)/capacity)
  }
  function Installations(capacity: nat,operate: nat,fAR: bool): int {
    if fAR then 0 else Max(10,capacity*operate/100)
  }
  function Operable(pop: int,capacity: nat,operate: nat,fAR: bool): int {
    if fAR then 0 else Max(1,Min(Installations(capacity,operate,false),C.DivC(pop*operate,100)))
  }
  function Operating(built: nat,pop: int,capacity: nat,operate: nat,fOwned: bool,fAR: bool): int {
    if !fOwned || fAR then 0 else Min(built,Operable(pop,capacity,operate,false))
  }
  function Defenses(hab: int,fAR: bool): int { if fAR then 0 else Clamp(hab*4,10,100) }
  function OperableDefenses(pop: int,hab: int,fAR: bool): int {
    if fAR then 0 else Min(Defenses(hab,false),Min(1000,C.DivC(pop+24,25)))
  }
  datatype End = None | At(value: int)
  datatype Bounds = Bounds(low: End,high: End)
  function Terraform(current: int,original: int,ideal: int,ability: nat,fImmune: bool,fHelp: bool): Bounds
    requires 1<=current<=99 && 1<=original<=99 && 0<=ideal<=100
  {
    if ability==0 || fImmune then Bounds(None,None) else
    var lo := if original-ability<current then At(Max(1,original-ability)) else None;
    var hi := if original+ability>current then At(Min(99,original+ability)) else None;
    if fHelp then
      if current==ideal then Bounds(None,None)
      else if current>ideal then Bounds(if lo.At? then At(Max(lo.value,ideal)) else None,None)
      else Bounds(None,if hi.At? then At(Min(hi.value,ideal)) else None)
    else
      var distance := Abs(current-ideal);
      var dl := if lo.At? then Abs(lo.value-ideal) else 0;
      var dh := if hi.At? then Abs(hi.value-ideal) else 0;
      if distance>=dl && distance>=dh then Bounds(None,None)
      else if dl>=dh then Bounds(lo,None) else Bounds(None,hi)
  }
  function TerraformSteps(current: int,b: Bounds): int {
    (if b.low.At? then current-b.low.value else 0)+(if b.high.At? then b.high.value-current else 0)
  }
  datatype Selection = Missing | ScannerOnly | MainPlanet
  function Select(fFound: bool,fFull: bool,fOwned: bool): Selection {
    if !fFound || !fFull then ScannerOnly else if fOwned then MainPlanet else Missing
  }
  predicate ConstructionBase(fOwned: bool,fBase: bool,cargoCapacity: nat) { fOwned && fBase && cargoCapacity!=0 }
  function Offset(priorValue: int,content: nat,client: nat,fChanged: bool): int {
    if fChanged then 0 else Clamp(priorValue,0,Max(0,content-client))
  }
  // One element per occupied mass-driver slot, not per component in that slot.
  datatype Driver = Driver(warp: nat,fTwo: bool)
  function Drivers(slots: seq<nat>,best: Driver): Driver decreases |slots| {
    if |slots|==0 then best else
    Drivers(slots[1..],if slots[0]>best.warp then Driver(slots[0],false)
      else if slots[0]==best.warp then Driver(best.warp,true) else best)
  }
  lemma PercentBound(p: nat,k: int)
    ensures 0<=CapacityPercent(p,k)<=999
  {}
  lemma ARHasNoInstallations(p: int,k: nat,o: nat,h: int)
    ensures Installations(k,o,true)==0 && Operable(p,k,o,true)==0 && Defenses(h,true)==0
  {}
  lemma ScrollBound(x: int,c: nat,v: nat,f: bool)
    ensures 0<=Offset(x,c,v,f)<=Max(0,c-v)
  {}
  lemma HelpfulAxis(c: int,o: int,i: int,a: nat,immune: bool)
    requires 1<=c<=99 && 1<=o<=99 && 0<=i<=100
    ensures Terraform(c,o,i,a,immune,true).low.At? ==>
      1<=Terraform(c,o,i,a,immune,true).low.value<=c
    ensures Terraform(c,o,i,a,immune,true).high.At? ==>
      c<=Terraform(c,o,i,a,immune,true).high.value<=99
  {}
  lemma {:isolate_assertions} AcceptanceVectors()
    ensures CapacityPercent(12500,10000)==125 && CapacityPercent(100000,10000)==999
    ensures Installations(10000,10,false)==1000 && Operable(5,10000,10,false)==1
    ensures Defenses(-10,false)==10 && Defenses(30,false)==100
    ensures Terraform(70,70,50,10,false,true)==Bounds(At(60),None)
    ensures Terraform(70,70,50,10,true,true)==Bounds(None,None)
    ensures Drivers([5,7,7],Driver(0,false))==Driver(7,true)
    ensures Offset(300,100,200,false)==0 && Offset(40,500,200,true)==0
  {}
}
