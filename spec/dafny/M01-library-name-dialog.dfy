include "M02-race-import.dfy"

// Shared LibraryNameDlg; actual window/text/pointer/geometry extraction is S05.
module M01LibraryNameDialog {
  import S = S01
  import C = S01Codecs
  import K = S01LibraryNames
  import N = M02NativeRaceState
  import A = M02RaceAllocation
  import R = M02RaceActions
  import I = M02RaceImport
  function BackBoundary(original: S.Bytes,n: nat): nat
    requires n<=|original|
    ensures BackBoundary(original,n)<=n
    decreases n
  { if n==0 || n==|original| || original[n]/64!=2 then n else BackBoundary(original,n-1) }
  function GameBudget(root: nat): int { C.DivC(256-(root as int)-19,2) }
  function GameCandidate(original: S.Bytes,root: nat,i: nat): S.Bytes {
    var suffix:=[32,40]+A.Decimal(i)+[41];
    var room:=GameBudget(root)-|suffix|;var bounded:=if room<0 then 0 else room as nat;
    var count:=if |original|<bounded then |original| else bounded;
    original[..BackBoundary(original,count)]+suffix
  }
  datatype SearchStatus = Available | Exhausted | Pending
  datatype SearchEvent = Query(name: S.Bytes,free: bool) | Format(name: S.Bytes)
  datatype Search = Search(status: SearchStatus,buffer: S.Bytes,used: nat,events: seq<SearchEvent>)
  function GameLoop(original: S.Bytes,buffer: S.Bytes,root: nat,i: nat,replies: seq<bool>): Search
    requires |buffer|==256 && 2<=i<=10000
    ensures |GameLoop(original,buffer,root,i,replies).buffer|==256
    ensures GameLoop(original,buffer,root,i,replies).used<=|replies|
    ensures GameLoop(original,buffer,root,i,replies).used<=10000-i
    decreases 10000-i
  {
    if i==10000 then Search(Exhausted,buffer,0,[]) else
    if |replies|==0 then Search(Pending,buffer,0,[]) else
    var current:=N.Text(buffer);var prefix:=[Query(current,replies[0])];
    if replies[0] then Search(Available,buffer,1,prefix) else
    var candidate:=GameCandidate(original,root,i);var formatted:=I.Format(buffer,candidate);
    var next:=GameLoop(original,formatted,root,i+1,replies[1..]);
    next.(used:=next.used+1,events:=prefix+[Format(N.Text(formatted))]+next.events)
  }
  function SuggestGame(buffer: S.Bytes,root: nat,replies: seq<bool>): Search
    requires |buffer|==256
  { GameLoop(N.Text(buffer),buffer,root,2,replies) }
  datatype State = State(output: S.Bytes,edit: S.Bytes,open: bool,result: int,race: S.I16)
  datatype CommandEvent = End(okay: bool) | GetOutputPointer | ReadText | Portable(race: bool)
    | Free(race: bool,okay: bool) | SuggestRace(names: seq<S.Bytes>) | SuggestGameNames(events: seq<SearchEvent>)
    | SetEdit | CopyOutput | Body | CopyBody | Caption | Message
  datatype Error = NoError | InvalidName | NameTaken
  datatype Status = Returned(handled: bool) | Suspended | InvalidEvidence
  datatype Result = Result(status: Status,state: State,error: Error,calls: seq<CommandEvent>)
  function Unhandled(s: State): Result { Result(Returned(false),s,NoError,[]) }
  function Command(s: State,id: S.U16,read: S.Bytes,root: S.Bytes,free: bool,
    raceFree: set<nat>,gameFree: seq<bool>): Result
    requires |s.output|==256 && s.open
  {
    if id==2 then Result(Returned(true),s.(open:=false,result:=0),NoError,[End(false)]) else
    if id!=1 then Result(Returned(false),s,NoError,[]) else
    var prefix:=[GetOutputPointer,ReadText];
    if |read|>=256 || 0 in read then Result(InvalidEvidence,s,NoError,prefix) else
    var race:=s.race!=0;var valid:=if race then R.Filename(read,root) else K.FGameName(read,|root|,256);
    var checked:=prefix+[Portable(race)];
    if !valid then Result(Returned(true),s,InvalidName,checked+[Body,CopyBody,Caption,Message]) else
    var queried:=checked+[Free(race,free)];
    if free then Result(Returned(true),s.(output:=N.CopyString(s.output,read),open:=false,result:=1),NoError,queried+[CopyOutput,End(true)]) else
    var buffer:=I.Format(s.output,read);
    if race then
      var suggestion:=A.Suggest(read,raceFree);var names:=I.NameWrites(buffer,suggestion.calls);
      var text:=if |suggestion.name|==0 then [] else N.Text(names);
      Result(Returned(true),s.(edit:=text),NameTaken,queried+[SuggestRace(suggestion.calls),SetEdit,Body,CopyBody,Caption,Message])
    else
      var suggestion:=SuggestGame(buffer,|root|,gameFree);
      if suggestion.status==Pending then Result(Suspended,s,NoError,queried+[SuggestGameNames(suggestion.events)]) else
      Result(Returned(true),s.(edit:=N.Text(suggestion.buffer)),NameTaken,
        queried+[SuggestGameNames(suggestion.events),SetEdit,Body,CopyBody,Caption,Message])
  }
  datatype Rect = Rect(left: int,top: int,right: int,bottom: int)
  predicate I32(n: int) { -2147483648<=n<=2147483647 }
  predicate Fits(r: Rect) { I32(r.left) && I32(r.top) && I32(r.right) && I32(r.bottom) }
  datatype Point = Point(x: int,y: int)
  function Position(work: Rect,owner: Rect,dialog: Rect): Point {
    var width:=dialog.right-dialog.left;var height:=dialog.bottom-dialog.top;
    var x:=owner.left+C.DivC(owner.right-owner.left-width,2);
    var y:=owner.top+C.DivC(owner.bottom-owner.top-height,2);
    Point(if x<work.left then work.left else if x>work.right-width then (if work.right-width<work.left then work.left else work.right-width) else x,
      if y<work.top then work.top else if y>work.bottom-height then (if work.bottom-height<work.top then work.top else work.bottom-height) else y)
  }
  predicate GeometrySafe(work: Rect,owner: Rect,dialog: Rect) {
    Fits(work) && Fits(owner) && Fits(dialog) &&
    I32(dialog.right-dialog.left) && I32(dialog.bottom-dialog.top) &&
    I32(owner.right-owner.left) && I32(owner.bottom-owner.top) &&
    I32(owner.right-owner.left-(dialog.right-dialog.left)) && I32(owner.bottom-owner.top-(dialog.bottom-dialog.top)) &&
    I32(owner.left+C.DivC(owner.right-owner.left-(dialog.right-dialog.left),2)) &&
    I32(owner.top+C.DivC(owner.bottom-owner.top-(dialog.bottom-dialog.top),2)) &&
    I32(work.right-(dialog.right-dialog.left)) && I32(work.bottom-(dialog.bottom-dialog.top))
  }
  datatype InitEvent = WorkArea | ScreenWidth | ScreenHeight | Parent | Visible | OwnerRect | DialogRect
    | Move(point: Point) | BindPointer | Limit(bytes: nat) | SetInitialText
  datatype Initialization = Initialization(safe: bool,point: Point,calls: seq<InitEvent>)
  function Initialize(workOkay: bool,work: Rect,width: int,height: int,visible: bool,ownerOkay: bool,owner: Rect,
    dialogWritten: bool,dialog: Rect,widthFirst: bool): Initialization {
    var actualWork:=if workOkay then work else Rect(0,0,width,height);
    var actualOwner:=if visible && ownerOkay then owner else actualWork;
    var prefix:=[WorkArea]+(if workOkay then [] else if widthFirst then [ScreenWidth,ScreenHeight] else [ScreenHeight,ScreenWidth])+[Parent,Visible]+
      (if visible then [Parent,OwnerRect] else [])+[DialogRect];
    if !dialogWritten || !GeometrySafe(actualWork,actualOwner,dialog) then Initialization(false,Point(0,0),prefix) else
    var point:=Position(actualWork,actualOwner,dialog);
    Initialization(true,point,prefix+[Move(point),BindPointer,Limit(255),SetInitialText])
  }
  lemma InvalidOrCollisionKeepsOutput(s: State,id: S.U16,read: S.Bytes,root: S.Bytes,free: bool,race: set<nat>,game: seq<bool>)
    requires |s.output|==256 && s.open && !free
    ensures Command(s,id,read,root,free,race,game).state.output==s.output
  {}
  lemma CancelLeavesBuffers(s: State,read: S.Bytes,root: S.Bytes,free: bool,race: set<nat>,game: seq<bool>)
    requires |s.output|==256 && s.open
    ensures Command(s,2,read,root,free,race,game).state.output==s.output
    ensures Command(s,2,read,root,free,race,game).state.edit==s.edit
    ensures Command(s,2,read,root,free,race,game).state.result==0
  {}
  lemma FinalGameCandidateNotProbed(buffer: S.Bytes,root: nat)
    requires |buffer|==256
    ensures GameLoop(N.Text(buffer),buffer,root,9999,[false]).status==Exhausted
    ensures GameLoop(N.Text(buffer),buffer,root,9999,[false]).used==1
    ensures |GameLoop(N.Text(buffer),buffer,root,9999,[false]).events|==2
  {}
  lemma PositionLowerBounds(work: Rect,owner: Rect,dialog: Rect)
    ensures Position(work,owner,dialog).x>=work.left && Position(work,owner,dialog).y>=work.top
  {}
}
