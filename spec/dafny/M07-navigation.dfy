include "S04-arithmetic.dfy"

module M07Navigation {
  import opened SharedGameState
  import A = S04Arithmetic
  import C = S01Codecs

  type Zoom = z: int | -4 <= z <= 4 witness 0
  function PtToScan(d: int,z: Zoom): int {
    C.Signed(if z==4 then d*4 else if z==3 then d*2
      else if z==2 then d*3/2 else if z==1 then d*5/4
      else if z==0 then d else if z == -1 then d*3/4
      else if z == -2 then d/2 else if z == -3 then d*3/8 else d/4,16)
  }
  // Right shift floors negatives; fractional inverse conversions narrow BEFORE
  // signed division, as in ScanToPt. These are not exact inverse functions.
  function ScanToPt(d: int,z: Zoom): int {
    C.Signed(if z==4 then d/4 else if z==3 then d/2
      else if z==2 then C.DivC(C.Signed(d*2,16),3)
      else if z==1 then C.DivC(C.Signed(d*4,16),5)
      else if z==0 then d
      else if z == -1 then C.DivC(C.Signed(d*4,16),3)
      else if z == -2 then d*2
      else if z == -3 then C.DivC(C.Signed(d*8,16),3) else d*4,16)
  }
  datatype Viewport = Viewport(z: Zoom,xTop: int,yTop: int,dGal: int,dInv: int,
    cx: nat,cy: nat)
  function LogicalToScan(p: Point,v: Viewport): Point {
    Point(PtToScan(C.Signed(p.x-v.xTop,16),v.z),
      PtToScan(C.Signed(v.dInv-p.y-v.yTop,16),v.z))
  }
  function ScanToLogical(p: Point,v: Viewport): Point {
    Point(A.Min(C.Signed(ScanToPt(p.x,v.z)+v.xTop,16),v.dGal+1000),
      A.Max(C.Signed(v.dInv-ScanToPt(p.y,v.z)-v.yTop,16),1000))
  }
  predicate FOnScreen(p: Point,v: Viewport) { 0<=p.x<v.cx && 0<=p.y<v.cy }
  function AlignDown4(d: int): int { 4*(d/4) }
  function AlignNear4(d: int): int { AlignDown4(d+2) }
  // Normal viewport domain: spans and intermediate bounds fit signed 16 bits.
  // External scrollbar page/range calls are S05, not proof of native clipping.
  function LastTop(dInv: int,cSpan: nat): int {
    AlignDown4(A.Max(1000,dInv-1000-cSpan)+3)
  }
  function PageStep(cx: nat,cy: nat): int { AlignDown4(A.Min(cx,cy)/3) }
  function LineStep(cx: nat,cy: nat): int { AlignNear4(PageStep(cx,cy)/8) }
  function CenterTop(p: Point,dInv: int,cx: nat,cy: nat): Point {
    var x := A.Max(1000,A.Min(A.Max(1000,p.x-cx/2),dInv-1000-cx));
    var y := A.Max(1000,dInv-A.Max(cy+1000,A.Min(cy/2+p.y,dInv-1000)));
    Point(AlignNear4(x),AlignNear4(y))
  }
  predicate FInsideReveal(p: Point,v: Viewport,cx: int,cy: int) {
    v.xTop+10<=p.x<v.xTop+cx-20 &&
      v.dInv-v.yTop+10-cy<=p.y<v.dInv-v.yTop-10
  }
  // Reproduce strict comparisons in FEnsurePointOnScreen; its false return
  // means a centering attempt, not a failed load or a guarantee of visibility.
  function RevealCenter(p: Point,v: Viewport,cx: nat,cy: nat): Point {
    var left := v.xTop+10; var right := v.xTop+cx-20;
    var top := v.dInv-v.yTop+10-cy; var bottom := v.dInv-v.yTop-10;
    Point(v.xTop+cx/2+(if p.x<left then p.x-left else if p.x>right then p.x-right else 0),
      v.dInv-v.yTop-cy/2+(if p.y<top then p.y-top else if p.y>bottom then p.y-bottom else 0))
  }
  // Wheel/menu recentering, after the menu successfully installs the new zoom.
  function ZoomCenter(p: Point,v: Viewport,zNew: Zoom): Point {
    var cursor := Point(p.x,if p.y>=v.cy then v.cy-1 else p.y);
    var anchor := ScanToLogical(cursor,v);
    Point(C.Signed(anchor.x+ScanToPt(v.cx,zNew)/2-ScanToPt(cursor.x,zNew),16),
      C.Signed(anchor.y+ScanToPt(cursor.y,zNew)-ScanToPt(v.cy,zNew)/2,16))
  }
  function MenuCenter(selected: Option<Point>,v: Viewport,cyClient: nat): Point {
    if selected.Some? then selected.value else
      Point(C.Signed(v.xTop+ScanToPt(v.cx,v.z)/2,16),
        C.Signed(v.dInv-v.yTop-ScanToPt(cyClient,v.z)/2,16))
  }
  function WheelLines(cPage: int,cConfigured: nat,fPage: bool,fRead: bool): int {
    var page := A.Max(1,cPage);
    var lines := if fRead then cConfigured else 3;
    if (fRead && fPage) || lines>page then page else lines
  }
  function WheelTotal(remainder: int,delta: int): int {
    (if remainder*delta<0 then 0 else remainder)+delta
  }
  function WheelNotches(remainder: int,delta: int): int { C.DivC(WheelTotal(remainder,delta),120) }
  function WheelRemainder(remainder: int,delta: int): int {
    WheelTotal(remainder,delta)-120*WheelNotches(remainder,delta)
  }
  function WheelZoom(z: Zoom,c: int): Zoom { A.Clamp(z+c,-4,4) }
  predicate FPanStarts(dx: int,dy: int,cxDrag: nat,cyDrag: nat) {
    A.Abs(dx)>=cxDrag || A.Abs(dy)>=cyDrag
  }

  datatype Kind = Nothing | PlanetKind | FleetKind | ThingKind | WaypointKind
  datatype Candidate = Candidate(kind: Kind,index: int,pt: Point,fOurs: bool)
  datatype Hit = Hit(kind: Kind,pt: Point,d2: int,pl: int,fl: int,th: int,wp: int,fFleetOurs: bool)
  function EmptyHit(radius: int): Hit { Hit(Nothing,Point(0,0),radius,-1,-1,-1,-1,false) }
  function Distance2(a: Point,b: Point): int { (a.x-b.x)*(a.x-b.x)+(a.y-b.y)*(a.y-b.y) }
  function Radius2(z: Zoom,fRadius: bool,fExact: bool): int {
    if fRadius then ScanToPt(20,z)*ScanToPt(20,z) else if fExact then 0 else 1000000000
  }
  // Candidate order: planets ascending, fleets ascending to first null, things
  // ascending, selected-main-fleet waypoints descending. Distances here assume
  // signed coordinate differences and the squared sum fit the C intermediates.
  function ReplaceHit(h: Hit,c: Candidate,d: int): Hit {
    Hit(c.kind,c.pt,d,if c.kind==PlanetKind then c.index else -1,
      if c.kind==FleetKind then c.index else -1,if c.kind==ThingKind then c.index else -1,
      if c.kind==WaypointKind then c.index else -1,c.kind==FleetKind && c.fOurs)
  }
  function HitStep(h: Hit,c: Candidate,p: Point,iwpScan: int): Hit {
    var d := Distance2(p,c.pt);
    if c.kind==Nothing || d>h.d2 then h
    else if c.kind==PlanetKind then
      if d<h.d2 || d==0 then ReplaceHit(h,c,d) else h
    else
      var fHigher := if c.kind==FleetKind then h.kind==PlanetKind
        else if c.kind==ThingKind then h.kind in {PlanetKind,FleetKind}
        else h.kind in {PlanetKind,FleetKind,ThingKind};
      if (d==h.d2 && !fHigher) || c.pt==h.pt then
        if c.kind==FleetKind && h.fl != -1 && (h.fFleetOurs || !c.fOurs) then h
        else if c.kind==ThingKind && h.th != -1 then h
        else if c.kind==WaypointKind && h.wp != -1 && c.index!=iwpScan then h
        else if h.kind==Nothing then ReplaceHit(h,c,h.d2)
        else if c.kind==FleetKind then h.(fl:=c.index,fFleetOurs:=c.fOurs)
        else if c.kind==ThingKind then h.(th:=c.index)
        else h.(wp:=c.index)
      else if d<h.d2 then ReplaceHit(h,c,d) else h
  }
  function Hits(cs: seq<Candidate>,p: Point,iwp: int,h: Hit): Hit
    decreases |cs|
  { if |cs|==0 then h else Hits(cs[1..],p,iwp,HitStep(h,cs[0],p,iwp)) }
  function CompleteHit(h: Hit,other: Hit): Hit {
    h.(pl:=if other.pl != -1 then other.pl else h.pl,
       fl:=if other.fl != -1 then other.fl else h.fl,
       th:=if other.th != -1 then other.th else h.th,
       wp:=if other.wp != -1 then other.wp else h.wp,
       fFleetOurs:=if other.fl != -1 then other.fFleetOurs else h.fFleetOurs)
  }
  function SelectedKind(h: Hit,mode: int): Kind { if h.pl != -1 && mode!=2 then PlanetKind else h.kind }
  function ActiveWaypoint(prior: int,h: Hit): int { if h.wp != -1 && h.wp!=prior then h.wp else prior }
  predicate FCanPromote(kind: Kind,fOurs: bool) { kind in {PlanetKind,FleetKind} && fOurs }

  // Exact/prefix comparisons use current byte-oriented CRT results, not an
  // invented Unicode case fold. Inputs are candidate keys in source order.
  function Search(exactPlanets: seq<ObjectRef>,numericFleet: Option<ObjectRef>,
    exactFleets: seq<ObjectRef>,prefixPlanets: seq<ObjectRef>): Option<ObjectRef> {
    if |exactPlanets|>0 then Some(exactPlanets[0])
    else if numericFleet.Some? then numericFleet
    else if |exactFleets|>0 then Some(exactFleets[0])
    else if |prefixPlanets|>0 then Some(prefixPlanets[0]) else None
  }
  function SkipSpaces(rgb: seq<int>): seq<int>
    decreases |rgb|
  { if |rgb|>0 && rgb[0]==32 then SkipSpaces(rgb[1..]) else rgb }
  function FleetDigits(rgb: seq<int>,n: int): Option<int>
    requires 1<=n<=512
    ensures FleetDigits(rgb,n).Some? ==> 1<=FleetDigits(rgb,n).value<=512
    decreases |rgb|
  {
    if |rgb|==0 || rgb[0]<48 || rgb[0]>57 then Some(n)
    else if n*10+rgb[0]-48>512 then None
    else FleetDigits(rgb[1..],n*10+rgb[0]-48)
  }
  function ParseFleetNumber(rgb: seq<int>,fPrefixMatch: bool): Option<int>
    requires fPrefixMatch ==> |rgb|>=6
  {
    var afterPrefix := SkipSpaces(if fPrefixMatch then rgb[6..] else rgb);
    var digits := SkipSpaces(if |afterPrefix|>0 && afterPrefix[0]==35 then afterPrefix[1..] else afterPrefix);
    if |digits|==0 || digits[0]<49 || digits[0]>57 then None
    else FleetDigits(digits[1..],digits[0]-48)
  }
  function FleetNumber(iplr: PlayerId,n: int): FleetId
    requires 1<=n<=512
  { FleetId(iplr,n-1) }
  function NextAtPoint(after: seq<ObjectRef>,ownedPlanet: Option<ObjectRef>,
    wrapped: seq<ObjectRef>,current: ObjectRef,fFoundCurrent: bool): Option<ObjectRef> {
    if !fFoundCurrent then None else if |after|>0 then Some(after[0])
    else if ownedPlanet.Some? then ownedPlanet
    else if |wrapped|==0 || wrapped[0]==current then None else Some(wrapped[0])
  }
  datatype Button = Left | DoubleLeft | Right | Middle
  datatype Action = MassTarget | RouteTarget | Measure | ContextMenu | AddWaypoint | Inspect
  function Click(b: Button,fPlanet: bool,fFleet: bool,fMass: bool,fRoute: bool,
    fShift: bool,fCtrl: bool,fAlt: bool,fAdd: bool,fDriver: bool): Action {
    if b==Left && (fMass || (fPlanet && fShift && fDriver)) then MassTarget
    else if b==Left && (fRoute || (fPlanet && fCtrl)) then RouteTarget
    else if b==Middle || (b==Right && fShift) then Measure
    else if b==Right then ContextMenu
    else if fFleet && (fShift || fAlt || fAdd) then AddWaypoint else Inspect
  }
  predicate FPanArmed(b: Button,fShift: bool,fCtrl: bool,fMass: bool,fRoute: bool,
    fFleet: bool,fAdd: bool,fAlt: bool) {
    b==Left && !fShift && !fCtrl && !fMass && !fRoute && !(fFleet && (fAdd || fAlt))
  }
  function PlanetTarget(idSource: PlanetId,idTarget: PlanetId): int {
    if idSource==idTarget then 0 else idTarget+1
  }
  function DragOutcome(fMoved: bool,fAdjacent: bool,fConfirm: bool): int {
    if !fMoved || (fAdjacent && !fConfirm) then 0 else if fAdjacent then 2 else 1
  }

  predicate FIdleExcluded(fOurs: bool,cord: int,task: Task,warp: int) {
    if fOurs then cord!=1 || task.LayMines? || task.Scrap? || task.RemoteMine? || task.Patrol?
    else warp==0
  }
  function ShipSum(counts: seq<nat>,categories: seq<int>,chosen: set<int>,fFilter: bool,fCategory: bool): nat
    requires |counts|==|categories|
    decreases |counts|
  {
    if |counts|==0 then 0 else
      (if !fFilter || (if fCategory then categories[0] in chosen else 0 in chosen) then counts[0] else 0)
      + ShipSum(counts[1..],categories[1..],
          if fCategory then chosen else set i: int | i in chosen :: i-1,fFilter,fCategory)
  }
  function VisibleShips(fIdle: bool,fOurs: bool,cord: int,task: Task,warp: int,
    fDesign: bool,fEnemy: bool,designs: set<int>,classes: set<int>,counts: seq<nat>,categories: seq<int>): nat
    requires |counts|==|categories|
  {
    if fIdle && FIdleExcluded(fOurs,cord,task,warp) then 0
    else ShipSum(counts,categories,if fOurs then designs else classes,
      if fOurs then fDesign else fEnemy,!fOurs)
  }
  predicate FFleetShown(view: int,cShips: nat,fScan: bool,fMain: bool) {
    view!=5 && (cShips>0 || fScan || fMain)
  }
  function RelationBit(fOurs: bool,relation: int): int { if fOurs then 1 else if relation==1 then 2 else if relation==0 then 4 else 8 }
  predicate FMineShown(fOverlay: bool,fScan: bool,relationBit: int,chosen: set<int>) {
    fOverlay && (fScan || relationBit in chosen)
  }
  predicate FPathsShown(fOverlay: bool,view: int,fDead: bool,det: int,cord: int,fFleetShown: bool) {
    fOverlay && view!=5 && !fDead && det>=7 && cord>1 && fFleetShown
  }
  function FilterMenu(prior: set<int>,all: set<int>,choice: int,item: int): set<int> {
    if choice == -1 then prior else if choice==0 then all
    else if choice==1 then (prior-all)+(all-prior) else if choice<4 then {}
    else if item in prior then prior-{item} else prior+{item}
  }
  predicate FFilterEnabled(fOld: bool,next: set<int>,choice: int,item: int) {
    fOld || (choice != -1 && (if choice<4 then next!={} else item in next))
  }
  datatype MineMenu = MineMenu(chosen: set<int>,fOverlay: bool)
  function MineChoice(s: MineMenu,choice: int): MineMenu {
    var prior := if s.fOverlay then s.chosen else {};
    var next := if choice == -1 then prior else if choice==0 then {0,1,2,3}
      else if choice<3 then {} else
        if choice-3 in prior then prior-{choice-3} else prior+{choice-3};
    MineMenu(next,if choice == -1 then s.fOverlay else next!={})
  }
  function SetView(bits: U16,view: int): int
    requires 0<=view<=5
  { view+16*C.Bits(bits,4,10) }
  function LoadView(bits: U16): int {
    if C.Bits(bits,0,4)+16384*C.Bits(bits,14,2)>=6 then 0 else bits
  }
  predicate FResetDesignMask(bits: U16) { C.Bits(bits,0,4)+16384*C.Bits(bits,14,2)>=6 }
  function LoadZoom(saved: int,prior: Zoom): Zoom { if 1<=saved<=9 then saved-5 else prior }
  function LoadRadar(saved: int): int { if 0<=saved<=100 then saved else 100 }
  function LoadMineralScale(saved: int): int { if 100<=saved<=30000 then saved else 5000 }
  function RadarPercent(prior: int,parsed: int,fSuffixValid: bool,fCancel: bool): int {
    A.Clamp(if fCancel then prior else if fSuffixValid then parsed else 0,2,100)
  }
  function RadarRange(range: int,pct: int): int { if pct<100 then A.LMulDiv(range,pct,100) else range }
  function ValuePercent(current: int,optimal: int,fCA: bool): int {
    if current<0 || fCA then optimal else current
  }
  predicate FTerraColor(current: int,optimal: int,fCA: bool) { current<0 && optimal>=0 && !fCA }
  predicate FValueData(det: int) { det>=3 }
  predicate FMineralData(det: int,fConc: bool) { det>=4 || (fConc && det>=3) }
  predicate FPopulationData(det: int,fOwner: bool) { det>=3 && fOwner }
  function PopulationInput(fOurs: bool,cHundreds: int,uPopGuess: int): int {
    if fOurs then cHundreds else uPopGuess*4
  }
  predicate FWormholeLink(id: int,partner: int,fTraversed: bool,fPartnerExists: bool) {
    id<partner && fTraversed && fPartnerExists
  }
  function ValueRadius(pct: int): int { A.Min(10,(if pct<0 then -pct/5 else pct/11)+2) }
  function MineralBar(quantity: nat,concentration: nat,fConc: bool,scale: int,z: Zoom): int
    requires 100<=scale<=30000
  {
    var h := A.Min(20,if fConc then concentration/5 else (scale/40+quantity)/(scale/20));
    if z<0 then h/2 else h
  }
  function PopThresholds(): seq<int> { [25,50,100,200,400,800,1000,1500,2250,3000,4000,5000,6000,7500,9000,11000,14000,18000,25000] }
  function Below(pop: int,xs: seq<int>): nat
    decreases |xs|
  { if |xs|==0 || xs[0]>pop then 0 else 1+Below(pop,xs[1..]) }
  function PopulationRadius(pop: int,z: Zoom): int {
    var r := Below(pop,PopThresholds())+2;
    if z<3 then (r+1)/2 else r
  }
  function CountLabel(total: nat): nat { if total>999 then 999 else total }

  datatype SnapshotKey = SnapshotKey(game: U32,turn: Turn,player: PlayerId)
  predicate FScannerReady(fSDLWindow: bool,fWindow: bool,fVisible: bool,lid: int,
    player: int,cPlayer: int,fFleetStorage: bool,cPlanet: int,cFleet: int,cThing: int) {
    fSDLWindow && fWindow && fVisible && lid!=0 && 0<=player<cPlayer &&
      fFleetStorage && cPlanet>0 && cFleet>=0 && cThing>=0
  }
  predicate FSnapshotCurrent(a: SnapshotKey,b: SnapshotKey,fReady: bool) { fReady && a==b }
  predicate FSnapshotSelect(a: SnapshotKey,b: SnapshotKey,fReady: bool,
    requested: ObjectRef,live: set<ObjectRef>) {
    FSnapshotCurrent(a,b,fReady) && requested in live &&
      (requested.PlanetRef? || requested.FleetRef? || requested.ThingRef?)
  }
  // Projection of the logical navigation state. UI callbacks, redraw caches
  // and tutorial progression are external effects, not orders in this model.
  datatype Navigation = Navigation(main: ObjectRef,scan: ObjectRef,iwp: int,
    fDirty: bool,orders: seq<int>)
  function InspectObject(s: Navigation,key: ObjectRef): Navigation { s.(scan:=key) }
  lemma InspectPreservesOrders(s: Navigation,key: ObjectRef)
    ensures InspectObject(s,key).orders==s.orders && InspectObject(s,key).fDirty==s.fDirty
    ensures InspectObject(s,key).main==s.main && InspectObject(s,key).iwp==s.iwp
  {}
  lemma StaleSnapshotRejected(a: SnapshotKey,b: SnapshotKey,k: ObjectRef,live: set<ObjectRef>)
    requires a!=b
    ensures !FSnapshotSelect(a,b,true,k,live)
  {}
  lemma FilterSelectionOverride(view: int,fMain: bool)
    requires view!=5
    ensures FFleetShown(view,0,true,fMain)
    ensures !FFleetShown(5,1,true,true)
  {}
  lemma HitDistanceNeverIncreases(h: Hit,c: Candidate,p: Point,iwp: int)
    ensures HitStep(h,c,p,iwp).d2<=h.d2
  {}
  lemma WheelConservation(remainder: int,delta: int)
    ensures -120<WheelRemainder(remainder,delta)<120
    ensures 120*WheelNotches(remainder,delta)+WheelRemainder(remainder,delta)==WheelTotal(remainder,delta)
  {}
  lemma AlignedOrigins(p: Point,dInv: int,cx: nat,cy: nat)
    ensures CenterTop(p,dInv,cx,cy).x%4==0 && CenterTop(p,dInv,cx,cy).y%4==0
    ensures CenterTop(p,dInv,cx,cy).x>=1000 && CenterTop(p,dInv,cx,cy).y>=1000
  {}
  lemma SearchAbsent(ep: seq<ObjectRef>,nf: Option<ObjectRef>,ef: seq<ObjectRef>,pp: seq<ObjectRef>)
    ensures Search(ep,nf,ef,pp)==None <==> |ep|==0 && nf==None && |ef|==0 && |pp|==0
  {}
  lemma ZoomBound(z: Zoom,c: int)
    ensures -4<=WheelZoom(z,c)<=4
  {}
  lemma BelowAll(pop: int,xs: seq<int>)
    requires forall i | 0<=i<|xs| :: xs[i]<=pop
    ensures Below(pop,xs)==|xs|
    decreases |xs|
  {
    if |xs|>0 { BelowAll(pop,xs[1..]); }
  }
  lemma {:isolate_assertions} ArithmeticVectors() {
    assert C.Pow2(0)==1;
    assert C.Pow2(2)==4;
    assert C.Pow2(4)==16;
    assert C.Pow2(6)==64;
    assert C.Pow2(8)==256;
    assert C.Pow2(10)==1024;
    assert C.Pow2(12)==4096;
    assert C.Pow2(14)==16384;
    assert C.Pow2(15)==32768;
    assert C.Pow2(16)==65536;
    assert PtToScan(-341,-3)==-128 && PtToScan(-341,-1)==-256;
    assert PtToScan(-341,1)==-427 && PtToScan(-341,2)==-512;
    assert ScanToPt(4096,-3)==-10922;
    assert WheelNotches(60,60)==1 && WheelRemainder(60,60)==0;
    assert WheelNotches(60,-60)==0 && WheelRemainder(60,-60)==-60;
    assert !FPanStarts(3,3,4,4) && FPanStarts(4,0,4,4);
    assert LoadView(224)==224 && LoadView(6)==0;
  }
  lemma {:isolate_assertions} DisplayVectors() {
    assert PopulationRadius(24,3)==2 && PopulationRadius(25,3)==3;
    BelowAll(25000,PopThresholds());
    assert PopulationRadius(25000,3)==21 && CountLabel(1000)==999;
    assert MineralBar(125,0,false,5000,0)==1;
    assert FIdleExcluded(false,1,NoTask,0) && !FIdleExcluded(false,1,NoTask,5);
    assert FIdleExcluded(true,1,RemoteMine,0);
    assert !FMineShown(false,true,1,{1}) && FMineShown(true,true,1,{});
    assert MineChoice(MineMenu({0},false),-1)==MineMenu({},false);
    assert FFilterEnabled(true,{},2,0);
    assert RadarPercent(0,0,false,true)==2 && LoadRadar(0)==0;
    assert !FMineralData(3,false) && FMineralData(3,true);
    assert ShipSum([3],[0],{0},true,false)==3;
    assert ShipSum([2,3],[0,0],{1},true,false)==3;
    assert VisibleShips(false,true,1,NoTask,0,true,false,{1},{},[2,3],[0,0])==3;
  }
  lemma {:isolate_assertions} SearchVectors() {
    assert Search([PlanetRef(2)],Some(FleetRef(FleetId(0,0))),[],[])==Some(PlanetRef(2));
    assert Search([],None,[],[])==None;
    assert SkipSpaces([32,49,120])==[49,120];
    assert ParseFleetNumber([35,32,49,120],false)==Some(1);
    assert ParseFleetNumber([48,49],false)==None;
    assert FleetDigits([51],51)==None;
    assert FleetDigits([49,51],5)==None;
    assert ParseFleetNumber([53,49,51],false)==None;
  }
  lemma {:isolate_assertions} {:fuel Hits,4} HitVectors() {
    var p := Point(1000,1000);
    var h := Hits([Candidate(PlanetKind,0,p,false),Candidate(FleetKind,0,p,false),Candidate(FleetKind,1,p,true)],p,-1,EmptyHit(0));
    assert h.kind==PlanetKind && h.pl==0 && h.fl==1;
    assert SelectedKind(h.(kind:=FleetKind),1)==PlanetKind && SelectedKind(h.(kind:=FleetKind),2)==FleetKind;
    var tied := Hits([Candidate(FleetKind,0,Point(999,1000),false),Candidate(FleetKind,1,Point(1001,1000),true)],p,-1,EmptyHit(100));
    assert tied.pt==Point(999,1000) && tied.fl==1;
  }
  lemma NavigationVectors() {
    assert !FSnapshotSelect(SnapshotKey(1,0,0),SnapshotKey(1,0,1),true,PlanetRef(0),{PlanetRef(0)});
    assert Click(Left,true,false,false,false,true,true,false,false,true)==MassTarget;
    assert DragOutcome(true,true,false)==0 && DragOutcome(true,true,true)==2;
  }
}
