include "M08-colony.dfy"
include "M09-production.dfy"
include "M10-fleet.dfy"
include "M12-research.dfy"
include "M13-relations.dfy"
module M19Resolution {
  import opened S04Arithmetic
  import C = S01Codecs
  import G = SharedGameState
  function WormholeMove(lastMove: nat,stable: int): int { Clamp(lastMove/5-2+stable,0,6) }
  function Concentration(raw: nat,home: bool,remote: bool,ar: bool): int {
    if raw<30 && home && (!remote || ar) then 30 else raw
  }
  function MiningScaled(mines: nat,conc: nat,eff: nat,remote: bool): nat { if remote then mines*conc else mines*conc*eff/10 }
  opaque function Yield(scaled: nat,generating: bool,draw: nat): nat
    requires draw<100
  { scaled/100+(if scaled%100!=0 && generating && draw<scaled%100 then 1 else 0) }
  function MiningDraws(scaled: nat,generating: bool): bool { generating && scaled%100!=0 }
  datatype MiningFleet = MiningFleet(planet: int,player: int,dead: bool,orders: nat,firstTask: nat,mines: int)
  datatype MiningSlot = Hole | Present(fleet: MiningFleet)
  function RemoteEligible(f: MiningFleet,planet: int,player: int): bool {
    f.planet==planet && f.player==player && !f.dead && f.orders<=1 && f.firstTask==G.TaskCode(G.RemoteMine) && f.mines>0
  }
  function MiningMarks(rows: seq<MiningSlot>,planet: int,player: int,ar: bool,remote: bool,apply: bool): seq<bool>
    ensures |MiningMarks(rows,planet,player,ar,remote,apply)|==|rows|
    decreases |rows|
  {
    if |rows|==0 || !ar || remote || rows[0].Hole? then seq(|rows|,i=>false)
    else ([apply && RemoteEligible(rows[0].fleet,planet,player)]+MiningMarks(rows[1..],planet,player,ar,remote,apply))
  }
  lemma MiningHoleStops(tail: seq<MiningSlot>,planet: int,player: int,ar: bool,remote: bool,apply: bool)
    ensures MiningMarks([Hole]+tail,planet,player,ar,remote,apply)==seq(1+|tail|,i=>false)
  {}
  function DepletionBand(conc: nat): nat { if conc>100 then 100 else if conc<5 then 10 else if conc<25 then 25 else conc }
  function DepletionLeft(level: nat,conc: nat): nat { (if level==0 then 256 else level)*12500/256/DepletionBand(conc) }
  datatype Growth = Growth(change: int,remainder: int)
  function Carry(hundredths: int,remainder: int): Growth
    requires 0<=remainder<100
  { var whole := C.DivC(hundredths,100); var frac := hundredths-whole*100;
    var next := remainder+frac;
    if next>=100 then Growth(whole+1,next-100) else if next<0 then Growth(whole-1,next+100) else Growth(whole,next)
  }
  function HostileLoss(pop: nat,hab: int,remainder: int): Growth
    requires hab<0 && 0<=remainder<100
  { Carry(-Max(1,pop*(-hab)/10),remainder) }
  function CrowdedRate(pop: nat,capacity: nat,rate: nat): int
    requires capacity>0
  { if pop<=capacity/4 then rate else
    var fullness := pop*1000/capacity;
    if pop>=capacity then Max(-300,99-fullness/10)*4 else
    var retard := (1000-fullness)*(1000-fullness);
    if rate<1000 then rate*retard/562500 else (rate/10)*retard/562500*10
  }
  // Non-hostile branch; native overflow/cast domain is an adapter obligation.
  function GrowthAt(pop: nat,capacity: nat,rate: nat,remainder: int): Growth
    requires pop>0 && capacity>0 && 0<=remainder<100
  { if capacity<=pop<capacity+10 then Growth(0,remainder) else
    var pct := CrowdedRate(pop,capacity,rate);
    var rough := pop*C.DivC(pct,100);
    var inc := if rough<10000000 then C.DivC(pop*pct,100) else rough;
    Carry(if inc==0 then 1 else inc,remainder)
  }
  lemma CarryBound(h: int,r: int) requires 0<=r<100 ensures 0<=Carry(h,r).remainder<100 {}
  lemma YieldBound(s: nat,g: bool,d: nat) requires d<100 ensures s/100<=Yield(s,g,d)<=s/100+1 { reveal Yield(); }
  lemma {:isolate_assertions} MiningVectors() {
    assert MiningDraws(50,true) && !MiningDraws(50,false) && !MiningDraws(100,true);
    var miner:=Present(MiningFleet(5,0,false,1,3,2));
    assert MiningMarks([miner],5,0,true,false,true)==[true];
    assert MiningMarks([miner],5,0,true,false,false)==[false];
    assert MiningMarks([Hole,miner],5,0,true,false,true)==[false,false];
  }
  lemma {:isolate_assertions} AcceptanceVectors()
    ensures WormholeMove(100,0)==6 && WormholeMove(0,0)==0
    ensures Concentration(1,true,false,false)==30 && Concentration(1,true,true,false)==1
    ensures Yield(1234,true,33)==13 && Yield(1234,true,34)==12 && Yield(1234,false,0)==12
    ensures Carry(-1,0)==Growth(-1,99) && Carry(1,99)==Growth(1,0)
    ensures GrowthAt(10005,10000,1500,37)==Growth(0,37)
  {
    reveal Yield();
    assert 1234/100==12 && 1234%100==34;
    assert C.DivC(-1,100)==0 && C.DivC(1,100)==0;
    assert -1-0*100==-1 && 1-0*100==1;
    assert 0+(-1)<0 && 99+1>=100;
    assert Carry(-1,0).change==-1 && Carry(-1,0).remainder==99;
    assert Carry(1,99).change==1 && Carry(1,99).remainder==0;
  }
}
