include "M05-initial-state.dfy"
include "S01-files.dfy"

// M05 execution/event contracts. These constrain a supplied creation trace;
// they do not claim to execute the C generator or establish its final invariant.
module M05Creation {
  import opened S04Arithmetic
  import U = M03Setup
  import I = M05InitialState
  import R = SharedGameState
  import S = S01
  datatype Stage = Coordinates | Names | Planets | PlanetZeroStocks | AssignHomes
    | Homes | FleetsAndSecondWorlds | ClearPlanetZero | BattlePlans | Wormholes
    | Tampering | SinglePlayer | Identity | UniverseFile | HostFile | PlayerFiles | OpenSession | Finalize
  function GenerationOrder(): seq<Stage> {
    [Coordinates,Names,Planets,PlanetZeroStocks,AssignHomes,Homes,FleetsAndSecondWorlds,
      ClearPlanetZero,BattlePlans,Wormholes,Tampering,SinglePlayer,Identity,UniverseFile,HostFile,PlayerFiles,OpenSession,Finalize]
  }
  function StagePlan(fBatch: bool,fManaged: bool): seq<Stage> {
    GenerationOrder()[..16]+(if fBatch then [] else [OpenSession])+(if fManaged then [Finalize] else [])
  }
  predicate FCompletedStages(stages: seq<Stage>,fBatch: bool,fManaged: bool) {
    stages==StagePlan(fBatch,fManaged)
  }
  datatype Event = Event(stage: Stage,player: int,operation: string)
  function AssignmentEvents(i: nat,fTemplate: bool): seq<Event> {
    [Event(AssignHomes,i,"swap home")]+(if fTemplate then [Event(AssignHomes,i,"CreateRandomRace")] else [])+
    [Event(AssignHomes,i,"technology"),Event(AssignHomes,i,"tip 0"),Event(AssignHomes,i,"tip 1"),Event(AssignHomes,i,"tip 2"),Event(AssignHomes,i,"tip 3")]
  }
  function FleetEvents(i: nat,fSecond: bool): seq<Event> {
    [Event(FleetsAndSecondWorlds,i,"starting fleets and fuel")]+
    (if fSecond then [Event(FleetsAndSecondWorlds,i,"second world and design 0 fleet")] else [])+
    [Event(FleetsAndSecondWorlds,i,"upgrade designs")]
  }
  predicate FStageOrder(events: seq<Event>) {
    forall i,j | 0<=i<j<|events| ::
      forall a,b | 0<=a<|GenerationOrder()| && 0<=b<|GenerationOrder()| && GenerationOrder()[a]==events[i].stage && GenerationOrder()[b]==events[j].stage :: a<=b
  }
  function HomeSwap(homes: seq<int>,i: nat,rng: R.Rng): (seq<int>,R.Rng)
    requires i<|homes|
  { var d := Take(rng,|homes|-i); var j := i+d.value;
    (homes[i:=homes[j]][j:=homes[i]],d.rng) }
  datatype Participant = Participant(fAI: bool,fMaid: bool,salt: int,fHacker: bool)
  datatype Scan = Scan(players: seq<Participant>,humans: nat,selected: int)
  function SingleScan(players: seq<Participant>,i: nat,humans: nat,selected: int): Scan
    requires i<=|players| decreases |players|-i
  {
    if i==|players| || humans>=2 then Scan(players,humans,selected)
    else if !players[i].fAI || players[i].fMaid then SingleScan(players,i+1,humans+1,i)
    else SingleScan(players[i:=players[i].(salt:=156085230)],i+1,humans,selected)
  }
  function Relation(owner: nat,other: nat,fSingle: bool): int { if fSingle && owner!=other then 2 else 0 }
  function PlanAttack(index: nat,catalogAttack: int,fIncomingSingle: bool): int {
    if index==0 && fIncomingSingle then 3 else catalogAttack
  }
  function PlayerIds(count: nat): set<int> decreases count { if count==0 then {} else PlayerIds(count-1)+{count-1} }
  function TamperingRecipients(players: seq<Participant>,sender: nat): set<int>
    requires sender<|players|
  { if !players[sender].fHacker then {} else if players[sender].fAI then {sender} else PlayerIds(|players|) }
  datatype Tutorial = Tutorial(players: nat,size: U.Size,density: U.Density,distance: U.Distance,
    options: U.Options,lid: int,turn: nat,victory: seq<int>,rng: R.Rng,removed: seq<string>)
  function TutorialSetup(): Tutorial {
    Tutorial(2,0,0,1,U.Options(false,false,true,true,false,true,false),9236297,0,
      [0,0,0,0,0,0,0,128,129,0],Seed(1234567890,false),[".h1",".x1",".h2",".x2"])
  }
  function GameIdentity(fTutorial: bool,tick: int): int { if fTutorial then 9236297 else tick%4294967296 }
  // Each file operation supplies its own observed clock and consumes its own RNG draw.
  function Salt(clock: int,rng: R.Rng): DrawResult {
    var d := Take(rng,2000); DrawResult(d.rng,(clock+d.value)%2048)
  }
  datatype GenerationResult = DestroyedFailure | LoadFailure | ReturnedTrue
  function GeneratorReturn(fAllocation: bool,fUniverseWrite: bool,fBatch: bool,fSingle: bool,fLoad: bool): GenerationResult {
    if !fAllocation || !fUniverseWrite then DestroyedFailure
    else if !fBatch && fSingle && !fLoad then LoadFailure else ReturnedTrue
  }
  function BatchReturn(fReachedCall: bool,result: GenerationResult): bool { fReachedCall }
  predicate FAllComplete(xs: seq<bool>) decreases |xs| { |xs|==0 || (xs[0] && FAllComplete(xs[1..])) }
  predicate FPublished(fGenerated: bool,fUniverseComplete: bool,fHostComplete: bool,playersComplete: seq<bool>,count: nat,fMetadata: bool) {
    fGenerated && fUniverseComplete && fHostComplete && |playersComplete|==count &&
    FAllComplete(playersComplete) && fMetadata
  }
  // Catalogs/helpers are pinned inputs, not arbitrary substitutes for audited rules.
  datatype Context = Context(revision: S.Bytes,catalogDigest: S.Bytes,textDigest: S.Bytes,
    clocks: seq<int>,simulation: R.RandomContext,fileSeeds: R.Rng,entry: string,
    fGameTutorial: bool,fContextTutorial: bool,fIncomingSingle: bool)
  predicate FContext(c: Context) {
    |c.revision|>0 && |c.catalogDigest|>0 && |c.textDigest|>0 && R.RngValid(c.simulation.current) &&
    |c.simulation.stack|<=4 && c.entry in {"desktop","batch","tutorial"}
  }
  lemma CreationVectors() {
    assert GeneratorReturn(true,true,true,false,false)==ReturnedTrue;
    assert BatchReturn(true,DestroyedFailure);
    assert !FPublished(true,true,true,[true,false],2,true) by {
      if FPublished(true,true,true,[true,false],2,true) { PublicationNeedsEveryPlayer([true,false],2,1); }
    }
    assert PlanAttack(0,1,false)==1 && PlanAttack(0,1,true)==3;
    assert Relation(0,1,true)==2 && Relation(0,0,true)==0;
    var roster := [Participant(false,false,1,false),Participant(false,false,2,false),Participant(true,false,3,false)];
    assert SingleScan(roster,0,0,-1).players[2].salt==3;
  }
  lemma PublicationNeedsEveryPlayer(xs: seq<bool>,count: nat,i: nat)
    requires FPublished(true,true,true,xs,count,true) && i<count
    ensures xs[i]
  {
    AllCompleteIndex(xs,i);
  }
  lemma AllCompleteIndex(xs: seq<bool>,i: nat)
    requires FAllComplete(xs) && i<|xs|
    ensures xs[i]
    decreases i
  { if i>0 { AllCompleteIndex(xs[1..],i-1); } }
}
