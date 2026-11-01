include "M08-native-economy.dfy"
include "M19-resolution.dfy"

// util.c ChgPopFromPlanet and race.c PctTrueMaxGrowth. Native integer order,
// single carry adjustment and optional writeback; supplied helper snapshots
// are not a proof of pointer/lifetime/catalog correspondence.
module M19NativeGrowth {
  import S = S01
  import C = S01Codecs
  import A = S04Arithmetic
  import E = M08NativeEconomy
  import R = M19Resolution
  datatype Carry = Carry(change: int,delta: S.U8)
  function TrueRate(primary: E.I8,ideal: E.I8): int { if primary==0 then (ideal as int)*2 else ideal }
  function CarryGrowth(hundredths: E.I32,prior: S.U8): Carry {
    var whole:=C.DivC(hundredths,100);
    var fraction:=hundredths-whole*100;
    var small:=if whole==0 && fraction==0 then 1 else fraction;
    var delta:=E.Word((prior as int)+small);
    if delta>=100 then Carry(whole+1,(delta-100)%256)
    else if delta<0 then Carry(whole-1,(delta+100)%256) else Carry(whole,delta as S.U8)
  }
  function CarryLoss(hundredths: E.I32,prior: S.U8): Carry
    requires hundredths>=1
  {
    var whole:=C.DivC(hundredths,100);var fraction:=hundredths-whole*100;
    var delta:=prior-fraction;
    if delta<0 then Carry(-whole-1,(delta+100)%256) else Carry(-whole,delta as S.U8)
  }
  datatype Result = Computed(change: E.I32,population: E.I32,delta: S.U8)
    | Undefined(reason: string,population: E.I32,delta: S.U8)
  function Publish(pop: E.I32,prior: S.U8,carry: Carry,update: bool): Result
    requires E.Fits32(carry.change)
  {
    if !update then Computed(carry.change as E.I32,pop,prior)
    else if !E.Fits32(pop+carry.change) then Undefined("population writeback addition",pop,carry.delta)
    else Computed(carry.change as E.I32,(pop+carry.change) as E.I32,carry.delta)
  }
  function FinishGrowth(pop: E.I32,prior: S.U8,rate: E.I32,update: bool): Result {
    var rough:=pop*C.DivC(rate,100);
    if !E.Fits32(rough) then Undefined("rough growth multiplication",pop,prior) else
    if rough<10000000 && !E.Fits32(pop*rate) then Undefined("fine growth multiplication",pop,prior) else
    var hundredths:=if rough<10000000 then C.DivC(pop*rate,100) else rough;
    var carry:=CarryGrowth(hundredths as E.I32,prior);
    Publish(pop,prior,carry,update)
  }
  function Grow(owned: bool,pop: E.I32,prior: S.U8,hab: S.I16,capacity: E.Number,primary: E.I8,ideal: E.I8,update: bool): Result {
    if !owned || pop==0 then Computed(0,pop,prior) else
    if hab<0 then
      var damage:=E.Word(-(hab as int));var product: int:=pop*damage;
      if !E.Fits32(product) then Undefined("hostile loss multiplication",pop,prior) else
      var loss:=A.Max(1,C.DivC(product,10));
      var carry:=CarryLoss(loss as E.I32,prior);
      Publish(pop,prior,carry,update)
    else if capacity.Undefined? then Undefined(capacity.reason,pop,prior) else
      var initial:=E.Word(TrueRate(primary,ideal)*hab);
      var maximum:=capacity.value;
      if pop<=C.DivC(maximum,4) then FinishGrowth(pop,prior,initial,update) else
      var product: int:=pop*1000;
      if !E.Fits32(product) then Undefined("fullness multiplication",pop,prior) else
      var divided:=E.Division(product as E.I32,maximum);
      if divided.Undefined? then Undefined(divided.reason,pop,prior) else
      var fullness:=divided.value;
      if pop>=maximum then
        if !E.Fits32((maximum as int)+10) then Undefined("capacity dead-band addition",pop,prior) else
        if pop<maximum+10 then Computed(0,pop,prior) else
        var retard:=A.Max(-300,99-C.DivC(fullness,10));
        FinishGrowth(pop,prior,(retard*4) as E.I32,update)
      else
        var retard:=1000-fullness;
        if !E.Fits32(retard) || !E.Fits32(retard*retard) then Undefined("crowding retardation square",pop,prior) else
        var square:=retard*retard;
        var product:=if initial<1000 then initial*square else C.DivC(initial,10)*square;
        if !E.Fits32(product) then Undefined("crowded growth multiplication",pop,prior) else
        var rate:=C.DivC(product,562500)*(if initial<1000 then 1 else 10);
        if !E.Fits32(rate) then Undefined("crowded growth rescaling",pop,prior) else
        FinishGrowth(pop,prior,rate as E.I32,update)
  }
  function Calls(owned: bool,pop: E.I32,hab: S.I16): seq<string> {
    if !owned || pop==0 then [] else ["habitability"]+(if hab<0 then [] else ["capacity","true growth rate"])
  }
  lemma NoUpdatePreserves(pop: E.I32,prior: S.U8,hab: S.I16,capacity: E.Number,primary: E.I8,ideal: E.I8,owned: bool)
    ensures Grow(owned,pop,prior,hab,capacity,primary,ideal,false).population==pop
    ensures Grow(owned,pop,prior,hab,capacity,primary,ideal,false).delta==prior
  {}
  lemma {:isolate_assertions} NormalGrowthCarry(hundredths: E.I32,prior: S.U8)
    requires prior<100
    ensures CarryGrowth(hundredths,prior).change==R.Carry(if hundredths==0 then 1 else hundredths,prior).change
    ensures CarryGrowth(hundredths,prior).delta==R.Carry(if hundredths==0 then 1 else hundredths,prior).remainder
  {
    var whole:=C.DivC(hundredths,100);
    var fraction:=hundredths-whole*100;
    var small:=if hundredths==0 then 1 else fraction;
    assert -99<=fraction<=99;
    assert (whole==0 && fraction==0) <==> hundredths==0;
    var adjusted:=if hundredths==0 then 1 else hundredths;
    assert C.DivC(adjusted,100)==whole;
    assert adjusted-whole*100==small;
    E.WordPreserves((prior as int)+small);
    var delta: int:=prior+small;
    R.CarryBound(adjusted,prior);
    if delta>=100 {
      assert CarryGrowth(hundredths,prior)==Carry(whole+1,(delta-100)%256);
      assert R.Carry(adjusted,prior)==R.Growth(whole+1,delta-100);
    } else if delta<0 {
      assert CarryGrowth(hundredths,prior)==Carry(whole-1,(delta+100)%256);
      assert R.Carry(adjusted,prior)==R.Growth(whole-1,delta+100);
    } else {
      assert CarryGrowth(hundredths,prior)==Carry(whole,delta as S.U8);
      assert R.Carry(adjusted,prior)==R.Growth(whole,delta);
    }
  }
  lemma NormalLossCarry(hundredths: E.I32,prior: S.U8)
    requires prior<100 && hundredths>=1
    ensures CarryLoss(hundredths,prior).change==R.Carry(-hundredths,prior).change
    ensures CarryLoss(hundredths,prior).delta==R.Carry(-hundredths,prior).remainder
  {}
  lemma {:isolate_assertions} Vectors() {
    assert C.Pow2(4)==16 && C.Pow2(8)==256 && C.Pow2(12)==4096;
    assert C.Pow2(15)==32768 && C.Pow2(16)==65536;
    assert Grow(false,100,0,0,E.Undefined("unused"),1,10,true)==Computed(0,100,0);
    assert Grow(true,10005,37,100,E.Value(10000),1,10,true)==Computed(0,10005,37);
    E.WordPreserves(1);
    assert CarryLoss(1,0)==Carry(-1,99);
    assert Publish(1,0,Carry(-1,99),true)==Computed(-1,0,99);
    assert Grow(true,1,0,-1,E.Undefined("unused"),1,10,true)==Computed(-1,0,99);
    assert Grow(true,100,0,100,E.Value(10000),1,10,false)==Computed(10,100,0);
    E.WordPreserves(201);
    assert CarryGrowth(0,200)==Carry(1,101);
    assert Grow(true,3000000,0,100,E.Value(10000),1,10,false).Undefined?;
  }
}
