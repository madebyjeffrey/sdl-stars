include "M05-geometry.dfy"
include "M04-race-pricing.dfy"

module M05InitialState {
  import opened S04Arithmetic
  import G = M05Geometry
  import U = M03Setup
  import R = SharedGameState
  import Race = M04Race
  import P = M04Pricing
  import C = S01Codecs
  datatype Values = Values(values: seq<int>,rng: R.Rng)
  function Environment(rng: R.Rng): Values {
    var g1 := Take(rng,90); var g2 := Take(g1.rng,10);
    var t1 := Take(g2.rng,90); var t2 := Take(t1.rng,10); var r := Take(t2.rng,99);
    Values([1+g1.value+g2.value,1+t1.value+t2.value,1+r.value],r.rng)
  }
  function TutorialEnvironment(env: seq<int>,planet: nat,fTutorial: bool): seq<int>
    requires |env|==3
  { if !fTutorial then env else if planet==5 then [env[0]-5,env[1]-5,env[2]-5]
    else if planet==11 then env[0:=env[0]+20] else env }
  function Artifact(rng: R.Rng,fNoRandom: bool): (bool,R.Rng) {
    if fNoRandom then (false,rng) else var d := Take(rng,3); (d.value==0,d.rng)
  }
  function Concentration(rng: R.Rng,rad: int,fMax: bool,fBBS: bool): DrawResult {
    var a := if fMax then DrawResult(rng,0) else Take(rng,45);
    var b := if fMax then a else Take(a.rng,45);
    var c := if fMax then 100 else 31+a.value+b.value;
    var high := if !fMax && rad>=90 then Take(b.rng,99-c) else DrawResult(b.rng,0);
    var result := c+high.value/2;
    DrawResult(high.rng,result+(if fBBS && result<40 then 5 else 0))
  }
  function OneOverride(v: Values): Values requires |v.values|==3 {
    var amount := Take(v.rng,30); var index := Take(amount.rng,3);
    Values(v.values[index.value:=1+amount.value],index.rng)
  }
  function Overrides(v: Values,k: nat): Values
    requires |v.values|==3 && k>0
    ensures |Overrides(v,k).values|==3
    decreases if k<16 then 16-k else 0
  { if k>=16 then v else Overrides(OneOverride(v),2*k) }
  function Minerals(rng: R.Rng,rad: int,fMax: bool,fBBS: bool): Values {
    var a := Concentration(rng,rad,fMax,fBBS); var b := Concentration(a.rng,rad,fMax,fBBS); var c := Concentration(b.rng,rad,fMax,fBBS);
    var v := Values([a.value,b.value,c.value],c.rng);
    if fMax then v else var t := Take(v.rng,27); var next := v.(rng:=t.rng);
      if t.value>=18 then next else if t.value>=9 then OneOverride(next) else Overrides(next,t.value+1)
  }
  datatype PlanetSeed = PlanetSeed(owner: int,detail: int,scanner: int,artifact: bool,environment: seq<int>,original: seq<int>,concentrations: seq<int>,depletion: seq<int>,stocks: seq<int>,population: int,mines: int,factories: int,defenses: int,rng: R.Rng)
  function UnownedPlanet(rng: R.Rng,id: nat,fTutorial: bool,options: U.Options): PlanetSeed {
    var artifact := Artifact(rng,options.fNoRandom); var env := Environment(artifact.1);
    var adjusted := TutorialEnvironment(env.values,id,fTutorial);
    var minerals := Minerals(env.rng,adjusted[2],options.fExtraFuel,options.fBBS);
    PlanetSeed(-1,7,31,artifact.0,adjusted,adjusted,minerals.values,[0,0,0],[0,0,0],0,0,0,0,minerals.rng)
  }
  function Stock(rng: R.Rng,concentration: int,fBBS: bool): DrawResult {
    var a := Take(rng,concentration*10); var start := 10+a.value;
    var b := if start<200 then Take(a.rng,150) else DrawResult(a.rng,0);
    var amount := start+(if start<200 then 155+b.value else 0);
    DrawResult(b.rng,amount+(if fBBS then amount/4 else 0))
  }
  function Stocks(rng: R.Rng,conc: seq<int>,fBBS: bool): Values
    requires |conc|==3
  { var a := Stock(rng,conc[0],fBBS); var b := Stock(a.rng,conc[1],fBBS); var c := Stock(b.rng,conc[2],fBBS);
    Values([a.value,b.value,c.value],c.rng) }
  function Technology(primary: nat): seq<int> requires primary<10 {
    [[0,0,0,0,0,0],[0,0,0,0,5,0],[1,6,1,0,0,0],[1,1,1,2,0,6],[0,0,0,0,0,0],
     [0,0,2,0,0,2],[4,0,0,0,0,0],[0,0,5,5,0,0],[1,0,0,0,0,0],[3,3,3,3,3,3]][primary]
  }
  function StartingTech(r: Race.Race,fTutorial: bool): seq<int>
    requires Race.FPricing(r,true)
  { var primary := Race.Primary(r); var base := Technology(primary);
    var high := seq(6,i requires 0<=i<6 => if Race.Trait(r,29) && r.attributes[8+i]==0 then Max(base[i],if primary==9 then 4 else 3) else base[i]);
    high[2:=high[2]+(if Race.Trait(r,8) then 1 else 0)+(if Race.Trait(r,0) && !fTutorial then 1 else 0)] }
  function SurfaceLowest(xs: seq<int>): nat requires |xs|==3 {
    if xs[0]<xs[1] && xs[0]<xs[2] then 0 else if xs[1]<=xs[0] && xs[1]<xs[2] then 1 else 2
  }
  function FirstLowest(xs: seq<int>): nat requires |xs|==3 {
    if xs[0]<=xs[1] && xs[0]<=xs[2] then 0 else if xs[1]<=xs[2] then 1 else 2
  }
  function SurfaceBonus(xs: seq<int>,b: int): seq<int> requires |xs|==3 {
    var k := 10*b; var j := SurfaceLowest(xs);
    [xs[0]+C.DivC(k,4)+(if j==0 then C.DivC(k,4)+k%4 else 0),
      xs[1]+C.DivC(k,4)+(if j==1 then C.DivC(k,4)+k%4 else 0),
      xs[2]+C.DivC(k,4)+(if j==2 then C.DivC(k,4)+k%4 else 0)]
  }
  function ConcentrationBonus(xs: seq<int>,b: int): seq<int> requires |xs|==3 {
    var q := if b==1 || b==2 then 1 else C.DivC(b,2); var j := FirstLowest(xs);
    [xs[0]+C.DivC(q+1,2)+(if j==0 then q else 0),
      xs[1]+C.DivC(q+1,2)+(if j==1 then q else 0),
      xs[2]+C.DivC(q+1,2)+(if j==2 then q else 0)]
  }
  datatype Home = Home(pop: int,guess: int,stocks: seq<int>,concentrations: seq<int>,mines: int,factories: int,defenses: int,base: int,scanner: int)
  function HomeResources(r: Race.Race,points: int,fAI: bool,level: int,fBBS: bool,fTutorialContext: bool,planetZeroStocks: seq<int>,planetZeroConc: seq<int>): Home
    requires Race.FPricing(r,true) && |planetZeroStocks|==3 && |planetZeroConc|==3
  {
    var primary := Race.Primary(r); var pop0 := if Race.Trait(r,11) then 175 else 250;
    var guess := (pop0/4)%4096; var b := if fAI then 50 else Min(points,50); var choice := r.attributes[7];
    var pop1 := pop0+(if fAI && level>=3 then pop0/10 else 0);
    var pop := if fBBS then pop1*(2*r.growth*(if primary==0 then 2 else 1)+10)/10 else pop1;
    var conc := seq(3,i requires 0<=i<3 => Max(if fTutorialContext then 25 else 30,planetZeroConc[i]));
    var surface := choice==0 || choice>=5;
    Home(pop,guess*(if fBBS then 4 else 1),if surface then SurfaceBonus(planetZeroStocks,b) else planetZeroStocks,
      if choice==1 || (surface && fAI && level>=2) then ConcentrationBonus(conc,b) else conc,
      if primary==8 then 0 else 10+(if choice==2 then b/2 else 0),
      if primary==8 then 0 else 10+(if choice==3 then C.DivC(b,5) else 0),
      if primary==8 then 0 else 10+(if choice==4 then C.DivC(b+5,10) else 0),
      if primary==8 then 1 else 0,if primary==8 then 31 else 0)
  }
  function HomeAxis(a: Race.Axis,rng: R.Rng): DrawResult {
    if a==Race.Immune() then var d := Take(rng,99); DrawResult(d.rng,1+d.value)
    else DrawResult(rng,a.minimum+C.DivC(a.maximum-a.minimum,2))
  }
  function HomeEnvironment(r: Race.Race,rng: R.Rng): Values requires Race.FShape(r) {
    var a := HomeAxis(r.habitat[0],rng); var b := HomeAxis(r.habitat[1],a.rng); var c := HomeAxis(r.habitat[2],b.rng);
    Values([a.value,b.value,c.value],c.rng)
  }
  function ClearPlanetZero(stocks: seq<int>,fOwned: bool): seq<int> { if fOwned then stocks else seq(|stocks|,i=>0) }
  // Base template identifiers: 0 Starbase, 1 Accelerator, 2 Porthole, 3 Starter.
  datatype BaseComponent = Unchanged | MassDriver5 | Stargate100250
  datatype Base = Base(slot: int,template: int,number: int,built: int,existing: int,component: BaseComponent)
  function Bases(primary: int,size: U.Size,fTutorial: bool): seq<Base> {
    if primary==8 then [Base(0,3,16,0,0,Unchanged),Base(1,0,17,1,1,Unchanged)] else
    [Base(0,0,16,1,1,if primary==6 then MassDriver5 else if primary==7 && !fTutorial then Stargate100250 else Unchanged)]+
    (if size>0 && primary==6 then [Base(1,1,17,0,0,Unchanged)] else if size>0 && primary==7 then [Base(1,2,17,0,0,Unchanged)] else [])
  }
  // Each entry requests one fleet; repeated template entries share its new design.
  function ShipTemplates(primary: int,energy: int,construction: int,fAI: bool,fARM: bool,fOBRM: bool): seq<int> {
    (if primary==6 then [4] else if primary==2 then [3]+(if construction>=3 then [7,13] else [])
     else if primary==9 then [3,4] else if primary==1 then [if energy<2 then 2 else 5]+(if fAI then [] else [1]) else [2])+
    (if primary==0 then [12,12,12] else if primary==7 then [11] else if primary==8 then [10] else [9])+
    (if primary==5 then [16,18] else if primary==3 then [17] else if primary==7 then [7,8]
     else if primary==9 then [if construction<4 then 6 else 8,7,14] else [])+
    (if fARM && !fOBRM then [15,15] else [])
  }
  predicate FSecondEligible(primary: int,size: U.Size) { size>0 && (primary==6 || primary==7) }
  function UpgradeCandidates(category: nat,fUnsafeRadiationColony: bool): seq<int>
    requires category<8
  { [[10,5,4,2,3],[2,1],[2,1],[3,1],[1],[1],[2,0],[4,2,1]][category][if category==0 && fUnsafeRadiationColony then 1 else 0..] }
  function FirstAvailable(candidates: seq<int>,available: set<int>,previous: int): int decreases |candidates| {
    if |candidates|==0 then previous else if candidates[0] in available then candidates[0] else FirstAvailable(candidates[1..],available,previous)
  }
  function Fuel(hull: int,tanks: int,superTanks: int,generators: int,fFull: bool): int {
    if !fFull then 32000 else hull+250*tanks+500*superTanks+200*generators
  }
  datatype FleetStart = FleetStart(design: int,count: int,planet: int,position: G.Point,fuel: int,plan: int,warp: int,task: int,fValid: bool,fRepeat: bool,cargo: seq<int>)
  function StartupFleet(design: int,planet: int,position: G.Point,capacityBeforeUpgrade: int): FleetStart {
    FleetStart(design,1,planet,position,capacityBeforeUpgrade,0,0,0,true,false,[0,0,0,0])
  }
  function SecondStocks(rng: R.Rng): Values {
    var a := Take(rng,200); var b := Take(a.rng,200); var c := Take(b.rng,200);
    Values([100+a.value,100+b.value,100+c.value],c.rng)
  }
  function RerollEnvironment(rng: R.Rng): Values {
    var a := Take(rng,97); var b := Take(a.rng,97); var c := Take(b.rng,97);
    Values([2+a.value,2+b.value,2+c.value],c.rng)
  }
  predicate FSecondRerolls(r: Race.Race,states: seq<Values>,desires: seq<int>,traces: seq<Evaluation>,home: seq<int>,result: seq<int>) {
    1<=|states|<=101 && |states|==|desires|==|traces| && |home|==3 &&
    (forall i | 0<=i<|states| :: P.FDesire(r,states[i].values,traces[i],desires[i])) &&
    (forall i | 0<=i<|states|-1 :: desires[i]<10 && states[i+1]==RerollEnvironment(states[i].rng)) &&
    (|states|==101 || desires[|states|-1]>=10) &&
    result==(if |states|==101 then home else states[|states|-1].values)
  }
  function SecondWorld(population: nat,stocks: seq<int>,concentrations: seq<int>): Home {
    Home(2*population/5,(2*population/5)/4,stocks,concentrations,10,4,0,1,0)
  }
  lemma InitialVectors() {
    assert SurfaceLowest([10,10,10])==2 && FirstLowest([10,10,10])==0;
    assert SurfaceBonus([0,0,0],1)==[2,2,6];
    assert ConcentrationBonus([30,30,30],1)==[32,31,31];
    assert Fuel(100,1,1,1,true)==1050;
    assert FirstAvailable([10,5,4],{4,5},2)==5;
    assert !FSecondEligible(7,0) && FSecondEligible(7,1);
  }
  lemma SurfaceConservation(xs: seq<int>,b: nat)
    requires |xs|==3
    ensures SurfaceBonus(xs,b)[0]+SurfaceBonus(xs,b)[1]+SurfaceBonus(xs,b)[2]==xs[0]+xs[1]+xs[2]+10*b
  {
    assert 0<=SurfaceLowest(xs)<=2;
    assert C.DivC(10*b,4)==10*b/4;
    assert 4*(10*b/4)+(10*b)%4==10*b;
  }
}
