// S06: proposed logical contracts, not a verification of the C implementation.
// Source correspondence and deliberately excluded obligations: ../S06-shared-game-state.md.
module SharedGameState {
  type PlayerId = i: int | 0 <= i < 16 witness 0
  type PlanetId = i: int | 0 <= i < 999 witness 0
  type FleetSlot = i: int | 0 <= i < 512 witness 0
  type ShipSlot = i: int | 0 <= i < 16 witness 0
  type BaseSlot = i: int | 0 <= i < 10 witness 0
  type Turn = i: int | 0 <= i < 65536 witness 0
  type U16 = i: int | 0 <= i < 65536 witness 0
  type U32 = i: int | 0 <= i < 4294967296 witness 0
  type Quantity = i: int | 0 <= i <= 2147483647 witness 0
  datatype Option<T> = None | Some(value: T)
  datatype FleetId = FleetId(iplr: PlayerId, ifl: FleetSlot)
  datatype DesignId = ShipDesign(iplr: PlayerId, ish: ShipSlot)
                    | BaseDesign(iplr: PlayerId, isb: BaseSlot)
  datatype ThingKind = Minefield | Packet | Wormhole | Trader
  datatype ThingId = ThingId(ith: ThingKind, iplr: PlayerId, id: FleetSlot)
  datatype Point = Point(x: int, y: int)
  predicate PointFits(pt: Point) { -32768 <= pt.x < 32768 && -32768 <= pt.y < 32768 }

  function FleetWord(id: FleetId): int { 512 * id.iplr + id.ifl }
  function DecodeFleet(w: int): FleetId
    requires 0 <= w < 8192
  { FleetId(w / 512, w % 512) }
  lemma FleetRoundTrip(id: FleetId)
    ensures 0 <= FleetWord(id) < 8192
    ensures DecodeFleet(FleetWord(id)) == id
  {}
  lemma FleetIdentityUnique(a: FleetId, b: FleetId)
    requires FleetWord(a) == FleetWord(b)
    ensures a == b
  { FleetRoundTrip(a); FleetRoundTrip(b); }

  function KindCode(ith: ThingKind): int {
    match ith case Minefield => 0 case Packet => 1 case Wormhole => 2 case Trader => 3
  }
  function ThingWord(id: ThingId): int { 8192 * KindCode(id.ith) + 512 * id.iplr + id.id }
  lemma ThingIdentityUnique(a: ThingId, b: ThingId)
    requires ThingWord(a) == ThingWord(b)
    ensures a == b
  {
    assert ThingWord(a) / 8192 == KindCode(a.ith);
    assert ThingWord(b) / 8192 == KindCode(b.ith);
    assert KindCode(a.ith) == KindCode(b.ith);
    assert a.ith == b.ith;
    assert ThingWord(a) % 8192 == 512 * a.iplr + a.id;
    assert ThingWord(b) % 8192 == 512 * b.iplr + b.id;
    FleetIdentityUnique(FleetId(a.iplr, a.id), FleetId(b.iplr, b.id));
  }
  function PlanetLink(id: Option<PlanetId>): int {
    match id case None => 0 case Some(i) => i + 1
  }
  function DecodePlanetLink(w: int): Option<PlanetId>
    requires 0 <= w <= 999
  { if w == 0 then None else Some(w - 1) }
  lemma PlanetLinkRoundTrip(id: Option<PlanetId>)
    ensures 0 <= PlanetLink(id) <= 999
    ensures DecodePlanetLink(PlanetLink(id)) == id
  {}
  // TASKSELL stores the recipient's position after excluding the giving player.
  function GiveWire(owner: PlayerId, recipient: PlayerId): int
    requires owner != recipient
  { if recipient < owner then recipient else recipient - 1 }
  function DecodeGive(owner: PlayerId, index: int): PlayerId
    requires 0 <= index < 15
  { if index < owner then index else index + 1 }
  lemma GiveRoundTrip(owner: PlayerId, recipient: PlayerId)
    requires owner != recipient
    ensures 0 <= GiveWire(owner, recipient) < 15
    ensures DecodeGive(owner, GiveWire(owner, recipient)) == recipient
  {}
  function CalendarYear(turn: Turn): int { 2400 + turn }

  // NoData is different from Known(None): unobserved ownership vs unowned.
  // Estimates retain their encoded buckets; they are not exact quantities.
  datatype Knowledge<T> = NoData | Known(value: T, turn: Turn)
  datatype PopulationView = UnknownPopulation
    | ExactPopulation(cHundreds: Quantity, turn: Turn)
    | EstimatedPopulation(uPopGuess: int, uDefGuess: int, turn: Turn)
  predicate PopulationViewFits(pop: PopulationView) {
    match pop
    case UnknownPopulation => true
    case ExactPopulation(_, _) => true
    case EstimatedPopulation(p, d, _) => 0 <= p < 4096 && 0 <= d < 16
  }
  predicate NotFuture<T>(k: Knowledge<T>, turn: Turn) {
    k.NoData? || k.turn <= turn
  }
  lemma UnknownIsNotUnowned(turn: Turn)
    ensures Knowledge<Option<PlayerId>>.NoData != Known(None, turn)
  {}

  datatype Cargo = Cargo(iron: Quantity, bor: Quantity, germ: Quantity,
                        cHundreds: Quantity, fuelMg: Quantity)
  function CargoMass(c: Cargo): int { c.iron + c.bor + c.germ + c.cHundreds }
  // Arithmetic contract for a caller that has already checked capacity and rights.
  method TransferQuantity(src: Quantity, dst: Quantity, c: nat)
      returns (srcNew: Quantity, dstNew: Quantity)
    requires c <= src && dst + c <= 2147483647
    ensures srcNew == src - c && dstNew == dst + c
    ensures srcNew + dstNew == src + dst
  { srcNew := src - c; dstNew := dst + c; }

  datatype ObjectRef = NoObject | PlanetRef(idPlanet: PlanetId)
    | FleetRef(idFleet: FleetId) | PositionOnly | ThingRef(idThing: ThingId)
  datatype Action = NoAction | LoadAll | UnloadAll | LoadExact(c: int)
    | UnloadExact(c: int) | FillPercent(pct: int) | WaitPercent(pct: int)
    | Dunnage | SetAmount(c: int) | SetWaypoint(c: int)
  predicate ActionFits(a: Action) {
    match a
    case LoadExact(c) => 0 <= c < 4096
    case UnloadExact(c) => 0 <= c < 4096
    case FillPercent(pct) => 0 <= pct <= 100
    case WaitPercent(pct) => 0 <= pct <= 100
    case SetAmount(c) => 0 <= c < 4096
    case SetWaypoint(c) => 0 <= c < 4096
    case _ => true
  }
  datatype Task = NoTask | Transport(rgia: seq<Action>) | Colonize | RemoteMine
    | Merge | Scrap | LayMines(cTime: U16, cTimeOld: U16)
    | Patrol(iWarp: U16, iDist: U16) | AutoRoute | Give(iplr: PlayerId)
  function TaskCode(t: Task): int {
    match t
    case NoTask => 0 case Transport(_) => 1 case Colonize => 2
    case RemoteMine => 3 case Merge => 4 case Scrap => 5 case LayMines(_, _) => 6
    case Patrol(_, _) => 7 case AutoRoute => 8 case Give(_) => 9
  }
  predicate TaskFits(t: Task, cPlayer: int) {
    match t
    case Transport(rgia) => |rgia| == 5 && forall a <- rgia :: ActionFits(a)
    case Give(iplr) => iplr < cPlayer
    case _ => true
  }
  datatype Waypoint = Waypoint(pt: Point, target: ObjectRef, iWarp: int,
                              task: Task, fValidTask: bool, fNoAutoTrack: bool)
  predicate WaypointFits(w: Waypoint, cPlayer: int, cPlanet: int) {
    PointFits(w.pt) && 0 <= w.iWarp <= 11 && TaskFits(w.task, cPlayer) &&
    (match w.target
      case PlanetRef(id) => id < cPlanet
      case FleetRef(id) => id.iplr < cPlayer
      case _ => true)
  }
  // Ship/starbase keys are scoped by player. A plan is also scoped by player.
  datatype ProductionItem = BuildShip(ish: ShipSlot) | BuildBase(isb: BaseSlot)
    | PlanetItem(iobj: int)
  predicate PlanetItemDeclared(i: int) {
    0 <= i <= 9 || 11 <= i <= 27 || i == 31
  }
  datatype Production = Production(item: ProductionItem, cItem: int, pct: int)
  predicate ProductionFits(p: Production) {
    0 <= p.cItem <= 1023 && 0 <= p.pct <= 100 &&
    (p.item.PlanetItem? ==> PlanetItemDeclared(p.item.iobj))
  }
  // Runtime research is separate from M04's race research-cost choices.
  datatype Research = Research(rgTech: seq<int>, rgResSpent: seq<U32>,
                               pctResearch: int, iTechNow: int, iTechNext: int)
  predicate ResearchFits(r: Research) {
    |r.rgTech| == 6 && (forall level <- r.rgTech :: 0 <= level <= 26) &&
    |r.rgResSpent| == 6 && 0 <= r.pctResearch <= 100 &&
    0 <= r.iTechNow < 6 && 0 <= r.iTechNext <= 7
  }
  predicate RelationsFit(rgmd: seq<int>, cPlayer: int) {
    1 <= cPlayer <= 16 && |rgmd| == cPlayer &&
    forall md <- rgmd :: 0 <= md <= 2
  }
  datatype HullSlot = HullSlot(category: U16, item: int, count: int)
  predicate HullSlotsFit(slots: seq<HullSlot>) {
    |slots| <= 16 && forall slot <- slots ::
      0 <= slot.item < 256 && 0 <= slot.count < 256
  }
  // Catalog membership, hull compatibility and technology remain M11/S03 rules.
  datatype BattlePlan = BattlePlan(iplr: PlayerId, iplan: int, tactic: int,
                                   target1: int, target2: int, attack: int)
  predicate BattlePlanFits(p: BattlePlan, cPlayer: int) {
    p.iplr < cPlayer && 0 <= p.iplan < 16 && 0 <= p.tactic <= 5 &&
    0 <= p.target1 <= 7 && 0 <= p.target2 <= 7 && 0 <= p.attack < 4 + cPlayer
  }

  datatype Planet = Planet(id: PlanetId, owner: Option<PlayerId>, pt: Point,
    rgEnv: seq<int>, rgEnvOrig: seq<int>, rgConc: seq<int>, rgDepletion: seq<int>,
    cargo: Cargo, cMines: int, cFactories: int, cDefenses: int, iScanner: int,
    base: Option<BaseSlot>, route: Option<PlanetId>, fling: Option<PlanetId>,
    queue: seq<Production>)
  predicate PlanetFits(p: Planet, cPlayer: int, cPlanet: int) {
    p.id < cPlanet && PointFits(p.pt) &&
    (p.owner.Some? ==> p.owner.value < cPlayer) &&
    |p.rgEnv| == 3 && (forall v <- p.rgEnv :: 0 <= v <= 100) &&
    |p.rgEnvOrig| == 3 && (forall v <- p.rgEnvOrig :: 0 <= v <= 100) &&
    |p.rgConc| == 3 && (forall v <- p.rgConc :: 0 <= v <= 255) &&
    |p.rgDepletion| == 3 && (forall v <- p.rgDepletion :: 0 <= v <= 255) &&
    0 <= p.cMines < 4096 && 0 <= p.cFactories < 4096 && 0 <= p.cDefenses < 4096 &&
    0 <= p.iScanner <= 31 && p.cargo.fuelMg == 0 &&
    (p.base.Some? ==> p.owner.Some?) &&
    (p.route.Some? ==> p.route.value < cPlanet) &&
    (p.fling.Some? ==> p.fling.value < cPlanet) &&
    |p.queue| <= 255 && (forall q <- p.queue :: ProductionFits(q))
  }
  datatype Fleet = Fleet(id: FleetId, iPlayer: PlayerId, pt: Point,
    orbit: Option<PlanetId>, rgcsh: seq<int>, cargo: Cargo,
    iplan: int, orders: seq<Waypoint>, fDead: bool)
  predicate FleetFits(f: Fleet, cPlayer: int, cPlanet: int,
                      designs: set<DesignId>, plans: set<(PlayerId, int)>) {
    f.id.iplr == f.iPlayer && f.iPlayer < cPlayer && PointFits(f.pt) &&
    (f.orbit.Some? ==> f.orbit.value < cPlanet) &&
    |f.rgcsh| == 16 && (forall i | 0 <= i < 16 ::
      0 <= f.rgcsh[i] <= 32767 &&
      (f.rgcsh[i] != 0 ==> ShipDesign(f.iPlayer, i) in designs)) &&
    (f.fDead || exists i | 0 <= i < 16 :: f.rgcsh[i] > 0) &&
    0 <= f.iplan < 16 && (f.iPlayer, f.iplan) in plans &&
    1 <= |f.orders| <= 255 &&
    (forall w <- f.orders :: WaypointFits(w, cPlayer, cPlanet) &&
      (w.task.Give? ==> w.task.iplr != f.iPlayer))
  }
  // Closure applies only to a complete authoritative snapshot at a stable boundary.
  // Does not require tracked fleet targets to exist: orders may outlive targets.
  predicate HostTopology(cPlayer: int, planets: seq<Planet>, fleets: seq<Fleet>,
                         designs: set<DesignId>, plans: set<(PlayerId, int)>) {
    1 <= cPlayer <= 16 && 1 <= |planets| <= 999 &&
    (forall i | 0 <= i < |planets| :: planets[i].id == i &&
      PlanetFits(planets[i], cPlayer, |planets|) &&
      (planets[i].base.Some? ==> BaseDesign(planets[i].owner.value,
                                          planets[i].base.value) in designs)) &&
    (forall f <- fleets :: FleetFits(f, cPlayer, |planets|, designs, plans) &&
      (f.orbit.Some? ==> f.pt == planets[f.orbit.value].pt)) &&
    (forall i, j | 0 <= i < j < |fleets| :: FleetWord(fleets[i].id) < FleetWord(fleets[j].id)) &&
    (forall d <- designs :: d.iplr < cPlayer) &&
    (forall p <- plans :: p.0 < cPlayer && 0 <= p.1 < 16)
  }

  // Creation-only: conquest later invalidates homeworld ownership deliberately.
  predicate CreationHomes(planets: seq<Planet>, homes: seq<PlanetId>) {
    1 <= |homes| <= 16 &&
    (forall i | 0 <= i < |homes| :: homes[i] < |planets| &&
      planets[homes[i]].id == homes[i] && planets[homes[i]].owner == Some(i)) &&
    (forall i, j | 0 <= i < j < |homes| :: homes[i] != homes[j])
  }

  datatype ThingPayload = MineData(cMines: Quantity, known: U16, now: U16,
                                   iType: int, fDetonate: bool)
    | PacketData(destination: PlanetId, iWarp: int, rgwt: seq<int>, wtMax: int,
                 decay: int, fMoved: bool, fInclude: bool)
    | WormData(iStable: int, cLastMove: int, known: U16, traversed: U16,
               partner: ThingId, fDestKnown: bool, fInclude: bool)
    | TraderData(ptDest: Point, iWarp: int, known: U16, rewards: U16, fInclude: bool)
  datatype Thing = Thing(id: ThingId, pt: Point, payload: ThingPayload, turn: Turn)
  predicate ThingFits(t: Thing, cPlanet: int) {
    PointFits(t.pt) &&
    match t.payload
    case MineData(_, _, _, iType, _) => t.id.ith == Minefield && 0 <= iType <= 2
    case PacketData(dst, warp, rgwt, wtMax, decay, _, _) =>
      t.id.ith == Packet && (warp == 0 || dst < cPlanet) && 0 <= warp <= 15 &&
      |rgwt| == 3 && (forall v <- rgwt :: 0 <= v <= 32767) &&
      0 <= wtMax < 16384 && 0 <= decay <= 3
    case WormData(stable, last, _, _, partner, _, _) =>
      t.id.ith == Wormhole && 0 <= stable <= 3 && 0 <= last < 1024 && partner.ith == Wormhole
    case TraderData(dst, warp, _, _, _) =>
      t.id.ith == Trader && PointFits(dst) && 0 <= warp <= 15
  }
  predicate WormholePairs(things: map<ThingId, Thing>) {
    forall id | id in things && things[id].payload.WormData? ::
      var partner := things[id].payload.partner;
      partner != id && partner in things && things[partner].payload.WormData? &&
      things[partner].payload.partner == id
  }

  predicate ThingSetFits(things: map<ThingId, Thing>, cPlanet: int) {
    |things| <= 4050 &&
    (forall id | id in things :: things[id].id == id && ThingFits(things[id], cPlanet)) &&
    WormholePairs(things)
  }
  // Event references are local to the recorded battle, not live fleet indices.
  datatype BattleEvent = BattleEvent(itok: int, attacker: int, victims: seq<int>)
  predicate BattleEventIndicesFit(e: BattleEvent, ctok: int) {
    0 < ctok <= 255 && 0 <= e.itok < ctok && 0 <= e.attacker < ctok &&
    forall i <- e.victims :: 0 <= i < ctok
  }

  datatype OrderEnvelope = OrderEnvelope(lid: U32, iplr: PlayerId, turn: Turn, submitted: bool)
  predicate Matches(e: OrderEnvelope, lid: U32, iplr: PlayerId, turn: Turn) {
    e.lid == lid && e.iplr == iplr && e.turn == turn
  }
  lemma WrongTurnCannotMatch(e: OrderEnvelope, lid: U32, iplr: PlayerId, turn: Turn)
    requires e.turn != turn
    ensures !Matches(e, lid, iplr, turn)
  {}

  // Normalized simulation seeds only. File XOR uses separate state.
  datatype Rng = Rng(s1: int, s2: int)
  predicate RngValid(r: Rng) { 0 < r.s1 < 2147483563 && 0 < r.s2 < 2147483399 }
  function StepSeed(s: int, a: int, q: int, b: int, modulus: int): int
    requires q > 0
  {
    var next := a * (s % q) - (s / q) * b;
    if next < 0 then next + modulus else next
  }
  function Step(r: Rng): Rng {
    Rng(StepSeed(r.s1, 40014, 53668, 12211, 2147483563),
        StepSeed(r.s2, 40692, 52774, 3791, 2147483399))
  }
  method Draw(r: Rng, c: int) returns (next: Rng, result: int)
    requires RngValid(r) && -32768 <= c <= 32767
    ensures next == Step(r) && RngValid(next)
    ensures c <= 0 ==> result == 0
    ensures c > 0 ==> 0 <= result < c
  {
    next := Step(r);
    var z := next.s1 - next.s2;
    if z < 1 { z := z + 2147483562; }
    result := if c <= 0 then 0 else z % c;
  }
  datatype RandomContext = RandomContext(current: Rng, stack: seq<Rng>)
  function Push(ctx: RandomContext, next: Rng): RandomContext
    requires |ctx.stack| < 4
  { RandomContext(next, ctx.stack + [ctx.current]) }
  function Pop(ctx: RandomContext): RandomContext
    requires 0 < |ctx.stack|
  { RandomContext(ctx.stack[|ctx.stack|-1], ctx.stack[..|ctx.stack|-1]) }
  lemma PushPopRestores(ctx: RandomContext, next: Rng)
    requires |ctx.stack| < 4
    ensures Pop(Push(ctx, next)) == ctx
  {}

  // Concrete witnesses prevent the contracts from only checking impossible cases.
  lemma {:isolate_assertions} AcceptanceVectors() {
    assert FleetWord(FleetId(15, 511)) == 8191;
    assert DecodeFleet(512) == FleetId(1, 0);
    assert PlanetLink(Some(0)) == 1 && DecodePlanetLink(0) == None;
    assert CalendarYear(0) == 2400;
    assert !ActionFits(FillPercent(101));
    assert ActionFits(LoadExact(4095)) && !ActionFits(LoadExact(4096));
    assert !TaskFits(Transport([NoAction]), 2);
    assert TaskFits(Give(1), 2) && !TaskFits(Give(2), 2);
    assert TaskCode(AutoRoute) == 8;
    assert DecodeGive(1,1) == 2 && GiveWire(1,2) == 1;
    assert ResearchFits(Research([26,0,0,0,0,0],[0,0,0,0,0,0],15,0,6));
    var badResearch := Research([27,0,0,0,0,0],[0,0,0,0,0,0],15,0,6);
    if ResearchFits(badResearch) {
      assert 27 in badResearch.rgTech;
      assert 0 <= 27 <= 26;
      assert false;
    }
    assert RelationsFit([0,1,2],3);
    if RelationsFit([0,3],2) {
      assert 3 in [0,3];
      assert 0 <= 3 <= 2;
      assert false;
    }
    assert BattlePlanFits(BattlePlan(0,0,5,7,7,5),2);
    assert !BattlePlanFits(BattlePlan(0,0,6,7,7,5),2);
    assert HullSlotsFit([HullSlot(1,0,255)]) && !HullSlotsFit([HullSlot(1,0,256)]);
    assert BattleEventIndicesFit(BattleEvent(0,1,[1]),2);
    if BattleEventIndicesFit(BattleEvent(0,1,[2]),2) {
      assert 2 in [2];
      assert 0 <= 2 < 2;
      assert false;
    }
    assert !PopulationViewFits(EstimatedPopulation(4096, 0, 0));
    assert CargoMass(Cargo(1, 2, 3, 4, 1000)) == 10;
    assert !ProductionFits(Production(PlanetItem(10), 1, 0));
    assert ProductionFits(Production(BuildShip(0), 1023, 100));
    assert !ProductionFits(Production(BuildShip(0), 1024, 0));
    var wp := Waypoint(Point(1000, 1000), PlanetRef(0), 0, NoTask, false, false);
    var pl := Planet(0, Some(0), wp.pt, [50,50,50], [50,50,50], [119,30,1], [0,0,0],
      Cargo(0,0,0,250,0), 10,10,10,31,Some(0),None,None,[]);
    var fl := Fleet(FleetId(0,0),0,wp.pt,Some(0),[1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
      Cargo(0,0,0,0,100),0,[wp],false);
    assert CreationHomes([pl],[0]);
    assert WaypointFits(wp,1,1);
    assert forall i | 0 <= i < 16 ::
      0 <= fl.rgcsh[i] <= 32767 &&
      (fl.rgcsh[i] != 0 ==> ShipDesign(fl.iPlayer, i) in {ShipDesign(0,0),BaseDesign(0,0)});
    assert fl.rgcsh[0] > 0;
    assert FleetFits(fl,1,1,{ShipDesign(0,0),BaseDesign(0,0)},{(0,0)});
    assert HostTopology(1,[pl],[fl],{ShipDesign(0,0),BaseDesign(0,0)},{(0,0)});
    assert !HostTopology(1,[pl],[fl],{BaseDesign(0,0)},{(0,0)});
    if HostTopology(1,[pl],[fl,fl],{ShipDesign(0,0),BaseDesign(0,0)},{(0,0)}) {
      assert FleetWord([fl,fl][0].id) < FleetWord([fl,fl][1].id);
      assert false;
    }
    var a := ThingId(Wormhole,0,0);
    var b := ThingId(Wormhole,0,1);
    var ta := Thing(a,wp.pt,WormData(0,0,0,0,b,false,false),0);
    var tb := Thing(b,wp.pt,WormData(0,0,0,0,a,false,false),0);
    assert ThingFits(ta,1) && ThingFits(tb,1);
    assert !ThingFits(Thing(ThingId(Packet,0,0),wp.pt,ta.payload,0),1);
    assert WormholePairs(map[a := ta,b := tb]);
    if WormholePairs(map[a := ta]) {
      assert (map[a := ta])[a].payload.partner in map[a := ta];
      assert false;
    }
    assert Step(Rng(3,5)) == Rng(120042,203460);
  }
}
