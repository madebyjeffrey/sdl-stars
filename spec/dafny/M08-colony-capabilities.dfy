include "M08-terraform-selection.dfy"
include "M08-mass-driver-capability.dfy"

// Native colony capability producers. Undefined is a reached read boundary,
// not a newly returned C error. Pointer storage/lifetimes are declared domains.
module M08ColonyCapabilities {
  import S = S01
  import G = SharedGameState
  import E = M08NativeEconomy
  import T = M08TerraformSelection
  import MD = M08MassDriver
  import A = M11NativeAvailability
  import B = M11DesignBuildability
  import I = S03LookupIdentities
  import C = S01Codecs
  import O = M08Colony
  function AxisSteps(current: E.I8,low: S.I16,high: S.I16): int {
    (if low==-1 then 0 else current-low)+(if high==-1 then 0 else high-current)
  }
  function SumSteps(env: seq<E.I8>,low: seq<S.I16>,high: seq<S.I16>,at: nat,total: S.I16): S.I16
    requires |env|==3 && |low|==3 && |high|==3 && at<=3
    decreases 3-at
  {
    if at==3 then total else
    var first:=if low[at]==-1 then total else E.Word((total as int)+env[at]-low[at]);
    var second:=if high[at]==-1 then first else E.Word((first as int)+high[at]-env[at]);
    SumSteps(env,low,high,at+1,second)
  }
  function MathematicalSteps(env: seq<E.I8>,low: seq<S.I16>,high: seq<S.I16>,at: nat): int
    requires |env|==3 && |low|==3 && |high|==3 && at<=3
    decreases 3-at
  { if at==3 then 0 else AxisSteps(env[at],low[at],high[at])+MathematicalSteps(env,low,high,at+1) }
  lemma WordAdd(a: int,b: int)
    ensures E.Word((E.Word(a) as int)+b)==E.Word(a+b)
  {
    assert C.Pow2(4)==16 && C.Pow2(8)==256 && C.Pow2(12)==4096 && C.Pow2(15)==32768 && C.Pow2(16)==65536;
  }
  lemma SumProjection(env: seq<E.I8>,low: seq<S.I16>,high: seq<S.I16>,at: nat,total: S.I16)
    requires |env|==3 && |low|==3 && |high|==3 && at<=3
    ensures SumSteps(env,low,high,at,total)==E.Word(total+MathematicalSteps(env,low,high,at))
    decreases 3-at
  {
    E.WordPreserves(total);
    if at<3 {
      var lo:=if low[at]==-1 then 0 else env[at]-low[at];
      var hi:=if high[at]==-1 then 0 else high[at]-env[at];
      WordAdd(total,lo);WordAdd(total+lo,hi);
      var first:=E.Word(total+lo);var second:=E.Word((first as int)+hi);
      SumProjection(env,low,high,at+1,second);
      WordAdd(total+lo+hi,MathematicalSteps(env,low,high,at+1));
    }
  }
  lemma ScalarSteps(current: E.I8,bounds: O.Bounds)
    requires bounds.low.At? ==> -32768<=bounds.low.value<32768 && bounds.low.value!=-1
    requires bounds.high.At? ==> -32768<=bounds.high.value<32768 && bounds.high.value!=-1
    ensures AxisSteps(current,T.Encode(bounds).low,T.Encode(bounds).high)==O.TerraformSteps(current,bounds)
  {
    if bounds.low.At? { E.WordPreserves(bounds.low.value); }
    if bounds.high.At? { E.WordPreserves(bounds.high.value); }
  }
  datatype CountResult = CountResult(status: T.Status,value: S.I16,global: S.I16,capability: T.CanResult)
  function TerraformCount(p: T.Planet,players: seq<T.Player>,rows: seq<seq<A.PartRow>>,terra: seq<T.Terra>,saved: S.I16): CountResult
    requires T.PlanetFits(p) && T.PlayersFit(players) && A.CatalogShape(rows) && |terra|==20
    ensures TerraformCount(p,players,rows,terra,saved).status==T.Done ==> TerraformCount(p,players,rows,terra,saved).global==saved
    ensures TerraformCount(p,players,rows,terra,saved).status!=T.Pending
  {
    var none: seq<G.Option<S.I16>>:=[G.None,G.None,G.None];
    var can:=T.Can(p,players,rows,terra,saved,true,none,none,none);
    if can.status.Undefined? then CountResult(can.status,0,can.global,can) else
    if !can.can then CountResult(T.Done,0,saved,can) else
    assert forall i | 0<=i<3 :: can.low[i].Some? && can.high[i].Some?;
    var low:=seq(3,i requires 0<=i<3 => can.low[i].value);var high:=seq(3,i requires 0<=i<3 => can.high[i].value);
    CountResult(T.Done,SumSteps(p.env,low,high,0,0),saved,can)
  }
  datatype Target = Planet(owner: S.I16,hasBase: bool,index: S.I16) | Direct(owner: S.I16,index: S.I16)
  predicate TargetFits(target: Target) { target.Planet? ==> 0<=target.index<16 }
  datatype Probe = Probe(item: S.U8,answer: G.Option<A.Result>)
  datatype GateEvent = RosterRead(owner: S.I16) | HullRead(index: int) | SlotRead(slot: nat) | PartLookup(item: S.U8)
  datatype GateResult = GateResult(status: T.Status,value: S.I16,events: seq<GateEvent>,lookup: G.Option<A.Result>)
  function GateSlot(slot: G.HullSlot): bool {
    slot.category==512 && slot.count>0 && I.ispecialSBStargate100250<=slot.item<=I.ispecialSBStargateAnyAny
  }
  function Range(ability: S.I16): S.I16 { if ability==-1 then 10000 else ability }
  function GateScan(d: MD.Design,at: nat,current: S.I16,players: seq<A.Context>,rows: seq<seq<A.PartRow>>,ranges: seq<S.I16>,events: seq<GateEvent>): GateResult
    requires MD.Shape(d) && at<=16 && at<=d.count && B.PlayersFit(players) && A.CatalogShape(rows) && |ranges|==16
    ensures GateScan(d,at,current,players,rows,ranges,events).status!=T.Pending
    ensures GateScan(d,at,current,players,rows,ranges,events).lookup.Some? ==>
      GateScan(d,at,current,players,rows,ranges,events).lookup.value.pointer.Row? &&
      GateScan(d,at,current,players,rows,ranges,events).lookup.value.pointer.kind==9 &&
      GateScan(d,at,current,players,rows,ranges,events).lookup.value.pointer.item<=6 &&
      GateScan(d,at,current,players,rows,ranges,events).value==Range(ranges[GateScan(d,at,current,players,rows,ranges,events).lookup.value.pointer.item])
    ensures d.count<=16 && B.ValidPlayer(current) ==> GateScan(d,at,current,players,rows,ranges,events).status==T.Done
    decreases 16-at
  {
    if at==d.count then GateResult(T.Done,0,events,G.None) else
    var read:=events+[SlotRead(at)];
    if at==16 then GateResult(T.Undefined("gate hull slot read"),0,read,G.None) else
    var slot:=d.slots[at];
    if !GateSlot(slot) then GateScan(d,at+1,current,players,rows,ranges,read) else
    var item:=slot.item as S.U8;var called:=read+[PartLookup(item)];
    if !B.ValidPlayer(current) then GateResult(T.Undefined("gate lookup player read"),0,called,G.None) else
    var answer:=A.LookupRow(B.Selected(players,current),rows,512,item,A.Prior(0));
    GatePointer(B.Selected(players,current),rows,item);
    GateResult(T.Done,Range(ranges[item]),called,G.Some(answer))
  }
  function Gate(target: Target,rosters: seq<MD.Roster>,current: S.I16,players: seq<A.Context>,rows: seq<seq<A.PartRow>>,ranges: seq<S.I16>): GateResult
    requires TargetFits(target) && MD.RostersFit(rosters) && B.PlayersFit(players) && A.CatalogShape(rows) && |ranges|==16
  {
    if target.Planet? && (target.owner==-1 || !target.hasBase) then GateResult(T.Done,0,[],G.None) else
    var events:=[RosterRead(target.owner)];
    if target.owner<0 || target.owner>=16 then GateResult(T.Undefined("gate roster owner"),0,events,G.None) else
    var row:=rosters[target.owner];
    if row.Missing? then GateResult(T.Undefined("gate roster pointer"),0,events,G.None) else
    var read:=events+[HullRead(target.index)];
    if target.index<0 || target.index>=10 then GateResult(T.Undefined("gate base design read"),0,read,G.None) else
    GateScan(row.entries[target.index],0,current,players,rows,ranges,read)
  }
  // Lookup result is recorded even when denied. A valid item assigns its row
  // pointer before denial; StargateRangeFromLppl ignores that status.
  lemma GatePointer(c: A.Context,rows: seq<seq<A.PartRow>>,item: S.U8)
    requires A.Shape(c) && A.CatalogShape(rows) && item<=6
    ensures A.LookupRow(c,rows,512,item,A.Prior(0)).pointer==A.Row(9,item)
    ensures A.LookupRow(c,rows,512,item,A.Prior(0)).status!=0
  {
    assert A.Kind(512,0)==9;
  }
  datatype ScannerResult = ScannerResult(status: T.Status,item: S.U8,pointer: A.Address,trace: seq<Probe>)
  lemma PlanetaryKind() ensures A.Kind(32768,0)==15 {
    assert A.Kind(32768,15)==15;assert A.Kind(32768,14)==15;assert A.Kind(32768,13)==15;
    assert A.Kind(32768,12)==15;assert A.Kind(32768,11)==15;assert A.Kind(32768,10)==15;
    assert A.Kind(32768,9)==15;assert A.Kind(32768,8)==15;assert A.Kind(32768,7)==15;
    assert A.Kind(32768,6)==15;assert A.Kind(32768,5)==15;assert A.Kind(32768,4)==15;
    assert A.Kind(32768,3)==15;assert A.Kind(32768,2)==15;assert A.Kind(32768,1)==15;
  }
  function ScannerScan(c: A.Context,rows: seq<seq<A.PartRow>>,item: S.U8,prior: A.Address,trace: seq<Probe>): ScannerResult
    requires A.Shape(c) && A.CatalogShape(rows) && item<=8
    ensures ScannerScan(c,rows,item,prior,trace).status==T.Done
    ensures ScannerScan(c,rows,item,prior,trace).item<=item
    ensures ScannerScan(c,rows,item,prior,trace).pointer==A.Row(15,ScannerScan(c,rows,item,prior,trace).item)
    ensures |trace|<|ScannerScan(c,rows,item,prior,trace).trace|<=|trace|+item+1
    ensures ScannerScan(c,rows,item,prior,trace).trace[|ScannerScan(c,rows,item,prior,trace).trace|-1].answer.Some?
    ensures ScannerScan(c,rows,item,prior,trace).item==0 ||
      ScannerScan(c,rows,item,prior,trace).trace[|ScannerScan(c,rows,item,prior,trace).trace|-1].answer.value.status==1
    decreases item
  {
    var answer:=A.LookupRow(c,rows,32768,item,prior);var recorded:=trace+[Probe(item,G.Some(answer))];
    PlanetaryKind();
    if answer.status==1 || item==0 then ScannerResult(T.Done,item,answer.pointer,recorded) else
    ScannerScan(c,rows,item-1,answer.pointer,recorded)
  }
  function Scanner(current: S.I16,players: seq<A.Context>,rows: seq<seq<A.PartRow>>,prior: A.Address): ScannerResult
    requires B.PlayersFit(players) && A.CatalogShape(rows)
    ensures Scanner(current,players,rows,prior).item<=8
    ensures Scanner(current,players,rows,prior).status==T.Done ==> Scanner(current,players,rows,prior).pointer==A.Row(15,Scanner(current,players,rows,prior).item)
  {
    if !B.ValidPlayer(current) then ScannerResult(T.Undefined("scanner lookup player read"),8,prior,[Probe(8,G.None)]) else
    ScannerScan(B.Selected(players,current),rows,8,prior,[])
  }
  lemma {:isolate_assertions} Vectors() {
    E.WordPreserves(0);E.WordPreserves(-24);
    assert AxisSteps(-23,1,-1)==-24;
    assert SumSteps([-23,50,50],[1,-1,-1],[-1,-1,-1],0,0)==-24;
    assert Range(-1)==10000 && Range(-2)==-2;
    assert GateSlot(G.HullSlot(512,0,255)) && !GateSlot(G.HullSlot(1536,0,255));
  }
}
