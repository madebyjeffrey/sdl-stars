include "M08-native-economy.dfy"

// Native planet.c PctPlanetDesirability prefix; the binary64 expression remains
// canonical M04Pricing.DesireExpr with S04's checked evaluation witness.
module M08NativeHabitat {
  import E = M08NativeEconomy
  import S = S01
  import A = S04Arithmetic
  import P = M04Pricing
  import C = S01Codecs
  import R = M04Race
  datatype Fields = Fields(ideal: seq<E.I8>,low: seq<E.I8>,high: seq<E.I8>)
  predicate Shape(f: Fields) { |f.ideal|==3 && |f.low|==3 && |f.high|==3 }
  datatype Total = Total(positive: E.I32,negative: E.I32,modifier: E.I32)
  datatype Prefix = State(total: Total) | Undefined(reason: string)
  function Axis(x: E.I8,ideal: E.I8,low: E.I8,high: E.I8,t: Total): Prefix {
    if high<0 then
      (if E.Fits32(t.positive+10000) then State(t.(positive:=t.positive+10000)) else Undefined("immune habitat sum")) else
    if x<low || x>high then
      var gap:=if x<low then low-x else x-high;
      var negative:=t.negative+(if gap<15 then gap else 15);
      if E.Fits32(negative) then State(t.(negative:=negative)) else Undefined("negative habitat sum")
    else
      var distance:=A.Abs(x-ideal);
      var d:=if x<ideal then ideal-low else high-ideal;
      if d==0 then Undefined("habitat width division") else
      var pct:=E.Word(E.Div(E.Word(distance*100),d));
      var penalty:=E.Word(distance*2-d);
      var q:=E.Word(100-pct);
      var positive:=E.Signed32(t.positive+q*q);
      if penalty<=0 then State(t.(positive:=positive)) else
      var product:=t.modifier*E.Word(d*2-penalty);
      if !E.Fits32(product) then Undefined("habitat modifier product") else
      var divisor:=E.Word(d*2);
      if divisor==0 || (product==-2147483648 && divisor==-1) then Undefined("habitat modifier division") else
      State(Total(positive,t.negative,E.Div(product,divisor)))
  }
  function Accumulate(env: seq<E.I8>,f: Fields,at: nat,t: Total): Prefix
    requires |env|==3 && Shape(f) && at<=3
    decreases 3-at
  {
    if at==3 then State(t) else
    var step:=Axis(env[at],f.ideal[at],f.low[at],f.high[at],t);
    if step.Undefined? then step else Accumulate(env,f,at+1,step.total)
  }
  datatype Calculation = Integer(value: S.I16) | Floating(total: Total) | Invalid(reason: string)
  function Begin(env: seq<E.I8>,f: Fields): Calculation
    requires |env|==3 && Shape(f)
  {
    var h:=Accumulate(env,f,0,Total(0,0,10000));
    if h.Undefined? then Invalid(h.reason) else
    if h.total.negative!=0 then Integer(E.Word(-(h.total.negative%65536))) else Floating(h.total)
  }
  function Finish(t: Total,converted: E.I32): E.Number {
    var product:=converted*t.modifier;
    if !E.Fits32(product) then E.Undefined("habitat final product") else E.Value(E.Word(E.Div(product,10000)))
  }
  datatype Evidence = Missing | Float(trace: A.Evaluation,converted: E.I32)
  datatype Answer = Returned(value: S.I16) | NeedFloat(expression: A.Expr) | Failed(reason: string)
  function Desire(env: seq<E.I8>,f: Fields,evidence: Evidence): Answer
    requires |env|==3 && Shape(f)
  {
    var start:=Begin(env,f);
    if start.Invalid? then Failed(start.reason) else
    if start.Integer? then Returned(start.value) else
    var expression:=P.DesireExpr(start.total.positive);
    if evidence.Missing? || !A.FFloatInt(expression,evidence.trace,evidence.converted) then NeedFloat(expression) else
    var finish:=Finish(start.total,evidence.converted);
    if finish.Undefined? then Failed(finish.reason) else Returned(finish.value as S.I16)
  }
  lemma Signed32Preserves(n: int)
    requires E.Fits32(n)
    ensures E.Signed32(n)==n
  {
    assert C.Pow2(4)==16 && C.Pow2(8)==256 && C.Pow2(12)==4096 && C.Pow2(16)==65536;
    assert C.Pow2(20)==1048576 && C.Pow2(24)==16777216 && C.Pow2(28)==268435456 && C.Pow2(31)==2147483648 && C.Pow2(32)==4294967296;
  }
  lemma {:isolate_assertions} AxisRefines(a: R.Axis,x: E.I8,t: Total)
    requires R.FAxis(a,true) && 0<=t.positive<=20000 && 0<=t.negative<=30 && 0<=t.modifier<=10000
    ensures Axis(x,a.preferred as E.I8,a.minimum as E.I8,a.maximum as E.I8,t).State?
    ensures Axis(x,a.preferred as E.I8,a.minimum as E.I8,a.maximum as E.I8,t).total.positive==P.AxisTotal(a,x,P.HabitatTotal(t.positive,t.negative,t.modifier)).positive
    ensures Axis(x,a.preferred as E.I8,a.minimum as E.I8,a.maximum as E.I8,t).total.negative==P.AxisTotal(a,x,P.HabitatTotal(t.positive,t.negative,t.modifier)).negative
    ensures Axis(x,a.preferred as E.I8,a.minimum as E.I8,a.maximum as E.I8,t).total.modifier==P.AxisTotal(a,x,P.HabitatTotal(t.positive,t.negative,t.modifier)).modifier
  {
    // Use the proved narrowing identities rather than expanding modular
    // encodings inside the nonlinear habitat calculation.
    hide E.Word;
    hide E.Signed32;
    var expected:=P.AxisTotal(a,x,P.HabitatTotal(t.positive,t.negative,t.modifier));
    if a==R.Immune() {
      assert Axis(x,-1,-1,-1,t)==State(t.(positive:=t.positive+10000));
      assert expected==P.HabitatTotal(t.positive+10000,t.negative,t.modifier);
      return;
    } else if x<a.minimum {
      var gap:=a.minimum-x;var added:=if gap<15 then gap else 15;
      assert 0<added<=15;
      assert Axis(x,a.preferred as E.I8,a.minimum as E.I8,a.maximum as E.I8,t)==State(t.(negative:=t.negative+added));
      assert expected==P.HabitatTotal(t.positive,t.negative+added,t.modifier);
      return;
    } else if x>a.maximum {
      var gap:=x-a.maximum;var added:=if gap<15 then gap else 15;
      assert 0<added<=15;
      assert Axis(x,a.preferred as E.I8,a.minimum as E.I8,a.maximum as E.I8,t)==State(t.(negative:=t.negative+added));
      assert expected==P.HabitatTotal(t.positive,t.negative+added,t.modifier);
      return;
    } else {
      var distance:=A.Abs(x-a.preferred);
      var d:=if x<a.preferred then a.preferred-a.minimum else a.maximum-a.preferred;
      assert 0<d<=100 && distance<=d;
      E.WordPreserves(distance*100);
      PercentBounds(distance,d);
      E.WordPreserves(C.DivC(distance*100,d));
      E.WordPreserves(distance*2-d);
      var q:=100-C.DivC(distance*100,d);E.WordPreserves(q);
      assert 100*distance==distance*100;
      assert expected.positive==t.positive+q*q && expected.negative==t.negative;
      PositiveBounds(t.positive,q);
      Signed32Preserves(t.positive+q*q);
      var penalty:=distance*2-d;
      if penalty>0 {
        assert 0<=d*2-penalty<=200;
        E.WordPreserves(d*2-penalty);E.WordPreserves(d*2);
        ProductBounds(t.modifier,d*2-penalty);
        assert E.Fits32(t.modifier*(d*2-penalty));
        assert E.Div(t.modifier*(d*2-penalty),d*2)==C.DivC(t.modifier*(d*2-penalty),d*2);
        assert expected.modifier==C.DivC(t.modifier*(d*2-penalty),d*2);
        assert Axis(x,a.preferred as E.I8,a.minimum as E.I8,a.maximum as E.I8,t)==
          State(Total(t.positive+q*q,t.negative,C.DivC(t.modifier*(d*2-penalty),d*2)));
      } else {
        assert expected.modifier==t.modifier;
        assert Axis(x,a.preferred as E.I8,a.minimum as E.I8,a.maximum as E.I8,t)==State(Total(t.positive+q*q,t.negative,t.modifier));
      }
    }
  }
  lemma ProductBounds(a: int,b: int)
    requires 0<=a<=10000 && 0<=b<=200
    ensures 0<=a*b<=2000000
  {}
  lemma PercentBounds(distance: int,width: int)
    requires 0<=distance<=width && 0<width<=100
    ensures 0<=C.DivC(distance*100,width)<=100
  {
    var numerator:=distance*100;
    assert 0<=numerator<=width*100;
    assert numerator==numerator/width*width+numerator%width;
    assert 0<=numerator%width<width;
    assert numerator/width*width<=numerator;
    if numerator/width>100 {
      assert numerator/width*width>=101*width;
      assert 101*width>100*width;
    }
  }
  lemma PositiveBounds(prior: int,q: int)
    requires 0<=prior<=20000 && 0<=q<=100
    ensures 0<=q*q<=10000
    ensures 0<=prior+q*q<=30000 && E.Fits32(prior+q*q)
  {
    assert 0<=(100-q)*(100+q);
    assert 10000-q*q==(100-q)*(100+q);
  }
  lemma Immune(env: seq<E.I8>,f: Fields)
    requires |env|==3 && Shape(f) && forall i | 0<=i<3 :: f.high[i]<0
    ensures Begin(env,f)==Floating(Total(30000,0,10000))
  {
    E.WordPreserves(10000);E.WordPreserves(20000);E.WordPreserves(30000);
    assert Accumulate(env,f,2,Total(20000,0,10000))==State(Total(30000,0,10000));
    assert Accumulate(env,f,1,Total(10000,0,10000))==State(Total(30000,0,10000));
  }
  lemma {:isolate_assertions} Vectors() {
    assert C.Pow2(4)==16 && C.Pow2(8)==256 && C.Pow2(12)==4096 && C.Pow2(16)==65536;
    assert C.Pow2(20)==1048576 && C.Pow2(24)==16777216 && C.Pow2(28)==268435456 && C.Pow2(31)==2147483648 && C.Pow2(32)==4294967296;
    var f:=Fields([50,50,50],[25,25,25],[75,75,75]);
    E.WordPreserves(0);E.WordPreserves(-25);E.WordPreserves(100);
    E.WordPreserves(10000);E.WordPreserves(20000);E.WordPreserves(30000);E.WordPreserves(-45);
    assert Axis(50,50,25,75,Total(0,0,10000))==State(Total(10000,0,10000));
    assert Axis(50,50,25,75,Total(10000,0,10000))==State(Total(20000,0,10000));
    assert Axis(50,50,25,75,Total(20000,0,10000))==State(Total(30000,0,10000));
    assert Accumulate([50,50,50],f,2,Total(20000,0,10000))==State(Total(30000,0,10000));
    assert Accumulate([50,50,50],f,1,Total(10000,0,10000))==State(Total(30000,0,10000));
    assert Begin([50,50,50],f)==Floating(Total(30000,0,10000));
    assert Accumulate([0,0,0],f,2,Total(0,30,10000))==State(Total(0,45,10000));
    assert Accumulate([0,0,0],f,1,Total(0,15,10000))==State(Total(0,45,10000));
    assert Begin([0,0,0],f)==Integer(-45);
    assert Axis(50,50,50,50,Total(0,0,10000))==Undefined("habitat width division");
    assert Axis(99,50,25,-2,Total(0,0,10000))==State(Total(10000,0,10000));
  }
}
