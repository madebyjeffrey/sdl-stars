include "S01-orders.dfy"
module S01Files {
  import opened S01
  import opened S01Codecs
  import opened S01Records
  import opened S01Orders
  datatype Snapshot = Snapshot(bof: Bof, battles: seq<seq<Record>>, players: seq<Record>,
    credential: seq<Record>, messages: seq<Record>, playerMessages: seq<Record>,
    planets: seq<PlanetGroup>, ships: seq<Record>, fleets: seq<FleetGroup>,
    bases: seq<Record>, scores: seq<Record>, things: seq<Record>, plans: seq<Record>)
  function Flatten(groups: seq<seq<Record>>): seq<Record>
    decreases |groups|
  { if |groups|==0 then [] else groups[0]+Flatten(groups[1..]) }
  function PlanetRecords(groups: seq<PlanetGroup>): seq<Record>
    decreases |groups|
  { if |groups|==0 then [] else [groups[0].planet]+groups[0].rgqueue+PlanetRecords(groups[1..]) }
  function FleetRecords(groups: seq<FleetGroup>): seq<Record>
    decreases |groups|
  { if |groups|==0 then [] else [groups[0].fleet]+groups[0].rgorder+groups[0].rgname+FleetRecords(groups[1..]) }
  function SnapshotRecords(s: Snapshot): seq<Record> {
    Flatten(s.battles)+s.players+s.credential+s.messages+s.playerMessages+PlanetRecords(s.planets)+
    s.ships+FleetRecords(s.fleets)+s.bases+s.scores+
    (if |s.things|==0 then [] else [Record(43,LE(|s.things|,2))]+s.things)+s.plans+
    [Record(0,LE(s.bof.turn,2))]
  }
  predicate FPlayers(players: seq<Record>) {
    (forall r <- players :: FTagged(r,6) && FPlayer(r.rgb,false,false)) &&
    (forall i | 0<=i<|players| :: players[i].rgb[0]<16) &&
    (forall i,j | 0<=i<j<|players| :: players[i].rgb[0]<players[j].rgb[0])
  }
  function PlayerCount(players: seq<Record>, kind: nat): nat
    requires FPlayers(players) && kind<4
    decreases |players|
  {
    if |players|==0 then 0 else
      (if kind==0 then players[0].rgb[1] as int else if kind==1 then W(players[0].rgb,4)/4096
       else if kind==2 then W(players[0].rgb,2) else W(players[0].rgb,4)%4096)+
      PlayerCount(players[1..],kind)
  }
  // Design ownership is positional: each player's counted group is contiguous.
  predicate FDesignGroups(players: seq<Record>, designs: seq<Record>, fBase: bool)
    requires FPlayers(players)
    decreases |players|
  {
    if |players|==0 then |designs|==0
    else var count:=if fBase then W(players[0].rgb,4)/4096 else players[0].rgb[1] as int;
      count<=|designs| &&
      (forall r <- designs[..count] :: FTagged(r,26) && FDesign(r.rgb) &&
        (if fBase then 16<=Bits(W(r.rgb,0),10,5)<=25 else Bits(W(r.rgb,0),10,5)<16)) &&
      FDesignGroups(players[1..],designs[count..],fBase)
  }
  predicate FSnapshot(s: Snapshot, args: seq<nat>) {
    FCurrentBof(s.bof) && s.bof.kind in {Host,Turn} && FPlayers(s.players) &&
    FDesignGroups(s.players,s.ships,false) && FDesignGroups(s.players,s.bases,true) &&
    |s.credential|<=1 && (forall r <- s.credential :: FTagged(r,36) && |r.rgb|==4) &&
    (s.bof.kind==Host ==> |s.battles|==0 && |s.messages|==0 && |s.playerMessages|==0 && |s.scores|==0) &&
    (s.bof.kind==Turn ==> |s.credential|==0) &&
    (forall g <- s.battles :: FBattleChunks(g)) &&
    (forall r <- s.messages :: FTagged(r,12) && CatalogEnd(r.rgb,args)==|r.rgb|) &&
    (forall r <- s.playerMessages :: FTagged(r,40) && FPlayerMessage(r.rgb,true)) &&
    (forall g <- s.planets :: FPlanetGroup(g) && FPlanet(g.planet.rgb,g.planet.rt,false) &&
       forall q <- g.rgqueue :: FQueue(q.rgb)) &&
    (forall g <- s.fleets :: FFleetGroup(g) && FFleet(g.fleet.rgb,g.fleet.rt) &&
       (g.fleet.rt==16 ==> g.cord==g.fleet.rgb[|g.fleet.rgb|-1]) &&
       (forall r <- g.rgorder :: FWaypoint(r.rgb,r.rt)) &&
       (forall r <- g.rgname :: FExactName(r.rgb))) &&
    |s.fleets|==PlayerCount(s.players,3) &&
    (forall r <- s.scores :: FTagged(r,45) && |r.rgb|==24) &&
    |s.things|<=32767 && (forall r <- s.things :: FTagged(r,43) && FThing(r.rgb)) &&
    (forall r <- s.plans :: FTagged(r,30) && FPlan(r.rgb,false))
  }
  predicate FOrdersFile(data: OrdersData,cPlayer: nat) {
    FOrdersEnvelope(data) && (forall r <- data.rgrec :: FCommand(r,cPlayer)) &&
    (forall r <- data.rgmsg :: FPlayerMessage(r.rgb,true))
  }
  function OrdersRecords(data: OrdersData): seq<Record> {
    [Record(9,LE(data.cbLog,2)+Zeros(15))]+data.rgrec+data.rgmsg+[Record(0,[])]
  }
  predicate FHistory(data: HistoryData, ai: AiKind,cPlanMax: nat) {
    FHistoryProjection(data) &&
    (forall r <- data.rgplanet :: FPlanet(r.rgb,14,true)) &&
    (forall r <- data.rgplayer :: FPlayer(r.rgb,false,false) && r.rgb[0]!=data.bof.iPlayer) &&
    (forall r <- data.rgdesign :: FDesign(r.rgb)) &&
    (forall r <- data.rgscore :: |r.rgb|==24 && Bits(W(r.rgb,0),15,1)==1 &&
      (if data.bof.turn>=100 then data.bof.turn-100 else 0)<=W(r.rgb,2)<=data.bof.turn) &&
    (|data.rgai|==0 || FAiHistory(Join(data.rgai),ai,cPlanMax))
  }
  function HistoryRecords(data: HistoryData): seq<Record> {
    [Record(32,LE(data.cPlanet,2)+LE(data.cPlanetExtra,2))]+data.rgplanet+
    [Record(33,data.rgbFilter)]+data.rgplayer+data.rgdesign+data.rgscore+data.rgai+[Record(0,[])]
  }
  datatype Segment = Segment(bof: Bof, records: seq<Record>)
  predicate FSegmentEnvelope(segment: Segment) {
    FCurrentBof(segment.bof) && |segment.records|>0 &&
    (forall r <- segment.records :: FRecord(r)) &&
    (forall i | 0<=i<|segment.records|-1 :: segment.records[i].rt!=0 && segment.records[i].rt!=8) &&
    segment.records[|segment.records|-1].rt==0 &&
    (if segment.bof.kind in {Orders,History} then |segment.records[|segment.records|-1].rgb|==0
     else |segment.records[|segment.records|-1].rgb|==2)
  }
  // Proposed strict multi-segment identity check; stronger than the core reader.
  predicate FAppendedTurns(segments: seq<Segment>) {
    |segments|>0 && (|segments|>1 ==> segments[0].bof.fMulti) &&
    (forall s <- segments :: FSegmentEnvelope(s) && s.bof.kind==Turn &&
      s.bof.lidGame==segments[0].bof.lidGame && s.bof.iPlayer==segments[0].bof.iPlayer &&
      W(s.records[|s.records|-1].rgb,0)==s.bof.turn) &&
    (forall i | 0<=i<|segments|-1 :: segments[i].bof.turn<segments[i+1].bof.turn)
  }
  predicate FLibrarySegments(segments: seq<Segment>) {
    |segments|==1 && FSegmentEnvelope(segments[0]) && segments[0].bof.lidGame!=0
  }
  datatype Family = Family(universe: Bof, cPlayer: nat, members: seq<Bof>)
  predicate FLibraryFamily(family: Family) {
    FCurrentBof(family.universe) && family.universe.kind==Universe && family.universe.lidGame!=0 &&
    1<=family.cPlayer<=16 &&
    (exists b <- family.members :: b.kind in {Host,Turn}) &&
    (forall b <- family.members :: FCurrentBof(b) && b.lidGame==family.universe.lidGame &&
      b.kind in {Host,Turn,Orders,History}) &&
    (forall i,j | 0<=i<j<|family.members| ::
      family.members[i].kind==Turn && family.members[j].kind==Turn ==>
      family.members[i].iPlayer!=family.members[j].iPlayer)
  }
  predicate FHostOrders(family: Family, host: Bof, orders: Bof,iPlayer: PlayerId) {
    FLibraryFamily(family) && host in family.members && host.kind==Host &&
    iPlayer<family.cPlayer && FOrdersFor(orders,host,iPlayer)
  }
  // Observed load repairs and information projection are value-level relations.
  function ObservationDetail(prior: nat,incoming: nat,fHistory: bool): nat {
    if fHistory && prior>incoming then prior else incoming
  }
  predicate FScoreVisible(iOwner: PlayerId,iRecipient: PlayerId,fDead: bool,
                          fGameOver: bool,fPublic: bool,turn: U16) {
    iOwner==iRecipient || fDead || fGameOver || (fPublic && turn>=20)
  }
  function ClaimAdjusterPlayer(rgb: Bytes): Bytes
    requires |rgb|==192
    ensures |ClaimAdjusterPlayer(rgb)|==192
  { rgb[..8]+Zeros(8)+rgb[16..25]+Zeros(103)+rgb[128..] }
  // Core opening and library inspection have different observed checks.
  predicate FCoreVersionPlayer(bof: Bof,expected: int,lid: U32,turn: U16,requestedKind: FileKind) {
    FCurrentVersion(bof) && bof.iPlayer==expected && (lid==0 || bof.lidGame==lid) &&
    (requestedKind==History || turn==0 || bof.turn==turn)
  }
  predicate FCoreMultiFooterObserved(rt: RecordTag,cb: nat) { rt==0 || cb==2 }
  predicate FStrictFooter(rt: RecordTag,cb: nat) { rt==0 && cb==2 }
  datatype WireResult = WireResult(rgb: Bytes,state: XorState)
  function WireRecords(records: seq<Record>,state: XorState): WireResult
    requires forall r <- records :: FRecord(r)
    ensures |WireRecords(records,state).rgb|==CbRecords(records)
    decreases |records|
  {
    if |records|==0 then WireResult([],state)
    else
      assert records[0] in records;
      assert FRecord(records[0]);
      assert forall r <- records[1..] :: FRecord(r);
      var r:=records[0]; var transformed:=Transform(r.rt,r.rgb,state);
      var rest:=WireRecords(records[1..],transformed.state);
      WireResult(WordBytes(Header(r.rt,|r.rgb|))+transformed.rgb+rest.rgb,rest.state)
  }
  function SegmentWire(rgbBof: Bytes,segment: Segment): Bytes
    requires FBofBytes(rgbBof,segment.bof) && FSegmentEnvelope(segment)
  {
    Frame(Record(8,rgbBof))+WireRecords(segment.records,
      InitXor(segment.bof.lidGame,segment.bof.salt,segment.bof.turn,segment.bof.iPlayer,
        Bits(W(rgbBof,14),12,1)==1)).rgb
  }
  function StarBytes(stars: seq<StarPack>): Bytes
    requires forall star <- stars :: FStarPack(star)
    ensures |StarBytes(stars)|==4*|stars|
    decreases |stars|
  { if |stars|==0 then [] else LE(StarWord(stars[0]),4)+StarBytes(stars[1..]) }
  function UniverseWire(rgbBof: Bytes,bof: Bof,rgbGame: Bytes,data: UniverseData): Bytes
    requires FBofBytes(rgbBof,bof) && FCurrentBof(bof) && bof.kind==Universe
    requires FUniverse(data) && FGameBytes(rgbGame,data.game) && bof.lidGame==data.game.lid
    ensures |UniverseWire(rgbBof,bof,rgbGame,data)|==88+4*data.game.cPlanMax
  {
    Frame(Record(8,rgbBof))+WireRecords([Record(7,rgbGame)],
      InitXor(bof.lidGame,bof.salt,bof.turn,bof.iPlayer,Bits(W(rgbBof,14),12,1)==1)).rgb+
      StarBytes(data.rgstar)+Frame(Record(0,LE(data.game.cPlayer,2)))
  }
  function RaceRecords(player: Bytes,checksum: U16): seq<Record> {
    [Record(6,player),Record(0,LE(checksum,2))]
  }
  predicate FRaceFile(bof: Bof,player: Bytes,checksum: U16) {
    bof.kind==Race &&
    (FCurrentVersion(bof) || FLegacyRaceVersion(bof)) &&
    FRacePayload(player,checksum,FLegacyRaceVersion(bof),(bof.wVersion/32)%128<55)
  }
  predicate FLegacyRaceOutput(bof: Bof,raw: Bytes,checksum: U16,name: Bytes,names: Bytes,
    fNameConsent: bool,policy: LegacyPolicy,password: Bytes,fPasswordConsent: bool,newSalt: U32) {
    FLegacyRaceVersion(bof) && FRaceFile(bof,raw,checksum) &&
    var first:=ReadText(raw[PlayerNamesOffset(raw)..],31,false);
    first.TextValue? &&
    var single:=(bof.wVersion/32)%128<55;
    var second:=if single then TextValue([],0)
      else ReadText(raw[PlayerNamesOffset(raw)+first.cbUsed..],31,false);
    second.TextValue? &&
    FLegacyConversion(first.rgbText,second.rgbText,single,name,names,fNameConsent,D(raw,12)!=0,
      policy,password,fPasswordConsent) &&
    (if D(raw,12)==0 then newSalt==0 else 0 !in password && newSalt==Password(password))
  }
  lemma AppendedImportMismatch(a: Segment,b: Segment)
    requires FAppendedTurns([a,b])
    ensures !FLibrarySegments([a,b])
  {}
  lemma FileBoundaryVectors()
    ensures FCoreMultiFooterObserved(1,2) && !FStrictFooter(1,2)
    ensures !FScoreVisible(1,0,false,false,true,19)
    ensures FScoreVisible(1,0,false,false,true,20)
    ensures ObservationDetail(7,3,true)==7
  {}
}
