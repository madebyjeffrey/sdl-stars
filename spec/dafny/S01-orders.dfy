include "S01-records.dfy"
module S01Orders {
  import opened S01
  import opened S01Codecs
  import opened S01Records

  // Strict payload validation: intentionally stronger than FRunLogRecord.
  predicate FCommand(rec: Record, cPlayer: nat) {
    var b:=rec.rgb; var n:=|b|;
    FRecord(rec) && 1<=cPlayer<=16 &&
    (if rec.rt==0 then n==0
     else if rec.rt in {1,2,25} then n>=6 &&
       n==6+PopCount(b[5]%32)*(if rec.rt==1 then 1 else if rec.rt==2 then 2 else 4) &&
       b[4]%16 in {1,2,4,8} && b[4]/16 in {1,2,4,8}
     else if rec.rt in {3,10,36,42,43} then n==4
     else if rec.rt in {4,5} then 4<=n<=22 && Signed(W(b,2),16)>=0
     else if rec.rt==11 then n==6 && Signed(W(b,2),16)>=0 && W(b,4)<10
     else if rec.rt==23 then n>=7 && n==7+2*PopCount(W(b,5))
     else if rec.rt==24 then n==2
     else if rec.rt==27 then n>=2 && Bits(W(b,0),4,4)<cPlayer && Bits(W(b,0),8,5)<=25 &&
       (if W(b,0)%16==0 then n==2 else W(b,0)%16==1 && FDesign(b[2..]))
     else if rec.rt==29 then n>=2 && FQueue(b[2..])
     else if rec.rt==30 then FPlan(b,true)
     else if rec.rt==34 then n==2 && b[0]<=100 && b[1]%16<=5 && b[1]/16<=7
     else if rec.rt==35 then n==6
     else if rec.rt==37 then n>=2 && n%2==0
     else if rec.rt==38 then n==cPlayer
     else if rec.rt==44 then n>=4 && FExactName(b[4..])
     else if rec.rt==46 then n<=26
     else false)
  }
  // Values use the same offset/bit vocabulary as entity payloads. Variable tails
  // are interpreted by FCommand and the referenced entity/text decoders.
  function CommandFields(rt: RecordTag): seq<Field> {
    if rt in {1,2,25} then [Field("id1",0,2,0,16,false),Field("id2",2,2,0,16,false),
      Field("grobj1",4,1,0,4,false),Field("grobj2",4,1,4,4,false),Field("mask",5,1,0,5,false)]
    else if rt==23 then [Field("id1",0,2,0,16,false),Field("id2",2,2,0,16,false),
      Field("grobj1",4,1,0,4,false),Field("grobj2",4,1,4,4,false),Field("mask",5,2,0,16,false)]
    else if rt in {3,4,5,10,11,24,37,42,44} then [Field("idFleet",0,2,0,16,true)]+
      (if rt in {24,37} then [] else [Field("indexOrValue",2,2,0,16,true)])+
      (if rt==3 then [Field("index",2,2,0,15,false),Field("fTwo",2,2,15,1,false)]
       else if rt==11 then [Field("task",4,2,0,16,true)] else [])
    else if rt==27 then [Field("mdChg",0,2,0,4,false),Field("iPlayer",0,2,4,4,false),Field("ishdef",0,2,8,5,false)]
    else if rt==29 then [Field("idPlanet",0,2,0,16,true)]
    else if rt==30 then PlanFields()
    else if rt==34 then [Field("pctResearch",0,1,0,8,true),Field("iTechCur",1,1,0,4,false),Field("iTechNext",1,1,4,4,false)]
    else if rt==35 then [Field("idPlanet",0,2,0,16,true),Field("fNoResearch",2,4,0,1,false),
      Field("idFling",2,4,1,10,false),Field("iWarpFling",2,4,11,4,false),Field("idRoute",2,4,15,10,false)]
    else if rt==36 then [Field("lSalt",0,4,0,32,true)]
    else if rt==43 then [Field("idFull",0,2,0,16,false),Field("fDetonate",2,2,0,16,true)]
    else if rt==46 then TemplateFields() else []
  }
  // Coalescing is only across the immediately preceding compatible edit.
  predicate FSameWaypointEdit(a: Record,b: Record) {
    a.rt in {4,5} && b.rt==5 && |a.rgb|>=4 && |b.rgb|>=4 && a.rgb[..4]==b.rgb[..4]
  }
  predicate FSameQueueEdit(a: Record,b: Record) {
    a.rt==29 && b.rt==29 && |a.rgb|>=2 && |b.rgb|>=2 && a.rgb[..2]==b.rgb[..2]
  }
  predicate FSameCargoEndpoints(a: Record,b: Record) {
    a.rt in {1,2,25} && b.rt in {1,2,25} && |a.rgb|>=6 && |b.rgb|>=6 && a.rgb[..5]==b.rgb[..5]
  }
  function AddCargo(a: seq<int>,b: seq<int>): seq<int>
    requires |a|==|b|
    ensures |AddCargo(a,b)|==|a|
    decreases |a|
  { if |a|==0 then [] else [a[0]+b[0]]+AddCargo(a[1..],b[1..]) }
  datatype SimpleState = SimpleState(research: U8,tech: U8,credential: U32,
    template: Bytes,relations: Bytes,fleetPlans: map<U16,U8>,mineFlags: map<U16,U8>)
  function ApplySimple(s: SimpleState,rec: Record,cPlayer: nat,fGenerating: bool): SimpleState
    requires FCommand(rec,cPlayer)
    requires |s.template|==26
  {
    var b:=rec.rgb;
    if rec.rt==34 then s.(research:=b[0],tech:=b[1])
    else if rec.rt==36 && fGenerating then s.(credential:=D(b,0)%4294967296)
    else if rec.rt==38 then s.(relations:=b)
    else if rec.rt==42 && W(b,0) in s.fleetPlans then s.(fleetPlans:=s.fleetPlans[W(b,0):=Byte(W(b,2))])
    else if rec.rt==43 && W(b,0) in s.mineFlags then s.(mineFlags:=s.mineFlags[W(b,0):=Byte(W(b,2))])
    else if rec.rt==46 && fGenerating then s.(template:=b+s.template[|b|..])
    else s
  }
  function CargoWidth(delta: int): nat {
    var n:=if delta<0 then -delta else delta;
    if n<=127 then 1 else if n<=32767 then 2 else 4
  }
  function ExpandWaypoint(b: Bytes): Bytes
    requires 4<=|b|<=22
    ensures |ExpandWaypoint(b)|==18
  { b[4..]+Zeros(22-|b|) }
  function TrimZeros(b: Bytes): Bytes
    decreases |b|
  { if |b|==0 || b[|b|-1]!=0 then b else TrimZeros(b[..|b|-1]) }
  // Exactly the writer's trimming, including the potentially short prefix.
  function WaypointCommand(id: I16, index: I16, order: Bytes): Bytes
    requires |order|==18
  { TrimZeros(LE(id,2)+LE(index,2)+order) }

  datatype Context = Context(iPlayer: PlayerId, cPlayer: nat, fGenerating: bool,
    fleets: map<U16, PlayerId>, cords: map<U16,nat>, planets: map<U16,PlayerId>,
    mines: set<U16>, cPlans: nat)
  predicate FOwnedPlanet(b: Bytes, ctx: Context) {
    |b|>=2 && W(b,0) in ctx.planets && ctx.planets[W(b,0)]==ctx.iPlayer
  }
  predicate FReplayContext(rec: Record, ctx: Context) {
    FCommand(rec,ctx.cPlayer) && ctx.iPlayer<ctx.cPlayer &&
    var b:=rec.rgb;
    (if rec.rt in {3,4,5,10,11,24,42,44} then
       W(b,0) in ctx.fleets &&
       (if rec.rt in {3,4,5,11} then W(b,0) in ctx.cords &&
         (if rec.rt==3 then W(b,2)%32768 + (if W(b,2)>=32768 then 2 else 1)<=ctx.cords[W(b,0)]
          else if rec.rt==4 then W(b,2)<=ctx.cords[W(b,0)]
          else W(b,2)<ctx.cords[W(b,0)]) else true)
     else if rec.rt in {29,35} then FOwnedPlanet(b,ctx)
     else if rec.rt==27 then Bits(W(b,0),4,4)==ctx.iPlayer
     else if rec.rt==23 then W(b,0) in ctx.fleets && W(b,2) in ctx.fleets &&
       ctx.fleets[W(b,0)]==ctx.fleets[W(b,2)]
     else if rec.rt==30 then W(b,0)%16==ctx.iPlayer && Bits(W(b,0),4,4)<=ctx.cPlans && ctx.cPlans<=16
     else if rec.rt==43 then W(b,0) in ctx.mines else true)
  }
  predicate FGenerationOnly(rt: RecordTag) { rt in {36,46} }
  // A trace is supplied by the gameplay owner: S01 controls sequencing and
  // aggregate success, not cargo capacity, designs, colonization or combat.
  datatype Step<T> = Step(state: T, fSuccess: bool)
  function Replay<T>(state: T, records: seq<Record>, apply: (T,Record)->Step<T>): Step<T>
    decreases |records|
  {
    if |records|==0 then Step(state,true)
    else var first:=apply(state,records[0]);
      var rest:=Replay(first.state,records[1..],apply);
      Step(rest.state,first.fSuccess && rest.fSuccess)
  }
  function ApplyGenerationGate<T>(state: T, rec: Record, fGenerating: bool,
                                  apply: (T,Record)->Step<T>): Step<T> {
    if !FCommandTag(rec.rt) || (!fGenerating && FGenerationOnly(rec.rt)) then Step(state,true)
    else apply(state,rec)
  }
  datatype Prod = Prod(cItem: nat, iItem: nat, grobj: nat, pct: nat)
  predicate FProd(p: Prod) { p.cItem<1024 && p.iItem<128 && p.grobj<8 && p.pct<128 }
  function ProdWord(p: Prod): nat
    requires FProd(p)
  { p.cItem+1024*p.iItem+131072*p.grobj+1048576*p.pct }
  predicate FSameItem(a: Prod,b: Prod) { a.iItem==b.iItem && a.grobj==b.grobj }
  function ProgressSource(prior: seq<Prod>, p: Prod): nat
    ensures ProgressSource(prior,p)<=|prior|
    decreases |prior|
  { if |prior|==0 || (FSameItem(prior[0],p) && prior[0].pct!=0) then 0 else 1+ProgressSource(prior[1..],p) }
  function QueueProgress(prior: seq<Prod>, incoming: seq<Prod>): seq<Prod>
    ensures |QueueProgress(prior,incoming)|==|incoming|
    decreases |incoming|
  {
    if |incoming|==0 then []
    else var p:=incoming[0]; var i:=ProgressSource(prior,p);
      if p.pct==0 then [p]+QueueProgress(prior,incoming[1..])
      else if i==|prior| then [p.(pct:=0)]+QueueProgress(prior,incoming[1..])
      else [p]+QueueProgress(prior[i:=prior[i].(pct:=0)],incoming[1..])
  }
  function ClearDesignProgress(queue: seq<Prod>, ishdef: nat, grobjDesign: nat): seq<Prod>
    decreases |queue|
  {
    if |queue|==0 then [] else
      [if queue[0].iItem==ishdef && queue[0].grobj==grobjDesign then queue[0].(pct:=0) else queue[0]]+
      ClearDesignProgress(queue[1..],ishdef,grobjDesign)
  }
  // Conservation boundary for an undelivered transfer; gameplay decides moved.
  predicate FCargoResult(before1: int,before2: int,requested: int,moved: int,after1: int,after2: int) {
    (if requested>=0 then 0<=moved<=requested else requested<=moved<=0) &&
    after1==before1+moved && after2==before2-moved
  }
  lemma CargoConservation(a: int,b: int,r: int,m: int,x: int,y: int)
    requires FCargoResult(a,b,r,m,x,y)
    ensures x+y==a+b
  {}
  lemma ReplayContinuesAfterFailure()
  {
    var failThenSuccess: (int,Record)->Step<int> := (n,rec)=>Step(n+1,n!=0);
    assert Replay(0,[Record(0,[]),Record(0,[])],failThenSuccess)==Step(2,false);
  }
  lemma OrderVectors()
    ensures CargoWidth(-128)==2 && CargoWidth(127)==1 && CargoWidth(32768)==4
    ensures !FCommand(Record(34,[101,0]),2)
    ensures FCommand(Record(34,[100,117]),2)
    ensures !FCommand(Record(43,[0,0]),2)
  {}
}
