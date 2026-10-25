include "S01-codecs.dfy"
include "S06-shared-state.dfy"

// Bounded shared arithmetic used by the existing M03/M04/M05 specifications.
// C refinement and floating trace production are separate obligations.
module S04Arithmetic {
  import C = S01Codecs
  import R = SharedGameState
  function Min(a: int,b: int): int { if a<b then a else b }
  function Max(a: int,b: int): int { if a>b then a else b }
  function Abs(a: int): nat { if a<0 then -a else a }
  function Clamp(a: int,lo: int,hi: int): int
    requires lo<=hi
    ensures lo<=Clamp(a,lo,hi)<=hi
  { Max(lo,Min(a,hi)) }
  function Sum(xs: seq<int>): int
    decreases |xs|
  { if |xs|==0 then 0 else xs[0]+Sum(xs[1..]) }
  function LMulDiv(a: int,b: int,c: int): int {
    if c==0 then -1 else
    var q := (Abs(a*b)+Abs(c)/2)/Abs(c);
    var v := if (a*b<0)!=(c<0) then -q else q;
    if -2147483647<=v<=2147483647 then v else -1
  }
  function Trunc(x: real): int { if x<0.0 then -((-x).Floor) else x.Floor }
  datatype DrawResult = DrawResult(rng: R.Rng,value: int)
  function Take(rng: R.Rng,bound: int): DrawResult {
    var next := R.Step(rng);
    var z := next.s1-next.s2;
    var positive := if z<1 then z+2147483562 else z;
    DrawResult(next,if bound<=0 then 0 else (positive%4294967296)%bound)
  }
  function SeedTable(): seq<int> {
    C.Primes()+[317,331,337,347,349,353,359,367,373,379,383,389,397,401,
      409,419,421,431,433,439,443,449,457,461,463,467,479,487,491,499,503,
      509,521,523,541,547,557,563,569,571,577,587,593,599,601,607,613,617,
      619,631,641,643,647,653,659,661,673,677,683,691,701,709,719,727]
  }
  function Seed(seed: int,fWide: bool): R.Rng {
    var a := if fWide then ((seed%128) as bv7 ^ 0x35) as int else seed%64;
    var b := if fWide then (((seed/128)%128) as bv7 ^ 0x5c) as int else (seed/64)%64;
    var modulus := if fWide then 128 else 64;
    var b1 := if a==b then (b+1)%modulus else b;
    R.Rng(SeedTable()[a],SeedTable()[b1])
  }
  datatype Draw = Draw(bound: int,value: int)
  predicate FDraws(rng: R.Rng,draws: seq<Draw>,next: R.Rng)
    decreases |draws|
  { if |draws|==0 then next==rng else
    var d := Take(rng,draws[0].bound);
    draws[0].value==d.value && FDraws(d.rng,draws[1..],next) }

  // Exact finite IEEE binary64 values. Magnitudes exclude max-finite so both
  // adjacent finite values exist. Existing game calculations lie well inside
  // this boundary. NaN, infinities, overflow and signed-zero identity are outside it.
  datatype F64 = F64(fNegative: bool,magnitude: nat)
  predicate FFinite(f: F64) { f.magnitude<9218868437227405311 }
  function MagValue(m: nat): real {
    var e := m/4503599627370496;
    var fraction := m%4503599627370496;
    if e==0 then (fraction as real)/(C.Pow2(1074) as real)
    else if e>=1075 then ((4503599627370496+fraction)*C.Pow2(e-1075)) as real
    else ((4503599627370496+fraction) as real)/(C.Pow2(1075-e) as real)
  }
  function Value(f: F64): real { (if f.fNegative then -1.0 else 1.0)*MagValue(f.magnitude) }
  function LowerMid(f: F64): real {
    if f.magnitude==0 then 0.0 else (MagValue(f.magnitude-1)+MagValue(f.magnitude))/2.0
  }
  function UpperMid(f: F64): real { (MagValue(f.magnitude)+MagValue(f.magnitude+1))/2.0 }
  predicate FNearest(x: real,f: F64) {
    FFinite(f) && (x<0.0 ==> f.fNegative) && (x>0.0 ==> !f.fNegative) &&
    var a := if x<0.0 then -x else x;
    (if f.magnitude==0 then a>=0.0 else
      a>LowerMid(f) || (a==LowerMid(f) && f.magnitude%2==0)) &&
    (a<UpperMid(f) || (a==UpperMid(f) && f.magnitude%2==0))
  }
  predicate FSqrt(x: real,f: F64) {
    x>=0.0 && FFinite(f) && !f.fNegative &&
    (if f.magnitude==0 then true else
      x>LowerMid(f)*LowerMid(f) || (x==LowerMid(f)*LowerMid(f) && f.magnitude%2==0)) &&
    (x<UpperMid(f)*UpperMid(f) || (x==UpperMid(f)*UpperMid(f) && f.magnitude%2==0))
  }
  datatype Expr = Integer(n: int) | Literal(bits: F64)
                | Add(a: Expr,b: Expr) | Multiply(a: Expr,b: Expr)
                | Divide(a: Expr,b: Expr) | SquareRoot(a: Expr)
  datatype Evaluation = Evaluation(result: F64,children: seq<Evaluation>)
  predicate FEvaluate(e: Expr,t: Evaluation)
    decreases e
  {
    FFinite(t.result) &&
    (match e
    case Integer(n) => |t.children|==0 && FNearest(n as real,t.result)
    case Literal(bits) => |t.children|==0 && t.result==bits
    case Add(a,b) => |t.children|==2 && FEvaluate(a,t.children[0]) && FEvaluate(b,t.children[1]) &&
      FNearest(Value(t.children[0].result)+Value(t.children[1].result),t.result)
    case Multiply(a,b) => |t.children|==2 && FEvaluate(a,t.children[0]) && FEvaluate(b,t.children[1]) &&
      FNearest(Value(t.children[0].result)*Value(t.children[1].result),t.result)
    case Divide(a,b) => |t.children|==2 && FEvaluate(a,t.children[0]) && FEvaluate(b,t.children[1]) &&
      Value(t.children[1].result)!=0.0 &&
      FNearest(Value(t.children[0].result)/Value(t.children[1].result),t.result)
    case SquareRoot(a) => (|t.children|==1 && FEvaluate(a,t.children[0]) &&
      FSqrt(Value(t.children[0].result),t.result)))
  }
  function Bias9(): Expr { Literal(F64(false,0x3feccccccccccccd)) }
  function Bias999(): Expr { Literal(F64(false,0x3feff7ced916872b)) }
  function BiasHalf(): Expr { Literal(F64(false,0x3fe0000000000000)) }
  predicate FFloatInt(e: Expr,t: Evaluation,n: int) {
    FEvaluate(e,t) && -2147483648<=n<=2147483647 && n==Trunc(Value(t.result))
  }
  lemma ArithmeticVectors() {
    assert C.DivC(-2,3)==0 && C.DivC(-3,3)==-1;
    assert LMulDiv(242,60,100)==145;
    assert LMulDiv(-1,1,2)==-1 && LMulDiv(1,1,2)==1;
    assert LMulDiv(1,1,0)==-1 && LMulDiv(2147483647,2,1)==-1;
    assert Take(R.Rng(3,5),0).rng==Take(R.Rng(3,5),10).rng;
    assert Take(R.Rng(3,5),0).value==0;
  }
  lemma ZeroFloatWitness() {
    assert FNearest(0.0,F64(false,0));
    assert FEvaluate(Integer(0),Evaluation(F64(false,0),[]));
    assert !FSqrt(-1.0,F64(false,0));
  }
  lemma NearestBounds(x: real,f: F64)
    requires FNearest(x,f)
    ensures LowerMid(f)<=(if x<0.0 then -x else x)<=UpperMid(f)
  {}
  lemma ZeroBoundAdvances(r: R.Rng,bound: int)
    requires bound<=0
    ensures Take(r,bound).rng==R.Step(r) && Take(r,bound).value==0
  {}
}
