include "M04-race-model.dfy"

// Ordered formulas from M04-race-advantage-points.md / race.c / planet.c.
module M04Pricing {
  import opened M04Race
  import opened S04Arithmetic
  import C = S01Codecs
  datatype HabitatTotal = HabitatTotal(positive: int,negative: int,modifier: int)
  function AxisTotal(a: Axis,x: int,b: HabitatTotal): HabitatTotal
    requires FAxis(a,true)
  {
    if a==Immune() then b.(positive:=b.positive+10000)
    else if x<a.minimum then b.(negative:=b.negative+Min(15,a.minimum-x))
    else if x>a.maximum then b.(negative:=b.negative+Min(15,x-a.maximum))
    else var d := if x<a.preferred then a.preferred-a.minimum else a.maximum-a.preferred;
      var q := 100-C.DivC(100*Abs(x-a.preferred),d);
      var penalty := 2*Abs(x-a.preferred)-d;
      HabitatTotal(b.positive+q*q,b.negative,
        if penalty>0 then C.DivC(b.modifier*(2*d-penalty),2*d) else b.modifier)
  }
  function Habitat(r: Race,xyz: seq<int>): HabitatTotal
    requires FPricing(r,true) && |xyz|==3
  {
    AxisTotal(r.habitat[2],xyz[2],AxisTotal(r.habitat[1],xyz[1],
      AxisTotal(r.habitat[0],xyz[0],HabitatTotal(0,0,10000))))
  }
  function DesireExpr(positive: int): Expr {
    Add(SquareRoot(Divide(Integer(positive),Integer(3))),Bias9())
  }
  predicate FDesire(r: Race,xyz: seq<int>,trace: Evaluation,result: int) {
    FPricing(r,true) && |xyz|==3 &&
    var h := Habitat(r,xyz);
    if h.negative!=0 then result == -h.negative else
      FEvaluate(DesireExpr(h.positive),trace) &&
      result==C.DivC(Trunc(Value(trace.result))*h.modifier,10000)
  }
  function Terraform(r: Race,stage: nat): int
    requires stage<3
  { if stage==0 then 0 else if stage==1 then (if Trait(r,1) then 8 else 5) else (if Trait(r,1) then 17 else 15) }
  function Weight(stage: nat): int requires stage<3 { [7,5,6][stage] }
  datatype Samples = Samples(base: int,width: int,steps: nat)
  function AxisSamples(a: Axis,t: int): Samples {
    if a==Immune() then Samples(50,11,1)
    else var b := Max(0,a.minimum-t); Samples(b,Min(100,a.maximum+t)-b,11)
  }
  function Sample(a: Axis,t: int,j: nat): int {
    var d := AxisSamples(a,t);
    d.base+(if d.steps==1 then 0 else C.DivC(j*d.width,10))
  }
  function Residual(a: Axis,t: int,j: nat): int {
    if t==0 || a==Immune() then 0 else
    var delta := a.preferred-Sample(a,t,j);
    if Abs(delta)<=t then 0 else if delta<0 then delta+t else delta-t
  }
  function SamplePoint(a: Axis,t: int,j: nat): int {
    if t==0 || a==Immune() then Sample(a,t,j) else a.preferred-Residual(a,t,j)
  }
  function SampleXYZ(r: Race,s: nat,g: nat,t: nat,b: nat): seq<int>
    requires FPricing(r,true) && s<3
  { var allowance := Terraform(r,s);
    [SamplePoint(r.habitat[0],allowance,g),SamplePoint(r.habitat[1],allowance,t),SamplePoint(r.habitat[2],allowance,b)] }
  function SampleResidual(r: Race,s: nat,g: nat,t: nat,b: nat): int
    requires FPricing(r,true) && s<3
  { var allowance := Terraform(r,s);
    Residual(r.habitat[0],allowance,g)+Residual(r.habitat[1],allowance,t)+Residual(r.habitat[2],allowance,b) }
  function Steps(r: Race,axis: nat): nat
    requires FShape(r) && axis<3
    ensures Steps(r,axis)>0
  { if r.habitat[axis]==Immune() then 1 else 11 }
  datatype StageData = StageData(desires: seq<int>,traces: seq<Evaluation>)
  predicate FStageShape(r: Race,d: StageData) {
    FPricing(r,true) && |d.desires|==Steps(r,0)*Steps(r,1)*Steps(r,2) && |d.traces|==|d.desires|
  }
  function Index(r: Race,g: nat,t: nat,b: nat): nat
    requires FShape(r)
  { (g*Steps(r,1)+t)*Steps(r,2)+b }
  predicate FStage(r: Race,s: nat,d: StageData) {
    s<3 && FStageShape(r,d) &&
    forall g,t,b | 0<=g<Steps(r,0) && 0<=t<Steps(r,1) && 0<=b<Steps(r,2) ::
      FDesire(r,SampleXYZ(r,s,g,t,b),d.traces[Index(r,g,t,b)],d.desires[Index(r,g,t,b)])
  }
  function Term(r: Race,s: nat,g: nat,t: nat,b: nat,d: int): int
    requires FPricing(r,true) && s<3
  { var rem := SampleResidual(r,s,g,t,b); var allowance := Terraform(r,s);
    var adjusted := if rem>allowance then Max(0,d-(rem-allowance)) else d;
    adjusted*adjusted*Weight(s) }
  function RadiationSum(r: Race,s: nat,g: nat,t: nat,count: nat,d: StageData): int
    requires s<3 && FStageShape(r,d) && g<Steps(r,0) && t<Steps(r,1) && count<=Steps(r,2)
    decreases count
  { if count==0 then 0 else (RadiationSum(r,s,g,t,count-1,d)+
      Term(r,s,g,t,count-1,d.desires[Index(r,g,t,count-1)])) }
  function Radiation(r: Race,s: nat,g: nat,t: nat,d: StageData): int
    requires s<3 && FStageShape(r,d) && g<Steps(r,0) && t<Steps(r,1)
  { var sum := RadiationSum(r,s,g,t,Steps(r,2),d);
    if r.habitat[2]==Immune() then sum*11 else C.DivC(sum*AxisSamples(r.habitat[2],Terraform(r,s)).width,100) }
  function TemperatureSum(r: Race,s: nat,g: nat,count: nat,d: StageData): Expr
    requires s<3 && FStageShape(r,d) && g<Steps(r,0) && count<=Steps(r,1)
    decreases count
  { if count==0 then Integer(0) else
      Add(Integer(Radiation(r,s,g,count-1,d)),TemperatureSum(r,s,g,count-1,d)) }
  function Scale(e: Expr,a: Axis,allowance: int): Expr {
    if a==Immune() then Multiply(e,Integer(11)) else
      Divide(Multiply(e,Integer(AxisSamples(a,allowance).width)),Integer(100))
  }
  function GravitySum(r: Race,s: nat,count: nat,d: StageData): Expr
    requires s<3 && FStageShape(r,d) && count<=Steps(r,0)
    decreases count
  { if count==0 then Integer(0) else Add(GravitySum(r,s,count-1,d),
      Scale(TemperatureSum(r,s,count-1,Steps(r,1),d),r.habitat[1],Terraform(r,s))) }
  function StageExpr(r: Race,s: nat,d: StageData): Expr
    requires s<3 && FStageShape(r,d)
  { Scale(GravitySum(r,s,Steps(r,0),d),r.habitat[0],Terraform(r,s)) }
  function InnateExpr(r: Race,data: seq<StageData>): Expr
    requires |data|==3 && forall d <- data :: FStageShape(r,d)
  {
    Add(Divide(Add(Add(Add(Integer(0),StageExpr(r,0,data[0])),StageExpr(r,1,data[1])),
      StageExpr(r,2,data[2])),Integer(10)),BiasHalf())
  }
  predicate FInnate(r: Race,data: seq<StageData>,trace: Evaluation,result: int) {
    |data|==3 && (forall s | 0<=s<3 :: FStage(r,s,data[s])) &&
    FFloatInt(InnateExpr(r,data),trace,result)
  }

  function ImmuneCount(r: Race): int requires FShape(r) {
    (if r.habitat[0]==Immune() then 1 else 0)+(if r.habitat[1]==Immune() then 1 else 0)+
    (if r.habitat[2]==Immune() then 1 else 0)
  }
  function GrowthCredit(g: int): int {
    if g<=5 then (6-g)*4200 else if g==6 then 3600 else if g==7 then 2250
    else if g==8 then 600 else if g==9 then 225 else 0
  }
  function GrowthScale(g: int): int {
    if g<=5 then g else if g<=13 then 2*(g-5)+5 else if g<20 then 3*(g-13)+21 else 45
  }
  function Factory(a: seq<int>): int requires |a|==16 {
    var x := 10-a[1]; var y := 10-a[2]; var z := 10-a[3];
    var raw := (if x>0 then 100*x else 121*x)-(if y<0 then 55*y else 60*y*y)+(if z>0 then 40*z else 35*z);
    (if raw>700 then 700+C.DivC(raw-700,3) else raw)-
      (if -11<=z<=-7 then 30*(-6-z) else if -14<=z<=-12 then 45*(-12-z)+225 else if z < -14 then 360 else 0)-
      (if x<=-3 then 60*(-2-x) else 0)
  }
  function Mines(a: seq<int>): int requires |a|==16 {
    var x := 10-a[4]; var y := 3-a[5]; var z := 10-a[6];
    (if x>0 then 100*x else 169*x)-(if y<=0 then 65*y-80 else 360)+(if z>0 then 40*z else 35*z)
  }
  function TraitPrices(): seq<int> { [-235,-25,-159,-201,40,-240,-155,160,240,255,325,180,70,30] }
  function TraitSum(r: Race,n: nat): int
    requires n<=14 decreases n
  { if n==0 then 0 else TraitSum(r,n-1)+(if Trait(r,n-1) then TraitPrices()[n-1] else 0) }
  function TraitCount(r: Race,n: nat,fNegative: bool): int
    requires n<=14 decreases n
  { if n==0 then 0 else (TraitCount(r,n-1,fNegative)+
      (if Trait(r,n-1) && (TraitPrices()[n-1]<0)==fNegative then 1 else 0)) }
  function Research(a: seq<int>): int
    requires |a|==16 && forall i | 8<=i<14 :: 0<=a[i]<=2
  {
    var net := a[8]+a[9]+a[10]+a[11]+a[12]+a[13]-6;
    if net>0 then -130*net*net+(if net==6 then 1430 else if net==5 then 520 else 0)
    else if net<0 then [150,330,540,780,1050,1380][-net-1]-(if -net>4 && a[0]<10 then 190 else 0)
    else 0
  }
  function PointsInternal(r: Race,innate: int): int
    requires FPricing(r,true)
  {
    var a := r.attributes; var primary := a[14]; var immune := ImmuneCount(r);
    var axisCredit := Sum(seq(3,i requires 0<=i<3 => if r.habitat[i]==Immune() then 0 else 4*Abs(r.habitat[i].preferred-50)));
    var interaction := if a[3]>10 || a[1]>10 then
      C.DivC(Max(1,a[3]-9)*Max(1,a[1]-9)*(if primary==0 then 3 else 2)*r.growth,if immune>=2 then 2 else 9) else 0;
    var pop := if a[0]==7 then -2400 else if a[0]==8 then -1260 else if a[0]==9 then -600 else 120*(a[0]-10);
    var negative := TraitCount(r,14,true); var positive := TraitCount(r,14,false); var count := negative+positive;
    1650+GrowthCredit(r.growth)-C.DivC(C.DivC(innate,2000)*GrowthScale(r.growth),24)+axisCredit-
      (if immune>1 then 150 else 0)-interaction+pop+
      (if primary==8 then 210 else Factory(a)+Mines(a)-(if Trait(r,31) then 175 else 0))-
      [40,95,45,10,-100,-150,120,180,90,-66][primary]+TraitSum(r,14)-
      (if count>4 then 10*count*(count-4) else 0)-Max(0,positive-negative-3)*60-Max(0,negative-positive-3)*40-
      (if Trait(r,10) then (if primary==6 then 280 else if primary==1 then 200 else if primary==9 then 40 else 0) else 0)+
      Research(a)-(if Trait(r,29) then 180 else 0)-(if primary==8 && a[8]==2 then 100 else 0)
  }
  function Balance(r: Race,innate: int): int
    requires FPricing(r,true)
  { C.Signed(C.DivC(PointsInternal(r,innate),3),16) }
  predicate FPrice(original: Race,normalized: Race,data: seq<StageData>,trace: Evaluation,innate: int,balance: int) {
    FShape(original) && normalized==Bounds(original,true) && FPricing(normalized,true) &&
    FInnate(normalized,data,trace,innate) && balance==Balance(normalized,innate)
  }
  function GoldenRace(): Race {
    Race([],[],0,0,[Axis(31,51,71),Axis(61,79,97),Axis(9,41,73)],10,
      [10,10,10,10,10,5,10,0,1,1,1,1,1,1,9,0],0,false)
  }
  lemma EmptyTraits(r: Race,n: nat)
    requires n<=14 && r.traits==0
    ensures TraitSum(r,n)==0 && TraitCount(r,n,true)==0 && TraitCount(r,n,false)==0
    decreases n
  { if n>0 { EmptyTraits(r,n-1); } }
  lemma {:isolate_assertions} {:fuel C.Pow2, 18} {:fuel Sum, 5} PricingVectors() {
    EmptyTraits(GoldenRace(),14);
    assert FPricing(GoldenRace(),true);
    assert Research(GoldenRace().attributes)==0;
    assert !Trait(GoldenRace(),29) && !Trait(GoldenRace(),31);
    assert PointsInternal(GoldenRace(),1470287)==1623;
    assert Balance(GoldenRace(),1470287)==541;
    assert C.DivC(-1,3)==0 && C.DivC(-2,3)==0 && C.DivC(-3,3)==-1;
    assert Sample(Axis(10,10,11),0,1)==10;
    assert Residual(Axis(10,20,30),5,0)==10;
    assert GrowthScale(6)==7 && GrowthScale(20)==45;
  }
}
