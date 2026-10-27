include "M08-colony.dfy"
include "S01-codecs.dfy"

// planet.c caller/native-width contracts. Existing M03 capacity/population and
// binary64 expression definitions remain canonical; helper frames are separate.
module M08NativeEconomy {
  import S = S01
  import C = S01Codecs
  import A = S04Arithmetic
  import M = M03Scoring
  import O = M08Colony
  type I32 = n: int | -2147483648<=n<2147483648
  type I8 = n: int | -128<=n<128
  predicate Fits32(n: int) { -2147483648<=n<2147483648 }
  function Word(n: int): S.I16 {
    assert C.Pow2(4)==16 && C.Pow2(8)==256 && C.Pow2(12)==4096;
    assert C.Pow2(15)==32768 && C.Pow2(16)==65536;
    C.Signed(n,16) as S.I16
  }
  lemma WordPreserves(n: int)
    requires -32768<=n<32768
    ensures Word(n)==n
  {
    assert C.Pow2(4)==16 && C.Pow2(8)==256 && C.Pow2(12)==4096;
    assert C.Pow2(15)==32768 && C.Pow2(16)==65536;
  }
  function Signed32(n: int): I32 {
    assert C.Pow2(4)==16 && C.Pow2(8)==256 && C.Pow2(12)==4096 && C.Pow2(16)==65536;
    assert C.Pow2(20)==1048576 && C.Pow2(24)==16777216 && C.Pow2(28)==268435456;
    assert C.Pow2(31)==2147483648 && C.Pow2(32)==4294967296;
    C.Signed(n,32) as I32
  }
  datatype Number = Value(value: I32) | Undefined(reason: string)
  function Result(n: int,reason: string): Number { if Fits32(n) then Value(n as I32) else Undefined(reason) }
  function Div(n: int,d: int): int requires d!=0 { if d>0 then C.DivC(n,d) else -C.DivC(n,-d) }
  function Division(n: I32,d: int): Number {
    if d==0 || (n==-2147483648 && d==-1) then Undefined("integer division") else Result(Div(n,d),"integer quotient")
  }
  function Capacity(found: bool,primary: I8,ownerMatches: bool,base: bool,baseSlot: nat,hull: int,hab: S.I16,obrm: bool): Number
    requires baseSlot<16
  {
    if !found then Undefined("lookup leaves capacity snapshot undefined") else
    if primary==8 && (!ownerMatches || !base) then Value(0) else
    if primary==8 && (baseSlot>=10 || hull<0 || hull>=5) then Undefined("AR base/population table index") else
    Result(M.Capacity(hab,primary,obrm,ownerMatches && base,hull),"capacity arithmetic")
  }
  function CapacityCalls(primary: I8,ownerMatches: bool,base: bool): seq<string> {
    ["lookup","primary"]+(if primary==8 then
      (if !ownerMatches || !base then [] else ["base hull","OBRM"])
      else ["habitability","primary"]+(if primary==0 then [] else ["primary"])+["OBRM"])
  }
  function Absolute(capacity: Number,operate: I8,primary: I8): Number
    ensures Absolute(capacity,operate,primary).Value? ==> -32768<=Absolute(capacity,operate,primary).value<32768
  {
    if capacity.Undefined? then capacity else
    var product: int:=capacity.value*operate;
    if !Fits32(product) then Undefined("absolute installation multiplication") else
    var count:=A.Max(10,C.DivC(product,100));
    Value(if primary==8 then 0 else Word(count))
  }
  function NextPopulation(pop: I32,next: bool,growth: Number): Number {
    if !next then Value(pop) else if growth.Undefined? then growth else Result(pop+growth.value,"next population addition")
  }
  function Operable(absolute: Number,pop: I32,operate: I8,next: bool,growth: Number,primary: I8): Number
    requires absolute.Value? ==> -32768<=absolute.value<32768
    ensures Operable(absolute,pop,operate,next,growth,primary).Value? ==> -32768<=Operable(absolute,pop,operate,next,growth,primary).value<32768
  {
    if absolute.Undefined? then absolute else
    var p:=NextPopulation(pop,next,growth);
    if p.Undefined? then p else
    var product: int:=p.value*operate;
    if !Fits32(product) then Undefined("operable installation multiplication") else
    var current:=C.DivC(product,100);
    // Compare signed-16 maximum with signed-32 current BEFORE narrowing.
    var count:=if absolute.value<current then absolute.value else Word(current);
    Value(if primary==8 then 0 else if count<=0 then 1 else count as I32)
  }
  function DefenseMaximum(hab: S.I16,primary: I8): S.I16 {
    (if primary==8 then 0 else A.Clamp((hab as int)*4,10,100)) as S.I16
  }
  function DefenseOperable(maximum: S.I16,pop: I32,next: bool,growth: Number,primary: I8): Number {
    var p:=NextPopulation(pop,next,growth);
    if p.Undefined? then p else
    if !Fits32(p.value+24) then Undefined("defense population rounding addition") else
    var current:=A.Min(1000,C.DivC(p.value+24,25));
    var count:=if maximum>=Word(current) then Word(current) else maximum;
    Value(if primary==8 then 0 else count)
  }
  function OperatingFactories(owned: bool,primary: I8,built: nat,limit: Number): Number
    requires built<4096
    requires owned && primary!=8 && limit.Value? ==> -32768<=limit.value<32768
  { if !owned || primary==8 then Value(0) else if limit.Undefined? then limit else Result(A.Min(built,limit.value),"factory operation") }
  datatype Calculation = Integer(result: Number) | Floating(expression: A.Expr,minimumOne: bool)
  function OperatingMines(owned: bool,primary: I8,built: nat,pop: I32,limit: Number): Calculation
    requires built<4096
    requires owned && primary!=8 && limit.Value? ==> -32768<=limit.value<32768
  {
    if !owned then Integer(Value(0)) else if primary==8 then Floating(A.SquareRoot(A.Integer(pop)),false)
    else Integer(if limit.Undefined? then limit else Result(A.Min(built,limit.value),"mine operation"))
  }
  function Effective(pop: I32,capacity: I32): Number {
    if pop<=capacity then Value(pop) else
    if !Fits32((pop as int)-capacity) then Undefined("effective population subtraction") else
    var half:=C.DivC((pop as int)-capacity,2)+capacity;
    if !Fits32(half) || !Fits32((capacity as int)*2) then Undefined("effective population cap") else
    Result(M.EffectivePopulation(pop,capacity),"effective population result")
  }
  function NonzeroWord(n: int): S.I16
    ensures NonzeroWord(n)!=0
  { var word:=Word(n); if word==0 then 1 else word }
  function Resources(pop: I32,capacity: Number,efficiency: I8,primary: I8,operableFactories: Number,built: nat,productivity: I8,
    energy: I8,hab: S.I16): Calculation
    requires built<4096
    requires pop!=0 && primary!=8 && operableFactories.Value? ==> -32768<=operableFactories.value<32768
    ensures (Resources(pop,capacity,efficiency,primary,operableFactories,built,productivity,energy,hab).Integer? &&
      Resources(pop,capacity,efficiency,primary,operableFactories,built,productivity,energy,hab).result.Value?) ==>
      -32768<=Resources(pop,capacity,efficiency,primary,operableFactories,built,productivity,energy,hab).result.value<32768
  {
    if pop==0 then Integer(Value(0)) else if capacity.Undefined? then Integer(capacity) else
    var effective:=Effective(pop,capacity.value);
    if effective.Undefined? then Integer(effective) else
    if primary==8 then Floating(M.ARExpr(pop,capacity.value,energy,efficiency,hab),true) else
    var colonists:=Division(effective.value,efficiency);
    if colonists.Undefined? then Integer(colonists) else
    if operableFactories.Undefined? then Integer(operableFactories) else
    var factories:=A.Min(built,operableFactories.value);
    var product:=factories*productivity;
    if !Fits32(product) then Integer(Undefined("factory resource multiplication")) else
    // Source casts the unsigned product+9 back to int32 BEFORE signed /10.
    var factoryResources:=C.DivC(Signed32(product+9),10);
    Integer(Value(NonzeroWord((Word(colonists.value) as int)+factoryResources)))
  }
  function FinishFloat(calculation: Calculation,trace: A.Evaluation,value: int): Number
    requires calculation.Floating? && A.FFloatInt(calculation.expression,trace,value)
    ensures FinishFloat(calculation,trace,value).Value?
  { Value(if calculation.minimumOne then NonzeroWord(value) else Word(value)) }
  lemma NormalResourceFinish(value: int)
    requires 0<=value<32768
    ensures NonzeroWord(value)==Word(A.Max(1,value))
  { WordPreserves(value);WordPreserves(A.Max(1,value)); }
  lemma FloatingResourceNonzero(calculation: Calculation,trace: A.Evaluation,value: int)
    requires calculation.Floating? && calculation.minimumOne && A.FFloatInt(calculation.expression,trace,value)
    ensures FinishFloat(calculation,trace,value).value!=0
  {}
  lemma CapacityBound(hab: S.I16,primary: I8,obrm: bool,ownedBase: bool,hull: int)
    requires primary==8 && ownedBase ==> 0<=hull<5
    ensures 0<=M.Capacity(hab,primary,obrm,ownedBase,hull)<=4325244
  {}
  lemma CapacityDefined(primary: I8,ownerMatches: bool,base: bool,slot: nat,hull: int,hab: S.I16,obrm: bool)
    requires slot<16
    requires primary==8 && ownerMatches && base ==> slot<10 && 0<=hull<5
    ensures Capacity(true,primary,ownerMatches,base,slot,hull,hab,obrm).Value?
    ensures 0<=Capacity(true,primary,ownerMatches,base,slot,hull,hab,obrm).value<=4325244
  { if primary!=8 || (ownerMatches && base) { CapacityBound(hab,primary,obrm,ownerMatches && base,hull); } }
  lemma AbsoluteProjection(capacity: I32,operate: I8,primary: I8)
    requires capacity>=0 && operate>=0 && Fits32((capacity as int)*operate)
    requires primary!=8 ==> A.Max(10,C.DivC((capacity as int)*operate,100))<32768
    ensures Absolute(Value(capacity),operate,primary).Value?
    ensures Absolute(Value(capacity),operate,primary).value==O.Installations(capacity as nat,operate as nat,primary==8)
  {
    if primary!=8 { WordPreserves(A.Max(10,C.DivC((capacity as int)*operate,100))); }
  }
  lemma {:isolate_assertions} OperableProjection(capacity: I32,pop: I32,operate: I8)
    requires capacity>=0 && pop>=0 && operate>=0
    requires Fits32((capacity as int)*operate) && Fits32((pop as int)*operate)
    requires A.Max(10,C.DivC((capacity as int)*operate,100))<32768
    ensures Operable(Absolute(Value(capacity),operate,1),pop,operate,false,Undefined("unused"),1).Value?
    ensures Operable(Absolute(Value(capacity),operate,1),pop,operate,false,Undefined("unused"),1).value==
      O.Operable(pop,capacity as nat,operate as nat,false)
  {
    AbsoluteProjection(capacity,operate,1);
    var maximum:=Absolute(Value(capacity),operate,1).value;
    var current:=C.DivC((pop as int)*operate,100);
    if current<=maximum { WordPreserves(current); }
  }
  function DisplayCapacity(pop: I32,capacity: I32): Number {
    if capacity<=0 then Value(0) else
    var product: int:=pop*100;
    if !Fits32(product) then Undefined("capacity percentage multiplication") else
    var numerator:=Signed32(product+C.DivC(capacity,2));
    var percentage:=C.DivC(numerator,capacity);
    Value(if percentage>=1000 then 999 else Word(percentage))
  }
  function ResourceCalls(pop: I32,ar: bool): seq<string> {
    if pop==0 then [] else (["resource efficiency","population capacity","primary"]+
      (if ar then ["energy","habitability"] else ["operable factories","factory productivity"]))
  }
  lemma NonzeroOrdinary(pop: I32,capacity: Number,e: I8,primary: I8,limit: Number,built: nat,prod: I8,energy: I8,hab: S.I16)
    requires built<4096 && pop!=0 && primary!=8
    requires limit.Value? ==> -32768<=limit.value<32768
    ensures (Resources(pop,capacity,e,primary,limit,built,prod,energy,hab).Integer? &&
      Resources(pop,capacity,e,primary,limit,built,prod,energy,hab).result.Value?) ==>
      Resources(pop,capacity,e,primary,limit,built,prod,energy,hab).result.value!=0
  {}
  lemma {:isolate_assertions} Vectors() {
    assert C.Pow2(4)==16 && C.Pow2(8)==256 && C.Pow2(12)==4096;
    assert C.Pow2(15)==32768 && C.Pow2(16)==65536;
    assert C.Pow2(20)==1048576 && C.Pow2(24)==16777216 && C.Pow2(28)==268435456;
    assert C.Pow2(31)==2147483648 && C.Pow2(32)==4294967296;
    assert Capacity(true,8,false,true,15,-1,100,true)==Value(0);
    assert Capacity(true,8,true,true,10,0,100,false).Undefined?;
    assert Capacity(true,9,false,false,0,0,100,true)==Value(13200);
    assert Absolute(Value(10000),10,1)==Value(1000);
    assert Operable(Value(1000),655360,10,false,Undefined("unused"),1)==Value(1000);
    assert Operable(Value(1000),-327690,10,false,Value(0),1)==Value(32767);
    assert DefenseOperable(100,-1000024,false,Value(0),1)==Value(100);
    assert Resources(0,Undefined("unused"),0,1,Undefined("unused"),0,0,0,0)==Integer(Value(0));
    assert Resources(10000,Value(10000),1,1,Value(12700),4095,127,0,100)==Integer(Value(-3529));
    assert NonzeroWord(65536)==1 && NonzeroWord(65535)==-1;
    assert DisplayCapacity(12500,10000)==Value(125);
  }
}
