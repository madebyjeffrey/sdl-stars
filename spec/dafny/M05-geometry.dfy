include "M03-setup.dfy"

// M05 §§4.1–4.3, 4.7–4.8. Small-step relations deliberately make no
// total-termination claim for source retry loops.
module M05Geometry {
  import opened S04Arithmetic
  import U = M03Setup
  import R = SharedGameState
  import C = S01Codecs
  datatype Point = Point(x: int,y: int)
  function Distance(a: Point,b: Point): int { (a.x-b.x)*(a.x-b.x)+(a.y-b.y)*(a.y-b.y) }
  function Diameter(size: U.Size): int { 400*(size+1) }
  function Target(size: U.Size,density: U.Density): int {
    var d := Diameter(size); var n0 := d*d/5000; var n1 := n0+(n0/4)*(density-1);
    Min(999,if density>=3 then n1+n1/4 else n1)
  }
  function Candidates(size: U.Size,density: U.Density): int {
    var n := Target(size,density); Min(999,n+n/7)
  }
  datatype PointDraw = PointDraw(point: Point,rng: R.Rng)
  function DrawPoint(rng: R.Rng,bound: int,origin: int): PointDraw {
    var x := Take(rng,bound); var y := Take(x.rng,bound); PointDraw(Point(origin+x.value,origin+y.value),y.rng)
  }
  function Coordinate(rng: R.Rng,size: U.Size): PointDraw { DrawPoint(rng,Diameter(size)-19,1010) }
  function Swap(xs: seq<Point>,i: nat,j: nat): seq<Point>
    requires i<|xs| && j<|xs|
    ensures |Swap(xs,i,j)|==|xs|
  { xs[i:=xs[j]][j:=xs[i]] }
  // ICompLong's narrowed difference is x-only in the supported coordinate range.
  function Compare(a: Point,b: Point): int { C.Signed(a.x-b.x,16) }
  predicate FSorted(xs: seq<Point>) { forall i | 1<=i<|xs| :: Compare(xs[i],xs[i-1])>=0 }
  datatype Range = Range(lo: nat,hi: nat)
  datatype Phase = Start | Left | Right | Exchange | Pivot | Pop | Done
  datatype Sort = Sort(xs: seq<Point>,lo: nat,hi: nat,left: nat,right: nat,lastLess: nat,firstGreater: nat,pending: seq<Range>,phase: Phase)
  predicate FSort(s: Sort) {
    s.phase==Done || (|s.xs|>0 && s.lo<=s.hi<|s.xs| && s.lo<=s.left<=s.hi && s.lo<=s.right<=s.hi+1 &&
      s.lo<=s.lastLess<=s.hi && s.lo<=s.firstGreater<=s.hi &&
      forall r <- s.pending :: r.lo<=r.hi<|s.xs|)
  }
  function StartSort(xs: seq<Point>): Sort {
    if |xs|<2 || FSorted(xs) then Sort(xs,0,0,0,0,0,0,[],Done)
    else Sort(xs,0,|xs|-1,0,0,0,0,[],Start)
  }
  // One source control-flow action. FSort is checked at every trace boundary.
  function SortStep(s: Sort): Sort
    requires FSort(s)
    requires s.phase==Left ==> s.left<s.hi
    requires s.phase==Right ==> s.right>s.lo
    requires s.phase==Exchange || s.phase==Pivot ==> s.right<|s.xs|
  {
    if s.phase==Done then s
    else if s.phase==Start then
      if s.lo>=s.hi then s.(phase:=Pop) else s.(left:=s.lo,right:=s.hi+1,lastLess:=s.lo,firstGreater:=s.hi,phase:=Left)
    else if s.phase==Left then
      var i := s.left+1;
      if i==s.hi || Compare(s.xs[i],s.xs[s.lo])>0 then s.(left:=i,phase:=Right)
      else s.(left:=i,lastLess:=if Compare(s.xs[i],s.xs[s.lo])<0 then i else s.lastLess)
    else if s.phase==Right then
      var i := s.right-1; var cmp := Compare(s.xs[s.lo],s.xs[i]);
      s.(right:=i,firstGreater:=if cmp<0 then i else s.firstGreater,phase:=if cmp>0 || i==s.lo then Exchange else Right)
    else if s.phase==Exchange then
      if s.right<=s.left then s.(phase:=Pivot) else
        s.(xs:=Swap(s.xs,s.left,s.right),lastLess:=s.left,firstGreater:=s.right,phase:=Left)
    else if s.phase==Pivot then
      var xs := Swap(s.xs,s.lo,s.right);
      if s.hi-s.firstGreater>=s.lastLess-s.lo then s.(xs:=xs,hi:=s.lastLess,pending:=s.pending+[Range(s.firstGreater,s.hi)],phase:=Start,
        left:=s.lo,right:=s.lo,lastLess:=s.lo,firstGreater:=s.lo)
      else s.(xs:=xs,lo:=s.firstGreater,pending:=s.pending+[Range(s.lo,s.lastLess)],phase:=Start,
        left:=s.firstGreater,right:=s.firstGreater,lastLess:=s.firstGreater,firstGreater:=s.firstGreater)
    else if |s.pending|==0 then s.(phase:=Done)
    else var r := s.pending[|s.pending|-1]; s.(lo:=r.lo,hi:=r.hi,pending:=s.pending[..|s.pending|-1],phase:=Start,
      left:=r.lo,right:=r.lo,lastLess:=r.lo,firstGreater:=r.lo)
  }
  predicate FSortTrace(trace: seq<Sort>) {
    |trace|>0 && trace[0]==StartSort(trace[0].xs) && trace[|trace|-1].phase==Done &&
    (forall s <- trace :: FSort(s) && (s.phase==Left ==> s.left<s.hi) && (s.phase==Right ==> s.right>s.lo) &&
      (s.phase==Exchange || s.phase==Pivot ==> s.right<|s.xs|)) &&
    forall i | 0<=i<|trace|-1 :: trace[i+1]==SortStep(trace[i])
  }
  function PrunePair(xs: seq<Point>,i: nat,j: nat,removed: set<nat>): set<nat>
    requires i<j<|xs|
  { if i !in removed && j !in removed && xs[j].x<=xs[i].x+12 && Abs(xs[j].y-xs[i].y)<=12 && Distance(xs[i],xs[j])<=144
    then removed+{j} else removed }
  datatype Removal = Removal(removed: set<nat>,rng: R.Rng)
  function RandomRemoval(count: nat,s: Removal): Removal {
    var d := Take(s.rng,count); Removal(s.removed+{d.value as nat},d.rng)
  }
  predicate FNearestPoint(xs: seq<Point>,i: nat,j: nat) {
    i<|xs| && j<|xs| && i!=j && forall k | 0<=k<|xs| && k!=i ::
      Distance(xs[i],xs[j])<=Distance(xs[i],xs[k]) && (Distance(xs[i],xs[j])==Distance(xs[i],xs[k]) ==> j<=k)
  }
  function ClumpAxis(p: int,n: int,q: int): int {
    if q<=144 then p else if q<=324 then (4*p+n)/5 else if q<=625 then (2*p+n)/3 else if q<=1600 then (p+n)/2 else (p+2*n)/3
  }
  function Clump(p: Point,n: Point): Point { Point(ClumpAxis(p.x,n.x,Distance(p,n)),ClumpAxis(p.y,n.y,Distance(p,n))) }
  datatype Spacing = Spacing(initial: int,minimum: int,maximum: int)
  function HomeSpacing(size: U.Size,players: int,distance: U.Distance): Spacing
    requires 1<=players<=16
  { var d := Diameter(size); var a := Max(0,d*d/players-6*d)*9/10; var i := a*distance/3+6*d;
    Spacing(i,9*i/10,7*i/6) }
  function Relax(s: Spacing): Spacing { Spacing(s.initial,s.minimum-s.initial/35,s.maximum+s.initial/35) }
  predicate FInside(p: Point,lo: int,hi: int) { lo<=p.x<=hi && lo<=p.y<=hi }
  function SquareDistance(p: Point,lo: int,hi: int): int requires lo<=hi {
    Distance(p,Point(Clamp(p.x,lo,hi),Clamp(p.y,lo,hi)))
  }
  function HomeSquare(size: U.Size,players: int,fFirst: bool): (int,int) {
    var d := Diameter(size);
    if fFirst then (1000+d/4,1000+3*d/4) else if players>4 then (1000+d/20,1000+LMulDiv(d,19,20))
    else if players>=3 then (1000+d/10,1000+LMulDiv(d,9,10)) else (1000+LMulDiv(d,3,20),1000+LMulDiv(d,17,20))
  }
  predicate FHome(p: Point,prior: seq<Point>,lo: int,hi: int,s: Spacing) {
    FInside(p,lo,hi) && (forall q <- prior :: Distance(p,q)>0 && Distance(p,q)>=s.minimum) &&
      exists q <- prior :: Distance(p,q)<=s.maximum
  }
  // Failed placement restarts at first-home selection; no retry ceiling.
  datatype HomePhase = FirstRandom | LaterRandom | CircularScan | Restart | Assigned
  datatype Search = Search(phase: HomePhase,attempt: nat,last: nat,best: nat,bestDistance: int,spacing: Spacing)
  predicate FFirstStep(s: Search,next: Search,candidate: nat,q: int,fInside: bool) {
    s.phase==FirstRandom && s.attempt<50 && next==s.(attempt:=s.attempt+1,last:=candidate,
      best:=if q<s.bestDistance then candidate else s.best,bestDistance:=Min(q,s.bestDistance),
      phase:=if fInside || s.attempt==49 then Assigned else FirstRandom)
  }
  function ScanStart(last: nat,count: nat): nat requires count>0 { (last+1)%count }
  predicate FSecondWorld(p: Point,home: Point,size: U.Size,fOwned: bool) {
    !fOwned && var d := Diameter(size); var lo := 15*d/100; var hi := 23*d/100;
    lo*lo<=Distance(p,home)<=hi*hi
  }
  predicate FNearestUnowned(points: seq<Point>,owned: set<nat>,home: Point,chosen: nat) {
    chosen<|points| && chosen !in owned && forall i | 0<=i<|points| && i !in owned ::
      Distance(home,points[chosen])<=Distance(home,points[i]) &&
      (Distance(home,points[chosen])==Distance(home,points[i]) ==> chosen<=i)
  }
  function Reservoir(rng: R.Rng,k: nat,current: int,candidate: int): (int,R.Rng)
    requires k>0
  { var d := Take(rng,k); (if d.value==0 then candidate else current,d.rng) }
  function RerollFallback(attempts: nat): bool { attempts>=100 }
  function PopulationSplit(home: nat): (nat,nat) { (4*home/5,2*home/5) }
  function PairCount(size: U.Size,fNoRandom: bool,rng: R.Rng): DrawResult {
    if fNoRandom then DrawResult(rng,0) else var d := Take(rng,[3,3,5,4,5][size]); DrawResult(d.rng,[0,1,1,3,4][size]+d.value)
  }
  function ObstacleMask(q: int,kind: nat): bv4 requires kind<3 {
    var bounds := if kind==0 then [25,100,900,4900] else if kind==1 then [16,64,225,900] else [25,100,400,784];
    if q<bounds[0] then 8 else if q<bounds[1] then 4 else if q<bounds[2] then 2 else if q<bounds[3] then 1 else 0
  }
  function Masks(p: Point,obstacles: seq<(Point,nat)>): bv4
    requires forall o <- obstacles :: o.1<3 decreases |obstacles|
  { if |obstacles|==0 then 0 else ObstacleMask(Distance(p,obstacles[0].0),obstacles[0].1) | Masks(p,obstacles[1..]) }
  function WormholeScore(p: Point,size: U.Size,obstacles: seq<(Point,nat)>,allPositions: set<Point>): int
    requires forall o <- obstacles :: o.1<3
  { var d := Diameter(size);
    if !FInside(p,1000,1000+d) || p in allPositions then 15 else
      ((if !FInside(p,1010,1000+d-10) then 4 else 0) | Masks(p,obstacles)) as int }
  function BetterPosition(previous: Point,best: int,candidate: Point,score: int): Point { if score<best then candidate else previous }
  datatype LaterSearch = LaterSearch(attempts: nat,last: nat,startScan: nat,fScanning: bool,fRestart: bool,fAccepted: bool)
  function LaterDraw(s: LaterSearch,candidate: nat,fValid: bool): LaterSearch
    requires s.attempts<50 && !s.fScanning && !s.fRestart && !s.fAccepted
  { LaterSearch(s.attempts+1,candidate,candidate,!fValid && s.attempts==49,false,fValid) }
  function LaterScan(s: LaterSearch,count: nat,fValid: bool): LaterSearch
    requires count>0 && s.last<count && s.startScan<count && s.fScanning && !s.fAccepted && !s.fRestart
  { var next := (s.last+1)%count;
    s.(last:=next,fRestart:=next==s.startScan,fAccepted:=next!=s.startScan && fValid) }
  predicate FRestartSpacing(before: Spacing,after: Spacing,nextPhase: HomePhase) {
    after==Relax(before) && nextPhase==FirstRandom
  }
  predicate FNameDraw(rng: R.Rng,after: R.Rng,nameCount: nat,fTutorial: bool,used: set<int>,probe: seq<int>,chosen: int) {
    nameCount>0 && |probe|>0 && probe[0]==Take(rng,nameCount).value && after==Take(rng,nameCount).rng &&
    (forall i | 0<=i<|probe| :: 0<=probe[i]<nameCount+(if fTutorial then 1 else 0)) &&
    (forall i | 0<=i<|probe|-1 :: probe[i] in used && probe[i+1]==(probe[i]+1)%(nameCount+(if fTutorial then 1 else 0))) &&
    chosen==probe[|probe|-1] && chosen !in used
  }
  predicate FRemovalTrace(count: nat,target: nat,trace: seq<Removal>) {
    target<=count && |trace|>0 &&
    (forall s <- trace :: forall i <- s.removed :: i<count) &&
    (forall i | 0<=i<|trace|-1 :: |trace[i].removed|<count-target && trace[i+1]==RandomRemoval(count,trace[i])) &&
    |trace[|trace|-1].removed|>=count-target
  }
  predicate FCompact(original: seq<Point>,removed: set<nat>,kept: seq<nat>,points: seq<Point>) {
    |kept|==|points| && (forall i | 0<=i<|kept| :: kept[i]<|original| && kept[i] !in removed && points[i]==original[kept[i]]) &&
    (forall i,j | 0<=i<j<|kept| :: kept[i]<kept[j]) &&
    (forall i | 0<=i<|original| && i !in removed :: i in kept)
  }
  predicate FClumpStep(points: seq<Point>,next: seq<Point>,rng: R.Rng,after: R.Rng,nearest: nat) {
    |points|>1 && var draw := Take(rng,|points|); var i := draw.value;
    FNearestPoint(points,i,nearest) && after==draw.rng && next==points[i:=Clump(points[i],points[nearest])]
  }
  // Endpoint trials retain the strict numeric-mask minimum, stopping at zero
  // or the hundredth candidate. Stability is drawn before this sequence.
  datatype WormSearch = WormSearch(attempts: nat,best: Point,score: int,fDone: bool)
  function WormTrial(s: WormSearch,candidate: Point,score: int): WormSearch
    requires !s.fDone && s.attempts<100 && 0<=score<=15
  { WormSearch(s.attempts+1,BetterPosition(s.best,s.score,candidate,score),Min(s.score,score),score==0 || s.attempts==99) }
  function FirstHomeChoice(s: Search): nat
    requires s.phase==Assigned
  { if s.bestDistance==0 then s.last else s.best }
  lemma AllTargetCounts() {
    var expected := [[24,32,40,60],[96,128,160,240],[216,288,360,540],[384,512,640,960],[600,800,999,999]];
    forall size,density | 0<=size<5 && 0<=density<4
      ensures Target(size,density)==expected[size][density]
    {}
  }
  lemma GeometryVectors() {
    assert Target(0,0)==24 && Target(4,3)==999;
    assert Candidates(0,0)==27;
    assert ClumpAxis(100,120,400)==106;
    assert PopulationSplit(250)==(200,100);
    assert RerollFallback(100) && !RerollFallback(99);
    assert ObstacleMask(24,0)==8 && ObstacleMask(25,0)==4;
    assert PairCount(4,true,R.Rng(3,5))==DrawResult(R.Rng(3,5),0);
  }
}
