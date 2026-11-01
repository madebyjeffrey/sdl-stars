include "S01-codecs.dfy"
include "S06-lookups.dfy"

// util.c FDupPlanet / FLookupPlanet. Prefix is the native packed 52-byte
// PLANET body, not a newly defined disk record. Queue pointers are separate.
module S06PlanetEditor {
  import S = S01
  import C = S01Codecs
  import L = S06Lookups
  datatype Planet = Planet(prefix: S.Bytes,queue: nat)
  predicate PlanetFits(p: Planet) { |p.prefix|==52 }
  function Id(p: Planet): int
    requires PlanetFits(p)
  { C.Signed((p.prefix[0] as int)+256*(p.prefix[1] as int),16) }
  datatype Block = Block(maximum: S.U8,count: S.U8,rows: seq<S.U32>)
  datatype Mode = Duplicate | Write
  datatype Event = Allocate(size: nat) | Resize(handle: nat,size: nat) | Free(handle: nat)
    | CopyRows(count: nat) | InvalidateReport | LogChange | CopyPrefix | Tutorial
  datatype Status = Returned | Escaped | OutsideAllocation | InvalidHandle | OverlappingCopy
  datatype QueueResult = QueueResult(heap: map<nat,Block>,queue: nat,events: seq<Event>,status: Status)
  function Request(h: map<nat,Block>,src: nat,dst: nat,mode: Mode): int
    requires src in h && (dst==0 || dst in h)
    ensures -1<=Request(h,src,dst,mode)<=257
  {
    if dst==0 then (if mode==Duplicate then h[src].maximum else h[src].count)
    else if h[dst].maximum<h[src].count then (if mode==Duplicate then h[src].maximum else h[src].count+2)
    else -1
  }
  function Resized(b: Block,size: nat,extra: nat,fill: seq<S.U32>): Block
    requires size+extra<=|fill| && |b.rows|<=|fill|
    ensures |Resized(b,size,extra,fill).rows|==(if |b.rows|>size+extra then |b.rows| else size+extra)
    ensures Resized(b,size,extra,fill).maximum==size%256
    ensures Resized(b,size,extra,fill).count==b.count
  {
    var extent:=if |b.rows|>size+extra then |b.rows| else size+extra;
    Block(size%256,b.count,b.rows+fill[|b.rows|..extent])
  }
  function Transfer(h: map<nat,Block>,src: nat,dst: nat,mode: Mode,ok: bool,handleNew: nat,extra: nat,fill: seq<S.U32>): QueueResult
    requires 0 !in h && |fill|>=257+extra
    requires forall k <- h.Keys :: |h[k].rows|<=|fill|
    requires handleNew>0 && (handleNew==dst || handleNew !in h)
    requires handleNew!=src || src==dst
  {
    if (src!=0 && src !in h) || (dst!=0 && dst !in h) then QueueResult(h,dst,[],InvalidHandle)
    else if mode==Write && src==dst then QueueResult(h,dst,[],Returned)
    else if src==0 then
      if dst==0 then QueueResult(h,0,[],Returned)
      else QueueResult(h-{dst},0,[Free(dst)],Returned)
    else
      var request:=Request(h,src,dst,mode);
      var events:=if request<0 then [] else if dst==0 then [Allocate(request)] else [Resize(dst,request)];
      if request>=0 && !ok then QueueResult(h,dst,events,Escaped)
      else
        var target:=if request<0 then dst else handleNew;
        var b:=if request<0 then h[dst]
          else if dst==0 then Block(request%256,0,fill[..request+extra])
          else Resized(h[dst],request,extra,fill);
        var heap:=if request<0 then h else (h-{dst})[target:=b];
        if src !in heap then QueueResult(heap,target,events,InvalidHandle)
        else
          var n:=heap[src].count;
          if n>|heap[src].rows| || n>|b.rows| then QueueResult(heap,target,events,OutsideAllocation)
          else if n>0 && src==target then QueueResult(heap,target,events,OverlappingCopy)
          else QueueResult(heap[target:=b.(count:=n,rows:=heap[src].rows[..n]+b.rows[n..])],
            target,events+[CopyRows(n)],Returned)
  }
  datatype CopyResult = CopyResult(planet: Planet,heap: map<nat,Block>,events: seq<Event>,status: Status)
  function DuplicatePlanet(src: Planet,dst: Planet,h: map<nat,Block>,ok: bool,handleNew: nat,extra: nat,fill: seq<S.U32>): CopyResult
    requires PlanetFits(src) && PlanetFits(dst) && 0 !in h && |fill|>=257+extra
    requires forall k <- h.Keys :: |h[k].rows|<=|fill|
    requires handleNew>0 && (handleNew==dst.queue || handleNew !in h)
    requires handleNew!=src.queue || src.queue==dst.queue
    ensures DuplicatePlanet(src,dst,h,ok,handleNew,extra,fill).planet.prefix==src.prefix
  {
    var q:=Transfer(h,src.queue,dst.queue,Duplicate,ok,handleNew,extra,fill);
    CopyResult(Planet(src.prefix,q.queue),q.heap,[CopyPrefix]+q.events,q.status)
  }
  // Called after InvalidateReport and LogChangePlanet return. Callback effects
  // are supplied in src/dst/h. Queue escape occurs before the 52-byte copy.
  function WritePlanet(src: Planet,dst: Planet,h: map<nat,Block>,ok: bool,handleNew: nat,extra: nat,fill: seq<S.U32>): CopyResult
    requires PlanetFits(src) && PlanetFits(dst) && 0 !in h && |fill|>=257+extra
    requires forall k <- h.Keys :: |h[k].rows|<=|fill|
    requires handleNew>0 && (handleNew==dst.queue || handleNew !in h)
    requires handleNew!=src.queue || src.queue==dst.queue
    ensures WritePlanet(src,dst,h,ok,handleNew,extra,fill).status!=Returned ==>
      WritePlanet(src,dst,h,ok,handleNew,extra,fill).planet.prefix==dst.prefix
  {
    var q:=Transfer(h,src.queue,dst.queue,Write,ok,handleNew,extra,fill);
    CopyResult(Planet(if q.status==Returned then src.prefix else dst.prefix,q.queue),q.heap,
      q.events+(if q.status==Returned then [CopyPrefix] else []),q.status)
  }
  datatype Entry = NoPlanets | UndefinedDestination | LogOnly | Find(id: int,write: bool)
  function Enter(count: int,request: int,destination: bool,currentId: int): Entry {
    if count<=0 then NoPlanets else if request>=0 then Find(request,false)
    else if !destination then UndefinedDestination
    else if currentId==-1 then LogOnly else Find(currentId,true)
  }
  // L.Planet owns actual index resolution; positive native count/array domain
  // and post-lookup pointer lifetime remain the adapter's responsibility.
  function Resolve(rows: seq<L.Entry>,maximum: nat,id: int): L.Result { L.Planet(rows,maximum,id) }
  datatype Read = NotFound | FoundWithoutDestination | Borrow | CopySelected
  function ReadPath(found: bool,destination: bool,selected: bool): Read {
    if !found then NotFound else if !destination then FoundWithoutDestination
    else if selected then CopySelected else Borrow
  }
  function Shallow(src: Planet): Planet { src }
  datatype Phase = BeforeInvalidate | BeforeLog | BeforeQueue | AfterPrefix | Finished | Aborted
  datatype Dispatch = Dispatch(phase: Phase,events: seq<Event>)
  function WriteStep(p: Phase,queueStatus: Status,tutorial: bool,player: int): Dispatch {
    match p
    case BeforeInvalidate => Dispatch(BeforeLog,[InvalidateReport])
    case BeforeLog => Dispatch(BeforeQueue,[LogChange])
    case BeforeQueue => Dispatch(if queueStatus==Returned then AfterPrefix else Aborted,[])
    case AfterPrefix => Dispatch(Finished,if tutorial && player==0 then [Tutorial] else [])
    case Finished => Dispatch(Finished,[])
    case Aborted => Dispatch(Aborted,[])
  }
  lemma QueueFailureAborts(status: Status,tutorial: bool,player: int)
    requires status!=Returned
    ensures WriteStep(BeforeQueue,status,tutorial,player)==Dispatch(Aborted,[])
    ensures WriteStep(Aborted,status,tutorial,player)==Dispatch(Aborted,[])
  {}
  lemma EqualPointersSkipQueue(h: map<nat,Block>,p: nat,ok: bool,handleNew: nat,extra: nat,fill: seq<S.U32>)
    requires 0 !in h && (p==0 || p in h) && |fill|>=257+extra
    requires forall k <- h.Keys :: |h[k].rows|<=|fill|
    requires handleNew>0 && (handleNew==p || handleNew !in h)
    ensures Transfer(h,p,p,Write,ok,handleNew,extra,fill)==QueueResult(h,p,[],Returned)
  {}
  lemma CopiesKeepTail(h: map<nat,Block>,src: nat,dst: nat,handleNew: nat,extra: nat,fill: seq<S.U32>)
    requires 0 !in h && src in h && dst in h && src!=dst && |fill|>=257+extra
    requires h[dst].maximum>=h[src].count
    requires h[src].count<=|h[src].rows| && h[src].count<=|h[dst].rows|
    requires forall k <- h.Keys :: |h[k].rows|<=|fill|
    requires handleNew>0 && (handleNew==dst || handleNew !in h) && handleNew!=src
    ensures Transfer(h,src,dst,Duplicate,true,handleNew,extra,fill).status==Returned
    ensures Transfer(h,src,dst,Duplicate,true,handleNew,extra,fill).heap[dst].rows[h[src].count..]==h[dst].rows[h[src].count..]
    ensures Transfer(h,src,dst,Duplicate,true,handleNew,extra,fill).heap[src]==h[src]
    ensures Transfer(h,src,dst,Duplicate,true,handleNew,extra,fill).heap[dst].rows[..h[src].count]==h[src].rows[..h[src].count]
    ensures Transfer(h,src,dst,Duplicate,true,handleNew,extra,fill).heap[dst].count==h[src].count
    ensures Transfer(h,src,dst,Duplicate,true,handleNew,extra,fill).heap[dst].maximum==h[dst].maximum
  {}
  lemma {:isolate_assertions} Vectors() {
    var fill: seq<S.U32>:=seq(257,i=>99);
    var a:=Planet(seq(52,i=>1),1);
    var b:=Planet(seq(52,i=>2),2);
    var h:=map[1:=Block(4,2,[10,20,30,40]),2:=Block(1,1,[7])];
    assert Request(h,1,2,Duplicate)==4 && Request(h,1,2,Write)==4;
    assert DuplicatePlanet(a,b,h,false,3,0,fill).planet.prefix==a.prefix;
    assert DuplicatePlanet(a,b,h,false,3,0,fill).planet.queue==2;
    assert DuplicatePlanet(a,b,h,false,3,0,fill).status==Escaped;
    assert WritePlanet(a,b,h,false,3,0,fill).planet.prefix==b.prefix;
    assert Transfer(h,1,2,Duplicate,true,3,0,fill).heap[3].rows==[10,20,99,99];
    assert Transfer(h,1,2,Write,true,3,0,fill).heap[3].rows==[10,20,99,99];
    var large:=map[1:=Block(255,255,seq(255,i=>0)),2:=Block(0,0,[])];
    assert Request(large,1,2,Write)==257;
    assert Transfer(large,1,2,Write,true,3,0,fill).heap[3].maximum==1;
    var corrupt:=map[1:=Block(0,2,[10,20])];
    assert Transfer(corrupt,1,0,Duplicate,true,3,0,fill).status==OutsideAllocation;
    var padded: seq<S.U32>:=seq(259,i=>99);
    assert Transfer(corrupt,1,0,Duplicate,true,3,2,padded).status==Returned;
    assert Transfer(corrupt,1,0,Duplicate,true,3,2,padded).heap[3].maximum==0;
    assert Transfer(h,1,1,Duplicate,true,3,0,fill).status==OverlappingCopy;
    assert Enter(0,-1,false,-1)==NoPlanets;
    assert Enter(1,-32768,true,-1)==LogOnly;
    assert Enter(1,-32768,true,5)==Find(5,true);
    assert Enter(1,5,false,0)==Find(5,false);
    assert ReadPath(true,false,false)==FoundWithoutDestination;
    assert WriteStep(BeforeLog,Returned,true,0).events==[LogChange];
    assert WriteStep(AfterPrefix,Returned,true,0).events==[Tutorial];
  }
}
