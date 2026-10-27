include "M12-development-quotes.dfy"

// research.c ProjectedResearchSpending: capture bounds, native allocation byte,
// unconditional gross query, post-query queue branch, then current-player restore.
module M12BudgetPreview {
  import E = M08NativeEconomy
  import S = S01
  import C = S01Codecs
  datatype Queue = Null | Allocated(count: S.U8)
  datatype Planet = Planet(owner: S.I16,queue: Queue)
  datatype World = World(allocations: seq<E.I8>,planets: seq<Planet>,player: S.I16)
  predicate Shape(w: World,count: nat) { |w.allocations|==16 && count<=|w.planets| && count<=32767 }
  function Byte(n: int): E.I8 {
    assert C.Pow2(4)==16 && C.Pow2(7)==128 && C.Pow2(8)==256;
    C.Signed(n,8) as E.I8
  }
  datatype Call = Resource(planet: nat,player: S.I16) | Forecast(planet: nat)
  datatype Reply = Reply(call: Call,world: World,value: S.I16,returned: bool)
  datatype Status = Done | Pending | Escaped | Undefined(reason: string)
  datatype Event = Allocate(player: S.I16,value: E.I8) | Query(call: Call) | Restore(player: S.I16,value: E.I8)
  datatype Read = Read(status: Status,world: World,value: S.I16,cursor: nat,events: seq<Event>)
  function Receive(w: World,count: nat,trace: seq<Reply>,cursor: nat,call: Call,events: seq<Event>): Read
    requires Shape(w,count) && cursor<=|trace|
    ensures Receive(w,count,trace,cursor,call,events).cursor<=|trace|
    ensures Receive(w,count,trace,cursor,call,events).status.Done? ==> Shape(Receive(w,count,trace,cursor,call,events).world,count)
  {
    var issued:=events+[Query(call)];
    if cursor==|trace| then Read(Pending,w,0,cursor,issued) else
    var reply:=trace[cursor];
    if reply.call!=call then Read(Undefined("helper trace/call mismatch"),w,0,cursor,issued) else
    if !reply.returned then Read(Escaped,reply.world,0,cursor+1,issued) else
    if !Shape(reply.world,count) then Read(Undefined("captured array/player storage adapter"),reply.world,reply.value,cursor+1,issued) else
    Read(Done,reply.world,reply.value,cursor+1,issued)
  }
  datatype Result = Result(status: Status,world: World,total: E.I32,cursor: nat,events: seq<Event>)
  function Loop(w: World,count: nat,at: nat,saved: E.I8,total: E.I32,trace: seq<Reply>,cursor: nat,events: seq<Event>): Result
    requires Shape(w,count) && at<=count && cursor<=|trace|
    requires -32768*at<=total<=32767*at
    ensures Loop(w,count,at,saved,total,trace,cursor,events).cursor<=|trace|
    ensures Loop(w,count,at,saved,total,trace,cursor,events).status.Done? ==> Shape(Loop(w,count,at,saved,total,trace,cursor,events).world,count)
    ensures -32768*count<=Loop(w,count,at,saved,total,trace,cursor,events).total<=32767*count
    ensures Loop(w,count,at,saved,total,trace,cursor,events).status.Done? ==>
      0<=Loop(w,count,at,saved,total,trace,cursor,events).world.player<16 &&
      Loop(w,count,at,saved,total,trace,cursor,events).world.allocations[Loop(w,count,at,saved,total,trace,cursor,events).world.player]==saved
    decreases count-at
  {
    if at==count then
      if w.player<0 || w.player>=16 then Result(Undefined("allocation restore player index"),w,total,cursor,events) else
      Result(Done,w.(allocations:=w.allocations[w.player:=saved]),total,cursor,events+[Restore(w.player,saved)])
    else if w.planets[at].owner!=w.player then Loop(w,count,at+1,saved,total,trace,cursor,events) else
    var gross:=Receive(w,count,trace,cursor,Resource(at,w.player),events);
    if !gross.status.Done? then Result(gross.status,gross.world,total,gross.cursor,gross.events) else
    var queue:=gross.world.planets[at].queue;
    var queried:=if queue.Null? || queue.count==0 then gross
      else Receive(gross.world,count,trace,gross.cursor,Forecast(at),gross.events);
    if !queried.status.Done? then Result(queried.status,queried.world,total,queried.cursor,queried.events) else
    var sum: int:=(total as int)+queried.value;
    BoundedAddition(total,queried.value,at);
    if !E.Fits32(sum) then Result(Undefined("research preview total addition"),queried.world,total,queried.cursor,queried.events) else
    Loop(queried.world,count,at+1,saved,sum as E.I32,trace,queried.cursor,queried.events)
  }
  function Preview(w: World,count: nat,pct: E.I32,trace: seq<Reply>): Result
    requires Shape(w,count)
    ensures Preview(w,count,pct,trace).status.Done? ==> Shape(Preview(w,count,pct,trace).world,count)
    ensures Preview(w,count,pct,trace).status.Done? ==>
      (0<=w.player<16 && 0<=Preview(w,count,pct,trace).world.player<16 &&
       Preview(w,count,pct,trace).world.allocations[Preview(w,count,pct,trace).world.player]==w.allocations[w.player])
    ensures -32768*count<=Preview(w,count,pct,trace).total<=32767*count
  {
    if w.player<0 || w.player>=16 then Result(Undefined("allocation entry player index"),w,0,0,[]) else
    var saved:=w.allocations[w.player];var value:=Byte(pct);
    var entered:=w.(allocations:=w.allocations[w.player:=value]);
    Loop(entered,count,0,saved,0,trace,0,[Allocate(w.player,value)])
  }
  function ResourceReply(call: Call,w: World,value: E.Number): Reply
    requires call.Resource?
    requires value.Value? ==> -32768<=value.value<32768
  { Reply(call,w,if value.Value? then value.value as S.I16 else 0,value.Value?) }
  lemma BoundedAddition(total: E.I32,amount: S.I16,at: nat)
    requires at<32767 && -32768*at<=total<=32767*at
    ensures E.Fits32((total as int)+amount)
    ensures -32768*(at+1)<=(total as int)+amount<=32767*(at+1)
  {}
  lemma {:isolate_assertions} Vectors() {
    assert C.Pow2(4)==16 && C.Pow2(7)==128 && C.Pow2(8)==256;
    assert Byte(255)==-1 && Byte(256)==0;
    var allocations: seq<E.I8>:=seq(16,i=>10);
    var w:=World(allocations,[],0);
    assert Preview(w,0,255,[]).status.Done?;
    assert Preview(w,0,255,[]).world.allocations==allocations;
  }
}
