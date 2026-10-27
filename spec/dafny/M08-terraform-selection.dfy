include "M08-native-habitat.dfy"
include "M11-design-buildability.dfy"

module M08TerraformSelection {
  import S = S01
  import G = SharedGameState
  import E = M08NativeEconomy
  import A = M11NativeAvailability
  import B = M11DesignBuildability
  import O = M08Colony
  import H = M08NativeHabitat
  import Math = S04Arithmetic
  datatype Planet = Planet(owner: S.I16,env: seq<E.I8>,original: seq<E.I8>)
  predicate PlanetFits(p: Planet) { |p.env|==3 && |p.original|==3 }
  datatype Player = Player(context: A.Context,habitat: H.Fields)
  predicate PlayersFit(players: seq<Player>) {
    |players|==16 && forall p <- players :: A.Shape(p.context) && !p.context.headless && H.Shape(p.habitat)
  }
  function Contexts(players: seq<Player>): seq<A.Context>
    requires PlayersFit(players)
    ensures B.PlayersFit(Contexts(players))
  { seq(16,i requires 0<=i<16 => players[i].context) }
  datatype Terra = Terra(ability: S.I16,cost: S.U16)
  datatype Choice = Choice(item: G.Option<nat>,calls: seq<nat>)
  function Find(codes: seq<S.I16>,high: nat,low: nat): Choice
    requires |codes|==20 && low<=high<20
    ensures |Find(codes,high,low).calls|<=high-low+1
    ensures Find(codes,high,low).item.Some? ==> low<=Find(codes,high,low).item.value<=high && codes[Find(codes,high,low).item.value]==1
    ensures Find(codes,high,low).item.Some? ==> forall i | Find(codes,high,low).item.value<i<=high :: codes[i]!=1
    ensures Find(codes,high,low).item.None? ==> forall i | low<=i<=high :: codes[i]!=1
    decreases high-low
  {
    if codes[high]==1 then Choice(G.Some(high),[high]) else
    if high==low then Choice(G.None,[high]) else
    var tail:=Find(codes,high-1,low);Choice(tail.item,[high]+tail.calls)
  }
  function Replies(c: A.Context,rows: seq<seq<A.PartRow>>): seq<A.Result>
    requires A.Shape(c) && A.CatalogShape(rows)
    ensures |Replies(c,rows)|==20
  { seq(20,i requires 0<=i<20 => A.LookupRow(c,rows,8192,i,A.Prior(0))) }
  lemma NoChoice(codes: seq<S.I16>,high: nat,low: nat)
    requires |codes|==20 && low<=high<20 && forall i | low<=i<=high :: codes[i]!=1
    ensures Find(codes,high,low).item.None?
  {
    var choice:=Find(codes,high,low);
    if choice.item.Some? {
      assert low<=choice.item.value<=high;
      assert codes[choice.item.value]!=1;
    }
  }
  function Codes(replies: seq<A.Result>): seq<S.I16>
    requires |replies|==20
    requires forall r <- replies :: -32768<=r.status<32768
    ensures |Codes(replies)|==20
  { seq(20,i requires 0<=i<20 => replies[i].status as S.I16) }
  datatype Plan = Plan(found: bool,moves: seq<S.I16>,costs: seq<G.Option<S.I16>>,calls: seq<nat>)
  predicate PlanFits(p: Plan) { |p.moves|==3 && |p.costs|==3 }
  function Upgrade(plan: Plan,choice: Choice,axis: nat,terra: seq<Terra>): Plan
    requires PlanFits(plan) && axis<3 && |terra|==20
    requires choice.item.Some? ==> choice.item.value<20
    ensures PlanFits(Upgrade(plan,choice,axis,terra))
    ensures forall i | 0<=i<3 && i!=axis :: Upgrade(plan,choice,axis,terra).moves[i]==plan.moves[i] && Upgrade(plan,choice,axis,terra).costs[i]==plan.costs[i]
    ensures choice.item.Some? && terra[choice.item.value].ability<=plan.moves[axis] ==>
      Upgrade(plan,choice,axis,terra).moves==plan.moves && Upgrade(plan,choice,axis,terra).costs==plan.costs
  {
    var called:=plan.(calls:=plan.calls+choice.calls);
    if choice.item.None? || terra[choice.item.value].ability<=plan.moves[axis] then called else
    called.(found:=true,moves:=plan.moves[axis:=terra[choice.item.value].ability],costs:=plan.costs[axis:=G.Some(E.Word(terra[choice.item.value].cost))])
  }
  function Prepare(codes: seq<S.I16>,terra: seq<Terra>,costs: seq<G.Option<S.I16>>): Plan
    requires |codes|==20 && |terra|==20 && |costs|==3
    ensures PlanFits(Prepare(codes,terra,costs))
    ensures |Prepare(codes,terra,costs).calls|<=20
  {
    var all:=Find(codes,7,0);
    var initial:=if all.item.None? then Plan(false,[0,0,0],costs,all.calls) else
      Plan(true,seq(3,i => terra[all.item.value].ability),seq(3,i => G.Some(E.Word(terra[all.item.value].cost))),all.calls);
    Upgrade(Upgrade(Upgrade(initial,Find(codes,11,8),0,terra),Find(codes,15,12),1,terra),Find(codes,19,16),2,terra)
  }
  lemma Unavailable(codes: seq<S.I16>,terra: seq<Terra>,costs: seq<G.Option<S.I16>>)
    requires |codes|==20 && |terra|==20 && |costs|==3 && forall i | 0<=i<20 :: codes[i]!=1
    ensures !Prepare(codes,terra,costs).found && Prepare(codes,terra,costs).moves==[0,0,0] && Prepare(codes,terra,costs).costs==costs
  {
    NoChoice(codes,7,0);NoChoice(codes,11,8);NoChoice(codes,15,12);NoChoice(codes,19,16);
    assert Find(codes,7,0).item.None? && Find(codes,11,8).item.None?;
    assert Find(codes,15,12).item.None? && Find(codes,19,16).item.None?;
  }
  datatype Bounds = Bounds(low: S.I16,high: S.I16)
  function End(value: int): S.I16 { E.Word(value) }
  function NativeBounds(current: E.I8,original: E.I8,ideal: E.I8,immune: bool,ability: S.I16,help: bool): Bounds {
    if ability==0 || immune then Bounds(-1,-1) else
    var rawLow:=E.Word(original-ability);var rawHigh:=E.Word(original+ability);
    var low:=if rawLow>=current then -1 else if rawLow<1 then 1 else rawLow;
    var high:=if rawHigh<=current then -1 else if rawHigh>99 then 99 else rawHigh;
    if help then
      if current==ideal then Bounds(-1,-1) else
      if current>ideal then Bounds(if low==-1 then -1 else if low<=ideal then ideal else low,-1) else
      Bounds(-1,if high==-1 then -1 else if high>=ideal then ideal else high)
    else
      var distance:=Math.Abs(current-ideal);
      var dl:=if low==-1 then 0 else Math.Abs(low-ideal);var dh:=if high==-1 then 0 else Math.Abs(high-ideal);
      if distance>=dl && distance>=dh then Bounds(-1,-1) else if dl>=dh then Bounds(low,-1) else Bounds(-1,high)
  }
  function Encode(b: O.Bounds): Bounds { Bounds(if b.low.At? then End(b.low.value) else -1,if b.high.At? then End(b.high.value) else -1) }
  lemma BoundsRefine(current: E.I8,original: E.I8,ideal: E.I8,immune: bool,ability: nat,help: bool)
    requires 1<=current<=99 && 1<=original<=99 && 0<=ideal<=100 && ability<=30
    ensures NativeBounds(current,original,ideal,immune,ability,help)==Encode(O.Terraform(current,original,ideal,ability,immune,help))
  {
    E.WordPreserves(original-ability);E.WordPreserves(original+ability);
    var b:=O.Terraform(current,original,ideal,ability,immune,help);
    if b.low.At? { E.WordPreserves(b.low.value); }
    if b.high.At? { E.WordPreserves(b.high.value); }
  }
  datatype Status = Done | Undefined(reason: string) | Pending
  datatype CanResult = CanResult(status: Status,can: bool,global: S.I16,low: seq<G.Option<S.I16>>,high: seq<G.Option<S.I16>>,plan: Plan)
  function BoundsLoop(p: Planet,players: seq<Player>,selected: S.I16,saved: S.I16,help: bool,plan: Plan,
    at: nat,low: seq<G.Option<S.I16>>,high: seq<G.Option<S.I16>>): CanResult
    requires PlanetFits(p) && PlayersFit(players) && PlanFits(plan) && at<=3 && |low|==3 && |high|==3
    ensures |BoundsLoop(p,players,selected,saved,help,plan,at,low,high).low|==3 && |BoundsLoop(p,players,selected,saved,help,plan,at,low,high).high|==3
    ensures BoundsLoop(p,players,selected,saved,help,plan,at,low,high).status==Done ==> BoundsLoop(p,players,selected,saved,help,plan,at,low,high).global==saved
    decreases 3-at
  {
    if at==3 then CanResult(Done,exists i | 0<=i<3 :: low[i]!=G.Some(-1) || high[i]!=G.Some(-1),saved,low,high,plan) else
    if plan.moves[at]!=0 && (selected<0 || selected>=16) then CanResult(Undefined("terraform player habitat read"),false,selected,low,high,plan) else
    var b:=if plan.moves[at]==0 then Bounds(-1,-1) else
      NativeBounds(p.env[at],p.original[at],players[selected].habitat.ideal[at],players[selected].habitat.low[at]==-1,plan.moves[at],help);
    BoundsLoop(p,players,selected,saved,help,plan,at+1,low[at:=G.Some(b.low)],high[at:=G.Some(b.high)])
  }
  function Can(p: Planet,players: seq<Player>,rows: seq<seq<A.PartRow>>,terra: seq<Terra>,saved: S.I16,help: bool,
    low: seq<G.Option<S.I16>>,high: seq<G.Option<S.I16>>,costs: seq<G.Option<S.I16>>): CanResult
    requires PlanetFits(p) && PlayersFit(players) && A.CatalogShape(rows) && |terra|==20 && |low|==3 && |high|==3 && |costs|==3
    ensures |Can(p,players,rows,terra,saved,help,low,high,costs).low|==3 && |Can(p,players,rows,terra,saved,help,low,high,costs).high|==3
    ensures Can(p,players,rows,terra,saved,help,low,high,costs).status==Done ==> Can(p,players,rows,terra,saved,help,low,high,costs).global==saved
  {
    var selected:=if saved==-1 then p.owner else saved;
    var empty:=Plan(false,[0,0,0],costs,[7]);
    if !B.ValidPlayer(selected) then CanResult(Undefined("terraform lookup player read"),false,selected,low,high,empty) else
    var replies:=Replies(B.Selected(Contexts(players),selected),rows);
    var plan:=Prepare(Codes(replies),terra,costs);
    if !plan.found then CanResult(Done,false,saved,low,high,plan) else BoundsLoop(p,players,selected,saved,help,plan,0,low,high)
  }
  function Byte(value: int): E.I8 { var n:=value%256; (if n<128 then n else n-256) as E.I8 }
  function Score(baseline: S.I16,trial: S.I16,prior: E.I8,endpoint: S.I16): E.Number {
    var delta:=E.Word(trial-baseline);var absolute:=if delta<0 then E.Word(-(delta as int)) else delta;
    var numerator:=E.Word(100*absolute);var divisor:=Math.Abs(prior-endpoint);
    if divisor==0 then E.Undefined("terraform score division") else E.Value(E.Word(E.Div(numerator,divisor)+1))
  }
  function Winner(scores: seq<S.I16>): nat
    requires |scores|==3
    ensures Winner(scores)<3
    ensures forall i | 0<=i<3 :: scores[Winner(scores)]>=scores[i]
    ensures forall i | 0<=i<Winner(scores) :: scores[Winner(scores)]>scores[i]
  { var first:=if scores[1]>scores[0] then 1 else 0; if scores[2]>scores[first] then 2 else first }
  datatype Event = CanCall | DesireCall(env: seq<E.I8>,player: S.I16) | Trial(axis: nat,endpoint: S.I16) | Restore(axis: nat)
  datatype BestResult = BestResult(status: Status,choice: S.I16,global: S.I16,planet: Planet,scores: seq<S.I16>,events: seq<Event>)
  function EvidenceAt(evidence: seq<H.Evidence>,at: nat): H.Evidence { if at<|evidence| then evidence[at] else H.Missing }
  function Trials(p: Planet,fields: H.Fields,saved: S.I16,low: seq<G.Option<S.I16>>,high: seq<G.Option<S.I16>>,
    baseline: S.I16,at: nat,scores: seq<S.I16>,evidence: seq<H.Evidence>,used: nat,events: seq<Event>): BestResult
    requires PlanetFits(p) && H.Shape(fields) && |low|==3 && |high|==3 && at<=3 && |scores|==at
    requires forall i | 0<=i<3 :: low[i].Some? && high[i].Some?
    ensures Trials(p,fields,saved,low,high,baseline,at,scores,evidence,used,events).status==Done ==>
      Trials(p,fields,saved,low,high,baseline,at,scores,evidence,used,events).planet==p && Trials(p,fields,saved,low,high,baseline,at,scores,evidence,used,events).global==saved
    decreases 3-at
  {
    if at==3 then
      var best:=Winner(scores);BestResult(Done,if low[best].value!=-1 then -(best+1) else best+1,saved,p,scores,events)
    else if low[at].value==-1 && high[at].value==-1 then Trials(p,fields,saved,low,high,baseline,at+1,scores+[0],evidence,used,events) else
    var endpoint:=if low[at].value!=-1 then low[at].value else high[at].value;
    var trial:=p.(env:=p.env[at:=Byte(endpoint)]);
    var called:=events+[Trial(at,endpoint),DesireCall(trial.env,p.owner)];
    var answer:=H.Desire(trial.env,fields,EvidenceAt(evidence,used));
    if !answer.Returned? then BestResult(if answer.Failed? then Undefined(answer.reason) else Pending,0,p.owner,trial,scores,called) else
    var score:=Score(baseline,answer.value,p.env[at],endpoint);
    if score.Undefined? then BestResult(Undefined(score.reason),0,p.owner,trial,scores,called) else
    Trials(p,fields,saved,low,high,baseline,at+1,scores+[score.value as S.I16],evidence,used+1,called+[Restore(at)])
  }
  function Best(p: Planet,players: seq<Player>,rows: seq<seq<A.PartRow>>,terra: seq<Terra>,saved: S.I16,help: bool,evidence: seq<H.Evidence>): BestResult
    requires PlanetFits(p) && PlayersFit(players) && A.CatalogShape(rows) && |terra|==20
    ensures Best(p,players,rows,terra,saved,help,evidence).status==Done ==> Best(p,players,rows,terra,saved,help,evidence).global==saved && Best(p,players,rows,terra,saved,help,evidence).planet==p
    ensures -3<=Best(p,players,rows,terra,saved,help,evidence).choice<=3
  {
    if p.owner==-1 then BestResult(Done,0,saved,p,[],[]) else
    var none: seq<G.Option<S.I16>>:=[G.None,G.None,G.None];
    var can:=Can(p,players,rows,terra,p.owner,help,none,none,none);
    if can.status.Undefined? then BestResult(can.status,0,can.global,p,[],[CanCall]) else
    if !can.can then BestResult(Done,0,saved,p,[],[CanCall]) else
    assert 0<=p.owner<16;
    assert forall i | 0<=i<3 :: can.low[i].Some? && can.high[i].Some?;
    var fields:=players[p.owner].habitat;
    var answer:=H.Desire(p.env,fields,EvidenceAt(evidence,0));
    var events:=[CanCall,DesireCall(p.env,p.owner)];
    if !answer.Returned? then BestResult(if answer.Failed? then Undefined(answer.reason) else Pending,0,p.owner,p,[],events) else
    Trials(p,fields,saved,can.low,can.high,answer.value,0,[],evidence,1,events)
  }
  datatype RemoteResult = RemoteResult(result: BestResult,players: seq<Player>)
  function Remote(p: Planet,players: seq<Player>,actor: S.I16,rows: seq<seq<A.PartRow>>,terra: seq<Terra>,saved: S.I16,help: bool,evidence: seq<H.Evidence>): RemoteResult
    requires PlanetFits(p) && PlayersFit(players) && A.CatalogShape(rows) && |terra|==20
    ensures Remote(p,players,actor,rows,terra,saved,help,evidence).result.status==Done ==> Remote(p,players,actor,rows,terra,saved,help,evidence).players==players
  {
    if p.owner<0 || p.owner>=16 then RemoteResult(BestResult(Undefined("remote owner snapshot"),0,saved,p,[],[]),players) else
    if actor<0 || actor>=16 then RemoteResult(BestResult(Undefined("remote actor copy"),0,saved,p,[],[]),players) else
    var injected:=players[p.owner].(context:=players[actor].context);
    var swapped:=players[p.owner:=injected];
    var result:=Best(p,swapped,rows,terra,saved,help,evidence);
    RemoteResult(result,if result.status==Done then players else swapped)
  }
  function OptAxes(p: Planet,f: H.Fields,low: seq<G.Option<S.I16>>,high: seq<G.Option<S.I16>>,at: nat): Planet
    requires PlanetFits(p) && H.Shape(f) && |low|==3 && |high|==3 && at<=3
    requires forall i | 0<=i<3 :: low[i].Some? && high[i].Some?
    ensures PlanetFits(OptAxes(p,f,low,high,at))
    decreases 3-at
  {
    if at==3 then p else
    var current:=p.env[at];var ideal:=f.ideal[at];
    var endpoint:=if f.low[at]==-1 || current==ideal then -1 else
      if current<ideal then (if high[at].value>current then (if ideal>=high[at].value then high[at].value else ideal) else -1)
      else if low[at].value!=-1 && low[at].value<current then (if ideal<=low[at].value then low[at].value else ideal) else -1;
    var next:=if endpoint==-1 then p else p.(env:=p.env[at:=Byte(endpoint)]);
    OptAxes(next,f,low,high,at+1)
  }
  datatype OptResult = OptResult(status: Status,value: S.I16,global: S.I16,planet: Planet,events: seq<Event>)
  function Optimum(p: Planet,players: seq<Player>,requested: S.I16,rows: seq<seq<A.PartRow>>,terra: seq<Terra>,saved: S.I16,evidence: H.Evidence): OptResult
    requires PlanetFits(p) && PlayersFit(players) && A.CatalogShape(rows) && |terra|==20
    ensures Optimum(p,players,requested,rows,terra,saved,evidence).status==Done ==> Optimum(p,players,requested,rows,terra,saved,evidence).planet==p && Optimum(p,players,requested,rows,terra,saved,evidence).global==saved
  {
    var none: seq<G.Option<S.I16>>:=[G.None,G.None,G.None];
    var can:=Can(p,players,rows,terra,saved,true,none,none,none);
    if can.status.Undefined? then OptResult(can.status,0,can.global,p,[CanCall]) else
    if requested<0 || requested>=16 then OptResult(Undefined(if can.can then "optimum requested habitat read" else "optimum evaluated habitat read"),0,saved,p,
      if can.can then [CanCall] else [CanCall,DesireCall(p.env,requested)]) else
    assert !can.can || (forall i | 0<=i<3 :: can.low[i].Some? && can.high[i].Some?);
    var trial:=if !can.can then p else OptAxes(p,players[requested].habitat,can.low,can.high,0);
    var evaluated:=if can.can then saved else requested;
    var events:=[CanCall,DesireCall(trial.env,evaluated)];
    if evaluated<0 || evaluated>=16 then OptResult(Undefined("optimum evaluated habitat read"),0,saved,trial,events) else
    var answer:=H.Desire(trial.env,players[evaluated].habitat,evidence);
    if !answer.Returned? then OptResult(if answer.Failed? then Undefined(answer.reason) else Pending,0,saved,trial,events) else
    OptResult(Done,answer.value,saved,p,events)
  }
  lemma Vectors() {
    E.WordPreserves(400);E.WordPreserves(-25536);
    assert Score(0,400,50,60)==E.Value(-2552);
    assert Score(0,0,50,50)==E.Undefined("terraform score division");
    assert Winner([10,10,10])==0 && Winner([0,5,5])==1;
    assert Byte(255)==-1 && Byte(128)==-128;
  }
}
