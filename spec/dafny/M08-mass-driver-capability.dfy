include "S03-lookup-identities.dfy"
include "S06-shared-state.dfy"
include "M08-colony.dfy"

// IWarpMAFromLppl. Output pointer is null or disjoint, live signed-word storage.
// Undefined identifies a reached native read boundary, never a C error return.
module M08MassDriver {
  import S = S01
  import G = SharedGameState
  import I = S03LookupIdentities
  import C = M08Colony
  datatype Design = Design(detail: nat,count: S.U8,slots: seq<G.HullSlot>)
  predicate Shape(d: Design) {
    d.detail<8 && |d.slots|==16 && G.HullSlotsFit(d.slots)
  }
  datatype Roster = Missing | Present(entries: seq<Design>)
  predicate RostersFit(rows: seq<Roster>) {
    |rows|==16 && (forall row <- rows :: row.Present? ==>
      |row.entries|==10 && forall d <- row.entries :: Shape(d))
  }
  datatype Capability = Capability(warp: nat,two: bool)
  predicate Fits(c: Capability) { (c.warp==0 || 5<=c.warp<=13) && (c.two ==> c.warp>0) }
  function Candidate(slot: G.HullSlot): nat {
    if slot.category==512 && slot.count>0 && I.ispecialSBMassDriver5<=slot.item<=I.ispecialSBUltraDriver13
    then slot.item-2 else 0
  }
  function Add(c: Capability,value: nat): Capability
    requires Fits(c) && (value==0 || 5<=value<=13)
    ensures Fits(Add(c,value))
  { if value==0 || value<c.warp then c else if value>c.warp then Capability(value,false) else Capability(value,true) }
  function Max(slots: seq<G.HullSlot>): nat
    ensures Max(slots)==0 || 5<=Max(slots)<=13
    decreases |slots|
  {
    if |slots|==0 then 0 else
    var tail:=Max(slots[1..]);var value:=Candidate(slots[0]);if value>tail then value else tail
  }
  function Count(slots: seq<G.HullSlot>,value: nat): nat
    ensures value==0 ==> Count(slots,value)==0
    decreases |slots|
  { if |slots|==0 then 0 else (if Candidate(slots[0])==value && value>0 then 1 else 0)+Count(slots[1..],value) }
  lemma CountFacts(slots: seq<G.HullSlot>,value: nat)
    ensures value>Max(slots) ==> Count(slots,value)==0
    ensures Max(slots)>0 ==> Count(slots,Max(slots))>0
    decreases |slots|
  {
    if |slots|>0 {
      CountFacts(slots[1..],value);
      CountFacts(slots[1..],Max(slots));
    }
  }
  function Fold(slots: seq<G.HullSlot>,c: Capability): Capability
    requires Fits(c)
    ensures Fits(Fold(slots,c))
    ensures Fold(slots,c).warp==(if Max(slots)>c.warp then Max(slots) else c.warp)
    ensures Fold(slots,c).two==(if Max(slots)>c.warp then Count(slots,Max(slots))>=2
      else if Max(slots)==c.warp && c.warp>0 then c.two || Count(slots,c.warp)>0 else c.two)
    decreases |slots|
  {
    if |slots|==0 then c else
    var value:=Candidate(slots[0]);var next:=Add(c,value);
    CountFacts(slots[1..],Max(slots));
    CountFacts(slots[1..],c.warp);
    CountFacts(slots[1..],next.warp);
    Fold(slots[1..],next)
  }
  function Occupied(slots: seq<G.HullSlot>): seq<nat>
    decreases |slots|
  { if |slots|==0 then [] else (if Candidate(slots[0])>0 then [Candidate(slots[0])] else [])+Occupied(slots[1..]) }
  lemma Scalar(slots: seq<G.HullSlot>,c: Capability)
    requires Fits(c)
    ensures C.Drivers(Occupied(slots),C.Driver(c.warp,c.two))==
      C.Driver(Fold(slots,c).warp,Fold(slots,c).two)
    decreases |slots|
  {
    if |slots|>0 {
      var value:=Candidate(slots[0]);var next:=Add(c,value);
      Scalar(slots[1..],next);
      if value>0 {
        assert Occupied(slots)==[value]+Occupied(slots[1..]);
        assert C.Drivers(Occupied(slots),C.Driver(c.warp,c.two))==
          C.Drivers(Occupied(slots[1..]),C.Driver(next.warp,next.two));
      } else { assert Occupied(slots)==Occupied(slots[1..]); }
    }
  }
  datatype Event = ClearOutput | RosterRead(owner: S.I16) | DetailRead(index: nat)
    | HullRead(index: nat) | SlotRead(index: nat) | StoreOutput(value: bool)
  datatype Status = Done | Undefined(reason: string)
  datatype Outcome = Outcome(status: Status,capability: Capability,output: G.Option<bool>,events: seq<Event>)
  function Output(present: bool,value: bool): G.Option<bool> { if present then G.Some(value) else G.None }
  function Scan(d: Design,at: nat,c: Capability,output: bool,events: seq<Event>): Outcome
    requires Shape(d) && at<=16 && at<=d.count && Fits(c)
    ensures Fits(Scan(d,at,c,output,events).capability)
    ensures d.count<=16 ==> Scan(d,at,c,output,events).status==Done
    ensures d.count<=16 ==> Scan(d,at,c,output,events).capability==Fold(d.slots[at..d.count],c)
    ensures Scan(d,at,c,output,events).status==Done ==>
      Scan(d,at,c,output,events).output==Output(output,Scan(d,at,c,output,events).capability.two)
    ensures Scan(d,at,c,output,events).status.Undefined? ==> Scan(d,at,c,output,events).output==Output(output,false)
    decreases 16-at
  {
    if at==d.count then Outcome(Done,c,Output(output,c.two),events+(if output then [StoreOutput(c.two)] else [])) else
    if at==16 then Outcome(Undefined("hull slot read"),c,Output(output,false),events+[SlotRead(at)]) else
    Scan(d,at+1,Add(c,Candidate(d.slots[at])),output,events+[SlotRead(at)])
  }
  function Lookup(owner: S.I16,hasBase: bool,index: nat,current: S.I16,rows: seq<Roster>,output: bool): Outcome
    requires index<16 && RostersFit(rows)
    ensures Fits(Lookup(owner,hasBase,index,current,rows,output).capability)
    ensures Lookup(owner,hasBase,index,current,rows,output).status==Done ==>
      Lookup(owner,hasBase,index,current,rows,output).output==Output(output,Lookup(owner,hasBase,index,current,rows,output).capability.two)
    ensures Lookup(owner,hasBase,index,current,rows,output).status.Undefined? ==>
      Lookup(owner,hasBase,index,current,rows,output).output==Output(output,false)
  {
    var events:=if output then [ClearOutput] else [];
    var zero:=Capability(0,false);
    if owner==-1 || !hasBase then Outcome(Done,zero,Output(output,false),events) else
    var read:=events+[RosterRead(owner)];
    if owner<0 || owner>=16 then Outcome(Undefined("roster owner"),zero,Output(output,false),read) else
    var row:=rows[owner];
    if row.Missing? then Outcome(Undefined("roster pointer"),zero,Output(output,false),read) else
    var hidden:=owner!=current && current!=-1;
    var access:=read+(if hidden then [DetailRead(index)] else [HullRead(index)]);
    if index>=10 then Outcome(Undefined("base design read"),zero,Output(output,false),access) else
    var d:=row.entries[index];
    if hidden && d.detail!=7 then Outcome(Done,zero,Output(output,false),access) else
    Scan(d,0,zero,output,access+(if hidden then [HullRead(index)] else []))
  }
  lemma Early(owner: S.I16,hasBase: bool,index: nat,current: S.I16,rows: seq<Roster>,output: bool)
    requires index<16 && RostersFit(rows) && (owner==-1 || !hasBase)
    ensures Lookup(owner,hasBase,index,current,rows,output)==Outcome(Done,Capability(0,false),Output(output,false),if output then [ClearOutput] else [])
  {}
  lemma Visible(d: Design,owner: nat,index: nat,current: S.I16,rows: seq<Roster>,output: bool)
    requires owner<16 && index<10 && RostersFit(rows) && rows[owner].Present?
    requires d==rows[owner].entries[index] && d.count<=16
    requires Shape(d)
    requires current==-1 || current==owner || d.detail==7
    ensures Lookup(owner,true,index,current,rows,output).status==Done
    ensures Lookup(owner,true,index,current,rows,output).capability==
      Capability(Max(d.slots[..d.count]),Count(d.slots[..d.count],Max(d.slots[..d.count]))>=2)
  {
    var slice:=d.slots[..d.count];
    assert Fold(slice,Capability(0,false))==Capability(Max(slice),Count(slice,Max(slice))>=2);
  }
  lemma Vectors() {
    var five:=G.HullSlot(512,7,1);var thirteen:=G.HullSlot(512,15,1);
    assert Fold([G.HullSlot(512,7,2)],Capability(0,false))==Capability(5,false);
    assert Fold([five,five],Capability(0,false))==Capability(5,true);
    assert Fold([thirteen],Capability(5,true))==Capability(13,false);
    assert Fold([five,five,thirteen],Capability(0,false))==Capability(13,false);
    assert Fold([five,thirteen],Capability(13,false))==Capability(13,true);
    assert Fold([thirteen,five,thirteen],Capability(0,false))==Capability(13,true);
    assert Candidate(G.HullSlot(1536,15,255))==0;
    assert Candidate(G.HullSlot(512,16,255))==0;
    assert Candidate(G.HullSlot(512,15,0))==0;
  }
}
