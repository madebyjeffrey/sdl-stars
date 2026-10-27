include "M10-construction-damage.dfy"
include "M09-construction-queues.dfy"
include "M08-mass-driver-capability.dfy"
include "M08-terraform-selection.dfy"

// FBuildObject's applied construction effects. Quantities and paid resources
// come from M09; helper results here retain their native widths and call order.
module M19ConstructionEffects {
  import S = S01
  import C = S01Codecs
  import N = M09Builder
  import G = SharedGameState
  import H = M09HostProduction
  import MD = M08MassDriver
  import T = M08TerraformSelection
  import A = M11NativeAvailability
  import TH = M08NativeHabitat
  import E = M08NativeEconomy
  import Cap = M08ColonyCapabilities
  import DB = M11DesignBuildability
  datatype Dispatch = Reject | InvalidDesign | Ship(index: nat) | Base(index: nat) | Planet(item: nat)
  function Route(kind: S.U16,item: S.I16,hasBase: bool): Dispatch {
    if kind==2 then (if item>=16 then (if item<26 then Base(item-16) else InvalidDesign)
      else if !hasBase then Reject else if item<0 then InvalidDesign else Ship(item))
    else if kind!=1 || item<0 || item>27 then Reject else Planet(item)
  }
  datatype PlanetFamily = InstallationFamily | TerraformFamily | GenesisFamily | ScannerFamily | PacketFamily | NoOpFamily
  function Family(item: nat): PlanetFamily
    requires item<=27
  {
    if item in {0,1,2,7,8,9} then InstallationFamily else if item in {4,5,12} then TerraformFamily
    else if item==13 then GenesisFamily else if item>=18 then ScannerFamily
    else if item in {6,14,15,16,17} then PacketFamily else NoOpFamily
  }
  datatype Installation = Installation(accepted: bool,installed: nat,applied: int,message: int,plural: bool)
  function Install(prior: nat,maximum: S.I16,built: S.I16,removedMessage: S.I16): Installation
    requires prior<4096
    ensures Install(prior,maximum,built,removedMessage).installed<4096
  {
    var allowed:=N.Signed16(maximum-prior);
    var count:=if built>=allowed then allowed else built;
    if count<=0 then Installation(false,prior,0,0,false) else
    var message:=N.Signed16(count+removedMessage);
    Installation(true,(prior+count)%4096,count,message,message>1)
  }
  datatype Counts = Counts(built: S.U32,exist: S.U32)
  function BaseCounters(rows: seq<Counts>,priorIndex: nat,next: nat,hadBase: bool): seq<Counts>
    requires |rows|==10 && next<10 && (hadBase ==> priorIndex<10)
    ensures |BaseCounters(rows,priorIndex,next,hadBase)|==10
  {
    var after:=if hadBase then rows[priorIndex:=rows[priorIndex].(exist:=(rows[priorIndex].exist-1)%4294967296)] else rows;
    after[next:=Counts((after[next].built+1)%4294967296,(after[next].exist+1)%4294967296)]
  }
  function Downgrade(hadBase: bool,oldHull: S.U16,newHull: S.U16): bool {
    hadBase && N.Signed16(oldHull)>N.Signed16(newHull)
  }
  function BaseMessage(cargo: S.U16): nat { if cargo==0 then 0 else if cargo==65535 then 2 else 1 }
  datatype BaseEvent = Availability | BuiltNotice | InvalidateShips | WarpBefore | DecrementOld
    | SetBase | WarpAfter | SetFling | ClearFling | InvalidatePackets | IncrementNew | ReturnTrue | ReturnFalse
  function BasePlan(free: bool,canBuild: bool,priorBase: bool,commitBase: bool,oldHull: S.U16,newHull: S.U16,
    oldAbility: S.I16,newAbility: S.I16): seq<BaseEvent> {
    if free then [ReturnFalse] else if !canBuild then [Availability,ReturnFalse] else
    [Availability,BuiltNotice]+(if Downgrade(priorBase,oldHull,newHull) then [InvalidateShips] else [])+
    [WarpBefore]+(if commitBase then [DecrementOld] else [])+[SetBase]+
    (if oldAbility>0 then [] else [WarpAfter]+(if newAbility>0 then [SetFling] else [ClearFling,InvalidatePackets]))+
    [IncrementNew,ReturnTrue]
  }
  datatype Fling = Fling(warp: nat,destination: nat,killPackets: bool,queryNew: bool)
  function BaseFling(priorWarp: nat,destination: nat,oldAbility: S.I16,newAbility: S.I16,newTwo: S.I16): Fling
    requires priorWarp<16 && destination<1024
    ensures BaseFling(priorWarp,destination,oldAbility,newAbility,newTwo).warp<16
  {
    if oldAbility>0 then Fling(priorWarp,destination,false,false) else
    if newAbility>0 then Fling((newAbility+newTwo-4)%16,destination,false,true)
    else Fling(0,0,true,true)
  }
  datatype DesignFlingResult = DesignFlingResult(before: MD.Outcome,after: G.Option<MD.Outcome>,fling: G.Option<Fling>)
  // Called after eligibility/notice/downgrade handling. Returning providers and
  // unchanged owner/current/design projections between reads are explicit.
  function DesignFling(owner: S.I16,hasBase: bool,prior: nat,next: nat,current: S.I16,
    rows: seq<MD.Roster>,priorWarp: nat,destination: nat): DesignFlingResult
    requires prior<16 && next<10 && MD.RostersFit(rows) && priorWarp<16 && destination<1024
    ensures DesignFling(owner,hasBase,prior,next,current,rows,priorWarp,destination).fling.Some? ==>
      DesignFling(owner,hasBase,prior,next,current,rows,priorWarp,destination).before.status==MD.Done
    ensures (DesignFling(owner,hasBase,prior,next,current,rows,priorWarp,destination).before.status==MD.Done &&
      DesignFling(owner,hasBase,prior,next,current,rows,priorWarp,destination).before.capability.warp>0) ==>
      DesignFling(owner,hasBase,prior,next,current,rows,priorWarp,destination).after==G.None &&
      DesignFling(owner,hasBase,prior,next,current,rows,priorWarp,destination).fling==G.Some(Fling(priorWarp,destination,false,false))
  {
    var before:=MD.Lookup(owner,hasBase,prior,current,rows,true);
    if before.status.Undefined? then DesignFlingResult(before,G.None,G.None) else
    if before.capability.warp>0 then DesignFlingResult(before,G.None,G.Some(BaseFling(priorWarp,destination,before.capability.warp,0,0))) else
    var after:=MD.Lookup(owner,true,next,current,rows,true);
    DesignFlingResult(before,G.Some(after),if after.status.Undefined? then G.None else
      G.Some(BaseFling(priorWarp,destination,0,after.capability.warp,if after.capability.two then 1 else 0)))
  }
  datatype Terraform = Terraformed(env: seq<int>,completed: nat) | UndefinedAxis(env: seq<int>,completed: nat)
  datatype SelectedTerraform = SelectedTerraform(selection: T.BestResult,application: G.Option<Terraform>)
  function SelectTerraform(p: T.Planet,players: seq<T.Player>,rows: seq<seq<A.PartRow>>,terra: seq<T.Terra>,saved: S.I16,evidence: seq<TH.Evidence>): SelectedTerraform
    requires T.PlanetFits(p) && T.PlayersFit(players) && A.CatalogShape(rows) && |terra|==20
    ensures SelectTerraform(p,players,rows,terra,saved,evidence).application.Some? ==>
      SelectTerraform(p,players,rows,terra,saved,evidence).selection.status==T.Done
  {
    var selected:=T.Best(p,players,rows,terra,saved,true,evidence);
    SelectedTerraform(selected,if selected.status!=T.Done then G.None else G.Some(TerraformStep(p.env,selected.choice)))
  }
  datatype TerraformPipeline = TerraformPipeline(status: T.Status,planet: T.Planet,global: S.I16,calls: nat,remaining: nat)
  function RunTerraform(p: T.Planet,players: seq<T.Player>,rows: seq<seq<A.PartRow>>,terra: seq<T.Terra>,saved: S.I16,remaining: nat,evidence: seq<seq<TH.Evidence>>): TerraformPipeline
    requires T.PlanetFits(p) && T.PlayersFit(players) && A.CatalogShape(rows) && |terra|==20 && remaining<=65535
    ensures T.PlanetFits(RunTerraform(p,players,rows,terra,saved,remaining,evidence).planet)
    ensures RunTerraform(p,players,rows,terra,saved,remaining,evidence).calls+RunTerraform(p,players,rows,terra,saved,remaining,evidence).remaining==remaining
    ensures RunTerraform(p,players,rows,terra,saved,remaining,evidence).status==T.Done ==> RunTerraform(p,players,rows,terra,saved,remaining,evidence).remaining==0 && RunTerraform(p,players,rows,terra,saved,remaining,evidence).global==saved
    decreases remaining
  {
    if remaining==0 then TerraformPipeline(T.Done,p,saved,0,0) else
    var selected:=SelectTerraform(p,players,rows,terra,saved,if |evidence|==0 then [] else evidence[0]);
    if selected.application.None? then TerraformPipeline(selected.selection.status,selected.selection.planet,selected.selection.global,1,remaining-1) else
    var step:=selected.application.value;
    if step.UndefinedAxis? then TerraformPipeline(T.Undefined("construction terraform axis"),p,saved,1,remaining-1) else
    var next:=p.(env:=seq(3,i requires 0<=i<3 => step.env[i] as E.I8));
    var tail:=RunTerraform(next,players,rows,terra,saved,remaining-1,if |evidence|==0 then [] else evidence[1..]);
    tail.(calls:=tail.calls+1)
  }
  function TerraformStep(env: seq<int>,choice: S.I16): Terraform
    requires |env|==3 && forall n <- env :: -128<=n<128
    ensures |TerraformStep(env,choice).env|==3
    ensures forall n <- TerraformStep(env,choice).env :: -128<=n<128
  {
    if choice==0 then Terraformed(env,1) else
    var selected: int:=choice as int;
    var axis: int:=(if selected<0 then -selected else selected)-1;
    if axis>=3 then UndefinedAxis(env,0) else
    var value:=env[axis]+(if choice<=0 then -1 else 1);
    var clamped:=if value<1 then 1 else if value>99 then 99 else value;
    assert 1<=clamped<=99;
    var revised:=env[axis:=clamped];
    assert forall i | 0<=i<3 :: -128<=revised[i]<128 by {
      forall i | 0<=i<3 ensures -128<=revised[i]<128 {
        if i!=axis { assert env[i] in env; }
      }
    }
    Terraformed(revised,1)
  }
  function TerraformCount(built: S.I16): nat { built%65536 }
  datatype TerraformStatus = Finished | Pending | InvalidChoice
  datatype TerraformRun = TerraformRun(env: seq<int>,calls: nat,remaining: nat,status: TerraformStatus)
  function TerraformTrace(env: seq<int>,remaining: nat,choices: seq<S.I16>): TerraformRun
    requires |env|==3 && forall n <- env :: -128<=n<128
    ensures |TerraformTrace(env,remaining,choices).env|==3
    ensures TerraformTrace(env,remaining,choices).calls+TerraformTrace(env,remaining,choices).remaining==remaining
    ensures TerraformTrace(env,remaining,choices).status==Finished ==> TerraformTrace(env,remaining,choices).remaining==0
    decreases remaining
  {
    if remaining==0 then TerraformRun(env,0,0,Finished) else
    if |choices|==0 then TerraformRun(env,0,remaining,Pending) else
    var step:=TerraformStep(env,choices[0]);
    if step.UndefinedAxis? then TerraformRun(env,1,remaining-1,InvalidChoice) else
    var tail:=TerraformTrace(step.env,remaining-1,choices[1..]);
    tail.(calls:=tail.calls+1)
  }
  datatype Genesis = Genesis(env: seq<int>,original: seq<int>,concentration: seq<int>,stocks: seq<int>,mines: nat,factories: nat,defenses: nat,scanner: nat)
  function Rebirth(draws: seq<nat>,ar: bool,mines: nat,factories: nat,defenses: nat,scanner: nat): Genesis
    requires |draws|==12 && mines<4096 && factories<4096 && defenses<4096 && scanner<32
    requires forall i | 0<=i<12 :: draws[i]<(if i%4<2 then 50 else 40)
    ensures |Rebirth(draws,ar,mines,factories,defenses,scanner).env|==3
    ensures forall v <- Rebirth(draws,ar,mines,factories,defenses,scanner).env :: 1<=v<=99
    ensures Rebirth(draws,ar,mines,factories,defenses,scanner).original==Rebirth(draws,ar,mines,factories,defenses,scanner).env
    ensures Rebirth(draws,ar,mines,factories,defenses,scanner).stocks==[0,0,0]
  {
    var env:=seq(3,i requires 0<=i<3 => draws[4*i]+draws[4*i+1]+1);
    Genesis(env,env,seq(3,i requires 0<=i<3 => draws[4*i+2]+draws[4*i+3]+25),[0,0,0],
      if ar then mines else 0,if ar then factories else 0,if ar then defenses else 0,if ar then scanner else 31)
  }
  function GenesisBounds(): seq<nat> { [50,50,40,40,50,50,40,40,50,50,40,40] }
  datatype GenesisEvent = Notify(player: nat) | RaceQuery | ClearInstallations
    | ClearMineral(axis: nat) | RandomEnvironment(axis: nat) | RandomConcentration(axis: nat)
  function GenesisPlan(players: nat,ar: bool): seq<GenesisEvent>
    requires players<=16
  {
    seq(players,i requires 0<=i<players => Notify(i))+[RaceQuery]+(if ar then [] else [ClearInstallations])+
    [ClearMineral(0),RandomEnvironment(0),RandomConcentration(0),ClearMineral(1),RandomEnvironment(1),RandomConcentration(1),
      ClearMineral(2),RandomEnvironment(2),RandomConcentration(2)]
  }
  datatype Scanner = Scanner(value: nat,globalPlayer: int,noticePart: nat)
  function ScannerInstall(item: S.I16,part: S.U8,priorPlayer: S.I16): Scanner
    requires 18<=item<=27
  { if item==27 then Scanner(part%32,-1,part) else Scanner((item-18)%32,priorPlayer,item-18) }
  datatype ScannerSelection = ScannerSelection(result: Cap.ScannerResult,effect: G.Option<Scanner>,global: S.I16)
  // Global prefix after generic selection; effect is the planned installation
  // after a returning message helper. Source sends before writing iScanner.
  function SelectScanner(owner: S.I16,players: seq<A.Context>,rows: seq<seq<A.PartRow>>): ScannerSelection
    requires DB.PlayersFit(players) && A.CatalogShape(rows)
    ensures SelectScanner(owner,players,rows).effect.Some? ==> SelectScanner(owner,players,rows).global==-1 && SelectScanner(owner,players,rows).effect.value.value<=8
  {
    var result:=Cap.Scanner(owner,players,rows,A.Prior(0));
    ScannerSelection(result,if result.status!=T.Done then G.None else G.Some(ScannerInstall(27,result.item,owner)),if result.status==T.Done then -1 else owner)
  }
  datatype PacketPlan = PacketRejected(reason: nat) | Packet(ore: seq<int>,warp: int,decay: S.U16,target: nat)
  function PlanPacket(item: S.I16,built: S.I16,primary: S.I16,ability: S.I16,two: S.I16,configured: nat,destination: nat): PacketPlan
    requires item in {6,14,15,16,17} && configured<16 && destination<1024
    ensures PlanPacket(item,built,primary,ability,two,configured,destination).Packet? ==>
      |PlanPacket(item,built,primary,ability,two,configured,destination).ore|==3
    ensures PlanPacket(item,built,primary,ability,two,configured,destination).Packet? ==>
      forall n <- PlanPacket(item,built,primary,ability,two,configured,destination).ore :: -32768<=n<32768
  {
    if ability==0 then PacketRejected(1) else if destination==0 then PacketRejected(2) else
    var mixed:=item in {6,17};
    var size:=if mixed then (if primary==6 then 25 else 40) else (if primary==6 then 70 else 100);
    var quantity:=N.Signed16(if size*built>32760 then 32760 else size*built);
    var requested:=configured+4;
    var asked:=if requested<5 || requested>ability+3 then N.Signed16(ability+two) else requested;
    var initial: S.U16:=(if asked<=ability+two then 0 else asked-ability-two)%65536;
    var decay: S.U16:=(initial+(if primary==7 && N.Signed16(initial)<3 then 1 else 0))%65536;
    Packet(seq(3,i requires 0<=i<3 => if mixed || i==item-14 then quantity else 0),N.Signed16(asked-4),decay,destination-1)
  }
  datatype PacketRow = PacketRow(owner: nat,kind: nat,point: G.Point,warp: nat,decay: nat,target: nat,weight: nat,ore: seq<S.I16>)
  predicate PacketFits(p: PacketRow) { p.owner<16 && p.kind<8 && p.warp<16 && p.decay<4 && p.target<1024 && p.weight<16384 && |p.ore|==3 }
  function Matches(p: PacketRow,owner: S.I16,point: G.Point,plan: PacketPlan): bool
    requires plan.Packet?
  { p.owner==owner && p.kind==1 && p.point==point && p.warp==plan.warp && p.decay==plan.decay && p.target==plan.target && p.weight<1630 }
  datatype PacketFound = NoPacket | PacketAt(index: nat)
  function FindPacket(rows: seq<PacketRow>,owner: S.I16,point: G.Point,plan: PacketPlan): PacketFound
    requires plan.Packet?
    ensures FindPacket(rows,owner,point,plan).PacketAt? ==> FindPacket(rows,owner,point,plan).index<|rows|
    decreases |rows|
  {
    if |rows|==0 then NoPacket else if Matches(rows[0],owner,point,plan) then PacketAt(0) else
    var next:=FindPacket(rows[1..],owner,point,plan);if next.NoPacket? then NoPacket else PacketAt(next.index+1)
  }
  function Weight(ore: seq<int>,initial: nat): nat
    ensures Weight(ore,initial)<16384
    decreases |ore|
  { if |ore|==0 then initial%16384 else Weight(ore[1..],(initial+C.DivC(N.Signed16(ore[0]+9),10))%16384) }
  function Combined(prior: S.I16,added: S.I16): S.I16 {
    var sum:=N.Signed16(prior+added);
    (if sum<0 then 32760 else sum) as S.I16
  }
  function Ore(plan: PacketPlan): seq<S.I16>
    requires plan.Packet? && |plan.ore|==3 && forall n <- plan.ore :: -32768<=n<32768
    ensures |Ore(plan)|==3
    ensures forall i | 0<=i<3 :: Ore(plan)[i]==plan.ore[i]
  {
    assert forall i | 0<=i<3 :: plan.ore[i] in plan.ore;
    assert forall i | 0<=i<3 :: -32768<=plan.ore[i]<32768;
    var values: seq<S.I16>:=seq(3,i requires 0<=i<3 => plan.ore[i] as S.I16);
    values
  }
  function MergePacket(p: PacketRow,plan: PacketPlan): PacketRow
    requires PacketFits(p) && plan.Packet? && |plan.ore|==3 && forall n <- plan.ore :: -32768<=n<32768
    ensures PacketFits(MergePacket(p,plan))
  {
    var added:=Ore(plan);
    var ore: seq<S.I16>:=seq(3,i requires 0<=i<3 => Combined(p.ore[i],added[i]));
    p.(ore:=ore,weight:=Weight(seq(3,i requires 0<=i<3 => ore[i] as int),0))
  }
  function NewPacket(owner: nat,point: G.Point,plan: PacketPlan): PacketRow
    requires owner<16 && plan.Packet? && |plan.ore|==3 && forall n <- plan.ore :: -32768<=n<32768
    ensures PacketFits(NewPacket(owner,point,plan))
  {
    PacketRow(owner,1,point,plan.warp%16,plan.decay%4,plan.target%1024,Weight(plan.ore,0),
      Ore(plan))
  }
  // Allocation failure is a TRUE return after a failure notice, unlike lost
  // ship construction at capacity. No object is manufactured by this predicate.
  datatype PacketOutcome = Rejected | Merged | Created | AllocationFailed
  function PacketApplication(plan: PacketPlan,found: bool,success: bool): PacketOutcome {
    if plan.PacketRejected? then Rejected else if found then Merged else if success then Created else AllocationFailed
  }
  function PacketAccepted(outcome: PacketOutcome): bool { outcome!=Rejected }
  lemma SameBaseExistence(rows: seq<Counts>,index: nat)
    requires |rows|==10 && index<10
    ensures BaseCounters(rows,index,index,true)[index].exist==rows[index].exist
  {}
  lemma NoOpOnlyKnown(item: nat)
    requires item<=27
    ensures Family(item)==NoOpFamily <==> item in {3,10,11}
  {}
  lemma {:isolate_assertions} Vectors() {
    assert C.Pow2(4)==16 && C.Pow2(8)==256 && C.Pow2(12)==4096;
    assert C.Pow2(15)==32768 && C.Pow2(16)==65536;
    assert Route(2,26,false)==InvalidDesign && Route(2,-1,false)==Reject && Route(2,-1,true)==InvalidDesign;
    assert Route(1,-1,false)==Reject && Route(1,10,false)==Planet(10);
    assert Install(10,12,5,3)==Installation(true,12,2,5,true);
    assert Install(4095,-32768,1,0).installed==0;
    assert Install(0,32767,32767,32767)==Installation(true,4095,32767,-2,false);
    assert BaseFling(9,5,0,0,0)==Fling(0,0,true,true);
    assert ScannerInstall(27,255,3)==Scanner(31,-1,255);
    assert TerraformCount(-1)==65535;
    assert TerraformStep([0,50,100],-1)==Terraformed([1,50,100],1);
    assert TerraformTrace([50,50,50],2,[0,1])==TerraformRun([51,50,50],2,0,Finished);
    assert TerraformTrace([50,50,50],2,[0]).status==Pending;
    var rebirth:=Rebirth(seq(12,i=>0),false,1,2,3,31);
    assert rebirth.stocks==[0,0,0];
    assert H.FinalStock([10,20,30,90])==[10,20,30];
    assert H.FinalStock([10,20,30,90])!=rebirth.stocks;
    assert Weight([32760,32760,32760],0)==6556;
    assert PlanPacket(17,1000,0,10,0,6,5).ore==[32760,32760,32760];
    assert PacketAccepted(PacketApplication(PlanPacket(17,1,0,10,0,6,5),false,false));
  }
}
