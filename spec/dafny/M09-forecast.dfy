include "M09-costs.dfy"
include "M09-presets.dfy"
include "S06-planet-editor.dfy"

// EstimateItemProdSched control and native boundary arithmetic. Mining,
// resource, CBuildProdItem and growth events resume with actual post-call state.
// Their full algorithms are not replaced by unconstrained claims of purity.
module M09Forecast {
  import S = S01
  import C = S01Codecs
  import R = M09Presets
  import K = M09Costs
  import Q = S06PlanetEditor
  import P = M09Production
  type I32 = n: int | -2147483648<=n<2147483648
  datatype Partial = CountOnlyZero | Known(row: R.Row)
  datatype Colony = Colony(stocks: seq<I32>,population: I32,mines: nat,factories: nat,noResearch: bool)
  predicate ColonyFits(p: Colony) { |p.stocks|==3 && p.mines<4096 && p.factories<4096 }
  datatype State = State(pl: Colony,queue: seq<R.Row>,partial: Partial,resources: seq<I32>,research: int,
    year: nat,index: int,alchemy: bool,first: int,last: int,buffer: nat,outputsAlias: bool)
  predicate Fits(s: State) {
    ColonyFits(s.pl) && |s.queue|<=255 && (forall r <- s.queue :: R.Fits(r)) &&
    (s.partial.Known? ==> R.Fits(s.partial.row)) && |s.resources|==4 &&
    1<=s.year<=100 && -1<=s.index<=|s.queue| &&
    -32768<=s.first<32768 && -32768<=s.last<32768 && -2147483648<=s.research<2147483648 && (!s.outputsAlias || s.first==s.last)
  }
  datatype Phase = Mine | Resources | Reserve | Rows | AfterBuild | YearEnd | AfterGrowth | Cleanup | Done
  datatype Event = Quiet | Mining(apply: bool,mines: int) | ResourceQuery(owner: int)
    | Build(index: int,partialOutput: bool,alchemy: bool,calcOnly: bool)
    | Growth(apply: bool) | Free(handle: nat) | Complete
  datatype Transition = Transition(state: State,phase: Phase,event: Event)
  datatype Budget = Budget(available: int,reserved: int)
  function ReserveResources(gross: S.I16,pct: K.I8,noResearch: bool): Budget
    ensures -74711<=ReserveResources(gross,pct,noResearch).available<=74711
    ensures -41944<=ReserveResources(gross,pct,noResearch).reserved<=41944
    ensures ReserveResources(gross,pct,noResearch).available+ReserveResources(gross,pct,noResearch).reserved==gross
  {
    var product: int:=(gross as int)*(pct as int);
    assert -4194304<=product<=4194304;
    var research:=if noResearch then 0 else C.DivC(product,100);
    Budget(gross-research,research)
  }
  function Query(resources: I32,research: int,request: S.U16): int
    ensures -32768<=Query(resources,research,request)<32768
  {
    assert C.Pow2(4)==16 && C.Pow2(8)==256 && C.Pow2(12)==4096;
    assert C.Pow2(15)==32768 && C.Pow2(16)==65536;
    var low:=C.Signed(resources,16);
    if request==65535 then C.Signed(low+research%65536,16) else low
  }
  function RowAt(s: State): R.Row
    requires Fits(s) && s.index<|s.queue|
    ensures R.Fits(RowAt(s))
  { if s.index>=0 then s.queue[s.index] else if s.partial.Known? then s.partial.row else R.ZeroRow() }
  function SetRow(s: State,row: R.Row): State
    requires Fits(s) && s.index<|s.queue| && R.Fits(row)
  { if s.index>=0 then s.(queue:=s.queue[s.index:=row]) else s.(partial:=Known(row)) }
  function AlchemyRow(r: R.Row): bool { r.value.grobj==1 && r.value.iItem==3 }
  function Advance(s: State): State { s.(index:=s.index+1) }
  function CompleteRow(s: State,built: S.I16): State
    requires Fits(s) && s.index<|s.queue|
  {
    var row:=RowAt(s);
    var pl:=s.pl;
    var revised:=if row.value.grobj!=1 then pl
      else if row.value.iItem in {0,8} then pl.(mines:=(pl.mines+built)%4096)
      else if row.value.iItem in {1,7} then pl.(factories:=(pl.factories+built)%4096)
      else pl;
    s.(pl:=revised,alchemy:=false)
  }
  function First(s: State,value: int): State
    requires Fits(s) && -32768<=value<32768
    ensures Fits(First(s,value))
  { s.(first:=value,last:=if s.outputsAlias then value else s.last) }
  function Last(s: State,value: int): State
    requires Fits(s) && -32768<=value<32768
    ensures Fits(Last(s,value))
  { s.(last:=value,first:=if s.outputsAlias then value else s.first) }
  function Step(s: State,p: Phase,request: S.U16,owner: S.I16,gross: S.I16,pct: K.I8,built: S.I16,status: S.U16): Transition
    requires Fits(s)
    requires p !in {Cleanup,Done} ==> s.year<100
    requires p==AfterBuild ==> s.index<|s.queue| && (s.index==-1 ==> s.partial.Known?)
    ensures Fits(Step(s,p,request,owner,gross,pct,built,status).state)
  {
    match p
    case Mine => Transition(s,Resources,Mining(true,-1))
    case Resources => Transition(s,Reserve,ResourceQuery(owner))
    case Reserve =>
      var budget:=ReserveResources(gross,pct,s.pl.noResearch);
      assert -2147483648<=budget.available<2147483648;
      Transition(s.(resources:=[s.pl.stocks[0],s.pl.stocks[1],s.pl.stocks[2],budget.available],
        research:=budget.reserved,index:=-1,alchemy:=false),Rows,Quiet)
    case Rows =>
      if s.index>=|s.queue| then Transition(s,YearEnd,Quiet) else
      var row:=RowAt(s);
      if row.value.cItem==0 then Transition(Advance(s),Rows,Quiet)
      else if AlchemyRow(row) && s.index<|s.queue|-1 then
        if s.index==request then Transition(Last(First(s,-1),-1),Cleanup,Quiet)
        else Transition(Advance(s).(alchemy:=true),Rows,Quiet)
      else
        var current:=if AlchemyRow(row) then SetRow(s,row.(value:=row.value.(cItem:=1020))) else s;
        Transition(current,AfterBuild,Build(s.index,s.index!=-1,s.alchemy,false))
    case AfterBuild =>
      var first:=if s.index==request && built>0 && s.first==0 then s.year else s.first;
      var tracked:=First(s,first);
      if s.index==request && status==P.StatusWord(P.SkippedAuto) then
        Transition(Last(tracked,if first!=0 then s.year-1 else tracked.last),Cleanup,Quiet)
      else if s.index==request && status in {P.StatusWord(P.Complete),P.StatusWord(P.CompleteAuto)} then Transition(Last(tracked,s.year),Cleanup,Quiet)
      else
        var updated:=CompleteRow(tracked,built);
        if C.Signed(status,16)>=P.StatusWord(P.Some) then Transition(updated,YearEnd,Quiet)
        else Transition(Advance(updated),Rows,Quiet)
    case YearEnd =>
      if C.Signed(request,16)<0 then Transition(First(s,Query(s.resources[3],s.research,request)),Cleanup,Quiet)
      else Transition(s.(pl:=s.pl.(stocks:=[s.resources[0],s.resources[1],s.resources[2]])),AfterGrowth,Growth(true))
    case AfterGrowth =>
      if s.year==99 then Transition(Last(if s.first==0 then First(s.(year:=100),100) else s.(year:=100),100),Cleanup,Quiet)
      else Transition(s.(year:=s.year+1),Mine,Quiet)
    case Cleanup => Transition(s,Done,if s.buffer!=0 then Free(s.buffer) else Quiet)
    case Done => Transition(s,Done,Complete)
  }
  function Start(pl: Colony,rows: seq<R.Row>,buffer: nat): State
    requires ColonyFits(pl) && |rows|<=255 && forall r <- rows :: R.Fits(r)
    ensures Fits(Start(pl,rows,buffer))
  { State(pl,rows,CountOnlyZero,[0,0,0,0],0,1,-1,false,0,0,buffer,false) }
  datatype Preparation = NoQueue | CopyAttempt(result: Q.QueueResult)
  function Source(original: nat,explicit: nat): nat { if explicit==0 then original else explicit }
  function Prepare(original: nat,explicit: nat,heap: map<nat,Q.Block>,ok: bool,handleNew: nat,extra: nat,fill: seq<S.U32>): Preparation
    requires 0 !in heap && |fill|>=257+extra
    requires forall k <- heap.Keys :: |heap[k].rows|<=|fill|
    requires handleNew>0 && handleNew !in heap
    requires handleNew!=Source(original,explicit)
  {
    var src:=Source(original,explicit);
    if src==0 then NoQueue else CopyAttempt(Q.Transfer(heap,src,0,Q.Duplicate,ok,handleNew,extra,fill))
  }
  function Word(row: R.Row): S.U32
    requires R.Fits(row)
  { OWord(row)+134217728*row.unused }
  function OWord(row: R.Row): nat
    requires R.Fits(row)
  { R.O.ProdWord(row.value) }
  // The cached row extent and local control/output values survive a returning
  // helper call. Helper-owned data fields may change; pointer liveness still
  // needs its native adapter. This is a contract, not an inferred frame proof.
  predicate Continuation(before: State,after: State) {
    |after.queue|==|before.queue| && after.year==before.year && after.index==before.index &&
    after.first==before.first && after.last==before.last && after.research==before.research &&
    after.outputsAlias==before.outputsAlias
  }
  predicate RowsMatch(block: Q.Block,rows: seq<R.Row>) {
    block.count==|rows| && block.count<=|block.rows| &&
    (forall r <- rows :: R.Fits(r)) && (forall i | 0<=i<|rows| :: Word(rows[i])==block.rows[i])
  }
  lemma RequestSource(original: nat,explicit: nat)
    ensures explicit!=0 ==> Source(original,explicit)==explicit
    ensures explicit==0 ==> Source(original,explicit)==original
  {}
  lemma TargetStopsBeforeInstall(s: State,request: S.U16,owner: S.I16,gross: S.I16,pct: K.I8,built: S.I16,status: S.U16)
    requires Fits(s) && s.year<100 && s.index<|s.queue| && s.index==request && status in {0,1,2}
    ensures Step(s,AfterBuild,request,owner,gross,pct,built,status).state.pl==s.pl
    ensures Step(s,AfterBuild,request,owner,gross,pct,built,status).phase==Cleanup
  {}
  lemma VisitBound(s: State,request: S.U16,owner: S.I16,gross: S.I16,pct: K.I8,built: S.I16,status: S.U16)
    requires Fits(s) && s.year==99
    ensures Step(s,AfterGrowth,request,owner,gross,pct,built,status).phase==Cleanup
    ensures Step(s,AfterGrowth,request,owner,gross,pct,built,status).state.last==100
    ensures Step(s,AfterGrowth,request,owner,gross,pct,built,status).state.year==100
  {}
  lemma {:isolate_assertions} Vectors() {
    assert C.Pow2(4)==16 && C.Pow2(8)==256 && C.Pow2(12)==4096;
    assert C.Pow2(15)==32768 && C.Pow2(16)==65536;
    var pl:=Colony([10,20,30],1000,4095,4095,false);
    var mine:=R.Row(R.O.Prod(1,8,1,0),9);
    var alchemy:=R.Row(R.O.Prod(1,3,1,0),0);
    var s:=Start(pl,[alchemy,R.ZeroRow(),mine],7).(index:=0);
    assert Step(s,Rows,0,0,0,0,0,7).state.first==-1;
    var armed:=Step(s,Rows,2,0,0,0,0,7).state;
    assert armed.alchemy && armed.index==1;
    var skipped:=Step(armed,Rows,2,0,0,0,0,7).state;
    assert skipped.alchemy && skipped.index==2;
    assert Step(skipped,Rows,2,0,0,0,0,7).event==Build(2,true,true,false);
    assert CompleteRow(skipped,1).pl.mines==0;
    assert ReserveResources(101,33,false)==Budget(68,33);
    assert ReserveResources(-101,33,false)==Budget(-68,-33);
    assert ReserveResources(101,-10,false)==Budget(111,-10);
    assert ReserveResources(101,33,true)==Budget(101,0);
    assert Query(65536,0,65534)==0 && Query(65536,17,65535)==17;
    assert Query(32767,1,65535)==-32768;
    assert P.DimensionPercent(10,5,0)==59;
    assert P.AlchemyMessage(1,false,true) && !P.AlchemyMessage(1,false,false);
    assert Step(skipped.(year:=4,first:=2),AfterBuild,2,0,0,0,0,2).state.last==3;
    assert Step(skipped,AfterBuild,2,0,0,0,0,2).state.first==0;
    assert Step(skipped,AfterBuild,2,0,0,0,0,2).state.last==0;
    assert Step(skipped,AfterBuild,1,0,0,0,1,7).phase==YearEnd;
    assert Step(skipped,AfterBuild,1,0,0,0,1,32768).phase==Rows;
    assert Step(s.(buffer:=0),Cleanup,2,0,0,0,0,7).event==Quiet;
    var alias:=skipped.(outputsAlias:=true,year:=99,first:=2,last:=2);
    assert Step(alias,AfterGrowth,2,0,0,0,0,7).state.first==100;
    assert Step(alias,YearEnd,65534,0,0,0,0,7).state.last==0;
    assert Word(mine)==R.O.ProdWord(mine.value)+134217728*9;
  }
}
