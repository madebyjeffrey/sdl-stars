include "M11-native-availability.dfy"

// FCanBuildShdef, util.c: availability controller under the actual returning
// lookup contract. Undefined outcomes mark native read-domain prefixes; they
// are not additional FALSE returns, validation or recovery in the C routine.
module M11DesignBuildability {
  import A = M11NativeAvailability
  import D = M11Design
  import G = SharedGameState
  import S = S01
  import E = M08NativeEconomy
  import I = S03LookupIdentities
  import K = S03ResearchCatalog
  datatype Design = Design(hull: S.U16,count: S.U8,slots: seq<G.HullSlot>)
  predicate Shape(d: Design) { |d.slots|==16 && G.HullSlotsFit(d.slots) }
  predicate PlayersFit(players: seq<A.Context>) {
    |players|==16 && (forall i | 0<=i<16 :: A.Shape(players[i]) && !players[i].headless)
  }
  datatype Quantity = Uninitialized | Known(count: S.U8)
  datatype Request = Request(mask: S.U16,item: S.U8,quantity: Quantity)
  function HullRequest(hull: S.U16): Request {
    var base:=E.Word(hull)>=I.ihuldefOrbitalFort;
    Request(K.Masks()[if base then 10 else 14],
      ((hull as int)-(if base then I.ihuldefOrbitalFort else 0))%256,Uninitialized)
  }
  function SlotRequest(slot: G.HullSlot): Request
    requires 0<=slot.item<256 && 0<=slot.count<256
  { Request(slot.category,slot.item as S.U8,Known(slot.count as S.U8)) }
  predicate ValidPlayer(player: S.I16) { player==-1 || 0<=player<16 }
  function Selected(players: seq<A.Context>,player: S.I16): A.Context
    requires PlayersFit(players) && ValidPlayer(player)
    ensures A.Shape(Selected(players,player))
  { if player==-1 then A.Context(true,0,0,0,[0,0,0,0,0,0],0) else players[player] }
  function Lookup(c: A.Context,rows: seq<seq<A.PartRow>>,request: Request): A.Result
    requires A.Shape(c) && A.CatalogShape(rows)
    ensures -1<=Lookup(c,rows,request).status<=256
  { A.LookupRow(c,rows,request.mask,request.item,A.Prior(0)) }
  // Internal PART quantity/prior pointer are unobserved when not initialized;
  // only a returned status and the actual getter/call sequence are recorded.
  datatype Answer = NoAnswer | Returned(status: S.I16,calls: seq<nat>)
  datatype Probe = Probe(request: Request,answer: Answer)
  datatype Status = Done(buildable: bool) | Undefined(reason: string)
  datatype Outcome = Outcome(status: Status,global: S.I16,trace: seq<Probe>)
  function Finish(saved: S.I16,value: bool,trace: seq<Probe>): Outcome
  { Outcome(Done(value),saved,trace) }
  function Decode(status: S.I16): D.Availability
    requires -1<=status<=256
    ensures A.Code(Decode(status))==status
    ensures Decode(status)==D.Available <==> status==1
  {
    if status==-1 then D.Restricted else if status==0 then D.Invalid else
    if status==1 then D.Available else if status==2 then D.NextTech else
    if status==99 then D.OtherTech else D.Later(status as nat)
  }
  function Requirements(d: Design,c: A.Context,rows: seq<seq<A.PartRow>>,at: nat): seq<D.Availability>
    requires Shape(d) && d.count<=16 && at<=d.count && A.Shape(c) && A.CatalogShape(rows)
    decreases d.count-at
  {
    if at==d.count then [] else
    (if d.slots[at].count==0 then [] else [Decode(Lookup(c,rows,SlotRequest(d.slots[at])).status)])+
      Requirements(d,c,rows,at+1)
  }
  lemma CanBuildCons(head: D.Availability,tail: seq<D.Availability>)
    ensures D.CanBuild(D.Available,[head]+tail)==(head==D.Available && D.CanBuild(D.Available,tail))
  {
    assert head in [head]+tail;
    if D.CanBuild(D.Available,[head]+tail) {
      forall part | part in tail ensures part==D.Available {
        assert part in [head]+tail;
      }
    } else if head==D.Available && D.CanBuild(D.Available,tail) {
      forall part | part in [head]+tail ensures part==D.Available {
        assert part==head || part in tail;
      }
    }
  }
  lemma RequirementStep(d: Design,c: A.Context,rows: seq<seq<A.PartRow>>,at: nat)
    requires Shape(d) && A.Shape(c) && A.CatalogShape(rows) && at<d.count && at<16
    ensures d.count<=16 ==> (D.CanBuild(D.Available,Requirements(d,c,rows,at))==
      ((d.slots[at].count==0 || Lookup(c,rows,SlotRequest(d.slots[at])).status==1) &&
       D.CanBuild(D.Available,Requirements(d,c,rows,at+1))))
  {
    if d.count<=16 {
      var tail:=Requirements(d,c,rows,at+1);
      if d.slots[at].count==0 {
        assert Requirements(d,c,rows,at)==tail;
      } else {
        var status:=Lookup(c,rows,SlotRequest(d.slots[at])).status;
        assert Requirements(d,c,rows,at)==[Decode(status)]+tail;
        CanBuildCons(Decode(status),tail);
      }
    }
  }
  function {:isolate_assertions} Traverse(d: Design,c: A.Context,rows: seq<seq<A.PartRow>>,
    selected: S.I16,saved: S.I16,at: nat,trace: seq<Probe>): Outcome
    requires Shape(d) && A.Shape(c) && A.CatalogShape(rows) && at<=16 && at<=d.count
    ensures Traverse(d,c,rows,selected,saved,at,trace).status.Done? ==> Traverse(d,c,rows,selected,saved,at,trace).global==saved
    ensures Traverse(d,c,rows,selected,saved,at,trace).status.Undefined? ==> Traverse(d,c,rows,selected,saved,at,trace).global==selected
    ensures |trace|<=|Traverse(d,c,rows,selected,saved,at,trace).trace|<=|trace|+16-at
    ensures Traverse(d,c,rows,selected,saved,at,trace).trace[..|trace|]==trace
    ensures d.count<=16 ==> Traverse(d,c,rows,selected,saved,at,trace).status.Done?
    ensures d.count<=16 ==> Traverse(d,c,rows,selected,saved,at,trace).status.buildable==D.CanBuild(D.Available,Requirements(d,c,rows,at))
    decreases 16-at
  {
    if at==d.count then Finish(saved,true,trace) else
    if at==16 then Outcome(Undefined("design slot read"),selected,trace) else
    RequirementStep(d,c,rows,at);
    var slot:=d.slots[at];
    if slot.count==0 then Traverse(d,c,rows,selected,saved,at+1,trace) else
    var request:=SlotRequest(slot);var answer:=Lookup(c,rows,request);
    var recorded:=trace+[Probe(request,Returned(answer.status,answer.calls))];
    if answer.status!=1 then Finish(saved,false,recorded) else Traverse(d,c,rows,selected,saved,at+1,recorded)
  }
  function {:isolate_assertions} Check(d: Design,players: seq<A.Context>,rows: seq<seq<A.PartRow>>,
    selected: S.I16,saved: S.I16): Outcome
    requires Shape(d) && PlayersFit(players) && A.CatalogShape(rows)
    ensures Check(d,players,rows,selected,saved).status.Done? ==> Check(d,players,rows,selected,saved).global==saved
    ensures Check(d,players,rows,selected,saved).status.Undefined? ==> Check(d,players,rows,selected,saved).global==selected
    ensures 1<=|Check(d,players,rows,selected,saved).trace|<=17
    ensures Check(d,players,rows,selected,saved).trace[0].request==HullRequest(d.hull)
    ensures ValidPlayer(selected) && d.count<=16 ==> Check(d,players,rows,selected,saved).status.Done?
    ensures ValidPlayer(selected) && d.count<=16 ==>
      Check(d,players,rows,selected,saved).status.buildable==D.CanBuild(
        Decode(Lookup(Selected(players,selected),rows,HullRequest(d.hull)).status),
        Requirements(d,Selected(players,selected),rows,0))
  {
    var request:=HullRequest(d.hull);
    if !ValidPlayer(selected) then Outcome(Undefined("lookup player index"),selected,[Probe(request,NoAnswer)]) else
    var c:=Selected(players,selected);var answer:=Lookup(c,rows,request);
    var trace:=[Probe(request,Returned(answer.status,answer.calls))];
    if answer.status!=1 then Finish(saved,false,trace) else Traverse(d,c,rows,selected,saved,0,trace)
  }
  lemma HullOnly(d: Design,players: seq<A.Context>,rows: seq<seq<A.PartRow>>,selected: S.I16,saved: S.I16)
    requires Shape(d) && d.count==0 && PlayersFit(players) && A.CatalogShape(rows) && ValidPlayer(selected)
    ensures Check(d,players,rows,selected,saved).status.Done?
    ensures Check(d,players,rows,selected,saved).global==saved
    ensures |Check(d,players,rows,selected,saved).trace|==1
    ensures Check(d,players,rows,selected,saved).status.buildable==
      (Lookup(Selected(players,selected),rows,HullRequest(d.hull)).status==1)
  {}
  lemma RejectBeforeExtent(d: Design,players: seq<A.Context>,rows: seq<seq<A.PartRow>>,selected: S.I16,saved: S.I16)
    requires Shape(d) && PlayersFit(players) && A.CatalogShape(rows) && ValidPlayer(selected)
    requires Lookup(Selected(players,selected),rows,HullRequest(d.hull)).status!=1
    ensures Check(d,players,rows,selected,saved).status==Done(false)
    ensures Check(d,players,rows,selected,saved).global==saved
    ensures |Check(d,players,rows,selected,saved).trace|==1
  {}
  lemma TraverseEmptyOverrun(d: Design,c: A.Context,rows: seq<seq<A.PartRow>>,
    selected: S.I16,saved: S.I16,at: nat,trace: seq<Probe>)
    requires Shape(d) && A.Shape(c) && A.CatalogShape(rows) && at<=16 && d.count>16
    requires forall slot <- d.slots :: slot.count==0
    ensures Traverse(d,c,rows,selected,saved,at,trace)==Outcome(Undefined("design slot read"),selected,trace)
    decreases 16-at
  {
    if at<16 { TraverseEmptyOverrun(d,c,rows,selected,saved,at+1,trace); }
  }
  lemma EmptyOverrun(d: Design,players: seq<A.Context>,rows: seq<seq<A.PartRow>>,selected: S.I16,saved: S.I16)
    requires Shape(d) && PlayersFit(players) && A.CatalogShape(rows) && ValidPlayer(selected) && d.count>16
    requires forall slot <- d.slots :: slot.count==0
    requires Lookup(Selected(players,selected),rows,HullRequest(d.hull)).status==1
    ensures Check(d,players,rows,selected,saved).status==Undefined("design slot read")
    ensures Check(d,players,rows,selected,saved).global==selected
    ensures |Check(d,players,rows,selected,saved).trace|==1
  {
    var c:=Selected(players,selected);var request:=HullRequest(d.hull);var answer:=Lookup(c,rows,request);
    TraverseEmptyOverrun(d,c,rows,selected,saved,0,[Probe(request,Returned(answer.status,answer.calls))]);
  }
  lemma {:isolate_assertions} HullVectors() {
    E.WordPreserves(0);E.WordPreserves(32);E.WordPreserves(288);
    assert HullRequest(0)==Request(16384,0,Uninitialized);
    assert HullRequest(32)==Request(1024,0,Uninitialized);
    assert HullRequest(288)==Request(1024,0,Uninitialized);
    assert HullRequest(32768)==Request(16384,0,Uninitialized);
  }
}
