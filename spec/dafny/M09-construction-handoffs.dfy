include "M09-builder.dfy"
include "M09-forecast.dfy"

// Numeric builder -> forecast adapter. Native helper/pointer/global frames
// remain separate; a supplied finite trace is never treated as completed work.
module M09ConstructionHandoffs {
  import B = M09Builder
  import F = M09Forecast
  import R = M09Presets
  import P = M09Production
  import S = S01
  function PartialIn(p: F.Partial): B.Partial {
    match p case CountOnlyZero => B.Empty case Known(row) => B.Value(row)
  }
  function PartialOut(p: B.Partial): F.Partial
    requires !p.Absent?
  { if p.Empty? then F.CountOnlyZero else F.Known(p.row) }
  function Begin(s: F.State,cost: seq<int>,limits: B.Limits): B.State
    requires F.Fits(s) && s.index<|s.queue| && B.LimitsFit(limits)
    requires |cost|==4 && forall n <- cost :: B.I32(n)
    ensures B.Fits(Begin(s,cost,limits))
    ensures Begin(s,cost,limits).partial.Absent? <==> s.index==-1
  {
    B.Start(F.RowAt(s),if s.index==-1 then B.Absent else PartialIn(s.partial),cost,
      seq(4,i requires 0<=i<4 => s.resources[i] as int),limits)
  }
  lemma Presence(s: B.State,x: B.Context,fuel: nat)
    requires B.Fits(s)
    ensures B.Run(s,x,fuel).partial.Absent? <==> s.partial.Absent?
    decreases fuel
  {
    if fuel>0 && !s.phase.Done? && !s.phase.Undefined? {
      assert B.Step(s,x).partial.Absent? <==> s.partial.Absent?;
      Presence(B.Step(s,x),x,fuel-1);
    }
  }
  function Resources(b: B.State): seq<F.I32>
    requires B.Fits(b)
    ensures |Resources(b)|==4
    ensures forall i | 0<=i<4 :: Resources(b)[i]==b.resources[i]
  {
    assert forall i | 0<=i<4 :: b.resources[i] in b.cost+b.paid+b.resources;
    assert forall i | 0<=i<4 :: B.I32(b.resources[i]);
    var values: seq<F.I32>:=seq(4,i requires 0<=i<4 => b.resources[i] as F.I32);
    values
  }
  function Resume(s: F.State,b: B.State,request: S.U16): F.Transition
    requires F.Fits(s) && s.year<100 && s.index<|s.queue|
    requires B.Fits(b) && b.phase.Done?
    requires b.partial.Absent? <==> s.index==-1
    ensures F.Fits(Resume(s,b,request).state)
    ensures Resume(s,b,request).state.resources==Resources(b)
  {
    var revised:=F.SetRow(s,b.caller);
    assert F.Fits(revised);
    var after:=revised.(resources:=Resources(b),
      partial:=if s.index==-1 then F.Known(b.caller) else PartialOut(b.partial));
    assert F.Fits(after);
    F.Step(after,F.AfterBuild,request,0,0,0,b.built as S.I16,P.StatusWord(b.status) as S.U16)
  }
  datatype Result = Returned(builder: B.State,forecast: F.Transition)
    | Pending(builder: B.State) | UndefinedBuilder(builder: B.State)
  function Execute(s: F.State,cost: seq<int>,limits: B.Limits,cheap: bool,generating: bool,request: S.U16,fuel: nat): Result
    requires F.Fits(s) && s.year<100 && s.index<|s.queue| && B.LimitsFit(limits)
    requires |cost|==4 && forall n <- cost :: B.I32(n)
    ensures Execute(s,cost,limits,cheap,generating,request,fuel).Returned? ==>
      Execute(s,cost,limits,cheap,generating,request,fuel).builder.phase.Done?
  {
    var start:=Begin(s,cost,limits);
    var x:=B.Context(s.alchemy,cheap,false,generating);
    var b:=B.Run(start,x,fuel);
    Presence(start,x,fuel);
    if b.phase.Done? then Returned(b,Resume(s,b,request))
    else if b.phase.Undefined? then UndefinedBuilder(b) else Pending(b)
  }
  lemma NoFuelIsPending(s: F.State,cost: seq<int>,limits: B.Limits,cheap: bool,generating: bool,request: S.U16)
    requires F.Fits(s) && s.year<100 && s.index<|s.queue| && B.LimitsFit(limits)
    requires |cost|==4 && forall n <- cost :: B.I32(n)
    ensures Execute(s,cost,limits,cheap,generating,request,0).Pending?
  {}
}
