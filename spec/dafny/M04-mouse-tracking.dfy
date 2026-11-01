include "M04-page-initialization.dfy"

// Mouse/cursor entry and complete finite returning-service tracking programs.
// A missing reply is a prefix, not native FALSE or proof of user completion.
module M04MouseTracking {
  import S = S01
  import C = S01Codecs
  import A = S04Arithmetic
  import N = M02NativeRaceState
  import P = M04PageCommands
  import I = M04PageInitialization
  import G = M01LibraryNameDialog
  import W = M04WizardController
  datatype Point = Point(x: S.I16,y: S.I16)
  function Decode(raw: int): Point { Point(I.I16(raw),I.I16(raw/65536)) }
  function Inside(r: G.Rect,p: Point): bool { r.left<=p.x<r.right && r.top<=p.y<r.bottom }
  datatype State = State(ui: I.State,panel: S.I16,spin: seq<G.Rect>,spinKnown: set<nat>)
  predicate Shape(s: State) { I.Shape(s.ui) && |s.spin|==2 && s.spinKnown<={0,1} }
  function Native(s: State): N.State { s.ui.command.native }
  function WithNative(s: State,n: N.State): State
    requires Shape(s) && N.Shape(n)
    ensures Shape(WithNative(s,n))
  { s.(ui:=s.ui.(command:=s.ui.command.(native:=n))) }
  function WithRace(s: State,r: S.Bytes): State
    requires Shape(s) && |r|==192
    ensures Shape(WithRace(s,r))
  { WithNative(s,Native(s).(race:=r)) }
  datatype HitKind = Miss | Found(index: nat) | UndefinedHit(reason: string) | UnknownHit(reason: string)
  datatype Hit = Hit(kind: HitKind,queries: seq<nat>)
  function Scan(s: State,p: Point,at: nat): Hit
    requires Shape(s)
    ensures Scan(s,p,at).kind.Found? ==> Scan(s,p,at).kind.index<17
    decreases if at<s.ui.count then s.ui.count-at else 0
  {
    if at>=s.ui.count then Hit(Miss,[]) else
    if at>=17 then Hit(UndefinedHit("rectangle index outside local 17-slot frame"),[]) else
    if at !in s.ui.initialized || !G.Fits(s.ui.rects[at]) then Hit(UnknownHit("unqualified rectangle fields"),[]) else
    if Inside(s.ui.rects[at],p) then Hit(Found(at),[at]) else
    var next:=Scan(s,p,at+1);next.(queries:=[at]+next.queries)
  }
  function HitTest(s: State,p: Point): Hit
    requires Shape(s)
    ensures HitTest(s,p).kind.Found? ==> HitTest(s,p).kind.index<17
  {
    if s.ui.command.readOnly!=0 then Hit(Miss,[]) else
    var found:=Scan(s,p,0);
    if !found.kind.Found? || s.panel!=4 then found else
    var axis:=found.kind.index/5;
    if axis>=3 then found.(kind:=UndefinedHit("habitat maximum index 3 at growth hit")) else
    if Native(s).race[22+axis]>=128 then found.(kind:=Miss) else found
  }
  function Shift(kbd: S.I16): bool { kbd%8>=4 }
  function SignedByte(n: int): int { C.Signed(n,8) }
  function Env(r: S.Bytes,offset: nat,axis: nat): int
    requires |r|==192 && offset in {16,19,22} && axis<3
  { SignedByte(r[offset+axis]) }
  function Center(r: S.Bytes,axis: nat): S.Bytes
    requires |r|==192 && axis<3
    ensures |Center(r,axis)|==192
  { r[16+axis:=C.Byte(Env(r,19,axis)+C.DivC(I.I16(Env(r,22,axis)-Env(r,19,axis)),2))] }
  function Axis(r: S.Bytes,axis: nat,lo: int,hi: int): S.Bytes
    requires |r|==192 && axis<3
    ensures |Axis(r,axis,lo,hi)|==192
  { Center(r[19+axis:=C.Byte(lo)][22+axis:=C.Byte(hi)],axis) }
  datatype Adjustment = Adjustment(width: int,shift: int,button: int,text: int)
  function Adjust(index: nat,kbd: S.I16): Adjustment
    requires index<17 && (index>=15 || index%5!=1)
  {
    var raw:=if index==15 then Adjustment(0,1,160,-1) else if index==16 then Adjustment(0,-1,161,-1) else
      if index%5==0 then Adjustment(0,-1,2,-1) else if index%5==2 then Adjustment(0,1,3,-1) else
      if index%5==3 then Adjustment(1,0,8,0) else Adjustment(-1,0,8,1);
    var factor:=if Shift(kbd) then 10 else 1;
    raw.(width:=raw.width*factor,shift:=raw.shift*factor)
  }
  datatype Pair = Pair(lo: int,hi: int)
  function Resize(lo: int,hi: int,a: Adjustment): Pair {
    var lower:=SignedByte(lo-(a.width-a.shift));var upper:=SignedByte(hi+(a.width+a.shift));
    var lower2:=if upper>100 then SignedByte(lower-(upper-100)) else lower;
    var upper2:=if upper>100 then 100 else upper;
    var upper3:=if lower2<0 then SignedByte(A.Min(100,upper2-lower2)) else upper2;
    var lower3:=if lower2<0 then 0 else lower2;
    var difference:=upper3-lower3;var delta:=if difference<20 then (20-difference)/2 else 0;
    Pair(SignedByte(lower3-delta),SignedByte(upper3+delta))
  }
  datatype Mutation = Mutation(race: S.Bytes,redraw: bool,mask: int)
  function ButtonStep(r: S.Bytes,index: nat,kbd: S.I16): Mutation
    requires |r|==192 && index<17 && (index>=15 || index%5!=1)
    ensures |ButtonStep(r,index,kbd).race|==192
  {
    var a:=Adjust(index,kbd);
    if index>=15 then
      var prior:=SignedByte(r[25]);var value:=A.Clamp(SignedByte(prior+a.shift),1,20);
      Mutation(r[25:=C.Byte(value)],value!=prior,8)
    else
      var axis:=index/5;var pair:=Resize(Env(r,19,axis),Env(r,22,axis),a);
      var changed:=pair.lo!=Env(r,19,axis) || pair.hi!=Env(r,22,axis);
      Mutation(if changed then Axis(r,axis,pair.lo,pair.hi) else r,changed,[1,2,4][axis])
  }
  datatype DragMutation = DragValue(value: Mutation) | BadDrag(reason: string)
  function DragStep(r: S.Bytes,index: nat,half: S.I16,box: G.Rect,p: Point): DragMutation
    requires |r|==192 && index<15 && index%5==1
    ensures DragStep(r,index,half,box,p).DragValue? ==> |DragStep(r,index,half,box,p).value.race|==192
  {
    var first:=if p.x<box.left then I.I16(box.left) else p.x;
    var second:=if first>box.right then I.I16(box.right) else first;
    if !G.I32(second-box.left) || !G.I32(box.right-box.left) then BadDrag("signed-32 drag subtraction") else
    var scaled:=I.I16(A.LMulDiv(second-box.left,100,box.right-box.left));
    var target:=I.I16(if half>(if scaled>=100-half then 100-half else scaled) then half else
      if scaled>=100-half then 100-half else scaled);
    var axis:=index/5;var changed:=Env(r,22,axis)!=target+half;
    DragValue(Mutation(if changed then Axis(r,axis,target-half,target+half) else r,changed,[1,2,4][axis]))
  }
  function EconomyDelta(index: nat,kbd: S.I16): int { (if index%2==0 then 1 else -1)*(if Shift(kbd) then 3 else 1) }
  function EconomyStep(r: S.Bytes,index: nat,kbd: S.I16): Mutation
    requires |r|==192 && index<14
    ensures |EconomyStep(r,index,kbd).race|==192
  {
    // rgRW3IStat is the source's fixed seven-entry identity map.
    var stat:=index/2;var prior:=P.Stat(r,stat);
    var updated:=P.SetStat(r,stat,I.I16(prior+EconomyDelta(index,kbd)));
    Mutation(updated,P.Stat(updated,stat)!=prior,stat)
  }
  datatype Kind = Portrait | HabitatButton | HabitatDrag | Economy
  predicate Valid(kind: Kind,index: nat) {
    if kind==Portrait then index<2 else if kind==HabitatDrag then index<15 && index%5==1 else
    if kind==HabitatButton then index<17 && (index>=15 || index%5!=1) else index<17
  }
  datatype Service = StartService | TickService | MoveService | DrawService | EndService | CursorService
  datatype Reply = Reply(service: Service,state: State,more: S.I16,point: Point,pointWritten: bool,escaped: bool,stop: nat)
  datatype Event = HitQueries(indices: seq<nat>) | SpinQuery(index: nat)
    | GetDC | SelectPalette | RealizePalette | Capture | GrabCursor | InitTrack(index: nat,button: int,timer: int,text: int)
    | Tick | Move | Draw(page: int,mask: int,trackDC: bool,before: State)
    | Bitmap(x: int,y: int,sourceX: int,sourceY: int) | SetPortrait(value: nat) | Recenter(axis: nat)
    | ReleaseDC | ReleaseCapture | GetCursor | ToClient | HandCursor | Return(handled: bool)
  datatype Status = Returned(handled: bool) | Pending | Escaped | UndefinedNative(reason: string)
    | Unqualified(reason: string) | InvalidEvidence
  datatype Result = Result(status: Status,state: State,used: nat,calls: seq<Event>)
  function Done(s: State,handled: bool): Result { Result(Returned(handled),s,0,[Return(handled)]) }
  function Boundary(s: State,h: Hit): Result {
    if h.kind.UndefinedHit? then Result(UndefinedNative(h.kind.reason),s,0,[HitQueries(h.queries)]) else
    if h.kind.UnknownHit? then Result(Unqualified(h.kind.reason),s,0,[HitQueries(h.queries)]) else
    Done(s,false).(calls:=[HitQueries(h.queries),Return(false)])
  }
  predicate DrawFrame(before: State,after: State) {
    Shape(after) && P.DrawFrame(Native(before),Native(after)) &&
    after.ui.command==before.ui.command.(native:=Native(after)) && after.panel==before.panel &&
    after.spin==before.spin && after.spinKnown==before.spinKnown
  }
  function End(s: State,kind: Kind,index: nat,cur: nat,replies: seq<Reply>): Result
    requires Shape(s) && Valid(kind,index) && cur<32
    ensures Shape(End(s,kind,index,cur,replies).state)
    ensures End(s,kind,index,cur,replies).used<=|replies|
  {
    var centered:=if kind==HabitatButton && index<15 then WithRace(s,Center(Native(s).race,index/5)) else s;
    var current:=if kind==Portrait then WithRace(centered,P.SetPortrait(Native(centered).race,cur)) else centered;
    var prefix:=if kind==Portrait then [SetPortrait(cur),ReleaseDC] else if kind==HabitatDrag then [ReleaseCapture] else
      if kind==HabitatButton && index<15 then [Recenter(index/5)] else [];
    if kind !in {Portrait,HabitatDrag} then Done(current,true).(calls:=prefix+[Return(true)]) else
    if |replies|==0 then Result(Pending,current,0,prefix) else
    var r:=replies[0];
    if r.service!=EndService || !Shape(r.state) then Result(InvalidEvidence,current,1,prefix) else
    if r.escaped then Result(Escaped,r.state,1,prefix) else Result(Returned(true),r.state,1,prefix+[Return(true)])
  }
  function DrawResume(s: State,kind: Kind,index: nat,kbd: S.I16,half: S.I16,cur: nat,mask: int,replies: seq<Reply>): Result
    requires Shape(s) && Valid(kind,index) && cur<32 && kind!=Portrait
    ensures Shape(DrawResume(s,kind,index,kbd,half,cur,mask,replies).state)
    ensures DrawResume(s,kind,index,kbd,half,cur,mask,replies).used<=|replies|
    decreases |replies|
  {
    var prefix:=[Draw(if kind==Economy then 5 else 4,mask,kind!=HabitatDrag,s)];
    if |replies|==0 then Result(Pending,s,0,prefix) else
    var r:=replies[0];
    if r.service!=DrawService || !Shape(r.state) then Result(InvalidEvidence,s,1,prefix) else
    if r.escaped then Result(Escaped,r.state,1,prefix) else
    if !DrawFrame(s,r.state) then Result(InvalidEvidence,r.state,1,prefix) else
    var next:=Run(r.state,kind,index,kbd,half,cur,replies[1..]);
    next.(used:=next.used+1,calls:=prefix+next.calls)
  }
  function Run(s: State,kind: Kind,index: nat,kbd: S.I16,half: S.I16,cur: nat,replies: seq<Reply>): Result
    requires Shape(s) && Valid(kind,index) && cur<32
    ensures Shape(Run(s,kind,index,kbd,half,cur,replies).state)
    ensures Run(s,kind,index,kbd,half,cur,replies).used<=|replies|
    decreases |replies|
  {
    var prefix:=[if kind==HabitatDrag then Move else Tick];
    if |replies|==0 then Result(Pending,s,0,prefix) else
    var r:=replies[0];var expected:=if kind==HabitatDrag then MoveService else TickService;
    if r.service!=expected || !Shape(r.state) then Result(InvalidEvidence,s,1,prefix) else
    if r.escaped then Result(Escaped,r.state,1,prefix) else
    var current:=r.state;
    if r.more==0 then
      var done:=End(current,kind,index,cur,replies[1..]);done.(used:=done.used+1,calls:=prefix+done.calls)
    else if kind==Portrait then
      if 0 !in current.spinKnown || !G.Fits(current.spin[0]) then Result(Unqualified("portrait draw rectangle"),current,1,prefix) else
      var box:=current.spin[0];
      if !G.I32(box.left-2) || !G.I32(box.top-35) then Result(UndefinedNative("portrait destination subtraction"),current,1,prefix) else
      var image:=(cur+32+(if index==0 then -1 else 1))%32;
      var next:=Run(current,kind,index,kbd,half,image,replies[1..]);
      next.(used:=next.used+1,calls:=prefix+[Bitmap(I.I16(box.left-2),I.I16(box.top-35),image%8*32,(3-image/8)*32)]+next.calls)
    else if kind==Economy && index>=14 then Result(UndefinedNative("seven-entry economy stat map"),current,1,prefix) else
    if kind==HabitatDrag && (!r.pointWritten || index !in current.ui.initialized || !G.Fits(current.ui.rects[index])) then
      Result(Unqualified("mouse point or drag rectangle after service"),current,1,prefix) else
    var drag:=if kind==HabitatDrag then DragStep(Native(current).race,index,half,current.ui.rects[index],r.point) else
      DragValue(if kind==Economy then EconomyStep(Native(current).race,index,kbd) else ButtonStep(Native(current).race,index,kbd));
    if drag.BadDrag? then Result(UndefinedNative(drag.reason),current,1,prefix) else
    var mutation:=drag.value;var changed:=WithRace(current,mutation.race);
    var next:=if mutation.redraw then DrawResume(changed,kind,index,kbd,half,cur,mutation.mask,replies[1..]) else
      Run(changed,kind,index,kbd,half,cur,replies[1..]);
    next.(used:=next.used+1,calls:=prefix+next.calls)
  }
  function Start(s: State,kind: Kind,index: nat,kbd: S.I16,cur: nat,replies: seq<Reply>): Result
    requires Shape(s) && Valid(kind,index) && cur<32
    ensures Shape(Start(s,kind,index,kbd,cur,replies).state)
    ensures Start(s,kind,index,kbd,cur,replies).used<=|replies|
  {
    var adjustment:=if kind==HabitatButton then Adjust(index,kbd) else Adjustment(0,0,0,-1);
    var prefix:=if kind==Portrait then [GetDC,SelectPalette,RealizePalette,InitTrack(index,34+index,80,-1)] else
      if kind==HabitatDrag then [Capture,GrabCursor] else
      if kind==Economy then [InitTrack(index,if index%2==0 then 160 else 161,80,-1)] else
        [InitTrack(index,adjustment.button,80,adjustment.text)];
    if |replies|==0 then Result(Pending,s,0,prefix) else
    var r:=replies[0];
    if r.service!=StartService || !Shape(r.state) then Result(InvalidEvidence,s,1,prefix) else
    if r.escaped then
      if 0<r.stop<=|prefix| then Result(Escaped,r.state,1,prefix[..r.stop]) else Result(InvalidEvidence,r.state,1,[]) else
    var half:=if kind==HabitatDrag then I.I16(C.DivC(I.I16(Env(Native(r.state).race,22,index/5)-Env(Native(r.state).race,19,index/5)),2)) else 0;
    var next:=Run(r.state,kind,index,kbd,half,cur,replies[1..]);
    next.(used:=next.used+1,calls:=prefix+next.calls)
  }
  function Track(s: State,economy: bool,kbd: S.I16,p: Point,replies: seq<Reply>): Result
    requires Shape(s)
    ensures Shape(Track(s,economy,kbd,p,replies).state)
    ensures Track(s,economy,kbd,p,replies).used<=|replies|
  {
    var h:=HitTest(s,p);
    if !h.kind.Found? then Boundary(s,h) else
    var index:=h.kind.index;
    var kind:=if economy then Economy else if index<15 && index%5==1 then HabitatDrag else HabitatButton;
    var result:=Start(s,kind,index,kbd,0,replies);
    result.(calls:=[HitQueries(h.queries)]+result.calls)
  }
  function PortraitMouse(s: State,p: Point,replies: seq<Reply>): Result
    requires Shape(s)
    ensures Shape(PortraitMouse(s,p,replies).state)
    ensures PortraitMouse(s,p,replies).used<=|replies|
  {
    if s.ui.command.readOnly!=0 then Done(s,false) else
    if 0 !in s.spinKnown || !G.Fits(s.spin[0]) then Result(Unqualified("first portrait hit rectangle"),s,0,[]) else
    var left:=Inside(s.spin[0],p);
    if !left && (1 !in s.spinKnown || !G.Fits(s.spin[1])) then Result(Unqualified("second portrait hit rectangle"),s,0,[SpinQuery(0)]) else
    var right:=!left && Inside(s.spin[1],p);
    var prefix:=if left then [SpinQuery(0)] else [SpinQuery(0),SpinQuery(1)];
    if !left && !right then Done(s,false).(calls:=prefix+[Return(false)]) else
    var result:=Start(s,Portrait,if left then 0 else 1,0,P.Portrait(Native(s).race),replies);
    result.(calls:=prefix+[SpinQuery(0)]+result.calls)
  }
  function Mouse(s: State,page: W.Page,keyboard: int,position: int,replies: seq<Reply>): Result
    requires Shape(s)
    ensures Shape(Mouse(s,page,keyboard,position,replies).state)
    ensures Mouse(s,page,keyboard,position,replies).used<=|replies|
  {
    var p:=Decode(position);
    if page==1 then PortraitMouse(s,p,replies) else if page in {4,5} then Track(s,page==5,I.I16(keyboard),p,replies) else Done(s,false)
  }
  function Cursor(s: State,page: W.Page,replies: seq<Reply>): Result
    requires Shape(s)
    ensures Shape(Cursor(s,page,replies).state)
    ensures Cursor(s,page,replies).used<=|replies|
  {
    if page !in {4,5} then Done(s,false) else
    var prefix:=[GetCursor,ToClient];
    if |replies|==0 then Result(Pending,s,0,prefix) else
    var r:=replies[0];
    if r.service!=CursorService || !Shape(r.state) then Result(InvalidEvidence,s,1,prefix) else
    if r.escaped then
      if 0<r.stop<=|prefix| then Result(Escaped,r.state,1,prefix[..r.stop]) else Result(InvalidEvidence,r.state,1,[]) else
    if !r.pointWritten then Result(Unqualified("cursor point after position/conversion calls"),r.state,1,prefix) else
    var h:=HitTest(r.state,r.point);
    var result:=if h.kind.Found? then Done(r.state,true).(calls:=[HitQueries(h.queries),HandCursor,Return(true)]) else Boundary(r.state,h);
    result.(used:=1,calls:=prefix+result.calls)
  }
  function ColorIds(page: W.Page): seq<nat> {
    if page==1 then I.Range(271,8) else if page==2 then I.Range(271,10) else
    if page==3 then I.Range(291,14) else if page==4 then I.Range(291,3) else
    if page==5 then [291] else I.Range(271,18)+[291]
  }
  function FirstHandle(target: nat,handles: seq<nat>): nat
    ensures FirstHandle(target,handles)<=|handles|
    decreases |handles|
  { if |handles|==0 || handles[0]==target then 0 else 1+FirstHandle(target,handles[1..]) }
  datatype AppearanceCode = Zero | One | FaceBrush(handle: nat)
  datatype AppearanceEvent = GetChild(id: nat) | Background(color: S.U32) | GetClient | FillFace
  datatype Appearance = Appearance(code: AppearanceCode,calls: seq<AppearanceEvent>)
  function Color(page: W.Page,staticMessage: bool,target: nat,handles: seq<nat>,faceColor: S.U32,brush: nat): Appearance
    requires |handles|==|ColorIds(page)|
    ensures |Color(page,staticMessage,target,handles,faceColor,brush).calls|<=20
  {
    var ids:=ColorIds(page);var first:=FirstHandle(target,handles);
    var count:=if first<|handles| then first+1 else |handles|;
    var queries:=seq(count,i requires 0<=i<count => GetChild(ids[i]));
    if first<|handles| || staticMessage then Appearance(FaceBrush(brush),queries+[Background(faceColor)]) else Appearance(Zero,queries)
  }
  function Erase(): Appearance { Appearance(One,[GetClient,FillFace]) }
  lemma StaticColorStillQueries(page: W.Page,target: nat,handles: seq<nat>,color: S.U32,brush: nat)
    requires |handles|==|ColorIds(page)|
    ensures Color(page,true,target,handles,color,brush).code==FaceBrush(brush)
    ensures |Color(page,true,target,handles,color,brush).calls|>=2
  {}
  lemma ReadOnlyHitReadsNothing(s: State,p: Point)
    requires Shape(s) && s.ui.command.readOnly!=0
    ensures HitTest(s,p)==Hit(Miss,[])
  {}
  lemma NoTickRecomputesCenter(s: State,index: nat,cur: nat)
    requires Shape(s) && index<15 && index%5!=1 && cur<32
    ensures End(s,HabitatButton,index,cur,[]).state==WithRace(s,Center(Native(s).race,index/5))
    ensures End(s,HabitatButton,index,cur,[]).status==Returned(true)
  {}
  lemma GrowthStepBounds(r: S.Bytes,index: nat,kbd: S.I16)
    requires |r|==192 && index in {15,16}
    ensures 1<=ButtonStep(r,index,kbd).race[25]<=20
  {}
  lemma PortraitSetBeforeCleanup(s: State,index: nat,cur: nat)
    requires Shape(s) && index<2 && cur<32
    ensures End(s,Portrait,index,cur,[]).status==Pending
    ensures P.Portrait(Native(End(s,Portrait,index,cur,[]).state).race)==cur
  {}
  lemma {:fuel C.Pow2,18} TrackingVectors() {
    assert Adjust(3,4)==Adjustment(10,0,8,0);
    assert Adjust(16,4)==Adjustment(0,-10,161,-1);
    assert EconomyDelta(0,4)==3 && EconomyDelta(1,4)==-3;
    assert Resize(0,19,Adjust(4,0))==Pair(0,19);
    assert Decode(0xffff8000)==Point(-32768,-1);
    var r:=C.Zeros(192)[25:=127];
    assert ButtonStep(r,15,4).race[25]==1;
  }
}
