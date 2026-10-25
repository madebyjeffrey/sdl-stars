include "M04-race-model.dfy"

// M03 accepted setup, raw controls, roster editing and top-level wizard transitions.
module M03Setup {
  import S = S01
  import R = SharedGameState
  import C = S01Codecs
  import Race = M04Race
  import opened S04Arithmetic
  type Size = n: int | 0<=n<5 witness 0
  type Density = n: int | 0<=n<4 witness 0
  type Distance = n: int | 0<=n<4 witness 0
  datatype Slot = Empty | Preset(id: int) | Cached(index: int) | Computer(kind: int,level: int) | Expansion
  predicate FSlot(s: Slot,cacheCount: nat) {
    match s
    case Empty => true
    case Preset(id) => 0<=id<=6
    case Cached(index) => 0<=index<cacheCount<=16
    case Computer(kind,level) => 0<=kind<=6 && 0<=level<=4
    case Expansion => true
  }
  function Selector(s: Slot): int {
    match s
    case Empty => 0
    case Preset(id) => 1+4*id
    case Cached(index) => 2+4*index
    case Computer(kind,level) => 3+4*kind+32*level
    case Expansion => 29
  }
  function DecodeSelector(code: int): Slot {
    if code==0 then Empty else if code%4==1 then (if code==29 then Expansion else Preset(code/4))
    else if code%4==2 then Cached(code/4) else Computer((code/4)%8,code/32)
  }
  predicate FPrefix(slots: seq<Slot>) {
    |slots|==16 && forall i,j | 0<=i<j<16 :: slots[i]==Empty ==> slots[j]==Empty
  }
  function Occupied(slots: seq<Slot>): nat
    decreases |slots|
  { if |slots|==0 || slots[0]==Empty then 0 else 1+Occupied(slots[1..]) }
  function ClearFrom(slots: seq<Slot>,index: nat): seq<Slot>
    requires index<|slots|
  { seq(|slots|,i requires 0<=i<|slots| => if i<index then slots[i] else Empty) }
  function FillBack(slots: seq<Slot>,index: nat,choice: Slot): seq<Slot>
    requires index<|slots|
    ensures |FillBack(slots,index,choice)|==|slots|
    decreases index
  { var changed := slots[index:=choice];
    if index==0 || slots[index-1]!=Empty then changed else FillBack(changed,index-1,choice) }
  function EditSlot(slots: seq<Slot>,index: nat,choice: Slot): seq<Slot>
    requires index<|slots|
  { if choice==Empty then ClearFrom(slots,index) else FillBack(slots,index,choice) }
  function FreeCache(slots: seq<Slot>,selected: nat,remaining: nat): int
    requires selected<|slots| && remaining<=16
    decreases remaining
  {
    if remaining==0 then -1 else
    var i := remaining-1;
    if forall j | 0<=j<|slots| && j!=selected :: slots[j]!=Cached(i) then i
    else FreeCache(slots,selected,i)
  }
  function CacheAllocation(slots: seq<Slot>,selected: nat,count: nat): int
    requires selected<|slots| && count<=16
  { if count<16 then count else FreeCache(slots,selected,16) }
  datatype Options = Options(fExtraFuel: bool,fSlowTech: bool,fBBS: bool,fNoRandom: bool,
    fAisBand: bool,fVisScores: bool,fClumping: bool)
  function NoOptions(): Options { Options(false,false,false,false,false,false,false) }
  function VCMax(): seq<int> { [16,18,4,19,28,49,29,87,7,47] }
  function VCBase(): seq<int> { [20,8,2,1000,20,10,10,30,0,30] }
  function VCStep(): seq<int> { [5,1,1,1000,10,10,10,10,1,10] }
  function DefaultVC(): seq<int> { [136,142,130,10,136,9,9,7,1,0] }
  predicate FVC(vc: seq<int>) { |vc|==10 && forall i | 0<=i<10 :: 0<=vc[i]<256 && vc[i]%128<=VCMax()[i] }
  function Enabled(vc: seq<int>,i: nat): bool requires |vc|==10 && i<10 { vc[i]>=128 }
  function EnabledCount(vc: seq<int>): int requires |vc|==10 {
    Sum(seq(7,i requires 0<=i<7 => if Enabled(vc,[0,1,3,4,5,6,7][i]) then 1 else 0))
  }
  function VCValue(vc: seq<int>,i: nat): int
    requires |vc|==10 && i<10
  { if i==8 then Min(vc[i]%128,EnabledCount(vc)) else VCBase()[i]+VCStep()[i]*(vc[i]%128) }
  function Toggle(vc: seq<int>,i: nat,fOn: bool): seq<int>
    requires |vc|==10 && i<10
  { vc[i:=(vc[i]%128)+(if fOn then 128 else 0)] }
  function SetVC(vc: seq<int>,i: nat,raw: int): seq<int>
    requires |vc|==10 && i<10
  { var v := Clamp(raw,0,VCMax()[i]);
    vc[i:=(if vc[i]>=128 then 128 else 0)+(if i==8 then Min(v,EnabledCount(vc)) else v)] }
  function ClickVC(vc: seq<int>,hit: nat,fShift: bool,fControl: bool): seq<int>
    requires |vc|==10 && hit<20
  { var i := hit/2; var delta := (if hit%2==0 then 1 else -1)*(if fShift || fControl then 5 else 1);
    SetVC(vc,i,vc[i]%128+delta) }
  function BatchRaw(i: nat,value: int): int
    requires i<10
  { C.DivC(value-VCBase()[i],VCStep()[i]) }
  predicate FBatchValue(i: nat,value: int) {
    i<10 && VCBase()[i]<=value<=VCBase()[i]+VCMax()[i]*VCStep()[i]
  }
  datatype Setup = Setup(title: S.Bytes,size: Size,density: Density,distance: Distance,
    options: Options,roster: seq<Slot>,cache: seq<Race.Race>,victory: seq<int>)
  predicate FSetup(s: Setup) {
    S.FName(s.title) && 0 !in s.title && FPrefix(s.roster) && s.roster[0]!=Empty && |s.cache|<=16 &&
    (forall slot <- s.roster :: FSlot(slot,|s.cache|)) && FVC(s.victory)
  }
  datatype Branch = Branch(fEnabled: bool,bound: int,count: int,extra: nat,fSubtract: bool)
  function CountBranches(size: Size,level: int): seq<Branch>
    requires 0<=level<=3
  {
    if size==0 then [Branch(level==3,3,3,0,false)]
    else if size==1 then [Branch(level==3,4,5,0,false),Branch(level>=2,6-level,4,0,false)]
    else if size==2 then [Branch(level==3,10,9,0,false),Branch(level==3,10,5,0,false),
      Branch(level>=2,7-level,8,0,false),Branch(level>=2,7-level,6,0,false)]
    else if size==3 then [Branch(level==3,10,14,2,false),Branch(level==3,10,10,2,true),
      Branch(level>=2,7-level,13,0,false),Branch(level>=2,7-level,11,0,false)]
    else [Branch(level==3,10,13,3,true),Branch(level>=2,9-level,14,0,false),Branch(level>=2,7-level,15,0,false)]
  }
  function ChooseCount(branches: seq<Branch>,fallback: int,rng: R.Rng): DrawResult
    requires 1<=fallback<=16
    requires forall b <- branches :: 1<=b.count-b.extra && b.count+b.extra<=16
    ensures 1<=ChooseCount(branches,fallback,rng).value<=16
    decreases |branches|
  {
    if |branches|==0 then DrawResult(rng,fallback) else
    var b := branches[0];
    if !b.fEnabled then ChooseCount(branches[1..],fallback,rng) else
    var first := Take(rng,b.bound);
    if first.value!=0 then ChooseCount(branches[1..],fallback,first.rng) else
    if b.extra==0 then DrawResult(first.rng,b.count) else
    var second := Take(first.rng,b.extra);
    DrawResult(second.rng,b.count+(if b.fSubtract then -second.value else second.value))
  }
  function DefaultCount(size: Size,level: int,rng: R.Rng): DrawResult
    requires 0<=level<=3
    ensures 1<=DefaultCount(size,level,rng).value<=16
  { ChooseCount(CountBranches(size,level),[2,3,7,12,16][size],rng) }
  function DefaultSelector(level: int,n: int,i: int): int
    requires 0<=level<=3 && 1<=i<n<=16
  {
    if level==0 then (if i<(n+1)/3+1 then 11 else if i<2*(n+1)/3+1 then 15 else if i<5*(n+1)/6+1 then 7 else 27)
    else if level==3 then (if i<(n+1)/3+1 then 99 else if i<(6*(n-1)+11)/12+1 then 119 else if i<(5*(n-1)+5)/6+1 then 115 else 123)
    else var choices := if level==1 then [39,35,43,47,51] else [83,71,67,87,91];
      if i<2*(n+5)/7+1 then choices[0] else if i<(3*(n-1)+6)/7+1 then choices[1]
      else if i<(4*(n-1)+6)/7+1 then choices[2] else if i<(5*(n-1)+6)/7+1 then choices[3]
      else if i<(6*(n-1)+6)/7+1 then choices[4] else 155
  }
  datatype RosterResult = RosterResult(roster: seq<Slot>,rng: R.Rng)
  function ShuffleLater(roster: seq<Slot>,i: nat,rng: R.Rng): RosterResult
    decreases if i<|roster| then |roster|-i else 0
  {
    if i+1>=|roster| then RosterResult(roster,rng) else
    var d := Take(rng,|roster|-i-1); var j := i+1+d.value;
    ShuffleLater(roster[i:=roster[j]][j:=roster[i]],i+1,d.rng)
  }
  function DefaultRoster(size: Size,level: int,first: Slot,rng: R.Rng): RosterResult
    requires 0<=level<=3
  {
    var n := DefaultCount(size,level,rng);
    var count := n.value;
    var slots := seq(count,i requires 0<=i<count => if i==0 then first else DecodeSelector(DefaultSelector(level,count,i)));
    var shuffled := ShuffleLater(slots,1,n.rng);
    RosterResult(shuffled.roster+seq(16-count,i=>Empty),shuffled.rng)
  }
  datatype Controller = Human | AI(kind: int,level: int) | Maid(preset: int)
  datatype Resolve = Resolve(controller: Controller,rng: R.Rng)
  function ResolveAI(kind: int,level: int,rng: R.Rng): Resolve
    requires 0<=kind<=6 && 0<=level<=4
  {
    var l := if level==4 then Take(rng,4) else DrawResult(rng,level);
    var k := if kind==6 then Take(l.rng,6) else DrawResult(l.rng,kind);
    Resolve(AI(k.value,l.value),k.rng)
  }
  function ResolveExpansion(rng: R.Rng): Resolve {
    var d := Take(rng,7); Resolve(Maid(d.value),d.rng)
  }
  function Probe(used: set<int>,start: int,modulus: int,remaining: nat): int
    requires 0<=start<modulus
    decreases remaining
  { if remaining==0 then -1 else if start !in used then start else Probe(used,(start+1)%modulus,modulus,remaining-1) }
  // Full name/portrait normalization consumes supplied catalog entries in exact
  // source order; the normalization trace records each pass, without assuming RNG rollback.
  datatype NormalizePass = Controllers | SubstitutionAndMissingNames | DuplicateNames | PortraitInvalidation | DuplicatePortraits | FillPortraits
  function NormalizationOrder(): seq<NormalizePass> {
    [Controllers,SubstitutionAndMissingNames,DuplicateNames,PortraitInvalidation,DuplicatePortraits,FillPortraits]
  }
  datatype Wizard = Wizard(page: int,maxStep: int,fReadOnly: bool,fIdleBefore: bool,
    fIdleNow: bool,setup: Setup,priorGame: S.Bytes,currentGame: S.Bytes,rng: R.Rng,
    savedRaces: seq<Race.Race>,fDestroyedPrior: bool)
  datatype WizardResult = Editing(state: Wizard) | Cancelled(game: S.Bytes,fIdle: bool,rng: R.Rng,saved: seq<Race.Race>)
    | Creating(state: Wizard) | TitleScreen(fResidue: bool) | Published
  function CancelWizard(w: Wizard): WizardResult {
    Cancelled(if w.fReadOnly || w.fDestroyedPrior then w.currentGame else w.priorGame,w.fIdleBefore,w.rng,w.savedRaces)
  }
  function LeavePage(w: Wizard,fNext: bool): Wizard
    requires 0<=w.page<3
  { w.(page:=Clamp(w.page+(if fNext then 1 else -1),0,2),maxStep:=Max(w.maxStep,w.page)) }
  predicate FCanLeave(w: Wizard) { w.fReadOnly || w.page!=1 || (|w.setup.roster|>0 && w.setup.roster[0]!=Empty) }
  predicate FInitialize(w: Wizard,next: Wizard,defaults: RosterResult) {
    |w.setup.victory|==10 &&
    next==w.(rng:=if !w.fReadOnly && w.maxStep<1 then defaults.rng else w.rng,setup:=w.setup.(victory:=if !w.fReadOnly && w.maxStep<2 then SetVC(w.setup.victory,9,2*w.setup.size) else w.setup.victory,
      roster:=if !w.fReadOnly && w.maxStep<1 then defaults.roster else w.setup.roster))
  }
  function BeginFinish(w: Wizard,fReserved: bool): WizardResult {
    if w.fReadOnly || !fReserved then Editing(w) else Creating(w.(fDestroyedPrior:=true))
  }
  function Complete(fGenerated: bool,fFamily: bool,fMetadata: bool,fCleanup: bool): WizardResult {
    if fGenerated && fFamily && fMetadata then Published else TitleScreen(!fCleanup)
  }
  function RowTop(i: int,h: int): int { 6+i*(h+4) }
  predicate FRowHit(x: int,y: int,h: int,labelWidth: int) {
    h>0 && y>=6 && (y-6)/(h+4)<16 && (y-6)%(h+4)<h+1 && labelWidth+12<=x<labelWidth+12+h+1
  }
  function RosterHeight(h: int): int { 16*(h+4)+8 }
  function DigitCounts(): seq<int> { [3,2,1,5,3,3,3,3,1,3] }
  datatype Rect = Rect(left: int,top: int,width: int,height: int)
  function Spinner(right: int,top: int,h: int,fDown: bool): Rect
    requires h>0
  { var height := h/2+3; Rect(right+4,top-3+(if fDown then height-1 else 0),15,height) }
  function CheckboxIndex(row: nat): nat { row+(if row>=2 then 1 else 0) }
  lemma TogglePreservesValue(vc: seq<int>,i: nat,f: bool)
    requires |vc|==10 && i<10
    ensures Toggle(vc,i,f)[i]%128==vc[i]%128
  {}
  lemma {:isolate_assertions} {:fuel Sum, 9} SetupVectors() {
    assert VCValue(DefaultVC(),0)==60 && VCValue(DefaultVC(),8)==1;
    assert BatchRaw(0,64)==8 && VCBase()[0]+VCStep()[0]*8==60;
    assert BatchRaw(3,1999)==0;
    assert ClickVC(DefaultVC()[0:=142],0,true,true)[0]%128==16;
    assert DefaultCount(1,1,R.Rng(3,5))==DrawResult(R.Rng(3,5),3);
    assert DefaultSelector(1,3,1)==39 && DefaultSelector(1,3,2)==39;
    assert Complete(true,true,false,false)==TitleScreen(true);
    assert DecodeSelector(Selector(Computer(6,4)))==Computer(6,4);
    assert Probe({0,1},1,4,4)==2;
  }
}
