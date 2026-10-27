include "M12-development-quotes.dfy"
include "S03-research-catalog.dfy"

// UpdateResearchStatus's entire equipment enumeration. Returning lookups and
// message calls preserve research/control fields; actual catalog/message frames
// and delivery remain separately qualified. Traces do not invent a terminator.
module M12ResearchAnnouncements {
  import E = M08NativeEconomy
  import S = S01
  import C = S01Codecs
  import K = S03ResearchCatalog
  function Masks(): seq<S.U16> { K.Masks() }
  datatype Notice = Ordinary | General | HullDesign | HullType | Build | Build2 | Benefit | Shared
  datatype Event = Lookup(player: nat,kind: nat,item: S.U8) | Trait(player: nat,name: string)
    | Message(player: nat,notice: Notice,target: S.I16,a: S.I16,b: S.I16,c: S.I16)
    | Cost(player: nat,field: nat,level: int,value: E.Number) | Global(identity: S.I16) | Recurse
  datatype Reply = Reply(kind: nat,item: S.U8,availability: S.I16,requirement: E.I8,pointer: bool,returned: bool)
  datatype Status = Done | Pending | Escaped | Undefined(reason: string)
  datatype Result = Result(status: Status,cursor: nat,events: seq<Event>)
  function PartTarget(kind: nat,item: S.I16): S.I16
    requires kind<16
  {
    var high: bv16:=(49152+kind*256) as bv16;var low: bv16:=((item as int)%65536) as bv16;
    E.Word((high|low) as int)
  }
  function AfterLookup(player: nat,field: nat,level: E.I8,tt: bool,kind: nat,item: S.I16,reply: Reply,issued: seq<Event>): seq<Event>
    requires player<16 && field<6 && kind<16
  {
    var matches:=reply.availability==1 && level==(reply.requirement as int)%256;
    var checked:=if matches && kind==13 then issued+[Trait(player,"TT")] else issued;
    var suppressed:=matches && kind==13 && tt && item in {8,12,16};
    var notice:=if kind==10 then HullDesign else if kind==14 then HullType
      else if kind==15 && 9<=item<=13 then Build else if kind==15 && 0<=item<=8 then Build2 else Benefit;
    var target:=if kind in {10,14} then -3 else PartTarget(kind,item);
    if matches && !suppressed then checked+[Message(player,notice,target,field as S.I16,E.Word(Masks()[kind]),item)] else checked
  }
  function Scan(player: nat,field: nat,level: E.I8,tt: bool,kind: nat,item: S.I16,trace: seq<Reply>,cursor: nat,events: seq<Event>): Result
    requires player<16 && field<6 && kind<=16 && cursor<=|trace|
    ensures Scan(player,field,level,tt,kind,item,trace,cursor,events).cursor<=|trace|
    ensures Scan(player,field,level,tt,kind,item,trace,cursor,events).cursor>=cursor
    decreases |trace|-cursor,16-kind
  {
    if kind==16 then Result(Done,cursor,events) else
    var issued:=events+[Lookup(player,kind,(item as int)%256)];
    if cursor==|trace| then Result(Pending,cursor,issued) else
    var reply:=trace[cursor];
    if reply.kind!=kind || reply.item!=(item as int)%256 then Result(Undefined("part trace/call mismatch"),cursor,issued) else
    if !reply.returned then Result(Escaped,cursor+1,issued) else
    if reply.availability==0 then Scan(player,field,level,tt,kind+1,0,trace,cursor+1,issued) else
    if reply.availability==1 && !reply.pointer then Result(Undefined("part requirement pointer"),cursor+1,issued) else
    var published:=AfterLookup(player,field,level,tt,kind,item,reply,issued);
    Scan(player,field,level,tt,kind,E.Word((item as int)+1),trace,cursor+1,published)
  }
  lemma {:isolate_assertions} Vectors() {
    assert C.Pow2(4)==16 && C.Pow2(8)==256 && C.Pow2(12)==4096;
    assert C.Pow2(15)==32768 && C.Pow2(16)==65536;
    E.WordPreserves(-3);E.WordPreserves(1);
    assert PartTarget(0,0)==-16384;
    assert PartTarget(15,-1)==-1;
    assert Scan(0,0,1,false,16,0,[],0,[]).status.Done?;
    assert Scan(0,0,1,false,0,0,[],0,[]).status.Pending?;
  }
}
