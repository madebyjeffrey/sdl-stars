include "M09-construction-handoffs.dfy"
include "M08-colony-capabilities.dfy"

// Produce's native caller arithmetic, validation and queue operations. Object
// application is a returning event with actual post-call inputs, not a boolean
// assertion that FBuildObject installs every charged item.
module M09HostProduction {
  import Cap = M08ColonyCapabilities
  import G = SharedGameState
  import T = M08TerraformSelection
  import A = M11NativeAvailability
  import S = S01
  import C = S01Codecs
  import R = M09Presets
  import B = M09Builder
  import F = M09Forecast
  import Q = S06PlanetEditor
  import P = M09Production
  import H = M09ConstructionHandoffs
  function Decode(word: S.U32): R.Row
    ensures R.Fits(Decode(word))
  { R.Row(R.O.Prod(word%1024,word/1024%128,word/131072%8,word/1048576%128),word/134217728%32) }
  datatype Number = Value(value: int) | Undefined(reason: string)
  function Divide(n: int,d: int): int
    requires d!=0
  { if d>0 then C.DivC(n,d) else -C.DivC(n,-d) }
  function Extra(gross: S.I16,extra: S.U16): Number {
    if gross==0 || extra==0 then Value(gross) else
    var product:=B.Signed32((gross as int)*(extra as int));
    var denominator:=B.Signed32((extra as int)+(gross as int));
    if denominator==0 || (product==-2147483648 && denominator==-1) then Undefined("extra-resource division") else
    var quotient:=Divide(product,denominator);
    if !B.I32(gross+quotient) then Undefined("extra-resource addition") else Value(gross+quotient)
  }
  datatype Budget = Reserved(available: int,research: int,annual: int) | UndefinedBudget(reason: string)
  function Reserve(resources: int,pct: int,suppressed: bool,annual: int): Budget
    requires B.I32(resources) && -128<=pct<128 && B.I32(annual)
  {
    if suppressed then Reserved(resources,0,annual) else
    var product:=resources*pct;
    if !B.I32(product) then UndefinedBudget("research multiplication") else
    var take:=C.DivC(product,100);
    if !B.I32(resources-take) || !B.I32(annual+take) then UndefinedBudget("research bookkeeping")
    else Reserved(resources-take,take,annual+take)
  }
  datatype Entry = NullUnowned | EmptyNoticeAndResources | SkipAllocated | ResourcesAndQueue
  function Enter(present: bool,count: S.U8,owner: S.I16): Entry {
    if !present then (if owner==-1 then NullUnowned else EmptyNoticeAndResources)
    else if owner==-1 || count==0 then SkipAllocated else ResourcesAndQueue
  }
  datatype EntryEvent = EmptyNotice | SnapshotSuppression | SnapshotMinerals | ResourceQuery
    | AdjustExtra | CheckResources | ReservePhase | QueueWork | StockWrite | AnnualAdd
  function EntryPlan(present: bool,count: S.U8,owner: S.I16,adjustedResources: int): seq<EntryEvent> {
    match Enter(present,count,owner)
    case NullUnowned => [] case SkipAllocated => []
    case EmptyNoticeAndResources => [EmptyNotice,ResourceQuery,AdjustExtra,AnnualAdd]
    case ResourcesAndQueue => ([SnapshotSuppression,SnapshotMinerals,ResourceQuery,AdjustExtra,CheckResources]+
      (if adjustedResources==0 then [] else [ReservePhase,QueueWork,StockWrite,AnnualAdd]))
  }
  datatype ValidationInputs = ValidationInputs(scanner: nat,warp: S.I16,destination: S.I16,
    absolute: S.I16,operable: S.I16,installed: nat,terraform: S.I16)
  predicate ValidationFits(v: ValidationInputs) { v.scanner<32 && v.installed<4096 }
  datatype Validation = Keep(row: R.Row,notice: nat) | Drop(notice: nat)
  // Notice codes: 0=no message, 1=scanner, 2=packet, 3=installation, 4=terraform.
  function Validate(row: R.Row,v: ValidationInputs): Validation
    requires R.Fits(row) && ValidationFits(v)
    ensures Validate(row,v).Keep? ==> R.Fits(Validate(row,v).row) && Validate(row,v).row.value.cItem>0
  {
    if row.value.cItem==0 then Drop(0) else if row.value.grobj!=1 then Keep(row,0) else
    var item:=row.value.iItem;
    if 18<=item<=27 then (if v.scanner!=31 then Drop(1) else Keep(row,0)) else
    if 14<=item<=17 then (if v.warp==0 || v.destination==0 then Drop(2) else Keep(row,0)) else
    if item !in {7,8,9,12} then Keep(row,0) else
    var highest:=if v.operable>v.absolute then v.operable else v.absolute;
    var cap:=if item==12 then v.terraform else B.Signed16(highest-v.installed);
    if cap>=row.value.cItem then Keep(row,0) else if cap<=0 then Drop(if item==12 then 4 else 3)
    else Keep(row.(value:=row.value.(cItem:=cap as nat)),if item==12 then 4 else 3)
  }
  // Validation queries after row/category checks, before any builder call.
  datatype TerraformValidation = TerraformValidation(input: G.Option<Cap.CountResult>,decision: G.Option<Validation>)
  function ValidateTerraform(row: R.Row,v: ValidationInputs,p: T.Planet,players: seq<T.Player>,rows: seq<seq<A.PartRow>>,terra: seq<T.Terra>,saved: S.I16): TerraformValidation
    requires R.Fits(row) && ValidationFits(v) && T.PlanetFits(p) && T.PlayersFit(players) && A.CatalogShape(rows) && |terra|==20
    ensures ValidateTerraform(row,v,p,players,rows,terra,saved).input.Some? <==> 4 in ValidationCalls(row)
    ensures ValidateTerraform(row,v,p,players,rows,terra,saved).input.None? ==> ValidateTerraform(row,v,p,players,rows,terra,saved).decision==G.Some(Validate(row,v))
  {
    if !(4 in ValidationCalls(row)) then TerraformValidation(G.None,G.Some(Validate(row,v))) else
    var count:=Cap.TerraformCount(p,players,rows,terra,saved);
    TerraformValidation(G.Some(count),if count.status!=T.Done then G.None else G.Some(Validate(row,v.(terraform:=count.value))))
  }
  // 1=warp; 2=absolute; 3=operable; 4=terraform.
  function ValidationCalls(row: R.Row): seq<nat> {
    if row.value.cItem==0 || row.value.grobj!=1 then [] else
    if 14<=row.value.iItem<=17 then [1] else
    if row.value.iItem in {7,8,9} then [2,3] else
    if row.value.iItem==12 then [4] else []
  }
  function CarryAlchemy(row: R.Row,index: nat,count: S.U8): bool {
    F.AlchemyRow(row) && index+1<count
  }
  function AutoDone(prior: bool,status: S.U16): bool {
    prior && status !in {P.StatusWord(P.SomeAuto),P.StatusWord(P.NoneAuto)}
  }
  datatype BuildResult = Built(builder: B.State,autoDone: bool,callApplication: bool)
    | Pending(builder: B.State) | UndefinedConstruction(builder: B.State)
  function ExecuteBuilder(row: R.Row,cost: seq<int>,resources: seq<int>,limits: B.Limits,
    alchemy: bool,cheap: bool,generating: bool,autoDone: bool,fuel: nat): BuildResult
    requires R.Fits(row) && B.LimitsFit(limits) && |cost|==4 && |resources|==4
    requires forall n <- cost+resources :: B.I32(n)
    ensures ExecuteBuilder(row,cost,resources,limits,alchemy,cheap,generating,autoDone,fuel).Built? ==>
      ExecuteBuilder(row,cost,resources,limits,alchemy,cheap,generating,autoDone,fuel).builder.phase.Done? &&
      !ExecuteBuilder(row,cost,resources,limits,alchemy,cheap,generating,autoDone,fuel).builder.partial.Absent?
  {
    var start:=B.Start(row,B.Empty,cost,resources,limits);
    var context:=B.Context(alchemy,cheap,false,generating);
    var b:=B.Run(start,context,fuel);
    H.Presence(start,context,fuel);
    if b.phase.Done? then Built(b,AutoDone(autoDone,P.StatusWord(b.status) as S.U16),b.built>0)
    else if b.phase.Undefined? then UndefinedConstruction(b) else Pending(b)
  }
  datatype Action = Restart | Remove | Stop | Next | UndefinedRow
  datatype Application = Application(clearCapturedCount: bool,action: Action)
  function AfterApplication(built: S.I16,status: S.U16,row: R.Row,applied: bool,live: bool,owner: S.I16,present: bool): Application {
    if built>0 && !applied && !live then Application(false,UndefinedRow) else
    var clear:=built>0 && !applied && (row.value.grobj!=1 || row.value.iItem>=7);
    Application(clear,if owner==-1 && !present then Restart
      else if status==P.StatusWord(P.Complete) then Remove
      else if B.Signed16(status)>=P.StatusWord(P.Some) then Stop else Next)
  }
  datatype Queue = NoQueue | Allocated(block: Q.Block)
  datatype Event = Free | Resize(size: nat) | Move(destination: nat,source: nat,count: nat)
    | Write(index: nat,word: S.U32) | Count(used: S.U8)
  datatype Status = Continued | Freed | Escaped | OutsideAllocation
  datatype QueueResult = QueueResult(queue: Queue,index: int,events: seq<Event>,status: Status)
  function RemoveRow(b: Q.Block,index: nat,previousAlchemy: bool): QueueResult
    requires index<=255 && (previousAlchemy ==> index>0)
    ensures RemoveRow(b,index,previousAlchemy).status==Freed ==> RemoveRow(b,index,previousAlchemy).queue==NoQueue
  {
    var prior:=if previousAlchemy then 1 else 0;
    if b.count==prior+1 then QueueResult(NoQueue,index,[Free],Freed) else
    var destination:=index-prior;
    var count:=if index+1<b.count then b.count-index-1 else 0;
    if count>0 && (b.count>|b.rows| || destination+count>|b.rows|) then
      QueueResult(Allocated(b),index,[],OutsideAllocation) else
    var rows:=if count==0 then b.rows else b.rows[..destination]+b.rows[index+1..b.count]+b.rows[destination+count..];
    var used: S.U8:=(b.count-prior-1)%256;
    QueueResult(Allocated(b.(count:=used,rows:=rows)),index-prior-1,
      (if count==0 then [] else [Move(destination,index+1,count)])+[Count(used)],Continued)
  }
  function Prepend(b: Q.Block,partial: R.Row,ok: bool,extra: nat,fill: seq<S.U32>): QueueResult
    requires R.Fits(partial) && partial.value.cItem>0 && |fill|>=256+extra && |b.rows|<=|fill|
    ensures Prepend(b,partial,ok,extra,fill).status==Continued ==> Prepend(b,partial,ok,extra,fill).queue.Allocated?
  {
    var resize:=b.count==b.maximum;
    var events:=if resize then [Resize(b.count+1)] else [];
    if resize && !ok then QueueResult(Allocated(b),0,events,Escaped) else
    var block:=if resize then Q.Resized(b,b.count+1,extra,fill) else b;
    if b.count+1>|block.rows| then QueueResult(Allocated(block),0,events,OutsideAllocation) else
    var rows:=[F.Word(partial)]+block.rows[..b.count]+block.rows[b.count+1..];
    var count: S.U8:=(b.count+1)%256;
    QueueResult(Allocated(block.(count:=count,rows:=rows)),0,
      events+[Move(1,0,b.count),Write(0,F.Word(partial)),Count(count)],Continued)
  }
  function CompletedNotice(queue: Queue,index: int,autoDone: bool): bool {
    queue.NoQueue? || (index>=queue.block.count && autoDone)
  }
  function FinalStock(resources: seq<int>): seq<int>
    requires |resources|==4
    ensures |FinalStock(resources)|==3
  { resources[..3] }
  lemma ResearchConservation(resources: int,pct: int,suppressed: bool,annual: int)
    requires B.I32(resources) && -128<=pct<128 && B.I32(annual)
    ensures Reserve(resources,pct,suppressed,annual).Reserved? ==>
      Reserve(resources,pct,suppressed,annual).available+Reserve(resources,pct,suppressed,annual).research==resources
  {}
  lemma {:isolate_assertions} PackedRoundTrip(row: R.Row)
    requires R.Fits(row)
    ensures Decode(F.Word(row))==row
  {
    var p:=row.value;var word:=F.Word(row);
    assert word==p.cItem+1024*(p.iItem+128*(p.grobj+8*(p.pct+128*row.unused)));
    assert word%1024==p.cItem;
    assert word/1024==p.iItem+128*(p.grobj+8*(p.pct+128*row.unused));
    assert word/1024%128==p.iItem;
    assert 0<=p.cItem+1024*p.iItem<131072;
    assert word/131072==p.grobj+8*(p.pct+128*row.unused);
    assert word/131072%8==p.grobj;
    assert 0<=p.cItem+1024*p.iItem+131072*p.grobj<1048576;
    assert word/1048576==p.pct+128*row.unused;
    assert word/1048576%128==p.pct;
    assert 0<=p.cItem+1024*p.iItem+131072*p.grobj+1048576*p.pct<134217728;
    assert word/134217728==row.unused;
  }
  lemma PrependPreservesOldActive(b: Q.Block,p: R.Row,extra: nat,fill: seq<S.U32>)
    requires R.Fits(p) && p.value.cItem>0 && |fill|>=256+extra && |b.rows|<=|fill| && b.count+1<=|b.rows|
    ensures Prepend(b,p,true,extra,fill).status==Continued
    ensures Prepend(b,p,true,extra,fill).queue.block.rows[1..b.count+1]==b.rows[..b.count]
  {}
  lemma {:isolate_assertions} Vectors() {
    assert C.Pow2(4)==16 && C.Pow2(8)==256 && C.Pow2(12)==4096 && C.Pow2(16)==65536;
    assert C.Pow2(20)==1048576 && C.Pow2(24)==16777216 && C.Pow2(28)==268435456;
    assert C.Pow2(31)==2147483648 && C.Pow2(32)==4294967296;
    assert Enter(false,0,0)==EmptyNoticeAndResources && Enter(true,0,0)==SkipAllocated;
    assert Extra(-10,10).Undefined? && Extra(100,100)==Value(150);
    assert Reserve(100,50,false,7)==Reserved(50,50,57);
    assert AfterApplication(1,5,R.Row(R.O.Prod(2,0,2,0),0),false,true,0,true)==Application(true,Stop);
    assert AfterApplication(1,0,R.ZeroRow(),true,false,-1,false)==Application(false,Restart);
    assert AfterApplication(1,0,R.ZeroRow(),false,false,-1,false).action==UndefinedRow;
    var rows: seq<S.U32>:=seq(256,i=>i);
    var p:=R.Row(R.O.Prod(1,11,1,5),31);
    var result:=Prepend(Q.Block(255,255,rows),p,true,0,rows);
    assert result.status==Continued && result.queue.block.maximum==0 && result.queue.block.count==0;
    assert result.queue.block.rows[1..256]==rows[..255];
    assert RemoveRow(Q.Block(3,3,[1,2,3]),1,true).queue.block.count==1;
    assert RemoveRow(Q.Block(3,3,[1,2,3]),1,true).queue.block.rows==[3,2,3];
    assert !AutoDone(true,3) && !AutoDone(true,4) && AutoDone(true,2);
    assert RemoveRow(Q.Block(1,0,[1]),3,false).queue.block.count==255;
    assert Reserve(20000000,127,false,0).UndefinedBudget?;
  }
}
