include "S06-shared-state.dfy"
include "S01-records.dfy"
include "M03-setup.dfy"

// Remaining specified logical groups. Raw record widths stay authoritative in
// S01; typed projections below do not assert a field-by-field serialization proof.
module S06Extensions {
  import opened SharedGameState
  import S = S01
  import B = S01Records
  import U = M03Setup
  import Race = M04Race
  datatype Configuration = Configuration(game: S.Game,options: U.Options,generation: int)
  predicate FConfiguration(c: Configuration) {
    S.FGame(c.game) && |c.game.rgvc|==12 && U.FVC(c.game.rgvc[..10]) && 0<=c.generation<8
  }
  datatype Counts = Counts(planets: int,fleets: int,ships: int,bases: int)
  predicate FCounts(c: Counts) {
    0<=c.planets<=999 && 0<=c.fleets<=512 && 0<=c.ships<=16 && 0<=c.bases<=10
  }
  datatype PlanMetadata = PlanMetadata(plan: BattlePlan,name: S.Bytes,fDumpCargo: bool)
  predicate FPlanMetadata(p: PlanMetadata,count: int) {
    BattlePlanFits(p.plan,count) && S.FName(p.name) && 0 !in p.name
  }
  datatype DerivedDesign = DerivedDesign(tech: seq<int>,mass: int,resources: int,minerals: seq<int>,cargo: int,fuel: int,
    armor: int,scan: int,penetratingScan: int,detection: int,steal: int)
  predicate FDerivedShape(d: DerivedDesign) { |d.tech|==6 && |d.minerals|==3 }
  // Numeric legality/calculation of these cache fields remains the M11 catalog
  // dependency; shape and representation do not invent those missing formulas.
  datatype Status = Status(fDead: bool,fCrippled: bool,fUnused: bool,fLearned: bool,fHacker: bool,trader: U16)
  datatype TemplateItem = TemplateItem(mode: int,count: int)
  predicate FProductionTemplate(xs: seq<TemplateItem>) {
    |xs|<=12 && forall x <- xs :: 0<=x.mode<64 && 0<=x.count<1024
  }
  datatype Player = Player(id: PlayerId,race: Race.Race,controller: U.Controller,research: Research,
    relations: seq<int>,plans: seq<BattlePlan>,production: seq<TemplateItem>,fNoResearch: bool,status: Status,
    home: Option<PlanetId>,scoreWord: U16,lastResources: int,counts: Counts)
  predicate FPlayer(p: Player,count: int,planets: int) {
    p.id<count && Race.FRace(p.race) && ResearchFits(p.research) && RelationsFit(p.relations,count) &&
    |p.plans|<=16 && (forall i | 0<=i<|p.plans| :: BattlePlanFits(p.plans[i],count) && p.plans[i].iplr==p.id && p.plans[i].iplan==i) &&
    FProductionTemplate(p.production) && FCounts(p.counts) && (p.home.Some? ==> p.home.value<planets) &&
    -2147483648<=p.lastResources<=2147483647
  }
  datatype PowerState = ScoringPower(power: int) | VisiblePower(visible: int)
  datatype Lifecycle = Lifecycle(fFree: bool,fGift: bool,turn: Turn,built: U32,existing: U32,known: U16)
  datatype DesignView = PartialHull(hull: S.U8,bitmap: S.U8,mass: U16,name: S.Bytes,turn: Turn)
    | FullDesign(hull: S.U8,bitmap: S.U8,name: S.Bytes,slots: seq<HullSlot>,life: Lifecycle,power: PowerState)
  predicate FDesignView(v: DesignView) {
    S.FName(v.name) && 0 !in v.name && (v.FullDesign? ==> HullSlotsFit(v.slots))
  }
  datatype MotionState = Moving(left: int,used: int) | FleetPower(power: int)
  datatype Damage = Damage(shipFraction: int,damageFraction: int)
  datatype FleetView = PartialFleet(id: FleetId,pt: Point,mass: int,turn: Turn)
    | FullFleet(fleet: Fleet,damage: seq<Damage>,motion: MotionState,fuelUsed: int,fRepeat: bool,name: Option<S.Bytes>)
  predicate FFleetExtras(v: FleetView) {
    if v.PartialFleet? then PointFits(v.pt) && -2147483648<=v.mass<=2147483647 else
      |v.damage|==16 && (forall d <- v.damage :: 0<=d.shipFraction<128 && 0<=d.damageFraction<512) &&
      (v.name.Some? ==> S.FUtf8(v.name.value) && 0 !in v.name.value)
  }
  datatype PlanetExtras = PlanetExtras(deltaPopulation: S.U8,damage: S.U16,fNoHeal: bool,flingWarp: int,
    fArtifact: bool,fNoResearch: bool,fHomeworld: bool,fFirstYear: bool,fWasInhabited: bool,population: PopulationView)
  predicate FPlanetExtras(p: PlanetExtras) { 0<=p.flingWarp<16 && p.damage<4096 && PopulationViewFits(p.population) }
  // Score history and current rank are distinct variants. Invalid is not zero.
  datatype ScoreKey = Current(rank: int) | History(turn: Turn)
  datatype Score = InvalidScore | ValidScore(player: PlayerId,key: ScoreKey,score: int,resources: int,
    planets: U16,bases: U16,unarmed: U16,escort: U16,capital: U16,totalTech: U16,fWinner: bool,victory: S.U8)
  predicate FScore(s: Score) {
    s.InvalidScore? || (-2147483648<=s.score<=2147483647 && -2147483648<=s.resources<=2147483647 &&
      (s.key.Current? ==> 1<=s.key.rank<=16))
  }
  datatype CatalogEvent = CatalogEvent(id: nat,goto: U16,parameters: seq<int>,recipient: PlayerId)
  predicate FEvent(e: CatalogEvent,arity: map<nat,nat>) {
    e.id in arity && |e.parameters|==arity[e.id] && |e.parameters|<=7
  }
  // Payload adapters reuse the exact S01 parser extent predicates. They don't
  // infer semantic battle legality, message recipient policy or AI decisions.
  datatype EncodedResult = Message(rgb: S.Bytes) | Battle(rgb: S.Bytes)
    | AiHistory(rgb: S.Bytes,kind: B.AiKind,planets: nat)
  predicate FEncodedResult(r: EncodedResult) {
    match r
    case Message(rgb) => B.FPlayerMessage(rgb,true)
    case Battle(rgb) => B.FBattle(rgb)
    case AiHistory(rgb,kind,planets) => B.FAiHistory(rgb,kind,planets)
  }
  datatype Lifetime = Universe | AuthoritativeTurn | Session | Commands | Generation | AiScratch | Cache | Editor | Application | FileOperation
  datatype Held = Held(owner: Lifetime,generation: nat,persistentIdentity: Option<U32>)
  predicate FLiveReference(h: Held,current: nat) { h.owner==Application || h.generation==current }
  datatype SessionState = SessionState(game: U32,turn: Turn,player: PlayerId,fDirty: bool,fSubmitted: bool,
    fReadOnly: bool,fPartial: bool,commands: seq<S.Record>,epoch: nat)
  predicate FSubmitEnvelope(s: SessionState,e: OrderEnvelope) { Matches(e,s.game,s.player,s.turn) }
  // Proposed invalidation boundary: data edits invalidate a derived cache, and
  // destroying a game changes its epoch. Neither operation proves disk rollback.
  datatype CacheState = CacheState(sourceVersion: nat,cachedVersion: nat,fValid: bool)
  function Invalidate(c: CacheState): CacheState { c.(sourceVersion:=c.sourceVersion+1,fValid:=false) }
  predicate FUsable(c: CacheState) { c.fValid && c.sourceVersion==c.cachedVersion }
  function DestroySession(s: SessionState): SessionState {
    s.(game:=0,commands:=[],fDirty:=false,fSubmitted:=false,epoch:=s.epoch+1)
  }
  lemma InvalidatedCacheUnusable(c: CacheState) ensures !FUsable(Invalidate(c)) {}
  lemma DestroyInvalidates(s: SessionState,h: Held)
    requires h.owner!=Application && h.generation==s.epoch
    ensures !FLiveReference(h,DestroySession(s).epoch)
  {}
  lemma ExtensionVectors() {
    assert !FProductionTemplate([TemplateItem(64,1)]);
    assert FProductionTemplate([TemplateItem(63,1023)]);
    assert InvalidScore!=ValidScore(0,Current(1),0,0,0,0,0,0,0,0,false,0);
    assert PartialHull(0,0,0,[],0)!=FullDesign(0,0,[],[],Lifecycle(false,false,0,0,0,0),ScoringPower(0));
    assert !FEvent(CatalogEvent(1,0,[1,2],0),map[1:=1]);
  }
}
