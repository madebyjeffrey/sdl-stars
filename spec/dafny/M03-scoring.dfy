include "M03-setup.dfy"
include "M04-race-pricing.dfy"

// M03 §4.4–4.6: owned by M19/M08/M11/M12, specified here as setup consumers.
// Normal game-domain arithmetic; overflow outside that domain is not redefined.
module M03Scoring {
  import opened S04Arithmetic
  import V = M03Setup
  import C = S01Codecs
  function TechPoints(level: int): int {
    if level<4 then level else if level<7 then 2*level-3 else if level<10 then 3*level-9 else 4*level-18
  }
  function ShipPoints(planets: nat,unarmed: nat,escort: nat,capital: nat): int {
    Min(unarmed,planets)/2+2*Min(escort,planets)+(if capital==0 then 0 else 8*capital*planets/(planets+capital))
  }
  datatype Packed = Packed(m: nat,e: nat)
  function Pack(n: nat): Packed decreases n {
    if n<=8191 then Packed(n,0) else var p := Pack(n/4); Packed(p.m,p.e+1)
  }
  function Decode(p: Packed): nat { p.m*C.Pow2(2*p.e) }
  function ScoredPopulation(owner: int,recordOwner: int,current: nat,recorded: nat): nat {
    if owner==recordOwner then recorded else current
  }
  function PlanetPoints(q: nat): int { Min(6,(q+999)/1000) }
  function Score(populations: seq<nat>,resources: nat,bases: nat,tech: seq<int>,dead: bool,u: nat,e: nat,c: nat): int {
    Sum(seq(|populations|,i requires 0<=i<|populations| => PlanetPoints(populations[i])))+
    resources/30+3*bases+(if dead then 0 else Sum(seq(|tech|,i requires 0<=i<|tech| => TechPoints(tech[i]))))+
    ShipPoints(|populations|,u,e,c)
  }
  function Capacity(hab: int,primary: int,fOBRM: bool,fOwnedBase: bool,hull: int): int
    requires primary==8 && fOwnedBase ==> 0<=hull<5
  {
    var k := if primary==8 then (if fOwnedBase then [2500,5000,10000,20000,30000][hull] else 0)
      else var k0 := if hab<5 then 500 else 100*hab;
        if primary==0 then k0-C.DivC(k0,2) else if primary==9 then k0+C.DivC(k0,5) else k0;
    k+(if fOBRM then C.DivC(k,10) else 0)
  }
  function EffectivePopulation(q: int,k: int): int { if q<=k then q else Min(2*k,k+C.DivC(q-k,2)) }
  function Factories(q: int,k: int,operate: int,built: int,fAR: bool): int {
    if fAR then 0 else Min(built,Max(1,Min(Max(10,C.DivC(k*operate,100)),C.DivC(q*operate,100))))
  }
  function OrdinaryResources(q: int,k: int,e: int,f: int,o: int,built: int): int
    requires e>0
  { if q==0 then 0 else Max(1,C.DivC(EffectivePopulation(q,k),e)+C.DivC(Factories(q,k,o,built,false)*f+9,10)) }
  function ARExpr(q: int,k: int,energy: int,e: int,hab: int): Expr {
    Add(Divide(Multiply(SquareRoot(Divide(Multiply(Integer(EffectivePopulation(q,k)),Integer(Max(1,energy))),Integer(e))),
      Integer(Max(25,hab))),Integer(10)),Bias999())
  }
  predicate FARResources(q: int,k: int,energy: int,e: int,hab: int,t: Evaluation,result: int) {
    e>0 && (if q==0 then result==0 else (FEvaluate(ARExpr(q,k,energy,e,hab),t) &&
      result==C.Signed(Max(1,Trunc(Value(t.result))),16)))
  }
  function Beam(damage: int,count: int,range: int,fSapper: bool): int {
    var b := C.DivC(damage*count*(range+3),4); if fSapper then C.DivC(b,3) else b
  }
  function Torpedo(damage: int,count: int,range: int): int { C.DivC(damage*count*(range-2),2) }
  function Bomb(colonists: int,buildings: int,count: int): int { (colonists+buildings)*count*2 }
  function Capacitors(abilities: seq<int>,a: int): int decreases |abilities| {
    if |abilities|==0 then a else Capacitors(abilities[1..],C.DivC(a*(100+abilities[0]),100))
  }
  function Power(beams: int,torps: int,bombs: int,abilities: seq<int>,speed: int): int {
    var a := Capacitors(abilities,1000);
    var b := if a==1000 then beams else C.DivC(beams*Min(255,C.DivC(a,10)),100);
    b+C.DivC(b*(speed-4),10)+torps+bombs
  }
  function Class(power: int): int { if power<=0 then 0 else if power<2000 then 1 else 2 }
  function HighestWarp(fuel: seq<int>,n: nat): nat
    requires n<|fuel| decreases n
  { if n==0 || fuel[n]<=120 then n else HighestWarp(fuel,n-1) }
  function Speed(fEngine: bool,fWarp10: bool,fuel: seq<int>,jets: nat,pods: nat,over: nat,pulsars: nat,miners: nat,mass: nat,slot0: nat): int
    requires fEngine ==> |fuel|>=10 && slot0>0
  { if !fEngine then 0 else Clamp((if fWarp10 then 10 else HighestWarp(fuel,9))-4+
      jets+pods+2*over+(pulsars+miners+1)/2-(mass/70)/slot0,0,8) }
  function TechCosts(): seq<int> { [0,50,80,130,210,340,550,890,1440,2330,3770,6100,9870,13850,18040,22440,27050,31870,36900,42140,47590,53250,59120,65200,71490,77990,84700] }
  function ResearchCost(total: nat,level: nat,choice: int,fSlow: bool): int
    requires level<=26 && 0<=choice<=2
  { var b := 10*total+TechCosts()[level];
    (if choice==0 then 2*b-b/4 else if choice==1 then b else b/2)*(if fSlow then 2 else 1) }
  function ResearchRemainder(cost: int,spent: int,fSlow: bool): int { cost-spent*(if fSlow then 2 else 1) }
  predicate FScoreVisible(fOwn: bool,fDead: bool,fOver: bool,fPublic: bool,turn: int) {
    fOwn || fDead || fOver || (fPublic && turn>=20)
  }
  datatype Standing = Standing(planets: nat,score: nat,resources: nat,capital: nat,tech: seq<int>,fDead: bool)
  function Rank(xs: seq<Standing>,i: nat): nat requires i<|xs| {
    1+|set j | 0<=j<|xs| && xs[j].score>xs[i].score|
  }
  predicate FLeader(xs: seq<Standing>,i: nat) {
    i<|xs| && forall j | 0<=j<|xs| :: xs[j].score<=xs[i].score && (xs[j].score==xs[i].score ==> j<=i)
  }
  predicate FSecond(xs: seq<Standing>,leader: nat,second: nat) {
    FLeader(xs,leader) && second<|xs| && second!=leader &&
      forall j | 0<=j<|xs| && j!=leader :: xs[j].score<=xs[second].score
  }
  function Conditions(s: Standing,vc: seq<int>,planets: nat,turn: int,fSelected: bool,fUnique: bool,secondScore: nat): seq<bool>
    requires |vc|==10
  { [s.planets>=LMulDiv(planets,V.VCValue(vc,0),100),
     |set j | 0<=j<|s.tech| && s.tech[j]>=V.VCValue(vc,1)|>=V.VCValue(vc,2),
     s.score>=V.VCValue(vc,3),
     fSelected && s.score>=C.DivC(secondScore*(100+V.VCValue(vc,4)),100),
     s.resources/1000>=V.VCValue(vc,5),s.capital>=V.VCValue(vc,6),
     fUnique && C.Signed(turn,16)>=V.VCValue(vc,7)] }
  predicate FConditionMatrix(xs: seq<Standing>,vc: seq<int>,planets: nat,turn: int,leader: nat,second: nat,conditions: seq<seq<bool>>) {
    |vc|==10 && FSecond(xs,leader,second) && |conditions|==|xs| &&
    forall i | 0<=i<|xs| :: conditions[i]==Conditions(xs[i],vc,planets,turn,i==leader,
      i==leader && xs[leader].score>xs[second].score,xs[second].score)
  }
  predicate FDead(planets: nat,unarmed: nat,escort: nat,capital: nat) {
    planets==0 && unarmed==0 && escort==0 && capital==0
  }
  function Met(conditions: seq<bool>,vc: seq<int>): int
    requires |conditions|==7 && |vc|==10
  { Sum(seq(7,i requires 0<=i<7 => if conditions[i] && V.Enabled(vc,[0,1,3,4,5,6,7][i]) then 1 else 0)) }
  datatype Outcome = SinglePlayer | Elimination | Numeric(winners: set<int>)
  function Victory(xs: seq<Standing>,conditions: seq<seq<bool>>,vc: seq<int>,turn: int): Outcome
    requires |vc|==10 && |xs|==|conditions| && forall c <- conditions :: |c|==7
  { if |xs|==1 then SinglePlayer else if |set i | 0<=i<|xs| && xs[i].fDead|+1>=|xs| then Elimination
    else Numeric(set i | 0<=i<|xs| && turn>=V.VCValue(vc,9) && V.VCValue(vc,8)>=1 && Met(conditions[i],vc)>=V.VCValue(vc,8)) }
  lemma ScoringVectors() {
    assert ShipPoints(10,0,0,10)==40;
    assert Class(1999)==1 && Class(2000)==2;
    assert Pack(8192)==Packed(2048,1);
    assert Capacity(100,0,false,false,0)==5000;
    assert !FScoreVisible(false,false,false,true,19) && FScoreVisible(false,false,false,true,20);
    assert ResearchCost(0,1,0,true)==176;
  }
}
