include "M09-presets.dfy"
include "M07-adjacent.dfy"

// Native control flow, with explicit callbacks. Step input after an event is
// the actual post-call state, not necessarily the preceding output state.
// Nonzero handles denote live allocations/windows only when the adapter proves
// this. The model does not bless aliasing, dangling pointers or reentrant frees.
module M09Lifecycle {
  import R = M09Presets
  import N = M07Adjacent
  import C = S01Codecs
  datatype State = State(selected: nat,working: nat,count: nat,research: bool,saved: bool,
    id: int,kind: int,ai: bool,tutorial: bool,player: int,progress: bool,
    dialog: nat,up: bool,environment: nat)
  datatype Event = Quiet | Free(handle: nat) | WriteBack | Reload(id: int) | Fill | Redraw
    | AdvanceTutorial | Finish(write: bool) | DrawStats | Return(value: bool)
    | EndDialog(handle: nat,result: int) | AlertMemory | Select(delta: int,id: int)
    | Initialize(reuseCatalog: bool) | Controls | ClientRect | Invalidate
    | StickyPosition
  datatype Phase = BeginFinish | AfterOldFree | AfterEmptyFree | Install | AfterWrite
    | AfterReload | AfterFill | FinalClear | FinishDone
  datatype Transition = Transition(state: State,phase: Phase,event: Event)
  function Step(s: State,p: Phase,write: bool): Transition {
    match p
    case BeginFinish =>
      if write then Transition(s,AfterOldFree,Free(s.selected))
      else Transition(s.(research:=s.saved),FinalClear,Free(s.working))
    case AfterOldFree =>
      if s.working!=0 && s.count==0 then Transition(s,AfterEmptyFree,Free(s.working))
      else Transition(s,Install,Quiet)
    case AfterEmptyFree => Transition(s.(working:=0),Install,Quiet)
    case Install => Transition(s.(selected:=s.working,working:=0),AfterWrite,WriteBack)
    case AfterWrite => Transition(s,AfterReload,Reload(s.id))
    case AfterReload => if !s.ai then Transition(s,AfterFill,Fill) else Transition(s,FinalClear,Quiet)
    case AfterFill => Transition(s,FinalClear,Redraw)
    case FinalClear =>
      var cleared:=s.(working:=0);
      if s.tutorial && s.player==0 then Transition(cleared.(progress:=true),FinishDone,AdvanceTutorial)
      else Transition(cleared,FinishDone,Quiet)
    case FinishDone => Transition(s,FinishDone,Quiet)
  }
  datatype ExitPhase = BeforeFinish | AfterFinish | RestoreEnvironment | ExitDone
  datatype Exit = Exit(state: State,phase: ExitPhase,event: Event)
  // DialogBox returns through int16_t fSuccess. Direct Clear assigns 1 and
  // bypasses initialization, modal cleanup and the snapshot of research.
  function Accepted(clear: bool,raw: int): bool { clear || R.OuterDialogResult(raw)!=0 }
  function ExitStep(s: State,p: ExitPhase,clear: bool,raw: int,priorEnvironment: nat): Exit {
    var accept:=Accepted(clear,raw);
    match p
    case BeforeFinish =>
      Exit(if clear then s else s.(dialog:=0,up:=false),AfterFinish,Finish(accept))
    case AfterFinish => Exit(s,RestoreEnvironment,if accept && s.kind==1 then DrawStats else Quiet)
    case RestoreEnvironment => Exit(s.(environment:=priorEnvironment),ExitDone,Return(true))
    case ExitDone => Exit(s,ExitDone,Quiet)
  }
  datatype RecoveryPhase = BeginRecovery | AfterFree | AfterEnd | AfterAlert | RecoveryDone
  datatype Recovery = Recovery(state: State,phase: RecoveryPhase,event: Event)
  function Recover(s: State,p: RecoveryPhase,priorEnvironment: nat): Recovery {
    match p
    case BeginRecovery => Recovery(s,AfterFree,if s.working!=0 then Free(s.working) else Quiet)
    case AfterFree => Recovery(s.(working:=0),AfterEnd,if s.dialog!=0 then EndDialog(s.dialog,0) else Quiet)
    case AfterEnd => Recovery(s.(dialog:=0,up:=false),AfterAlert,AlertMemory)
    case AfterAlert => Recovery(s.(environment:=priorEnvironment),RecoveryDone,Return(false))
    case RecoveryDone => Recovery(s,RecoveryDone,Quiet)
  }
  datatype NavigationPhase = Commit | Choose | Init | InitControls | ReadRect | Paint | NavigationDone
  datatype Navigation = Navigation(phase: NavigationPhase,event: Event)
  function Navigate(s: State,p: NavigationPhase,next: bool,shift: bool,bases: seq<N.Base>): Navigation {
    match p
    case Commit => Navigation(Choose,Finish(true))
    case Choose => Navigation(Init,if shift then Select(0,N.Target(bases,s.id,next)) else Select(if next then 1 else -1,0))
    case Init => Navigation(InitControls,Initialize(true))
    case InitControls => Navigation(ReadRect,Controls)
    case ReadRect => Navigation(Paint,ClientRect)
    case Paint => Navigation(NavigationDone,Invalidate)
    case NavigationDone => Navigation(NavigationDone,Quiet)
  }
  function CommandClose(s: State,accept: bool): State { s.(dialog:=0) }
  function CloseEvents(dialog: nat,accept: bool): seq<Event> {
    [StickyPosition,EndDialog(dialog,if accept then 1 else 0)]
  }
  // The last nonautomatic queue row is selected with the header offset; if
  // none exists, selection 0 is requested. This is a request, not GUI success.
  function QueueSelection(rows: seq<R.Row>): nat
    ensures QueueSelection(rows)<=|rows|
    ensures QueueSelection(rows)==0 <==> (forall r <- rows :: R.Automatic(r))
    ensures QueueSelection(rows)>0 ==> !R.Automatic(rows[QueueSelection(rows)-1])
    ensures forall i | QueueSelection(rows)<=i<|rows| :: R.Automatic(rows[i])
    decreases |rows|
  {
    if |rows|==0 then 0 else
    var tail:=QueueSelection(rows[1..]);
    if tail!=0 then tail+1 else if !R.Automatic(rows[0]) then 1 else 0
  }
  lemma CancelKeepsSelected(s: State)
    ensures Step(s,BeginFinish,false).state.selected==s.selected
    ensures Step(s,BeginFinish,false).state.research==s.saved
    ensures Step(s,BeginFinish,false).event==Free(s.working)
  {}
  lemma InstallTransfers(s: State)
    ensures Step(s,Install,true).state.working==0
    ensures Step(s,Install,true).state.selected==s.working
    ensures Step(s,Install,true).event==WriteBack
  {}
  lemma FailureDoesNotRestoreResearch(s: State,p: RecoveryPhase,prior: nat)
    ensures Recover(s,p,prior).state.research==s.research
    ensures Recover(s,p,prior).state.selected==s.selected
  {}
  lemma OrdinaryReturnDoesNotSignalAcceptance(s: State,clear: bool,raw: int,prior: nat)
    ensures ExitStep(s,RestoreEnvironment,clear,raw,prior).event==Return(true)
  {}
  lemma NavigationCommitsFirst(s: State,next: bool,shift: bool,bases: seq<N.Base>)
    ensures Navigate(s,Commit,next,shift,bases).event==Finish(true)
    ensures Navigate(s,Commit,next,shift,bases).phase==Choose
  {}
  lemma {:isolate_assertions} {:fuel QueueSelection,5} Vectors() {
    var s:=State(10,20,2,true,false,4,1,false,true,0,false,30,true,40);
    assert Step(s,BeginFinish,true).event==Free(10);
    assert Step(s.(count:=0),AfterOldFree,true).event==Free(20);
    assert Step(s.(working:=0),AfterOldFree,true).phase==Install;
    assert Step(s,AfterWrite,true).event==Reload(4);
    assert Step(s.(id:=9),AfterWrite,true).event==Reload(9);
    assert Step(s,AfterReload,true).event==Fill;
    assert Step(s.(ai:=true),AfterReload,true).phase==FinalClear;
    assert Step(s,FinalClear,false).event==AdvanceTutorial;
    assert Step(s,FinalClear,false).state.progress && Step(s,FinalClear,false).state.working==0;
    assert Step(s.(player:=1),FinalClear,true).event==Quiet;
    assert C.Pow2(4)==16 && C.Pow2(8)==256 && C.Pow2(12)==4096;
    assert C.Pow2(15)==32768 && C.Pow2(16)==65536;
    assert !Accepted(false,0) && !Accepted(false,65536) && Accepted(false,-1) && Accepted(true,0);
    assert ExitStep(s,BeforeFinish,true,0,99).state==s;
    assert ExitStep(s,BeforeFinish,false,0,99).state.dialog==0;
    assert ExitStep(s.(kind:=2),AfterFinish,false,1,99).event==Quiet;
    assert Recover(s,AfterFree,99).event==EndDialog(30,0);
    assert Recover(s,AfterEnd,99).state.working==s.working; // cleared earlier, not again
    assert Recover(s,AfterAlert,99).state.environment==99;
    assert Navigate(s,Choose,false,false,[]).event==Select(-1,0);
    assert Navigate(s,Choose,true,true,[]).event==Select(0,-1);
    assert Navigate(s,Init,true,true,[]).event==Initialize(true);
    var auto:=R.Row(R.O.Prod(1,0,1,0),0);
    var manual:=R.Row(R.O.Prod(1,7,1,0),0);
    assert QueueSelection([auto,manual,auto])==2 && QueueSelection([auto,auto])==0;
  }
}
