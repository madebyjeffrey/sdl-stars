include "S01-orders.dfy"
include "S02-presets.dfy"
include "S04-utf8-prefix.dfy"

// produceui.c preset commands. Preserve raw PROD payload and unused preset
// storage; S02 remains the canonical persisted codec. Native UI/allocator
// outcomes are explicit inputs, not a claim of successful publication.
module M09Presets {
  import O = S01Orders
  import P = S02Presets
  import S = S01
  import T = S04Utf8Prefix
  import C = S01Codecs
  datatype Row = Row(value: O.Prod,unused: nat)
  predicate Fits(r: Row) { O.FProd(r.value) && r.unused<32 }
  function ZeroRow(): Row { Row(O.Prod(0,0,0,0),0) }
  predicate Automatic(r: Row) { r.value.grobj==1 && r.value.iItem<7 }
  function Retained(q: seq<Row>): seq<Row>
    ensures |Retained(q)|<=|q|
    decreases |q|
  { if |q|==0 then [] else (if Automatic(q[0]) then [] else [q[0]])+Retained(q[1..]) }
  predicate Allowed(word: P.Word,primary: int) {
    (primary!=8 || word%64>2) && (primary!=3 || word%64 !in {4,5})
  }
  function Appended(words: seq<P.Word>,primary: int): seq<Row>
    ensures |Appended(words,primary)|<=|words|
    ensures forall r <- Appended(words,primary) :: Fits(r)
    decreases |words|
  {
    if |words|==0 then [] else
    (if Allowed(words[0],primary) then [Row(O.Prod(words[0]/64,words[0]%64,1,0),0)] else [])+
    Appended(words[1..],primary)
  }
  function Find(catalog: seq<Row>,value: O.Prod,i: nat,active: nat): nat
    requires i<=active<=|catalog|
    ensures i<=Find(catalog,value,i,active)<=active
    decreases active-i
  { if i==active || O.FSameItem(catalog[i].value,value) then i else Find(catalog,value,i+1,active) }
  function ReturnQuantity(row: Row,count: nat): Row
    requires Fits(row)
    ensures Fits(ReturnQuantity(row,count))
  { if row.value.cItem==1023 then row else row.(value:=row.value.(cItem:=(row.value.cItem+count)%1024)) }
  datatype Released = Released(catalog: seq<Row>,consumed: nat,fault: bool)
  function Release(q: seq<Row>,catalog: seq<Row>,active: nat): Released
    requires active<=|catalog| && (forall r <- q :: Fits(r)) && (forall r <- catalog :: Fits(r))
    ensures forall r <- Release(q,catalog,active).catalog :: Fits(r)
    ensures |Release(q,catalog,active).catalog|==|catalog|
    ensures Release(q,catalog,active).consumed<=|q|
    decreases |q|
  {
    if |q|==0 then Released(catalog,0,false) else
    var i := Find(catalog,q[0].value,0,active);
    if i==|catalog| then Released(catalog,0,true) else
    var row := catalog[i];
    var changed := catalog[i:=ReturnQuantity(row,q[0].value.cItem)];
    var rest := Release(q[1..],changed,active);
    Released(rest.catalog,rest.consumed+1,rest.fault)
  }
  function ZeroIndex(bytes: S.Bytes): nat
    ensures ZeroIndex(bytes)<=|bytes|
    decreases |bytes|
  { if |bytes|==0 || bytes[0]==0 then 0 else 1+ZeroIndex(bytes[1..]) }
  lemma ZeroPresent(bytes: S.Bytes)
    requires 0 in bytes
    ensures ZeroIndex(bytes)<|bytes|
    decreases |bytes|
  { if bytes[0]!=0 { ZeroPresent(bytes[1..]); } }
  datatype Slot = Slot(valid: bool,name: S.Bytes,noResearch: bool,count: nat,words: seq<P.Word>)
  predicate SlotFits(p: Slot) { |p.name|==13 && 0 in p.name && |p.words|==12 && p.count<=12 }
  function Stored(p: Slot): P.Production
    requires SlotFits(p)
  { P.Production(p.valid,T.StringOf(p.name[..ZeroIndex(p.name)]),p.noResearch,p.words[..p.count]) }
  lemma StoredNameBound(p: Slot)
    requires SlotFits(p)
    ensures |Stored(p).name|<=12
  { ZeroPresent(p.name); }
  function NameCopy(prior: S.Bytes,name: S.Bytes): S.Bytes
    requires |prior|==13 && |name|<=12 && 0 !in name
    ensures |NameCopy(prior,name)|==13 && 0 in NameCopy(prior,name)
  { name+[0]+prior[|name|+1..] }
  function Packed(r: Row): P.Word
    requires Fits(r) && Automatic(r)
  { r.value.iItem+64*r.value.cItem }
  function Capture(q: seq<Row>,limit: nat): seq<P.Word>
    requires forall r <- q :: Fits(r)
    ensures |Capture(q,limit)|<=limit
    decreases |q|
  {
    if |q|==0 || limit==0 then []
    else if Automatic(q[0]) then [Packed(q[0])]+Capture(q[1..],limit-1)
    else Capture(q[1..],limit)
  }
  function CopyPolicy(p: Slot,q: seq<Row>,research: bool): Slot
    requires SlotFits(p) && (forall r <- q :: Fits(r))
    ensures SlotFits(CopyPolicy(p,q,research))
  {
    var copied := Capture(q,12);
    p.(valid:=true,noResearch:=research,count:=|copied|,words:=copied+p.words[|copied|..])
  }
  datatype Edited = Edited(slot: Slot,dirty: bool,renameCalled: bool)
  function Edit(p: Slot,index: nat,copyQueue: bool,renameResult: int,name: S.Bytes,fallback: S.Bytes,q: seq<Row>,research: bool): Edited
    requires SlotFits(p) && index<4 && (forall r <- q :: Fits(r))
    requires |name|<=12 && 0 !in name && 1<=|fallback|<=12 && 0 !in fallback
    ensures SlotFits(Edit(p,index,copyQueue,renameResult,name,fallback,q,research).slot)
  {
    var renameCalled := index!=0;
    if renameCalled && renameResult==0 then Edited(p,true,true) else
    var renamed := if index==0 then p else p.(name:=NameCopy(p.name,if |name|==0 then fallback else name));
    Edited(if copyQueue then CopyPolicy(renamed,q,research) else renamed,true,renameCalled)
  }
  function Delete(p: Slot): Edited { Edited(p.(valid:=false),true,false) }
  predicate CanRenameDelete(p: Slot,index: nat) { p.valid && index>0 }
  // ProductionDlg and ClickInShipOrders store the modal result in int16_t.
  function OuterDialogResult(raw: int): int { C.Signed(raw,16) }
  function FinishCustom(snapshot: seq<Slot>,current: seq<Slot>,dialogResult: int): seq<Slot>
    requires |snapshot|==4 && |current|==5
    ensures |FinishCustom(snapshot,current,dialogResult)|==5
  { if OuterDialogResult(dialogResult)!=0 then current else snapshot+current[4..] }
  function ValidIndices(slots: seq<Slot>,i: nat): seq<nat>
    requires |slots|==5 && i<=4
    decreases 4-i
  { if i==4 then [] else (if slots[i].valid then [i] else [])+ValidIndices(slots,i+1) }
  datatype Popup = CancelPopup | Customize | ApplySlot(index: nat)
  function Choose(slots: seq<Slot>,result: int): Popup
    requires |slots|==5
  {
    var offered := ValidIndices(slots,0);
    if result==|offered|+1 then Customize else if result<0 then CancelPopup
    else if result<|offered| then ApplySlot(offered[result]) else ApplySlot(4)
  }
  datatype Applied = CatalogFault(catalog: seq<Row>,processed: nat)
    | AllocationEscape(catalog: seq<Row>)
    | Installed(catalog: seq<Row>,buffer: seq<Row>,countHeader: nat,capacityHeader: nat,noResearch: bool)
  function Assemble(q: seq<Row>,words: seq<P.Word>,primary: int): seq<Row>
    ensures |Assemble(q,words,primary)|<=|q|+|words|
  { Retained(q)+Appended(words,primary) }
  function Apply(q: seq<Row>,catalog: seq<Row>,active: nat,p: Slot,primary: int,allocationOK: bool): Applied
    requires |q|<=255 && |catalog|==64 && active<=64 && SlotFits(p)
    requires (forall r <- q :: Fits(r)) && (forall r <- catalog :: Fits(r))
  {
    var released := Release(q,catalog,active);
    if released.fault then CatalogFault(released.catalog,released.consumed)
    else if !allocationOK then AllocationEscape(released.catalog)
    else
      var capacity := if |q|+p.count<1 then 1 else |q|+p.count;
      var rows := Assemble(q,p.words[..p.count],primary);
      Installed(released.catalog,rows+seq(capacity-|rows|,i=>ZeroRow()),|rows|%256,capacity%256,p.noResearch)
  }
  // Cancel customization restores the copied slots only. Dirty state is not
  // part of the snapshot; neither this nor the outer queue Cancel rolls it back.
  datatype DisplayRow = Placeholder | ItemLabel(item: nat,quantity: nat,showQuantity: bool)
  function Labels(words: seq<P.Word>): seq<DisplayRow>
    ensures |Labels(words)|==|words|
    decreases |words|
  { if |words|==0 then [] else [ItemLabel(words[0]%64,words[0]/64,words[0]/64!=1 && words[0]%64!=3)]+Labels(words[1..]) }
  function Display(p: Slot): seq<DisplayRow>
    requires SlotFits(p)
  { if !p.valid || p.count==0 then [Placeholder] else Labels(p.words[..p.count]) }
  function EscapeLabel(s: S.Bytes): S.Bytes
    decreases |s|
  { if |s|==0 then [] else (if s[0]==38 then [38,38] else [s[0]])+EscapeLabel(s[1..]) }
  // Clear shares the release pass, but keeps the same allocation and research.
  datatype Cleared = ClearFault(catalog: seq<Row>,processed: nat)
    | Cleared(catalog: seq<Row>,buffer: seq<Row>,countHeader: nat,noResearch: bool)
  function Clear(q: seq<Row>,catalog: seq<Row>,active: nat,research: bool): Cleared
    requires active<=|catalog| && (forall r <- q :: Fits(r)) && (forall r <- catalog :: Fits(r))
  {
    var released := Release(q,catalog,active);
    if released.fault then ClearFault(released.catalog,released.consumed)
    else Cleared(released.catalog,q,0,research)
  }
  function DirtyAfterCustom(current: bool,dialogResult: int): bool { current }
  datatype Recovery = Recovery(freedWorking: bool,endedDialog: bool,workingNull: bool,dialogNull: bool,dialogUp: bool,research: bool,returned: bool)
  function AllocationRecovery(working: bool,dialog: bool,research: bool): Recovery {
    Recovery(working,dialog,true,true,false,research,false)
  }
  lemma RetainAll(q: seq<Row>)
    requires forall r <- q :: !Automatic(r)
    ensures Retained(q)==q
    decreases |q|
  { if |q|>0 { RetainAll(q[1..]); assert q==[q[0]]+q[1..]; } }
  lemma SnapshotKeepsFifth(snapshot: seq<Slot>,current: seq<Slot>)
    requires |snapshot|==4 && |current|==5
    ensures FinishCustom(snapshot,current,0)[..4]==snapshot
    ensures FinishCustom(snapshot,current,0)[4]==current[4]
  {}
  lemma CopyKeepsInactive(p: Slot,q: seq<Row>,research: bool)
    requires SlotFits(p) && (forall r <- q :: Fits(r))
    ensures CopyPolicy(p,q,research).words[|Capture(q,12)|..]==p.words[|Capture(q,12)|..]
    ensures CopyPolicy(p,q,research).name==p.name
  {}
  lemma {:isolate_assertions} {:fuel Release,4} AvailabilityVectors() {
    var a := Row(O.Prod(1020,7,1,0),0);
    var b := Row(O.Prod(3,7,1,0),0);
    assert Release([b],[a],1).catalog[0].value.cItem==1023;
    var unlimited := ReturnQuantity(a,3);
    assert unlimited.value.cItem==1023;
    assert Release([b],[unlimited],1).catalog==[unlimited];
    assert Release([b,b],[a],1).catalog[0].value.cItem==1023;
    var c := b.(value:=b.value.(cItem:=4));
    assert Release([c],[a],1).catalog[0].value.cItem==0;
    assert Release([b],[a],0).catalog[0].value.cItem==1023;
    assert Release([b],[],0).fault;
  }
  lemma {:isolate_assertions} PresetVectors() {
    var manual := Row(O.Prod(1,7,1,35),17);
    var automatic := Row(O.Prod(10,0,1,50),3);
    assert Retained([automatic,manual])==[manual];
    assert !Allowed(64,8) && Allowed(67,8) && !Allowed(68,3) && Allowed(66,3);
    assert Capture([automatic],12)==[640];
    assert Capture([manual,automatic],12)==[640];
    var p := Slot(false,seq(13,i=>0),false,0,seq(12,i=>511));
    assert p.name[0]==0;
    assert SlotFits(p);
    assert CopyPolicy(p,[manual,automatic],true).words[..2]==[640,511];
    assert CopyPolicy(p,[],true).count==0 && CopyPolicy(p,[],true).valid;
    assert Edit(p,1,true,0,[65],[67],[automatic],true)==Edited(p,true,true);
    assert Edit(p,0,true,0,[65],[67],[automatic],true).slot.name==p.name;
    assert Edit(p,1,false,1,[],[67],[],false).slot.name[..2]==[67,0];
    assert !Edit(p,1,false,1,[65],[67],[],false).slot.valid;
    var all := seq(5,i=>p);
    assert Choose(all,0)==ApplySlot(4) && Choose(all,1)==Customize;
    assert Choose(all,-1)==CancelPopup;
    var q := seq(255,i=>manual);
    RetainAll(q);
    assert |Assemble(q,[64],0)|==256;
    assert (|Assemble(q,[64],0)|%256)==0;
    assert Clear([],[],0,true)==Cleared([],[],0,true);
    assert Display(p)==[Placeholder];
    assert Labels([3,64,1])==[ItemLabel(3,0,false),ItemLabel(0,1,false),ItemLabel(1,0,true)];
    assert EscapeLabel([65,38])==[65,38,38];
    assert C.Pow2(4)==16;
    assert C.Pow2(8)==256;
    assert C.Pow2(12)==4096;
    assert C.Pow2(15)==32768 && C.Pow2(16)==65536;
    assert OuterDialogResult(65536)==0 && OuterDialogResult(65537)==1;
    assert DirtyAfterCustom(true,0);
    assert AllocationRecovery(true,true,true)==Recovery(true,true,true,true,false,true,false);
  }
}
