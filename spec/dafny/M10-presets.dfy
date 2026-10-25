include "M09-presets.dfy"
include "S02-host-profile.dfy"
include "S06-shared-state.dfy"

// shipui.c / ship2ui.c transport commands. Packed actions remain S02-owned;
// typed task meaning remains S06-owned. UI effects below assume live controls
// and the stated synchronous notification adapter, not arbitrary callbacks.
module M10Presets {
  import P = S02Presets
  import N = M09Presets // Existing shared 13-byte preset name/display operations.
  import S = S01
  import T = S04Utf8Prefix
  import C = S01Codecs
  import A = S04Arithmetic
  import H = S02HostProfile
  import G = SharedGameState
  predicate Payload(rows: seq<P.Action>) { |rows|==5 && forall i | 0<=i<|rows| :: P.ValidAction(rows[i]) }
  datatype Slot = Slot(valid: bool,name: S.Bytes,rows: seq<P.Action>)
  predicate Fits(p: Slot) { |p.name|==13 && 0 in p.name && Payload(p.rows) }
  function Stored(p: Slot): P.Transport
    requires Fits(p)
  { P.Transport(p.valid,T.StringOf(p.name[..N.ZeroIndex(p.name)]),p.rows) }
  function Builtin(rows: seq<P.Action>,command: int): seq<P.Action>
    requires Payload(rows)
    ensures Payload(Builtin(rows,command))
  {
    seq(5,i requires 0<=i<5 =>
      if command==3 then P.Action(0,0)
      else if command in {0,1,2} then
        P.Action(if i==4 then 7 else if command==1 then 2 else if i==3 then 0 else if command==0 then 1 else 6,
                 if command==2 && i<3 then 100 else rows[i].quantity)
      else rows[i])
  }
  function ValidIndices(slots: seq<Slot>,i: nat): seq<nat>
    requires |slots|==4 && i<=4
    ensures |ValidIndices(slots,i)|<=4-i
    ensures forall j <- ValidIndices(slots,i) :: i<=j<4 && slots[j].valid
    decreases 4-i
  { if i==4 then [] else (if slots[i].valid then [i] else [])+ValidIndices(slots,i+1) }
  function MenuCount(slots: seq<Slot>): nat
    requires |slots|==4
    ensures 6<=MenuCount(slots)<=11
  { var n:=|ValidIndices(slots,0)|; 6+n+(if n==0 then 0 else 1) }
  datatype Choice = Cancelled | Customize | BuiltinChoice(command: int)
    | CustomChoice(index: nat) | OutsideSlots
  function Choose(slots: seq<Slot>,result: int): Choice
    requires |slots|==4
    ensures Choose(slots,result).CustomChoice? ==> Choose(slots,result).index<4
  {
    var offered:=ValidIndices(slots,0);
    if result==MenuCount(slots)-1 then Customize
    else if result>4 then
      if result-5<|offered| then CustomChoice(offered[result-5]) else OutsideSlots
    else if result==-1 then Cancelled else BuiltinChoice(result)
  }
  datatype Dispatch = NoWrite(choice: Choice) | UndefinedSlotRead
    | BeforeRefresh(rows: seq<P.Action>,task: nat)
  function Apply(slots: seq<Slot>,rows: seq<P.Action>,task: nat,result: int): Dispatch
    requires |slots|==4 && (forall i | 0<=i<4 :: Fits(slots[i])) && Payload(rows) && task<16
    ensures Apply(slots,rows,task,result).BeforeRefresh? ==>
      Payload(Apply(slots,rows,task,result).rows) && Apply(slots,rows,task,result).task==task
  {
    var choice:=Choose(slots,result);
    match choice
    case Cancelled => NoWrite(choice)
    case Customize => NoWrite(choice)
    case OutsideSlots => UndefinedSlotRead
    case CustomChoice(i) => (assert Fits(slots[i]); BeforeRefresh(slots[i].rows,task))
    case BuiltinChoice(i) => BeforeRefresh(Builtin(rows,i),task)
  }
  // The direct command writes the union only; it neither changes grTask nor
  // checks that the current waypoint is transport. Normal UI eligibility is
  // an adapter obligation; snapshots copy all 96 bytes, excluding dirty state.
  datatype Edited = Edited(slot: Slot,dirty: bool)
  function Edit(p: Slot,copyOrder: bool,renameResult: int,name: S.Bytes,fallback: S.Bytes,current: seq<P.Action>): Edited
    requires Fits(p) && Payload(current)
    requires |name|<=12 && 0 !in name && 1<=|fallback|<=12 && 0 !in fallback
    ensures Fits(Edit(p,copyOrder,renameResult,name,fallback,current).slot)
  {
    if renameResult==0 then Edited(p,true) else
    var renamed:=p.(name:=N.NameCopy(p.name,if |name|==0 then fallback else name));
    Edited(if copyOrder then renamed.(valid:=true,rows:=current) else renamed,true)
  }
  function Delete(p: Slot): Edited { Edited(p.(valid:=false),true) }
  predicate CanRenameDelete(p: Slot) { p.valid }
  function FinishCustom(snapshot: seq<Slot>,current: seq<Slot>,result: int): seq<Slot>
    requires |snapshot|==4 && |current|==4
  { if N.OuterDialogResult(result)==0 then snapshot else current }
  function DirtyAfterCustom(dirty: bool,result: int): bool { dirty }
  // The paint path tests the localized label's final byte, not the action enum.
  // It overwrites the last three bytes when that byte is a period.
  datatype Label = Plain(text: string) | Quantity(prefix: string,quantity: nat) | OutsideLabelBuffer
  function ActionLabel(text: string,quantity: nat): Label {
    if |text|==0 then OutsideLabelBuffer
    else if text[|text|-1]!='.' then Plain(text)
    else if |text|<3 then OutsideLabelBuffer else Quantity(text[..|text|-3],quantity)
  }
  function CargoIndex(selection: nat): nat
    requires selection<5
    ensures CargoIndex(selection)<5
  { if selection==0 then 4 else selection-1 }
  function CargoChoice(index: nat): nat
    requires index<5
    ensures CargoChoice(index)<5
  { if index==4 then 0 else index+1 }
  function FirstActive(rows: seq<P.Action>,order: seq<nat>): nat
    requires Payload(rows) && (forall j | 0<=j<|order| :: order[j]<5)
    ensures FirstActive(rows,order)<5
    decreases |order|
  { if |order|==0 then 4 else if rows[order[0]].action!=0 then order[0] else FirstActive(rows,order[1..]) }
  function SelectedCargo(rows: seq<P.Action>,hasCargo: bool): nat
    requires Payload(rows)
    ensures SelectedCargo(rows,hasCargo)<5
  { FirstActive(rows,if hasCargo then [4,0,1,2,3] else [4]) }
  function SelectedAction(raw: P.Nibble): int { if raw<10 then raw else -1 }
  function ChangeAction(rows: seq<P.Action>,selection: nat,actionResult: int): seq<P.Action>
    requires Payload(rows) && selection<5
    ensures Payload(ChangeAction(rows,selection,actionResult))
  { var i:=CargoIndex(selection); rows[i:=rows[i].(action:=actionResult%16)] }
  datatype Amount = Amount(value: nat,alert: bool)
  function NormalizeAmount(parsed: int,actionSelection: int): Amount
    ensures NormalizeAmount(parsed,actionSelection).value<=4000
    ensures actionSelection in {5,6} ==> NormalizeAmount(parsed,actionSelection).value<=100
  {
    var narrowed:=C.Signed(parsed,16);
    var value:=A.Clamp(narrowed,0,if actionSelection in {5,6} then 100 else 4000);
    Amount(value,narrowed!=value)
  }
  // ASCII/C-locale bytes from GetWindowText(...,8): at most seven characters,
  // then atoi before int16 narrowing. Other locale classifications remain open.
  function ParseAmount(text: string): int { H.Decimal(text[..(if |text|>7 then 7 else |text|)]) }
  datatype Refreshed = Refreshed(rows: seq<P.Action>,cargo: nat,actionSelection: int,alert: bool,writes: nat)
  function RefreshTransport(rows: seq<P.Action>,hasCargo: bool): Refreshed
    requires Payload(rows)
    ensures Payload(RefreshTransport(rows,hasCargo).rows)
    ensures RefreshTransport(rows,hasCargo).cargo<5
  {
    var i:=SelectedCargo(rows,hasCargo);
    var selected:=SelectedAction(rows[i].action);
    var amount:=NormalizeAmount(rows[i].quantity,selected);
    Refreshed(rows[i:=rows[i].(quantity:=amount.value)],i,selected,amount.alert,if amount.alert then 2 else 1)
  }
  datatype QuantityControl = QuantityControl(enabled: bool,unitIndex: nat)
  function Control(actionSelection: int,cargo: nat): QuantityControl
    requires cargo<5
  { QuantityControl(actionSelection !in {0,1,2,7},if actionSelection in {5,6} then 5 else cargo) }
  datatype Completed = Unchanged(choice: Choice) | UndefinedRead
    | Completed(initial: seq<P.Action>,rows: seq<P.Action>,task: nat,writes: nat,alert: bool,draws: nat)
  function Complete(slots: seq<Slot>,rows: seq<P.Action>,task: nat,result: int,taskSelection: int,hasCargo: bool): Completed
    requires |slots|==4 && (forall i | 0<=i<4 :: Fits(slots[i])) && Payload(rows) && task<16
  {
    var applied:=Apply(slots,rows,task,result);
    match applied
    case NoWrite(choice) => Unchanged(choice)
    case UndefinedSlotRead => UndefinedRead
    case BeforeRefresh(initial,kind) =>
      if taskSelection!=1 then Completed(initial,initial,kind,1,false,1) else
      var refreshed:=RefreshTransport(initial,hasCargo);
      Completed(initial,refreshed.rows,kind,1+refreshed.writes,refreshed.alert,1)
  }
  function Typed(a: P.Action): G.Action
    requires P.ValidAction(a) && a.action<10
    ensures G.ActionFits(Typed(a)) <==> (a.action !in {5,6} || a.quantity<=100)
  {
    if a.action==0 then G.NoAction else if a.action==1 then G.LoadAll
    else if a.action==2 then G.UnloadAll else if a.action==3 then G.LoadExact(a.quantity)
    else if a.action==4 then G.UnloadExact(a.quantity) else if a.action==5 then G.FillPercent(a.quantity)
    else if a.action==6 then G.WaitPercent(a.quantity) else if a.action==7 then G.Dunnage
    else if a.action==8 then G.SetAmount(a.quantity) else G.SetWaypoint(a.quantity)
  }
  lemma QuantityPreserved(rows: seq<P.Action>,command: int,i: nat)
    requires Payload(rows) && i<5 && command in {0,1,2}
    requires command!=2 || i>=3
    ensures Builtin(rows,command)[i].quantity==rows[i].quantity
  {}
  lemma CargoRoundTrip(selection: nat)
    requires selection<5
    ensures CargoChoice(CargoIndex(selection))==selection
  {}
  lemma CancelRestores(snapshot: seq<Slot>,current: seq<Slot>,dirty: bool)
    requires |snapshot|==4 && |current|==4
    ensures FinishCustom(snapshot,current,0)==snapshot && DirtyAfterCustom(dirty,0)==dirty
  {}
  lemma RefreshOnlySelected(rows: seq<P.Action>,hasCargo: bool,i: nat)
    requires Payload(rows) && i<5 && i!=SelectedCargo(rows,hasCargo)
    ensures RefreshTransport(rows,hasCargo).rows[i]==rows[i]
  {}
  lemma {:isolate_assertions} {:fuel C.Pow2,18} {:fuel ValidIndices,6} {:fuel FirstActive,6} {:fuel H.DecimalDigits,10} Vectors() {
    var rows:=seq(5,i=>P.Action(15,4095));
    assert Payload(rows);
    assert Builtin(rows,0)[0]==P.Action(1,4095) && Builtin(rows,0)[4]==P.Action(7,4095);
    assert Builtin(rows,2)[0]==P.Action(6,100) && Builtin(rows,2)[3]==P.Action(0,4095);
    assert Builtin(rows,3)==seq(5,i=>P.Action(0,0));
    var p:=Slot(false,seq(13,i=>0),rows);
    assert p.name[0]==0;
    assert Fits(p);
    var slots:=seq(4,i=>p);
    assert MenuCount(slots)==6 && Choose(slots,5)==Customize && Choose(slots,4)==BuiltinChoice(4);
    assert Choose(slots,6)==OutsideSlots && Choose(slots,-1)==Cancelled && Choose(slots,-2)==BuiltinChoice(-2);
    var offered:=slots[1:=p.(valid:=true)][3:=p.(valid:=true)];
    assert Choose(offered,5)==CustomChoice(1) && Choose(offered,6)==CustomChoice(3);
    assert Choose(offered,7)==OutsideSlots && Choose(offered,8)==Customize;
    assert FinishCustom(slots,offered,65536)==slots;
    assert Edit(p,true,0,[65],[67],rows)==Edited(p,true);
    assert Edit(p,false,-1,[],[67],rows).slot.name[..2]==[67,0];
    assert !Edit(p,false,1,[65],[67],rows).slot.valid;
    assert ActionLabel("Load...",12)==Quantity("Load",12);
    assert ActionLabel("x.",12)==OutsideLabelBuffer;
    assert C.Pow2(4)==16;
    assert C.Pow2(8)==256;
    assert C.Pow2(12)==4096;
    assert C.Pow2(15)==32768 && C.Pow2(16)==65536;
    assert NormalizeAmount(65636,5)==Amount(100,false);
    assert NormalizeAmount(32768,0)==Amount(0,true);
    assert NormalizeAmount(-65535,0)==Amount(1,false);
    assert ParseAmount("40001234")==4000123;
    assert NormalizeAmount(ParseAmount("40001234"),3)==Amount(2427,false);
    assert RefreshTransport(Builtin(rows,2),true).cargo==4;
    assert RefreshTransport(Builtin(rows,2),true).rows[4]==P.Action(7,4000);
    assert RefreshTransport(Builtin(rows,2),true).rows[0]==P.Action(6,100);
    assert RefreshTransport(rows,true).actionSelection==-1;
    assert RefreshTransport(rows,true).writes==2;
    assert ChangeAction(rows,0,-1)[4].action==15;
    assert Control(7,4)==QuantityControl(false,4) && Control(5,0)==QuantityControl(true,5);
    assert Complete(slots,rows,1,2,1,true).writes==3;
    assert Complete(slots,rows,1,2,1,true).rows[4]==P.Action(7,4000);
    assert Complete(slots,rows,2,0,2,true).task==2 && Complete(slots,rows,2,0,2,true).writes==1;
    assert !G.ActionFits(Typed(P.Action(5,4095)));
  }
}
