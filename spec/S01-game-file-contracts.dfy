// S01 data and conditions. See S01-game-file-formats.md for the evidence boundary.
// Mathematical values describe disk fields; these types do not change C storage.
module S01 {
  type U8 = n: int | 0 <= n < 256
  type U16 = n: int | 0 <= n < 65536
  type U32 = n: int | 0 <= n < 4294967296
  type I16 = n: int | -32768 <= n < 32768
  type PlayerId = n: int | 0 <= n < 16
  type RecordTag = n: int | 0 <= n < 64
  type Bytes = seq<U8>

  datatype FileKind = Universe | Orders | Host | Turn | History | Race
  datatype Known<T> = Unknown | Observed(value: T, turnSeen: U16)
  datatype Record = Record(rt: RecordTag, rgb: Bytes)
  datatype ReadResult = Rejected(ibError: nat)
                      | Decoded(rec: Record, ibNext: nat)
  datatype Bof = Bof(wVersion: U16, lidGame: U32, turn: U16,
                     iPlayer: int, salt: nat, kind: FileKind,
                     wGen: nat, fMulti: bool, fDone: bool)
  datatype Game = Game(lid: U32, mdSize: nat, mdDensity: nat,
                       mdStartDist: nat, cPlayer: nat, cPlanMax: nat,
                       rgvc: Bytes, rgbTitle: Bytes)
  datatype StarPack = StarPack(dx: nat, y: nat, idName: nat)
  datatype UniverseData = UniverseData(game: Game, rgstar: seq<StarPack>)
  datatype OrdersData = OrdersData(bof: Bof, cbLog: nat,
                                   rgrec: seq<Record>, rgmsg: seq<Record>)
  datatype PlanetGroup = PlanetGroup(planet: Record, rgqueue: seq<Record>)
  datatype FleetGroup = FleetGroup(fleet: Record, cord: nat,
                                   rgorder: seq<Record>, rgname: seq<Record>)
  datatype SnapshotData = SnapshotData(bof: Bof, rgplayer: seq<Record>,
    rgplanet: seq<PlanetGroup>, rgship: seq<Record>, rgfleet: seq<FleetGroup>,
    rgbase: seq<Record>, rgplan: seq<Record>)
  datatype HistoryData = HistoryData(bof: Bof, cPlanet: nat, cPlanetExtra: nat,
    rgplanet: seq<Record>, rgbFilter: Bytes, rgplayer: seq<Record>,
    rgdesign: seq<Record>, rgscore: seq<Record>, rgai: seq<Record>)
  datatype RaceImport = RaceImport(bof: Bof, fProtected: bool,
                                   fChecksumValid: bool,
                                   fLegacyPayloadExact: bool)

  // Wire mappings and bounded framing (S01-game-file-records.md sections 1-2).
  function Dt(kind: FileKind): U8 {
    match kind
    case Universe => 0
    case Orders => 1
    case Host => 2
    case Turn => 3
    case History => 4
    case Race => 5
  }

  function Header(rt: RecordTag, cb: nat): U16
    requires cb <= 1023
    ensures Header(rt, cb) % 1024 == cb
    ensures Header(rt, cb) / 1024 == rt
  { cb + 1024 * rt }

  function Word(rgb: Bytes, ib: nat): U16
    requires ib + 2 <= |rgb|
  { (rgb[ib] as int) + 256 * (rgb[ib + 1] as int) }

  function WordBytes(w: U16): Bytes
    ensures |WordBytes(w)| == 2
    ensures Word(WordBytes(w), 0) == w
  { [w % 256, w / 256] }

  predicate FRecord(rec: Record) { |rec.rgb| <= 1023 }

  predicate FTagged(rec: Record, rt: RecordTag) { FRecord(rec) && rec.rt == rt }

  predicate FTaggedSequence(rgrec: seq<Record>, rt: RecordTag) {
    forall rec <- rgrec :: FTagged(rec, rt)
  }

  // Partial decoded projections, not complete byte parsers. Each Record still
  // has the payload-validation obligations in S01-game-file-records.md. Counts here
  // must be extracted and checked against those payloads by the future adapter.
  predicate FPlanetGroup(group: PlanetGroup) {
    FRecord(group.planet) && group.planet.rt in {13, 14} &&
    |group.rgqueue| <= 1 && FTaggedSequence(group.rgqueue, 28) &&
    (group.planet.rt == 14 ==> |group.rgqueue| == 0)
  }

  predicate FFleetGroup(group: FleetGroup) {
    FRecord(group.fleet) && group.fleet.rt in {16, 17} &&
    |group.rgorder| == group.cord && |group.rgname| <= 1 &&
    FTaggedSequence(group.rgname, 21) &&
    (forall rec <- group.rgorder :: FRecord(rec) &&
      ((rec.rt == 19 && |rec.rgb| == 18) || (rec.rt == 20 && |rec.rgb| == 8))) &&
    (group.fleet.rt == 17 ==> group.cord == 0 && |group.rgname| == 0)
  }

  predicate FSnapshotProjection(data: SnapshotData) {
    FCurrentBof(data.bof) && data.bof.kind in {Host, Turn} &&
    FTaggedSequence(data.rgplayer, 6) &&
    (forall group <- data.rgplanet :: FPlanetGroup(group)) &&
    FTaggedSequence(data.rgship, 26) &&
    (forall group <- data.rgfleet :: FFleetGroup(group)) &&
    FTaggedSequence(data.rgbase, 26) && FTaggedSequence(data.rgplan, 30)
  }

  predicate FHistoryProjection(data: HistoryData) {
    FCurrentBof(data.bof) && data.bof.kind == History &&
    data.cPlanet <= 32767 && data.cPlanetExtra <= 32767 &&
    |data.rgplanet| == data.cPlanet && FTaggedSequence(data.rgplanet, 14) &&
    |data.rgbFilter| == 49 && FTaggedSequence(data.rgplayer, 6) &&
    FTaggedSequence(data.rgdesign, 26) && FTaggedSequence(data.rgscore, 45) &&
    FTaggedSequence(data.rgai, 41)
  }

  function Frame(rec: Record): Bytes
    requires FRecord(rec)
    ensures |Frame(rec)| == 2 + |rec.rgb|
  { WordBytes(Header(rec.rt, |rec.rgb|)) + rec.rgb }

  // Framing only: payload remains ciphertext except for BOF/EOF. This function
  // does not recognize .xy raw star packs or validate any record's schema.
  function ReadRecord(rgb: Bytes, ib: nat): ReadResult
    requires ib <= |rgb|
    ensures ReadRecord(rgb, ib).Decoded? ==>
      ib + 2 <= ReadRecord(rgb, ib).ibNext <= |rgb| &&
      FRecord(ReadRecord(rgb, ib).rec)
  {
    if ib + 2 > |rgb| then Rejected(ib)
    else
      var w := Word(rgb, ib);
      var cb := w % 1024;
      if ib + 2 + cb > |rgb| then Rejected(ib)
      else Decoded(Record(w / 1024, rgb[ib + 2..ib + 2 + cb]), ib + 2 + cb)
  }

  lemma FrameRoundTrip(rec: Record)
    requires FRecord(rec)
    ensures ReadRecord(Frame(rec), 0) == Decoded(rec, 2 + |rec.rgb|)
  {
    var rgb := Frame(rec);
    assert rgb[2..] == rec.rgb;
  }

  function CbRecords(rgrec: seq<Record>): nat
    decreases |rgrec|
  {
    if |rgrec| == 0 then 0
    else 2 + |rgrec[0].rgb| + CbRecords(rgrec[1..])
  }

  // UTF-8 byte conditions, excluding NUL for application text. Restricting E0,
  // ED, F0 and F4 excludes overlong encodings, surrogates and values > U+10FFFF.
  predicate FContinuation(b: U8) { 128 <= b <= 191 }

  function CbCodePoint(rgb: Bytes, ib: nat): nat {
    if ib >= |rgb| then 0
    else if 1 <= rgb[ib] <= 127 then 1
    else if ib + 2 <= |rgb| && 194 <= rgb[ib] <= 223 &&
            FContinuation(rgb[ib + 1]) then 2
    else if ib + 3 <= |rgb| && FContinuation(rgb[ib + 2]) &&
            ((rgb[ib] == 224 && 160 <= rgb[ib + 1] <= 191) ||
             (225 <= rgb[ib] <= 236 && FContinuation(rgb[ib + 1])) ||
             (rgb[ib] == 237 && 128 <= rgb[ib + 1] <= 159) ||
             (238 <= rgb[ib] <= 239 && FContinuation(rgb[ib + 1]))) then 3
    else if ib + 4 <= |rgb| && FContinuation(rgb[ib + 2]) &&
            FContinuation(rgb[ib + 3]) &&
            ((rgb[ib] == 240 && 144 <= rgb[ib + 1] <= 191) ||
             (241 <= rgb[ib] <= 243 && FContinuation(rgb[ib + 1])) ||
             (rgb[ib] == 244 && 128 <= rgb[ib + 1] <= 143)) then 4
    else 0
  }

  predicate FUtf8(rgb: Bytes)
    decreases |rgb|
  {
    |rgb| == 0 ||
    (0 < CbCodePoint(rgb, 0) <= |rgb| && FUtf8(rgb[CbCodePoint(rgb, 0)..]))
  }

  predicate FName(rgb: Bytes) { |rgb| <= 31 && FUtf8(rgb) }

  // Proposed truncation postcondition; does not promise grapheme preservation.
  predicate FTextPrefix(rgb: Bytes, rgbOut: Bytes, cbMax: nat) {
    FUtf8(rgb) && FUtf8(rgbOut) && |rgbOut| <= cbMax &&
    |rgbOut| <= |rgb| && rgbOut == rgb[..|rgbOut|] &&
    (|rgbOut| == |rgb| || |rgbOut| + CbCodePoint(rgb, |rgbOut|) > cbMax)
  }

  predicate FCurrentVersion(bof: Bof) {
    bof.wVersion / 4096 == 2 && (bof.wVersion / 32) % 128 == 85
  }

  predicate FLegacyRaceVersion(bof: Bof) {
    bof.wVersion / 4096 == 2 && 49 <= (bof.wVersion / 32) % 128 <= 84
  }

  // Strict decoded current-version envelope. Unknown kinds and signature/framing failures must
  // be rejected before constructing this decoded Bof; this is not FOpenFile.
  predicate FCurrentBof(bof: Bof) {
    FCurrentVersion(bof) && bof.salt < 2048 && bof.wGen < 8 &&
    (if bof.kind in {Universe, Host, Race} then bof.iPlayer == -1
     else 0 <= bof.iPlayer < 16)
  }

  predicate FGame(game: Game) {
    game.mdSize <= 4 && game.mdDensity <= 3 && game.mdStartDist <= 3 &&
    1 <= game.cPlayer <= 16 && game.cPlanMax <= 999 &&
    |game.rgvc| == 12 && FName(game.rgbTitle)
  }

  predicate FStarPack(star: StarPack) {
    star.dx < 1024 && star.y < 4096 && star.idName < 1024
  }

  // Geometry/catalog bounds and victory semantics remain M03/S03 conditions.
  predicate FUniverse(data: UniverseData) {
    FGame(data.game) && |data.rgstar| == data.game.cPlanMax &&
    forall i | 0 <= i < |data.rgstar| :: FStarPack(data.rgstar[i])
  }

  function CbUniverse(data: UniverseData): nat { 88 + 4 * |data.rgstar| }

  // Companion checks for a proposed strict adapter. History deliberately has
  // no equal-turn requirement. Order generation must equal its starting state.
  predicate FSameGame(a: Bof, b: Bof) { a.lidGame == b.lidGame }

  predicate FOrdersFor(orders: Bof, snapshot: Bof, iPlayer: PlayerId) {
    FCurrentBof(orders) && FCurrentBof(snapshot) &&
    orders.kind == Orders && snapshot.kind in {Host, Turn} &&
    FSameGame(orders, snapshot) && orders.iPlayer == iPlayer &&
    (snapshot.kind == Host || snapshot.iPlayer == iPlayer) &&
    orders.turn == snapshot.turn && orders.wGen == snapshot.wGen
  }

  predicate FHistoryFor(history: Bof, snapshot: Bof) {
    FCurrentBof(history) && FCurrentBof(snapshot) &&
    history.kind == History && snapshot.kind == Turn &&
    FSameGame(history, snapshot) && history.iPlayer == snapshot.iPlayer
  }

  // Root-supported import policy, not the broader observed protected-legacy
  // password-replacement path. A checksum boolean records a prior validation
  // obligation; this model does not implement IRaceChecksum or CP1252 conversion.
  predicate FSupportedRaceImport(data: RaceImport) {
    data.bof.kind == Race && data.fChecksumValid &&
    (FCurrentVersion(data.bof) ||
      (FLegacyRaceVersion(data.bof) && !data.fProtected && data.fLegacyPayloadExact))
  }

  // This set describes commands handled by FRunLogRecord, not all possible HDRs.
  predicate FCommandTag(rt: RecordTag) {
    rt in {0, 1, 2, 3, 4, 5, 10, 11, 23, 24, 25, 27, 29, 30,
           34, 35, 36, 37, 38, 42, 43, 44, 46}
  }

  predicate FOrdersEnvelope(data: OrdersData) {
    FCurrentBof(data.bof) && data.bof.kind == Orders && data.cbLog <= 32767 &&
    data.cbLog == CbRecords(data.rgrec) &&
    (forall rec <- data.rgrec :: FRecord(rec) && FCommandTag(rec.rt)) &&
    (forall rec <- data.rgmsg :: FRecord(rec) && rec.rt == 40)
  }

  // Semantic payload shapes are decoded data. Parameterless tasks remain
  // distinct even though their 18-byte union can contain unused storage.
  datatype ObjectRef = NoObject | Planet(idPlanet: nat) | Fleet(idFleet: U16)
                     | FreeSpace | Thing(idThing: U16)
  datatype ItemAction = ItemAction(iAction: nat, cQuantity: nat)
  datatype WaypointTask = NoTask | Transport(rgaction: seq<ItemAction>)
                       | Colonize | RemoteMine | Merge | Scrap
                       | LayMines(cTime: U16, cTimeOld: U16)
                       | Patrol(iWarp: U16, iDist: U16) | AutoRoute
                       | Give(iPlayer: U16)
  datatype Waypoint = Waypoint(x: I16, y: I16, target: ObjectRef,
                               iWarp: nat, task: WaypointTask,
                               fValidTask: bool, fNoAutoTrack: bool)

  predicate FWaypointShape(order: Waypoint) {
    order.iWarp < 16 &&
    (order.target.Planet? ==> order.target.idPlanet < 999) &&
    (order.task.Transport? ==>
      |order.task.rgaction| == 5 &&
      forall a <- order.task.rgaction :: a.iAction <= 9 && a.cQuantity < 4096)
  }

  // A successful later-turn publication's observable effect (ADR 0001).
  // It models an in-memory transition, not filesystem atomicity or rollback.
  datatype ManagedGame = ManagedGame(lidGame: U32,
    turns: map<PlayerId, Bof>, orders: map<PlayerId, Bytes>,
    histories: map<PlayerId, Bytes>, backups: seq<Bytes>)

  predicate FManagedGame(game: ManagedGame) {
    forall iPlayer <- game.turns.Keys ::
      FCurrentBof(game.turns[iPlayer]) && game.turns[iPlayer].kind == Turn &&
      game.turns[iPlayer].iPlayer == iPlayer &&
      game.turns[iPlayer].lidGame == game.lidGame
  }

  predicate FLaterTurn(game: ManagedGame, incoming: Bof, iPlayer: PlayerId) {
    FManagedGame(game) && iPlayer in game.turns &&
    FCurrentBof(incoming) && incoming.kind == Turn &&
    incoming.iPlayer == iPlayer && incoming.lidGame == game.lidGame &&
    incoming.turn > game.turns[iPlayer].turn
  }

  function PublishLaterTurn(game: ManagedGame, incoming: Bof,
                            iPlayer: PlayerId): ManagedGame
    requires FLaterTurn(game, incoming, iPlayer)
    ensures FManagedGame(PublishLaterTurn(game, incoming, iPlayer))
    ensures PublishLaterTurn(game, incoming, iPlayer).lidGame == game.lidGame
    ensures PublishLaterTurn(game, incoming, iPlayer).turns[iPlayer] == incoming
    ensures iPlayer !in PublishLaterTurn(game, incoming, iPlayer).orders
    ensures PublishLaterTurn(game, incoming, iPlayer).histories == game.histories
    ensures PublishLaterTurn(game, incoming, iPlayer).backups == game.backups
    ensures forall other: PlayerId | other != iPlayer ::
      (other in PublishLaterTurn(game, incoming, iPlayer).turns <==>
        other in game.turns) &&
      (other in game.turns ==>
        PublishLaterTurn(game, incoming, iPlayer).turns[other] == game.turns[other])
    ensures forall other: PlayerId | other != iPlayer ::
      (other in PublishLaterTurn(game, incoming, iPlayer).orders <==>
        other in game.orders) &&
      (other in game.orders ==>
        PublishLaterTurn(game, incoming, iPlayer).orders[other] == game.orders[other])
  {
    ManagedGame(game.lidGame, game.turns[iPlayer := incoming],
      game.orders - {iPlayer}, game.histories, game.backups)
  }

  // A proposed decode-before-publish adapter has this failure contract. The
  // existing global-state loaders and replay do NOT provide this guarantee.
  datatype Candidate<T> = Invalid(ibError: nat) | Valid(value: T)

  function PublishDecoded<T>(prior: T, candidate: Candidate<T>): T
    ensures candidate.Invalid? ==> PublishDecoded(prior, candidate) == prior
    ensures candidate.Valid? ==> PublishDecoded(prior, candidate) == candidate.value
  { if candidate.Valid? then candidate.value else prior }

  // Checked witnesses and counterexamples, not assumptions about C behavior.
  lemma FramingVectors()
    ensures WordBytes(Header(8, 16)) == [16, 32]
    ensures ReadRecord([16], 0) == Rejected(0)
    ensures ReadRecord([16, 32], 0) == Rejected(0)
    ensures ReadRecord([0, 0], 0) == Decoded(Record(0, []), 2)
  {}

  lemma TextVectors()
    ensures FName([65]) && FName([195, 169])
    ensures !FName([195]) && !FName([192, 128]) && !FName([0])
    ensures !FName([237, 160, 128]) && !FName([244, 144, 128, 128])
    ensures FTextPrefix([65, 195, 169], [65], 2)
    ensures !FTextPrefix([65, 195, 169], [65, 195], 2)
  {}

  lemma CompatibilityVectors()
  {
    var turn := Bof(10912, 1, 10, 0, 0, Turn, 2, false, false);
    var history := Bof(10912, 1, 9, 0, 0, History, 0, false, false);
    var orders := Bof(10912, 1, 10, 0, 0, Orders, 2, false, true);
    assert FHistoryFor(history, turn);
    assert FOrdersFor(orders, turn, 0);
    assert !FOrdersFor(orders.(wGen := 3), turn, 0);
    assert !FOrdersFor(orders.(turn := 9), turn, 0);
    assert FCurrentVersion(turn.(wVersion := 10913));
    var legacy := RaceImport(Bof(10880, 0, 0, -1, 0, Race, 0, false, false),
                             false, true, true);
    assert FSupportedRaceImport(legacy);
    assert !FSupportedRaceImport(legacy.(fProtected := true));
  }
}
