include "M12-host-research.dfy"

// Well-founded control progress for normal 0..26 levels. This rules out an
// infinite represented host transition sequence; terminal model boundaries
// still require separate native domain/service refinement before claiming Done.
module M12HostTermination {
  import H = M12HostResearch
  import Q = M12DevelopmentQuotes
  import E = M08NativeEconomy
  lemma FieldBound(levels: seq<E.I8>)
    requires forall i | 0<=i<|levels| :: 0<=levels[i]<=26
    ensures 0<=Q.Total(levels)<=26*|levels|
    decreases |levels|
  { if |levels|>0 { FieldBound(levels[1..]); } }
  lemma TotalBound(players: seq<H.Player>)
    requires forall i | 0<=i<|players| :: (H.PlayerShape(players[i]) &&
      (forall j | 0<=j<6 :: 0<=players[i].levels[j]<=26))
    ensures 0<=H.LevelTotal(players)<=156*|players|
    decreases |players|
  {
    reveal H.LevelTotal();
    if |players|>0 {
      FieldBound(players[0].levels);
      TotalBound(players[1..]);
    }
  }
  function Debt(s: H.State): nat
    requires H.Shape(s) && H.NormalLevels(s)
  { TotalBound(s.players);(156*|s.players|-H.LevelTotal(s.players)) as nat }
  function Stage(s: H.State): nat
    ensures Stage(s)<3
  { if s.phase==H.EndGrants then 0 else if s.phase in {H.GrantPlayer,H.Grants,H.GrantMessage} then 1 else 2 }
  function GeneralWork(s: H.State): nat
    ensures GeneralWork(s)<2
  {
    if Stage(s)!=2 then 0 else if s.phase==H.StartPlayer then 1
    else if s.usingPool && s.general==1 then 1 else 0
  }
  function RedoWork(s: H.State): nat
    ensures RedoWork(s)<2
  {
    if Stage(s)!=2 || s.phase in {H.StartPlayer,H.StartPass} then 0
    else if s.phase in {H.Announce,H.Redirect} || s.redo then 1 else 0
  }
  function FieldWork(s: H.State): nat
    requires H.Shape(s)
    ensures FieldWork(s)<8
  {
    if s.phase in {H.StartPlayer,H.StartPass} then 6 else if s.phase in {H.EndPlayer,H.EndGrants} then 0
    else if s.phase==H.GrantPlayer then 7 else 6-s.field
  }
  function PhaseWork(s: H.State): nat
    requires H.Shape(s)
    ensures PhaseWork(s)<128
  {
    match s.phase
    case StartPlayer => 100
    case StartPass => 90
    case Load => 80
    case PrimaryPool => 75
    case OtherPools => 60+(6-s.other)
    case Check => 50
    case Announce => 70
    case Redirect => 60
    case Store => 40
    case NextField => 10
    case EndPlayer => 0
    case GrantPlayer => 30
    case Grants => 20
    case GrantMessage => 10
    case EndGrants => 0
  }
  function Rank(s: H.State): nat
    requires H.Shape(s) && H.NormalLevels(s)
  {
    if s.status!=H.Running then 0 else
    1+((((((Debt(s)*2+(if s.originPool then 1 else 0))*3+Stage(s))*17+(|s.players|-s.player))*2+
      GeneralWork(s))*2+RedoWork(s))*8+FieldWork(s))*128+PhaseWork(s)
  }
  predicate Before(t: H.State,s: H.State)
    requires H.Shape(t) && H.NormalLevels(t) && H.Shape(s) && H.NormalLevels(s)
  {
    if Debt(t)!=Debt(s) then Debt(t)<Debt(s)
    else if t.originPool!=s.originPool then !t.originPool
    else if Stage(t)!=Stage(s) then Stage(t)<Stage(s)
    else if |t.players|-t.player!=|s.players|-s.player then |t.players|-t.player<|s.players|-s.player
    else if GeneralWork(t)!=GeneralWork(s) then GeneralWork(t)<GeneralWork(s)
    else if RedoWork(t)!=RedoWork(s) then RedoWork(t)<RedoWork(s)
    else if FieldWork(t)!=FieldWork(s) then FieldWork(t)<FieldWork(s)
    else PhaseWork(t)<PhaseWork(s)
  }
  lemma OrderImpliesRank(t: H.State,s: H.State)
    requires H.Shape(t) && H.NormalLevels(t) && H.Shape(s) && H.NormalLevels(s)
    requires s.status==H.Running && Before(t,s)
    ensures Rank(t)<Rank(s)
  {}
  lemma {:isolate_assertions} StepDecreases(s: H.State)
    requires H.Shape(s) && H.NormalLevels(s) && s.status==H.Running
    ensures Rank(H.Step(s))<Rank(s)
  {
    var next:=H.Step(s);
    TotalBound(s.players);TotalBound(next.players);
    assert Debt(next)==Debt(s)-(if s.phase==H.Check && next.phase==H.Announce && next.status==H.Running then 1 else 0);
    if next.status==H.Running {
      match s.phase
      case StartPlayer => { assert Before(next,s); }
      case StartPass => { assert Before(next,s); }
      case Load => { assert Before(next,s); }
      case PrimaryPool => { assert Before(next,s); }
      case OtherPools => { assert Before(next,s); }
      case Check => { assert Before(next,s); }
      case Announce => { assert Before(next,s); }
      case Redirect => { assert Before(next,s); }
      case Store => { assert Before(next,s); }
      case NextField => { assert Before(next,s); }
      case EndPlayer => { assert Before(next,s); }
      case GrantPlayer => { assert Before(next,s); }
      case Grants => { assert Before(next,s); }
      case GrantMessage => { assert Before(next,s); }
      case EndGrants => { assert Before(next,s); }
      OrderImpliesRank(next,s);
    }
  }
  function Finish(s: H.State): H.State
    requires H.Shape(s) && H.NormalLevels(s)
    ensures H.Shape(Finish(s)) && H.NormalLevels(Finish(s))
    ensures Finish(s).status!=H.Running
    decreases Rank(s)
  { if s.status!=H.Running then s else StepDecreases(s);Finish(H.Step(s)) }
  lemma SufficientFuel(s: H.State,fuel: nat)
    requires H.Shape(s) && H.NormalLevels(s) && fuel>Rank(s)
    ensures H.Run(s,fuel).status!=H.Running
    ensures H.Run(s,fuel)==Finish(s)
    decreases fuel
  {
    if s.status==H.Running {
      StepDecreases(s);SufficientFuel(H.Step(s),fuel-1);
    }
  }
}
