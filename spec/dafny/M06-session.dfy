include "S06-state-extensions.dfy"
include "S01-external.dfy"

module M06Session {
  import opened SharedGameState
  import E = S06Extensions
  import S = S01
  import X = S01External

  // Tail of FOpenGame after loading and creating children. A cleared saved
  // marker is treated as handled; no successful main-object callback is tested.
  datatype OpenPresentation = OpenPresentation(clearMarker: bool,findSomething: bool,hostDialog: bool,returned: int)
  function FinishOpenPresentation(player: bool,initialKind: int,afterKind: int,planets: nat): OpenPresentation {
    if !player then OpenPresentation(false,false,true,0)
    else if initialKind!=0 && afterKind==0 then OpenPresentation(false,false,false,1)
    else OpenPresentation(true,planets!=0,false,1)
  }
  lemma {:isolate_assertions} OpenPresentationVectors() {
    assert FinishOpenPresentation(true,2,0,0)==OpenPresentation(false,false,false,1);
    assert FinishOpenPresentation(true,0,0,3)==OpenPresentation(true,true,false,1);
    assert FinishOpenPresentation(true,2,2,3)==OpenPresentation(true,true,false,1);
    assert FinishOpenPresentation(true,0,0,0)==OpenPresentation(true,false,false,1);
    assert FinishOpenPresentation(false,2,0,3)==OpenPresentation(false,false,true,0);
  }

  // S06's fSubmitted is the in-memory gd.fSubmit intent here, not an assertion
  // about a durable BOF or host receipt. Commands are already prepared by editors.
  datatype Choice = Save | Discard | Submit | Cancel
  function DialogChoice(iRet: int): Choice {
    if iRet == 1 then Save else if iRet == -1 then Submit
    else if iRet == 0 || iRet == -2 then Cancel else Discard
  }
  predicate FNeedsPrompt(fPlayer: bool,s: E.SessionState) { fPlayer && s.fDirty }
  datatype SaveTrace = SaveTrace(state: E.SessionState,fLeave: bool,
    fOrdersAttempt: bool,fHistoryAttempt: bool,completedDone: Option<bool>)

  // Outcomes are returned writer results. A failed direct write may leave a
  // truncated file: None means no completed orders publication is established.
  // Writer-side production-template logging is an external preparation step.
  function GuardedLeave(fPlayer: bool,s: E.SessionState,c: Choice,
    fOrders: bool,fHistory: bool): SaveTrace {
    if !FNeedsPrompt(fPlayer,s) then SaveTrace(s,true,false,false,None)
    else if c == Cancel then SaveTrace(s,false,false,false,None)
    else if c == Discard then SaveTrace(s.(fDirty:=false),true,false,false,None)
    else SaveTrace(
      if fOrders && fHistory then s.(fDirty:=false,fSubmitted:=c==Submit)
      else s.(fDirty:=true),
      fOrders && fHistory,true,fOrders,if fOrders then Some(c==Submit) else None)
  }
  // Direct menu save and PromptSaveGame attempt history even after orders fail.
  function DirectSave(s: E.SessionState,fSubmit: bool,fOrders: bool): SaveTrace {
    SaveTrace(s.(fDirty:=if fOrders then false else s.fDirty,fSubmitted:=fSubmit),
      false,true,true,if fOrders then Some(fSubmit) else None)
  }
  // PromptSaveGame has no cancellation result for DestroyCurGame to honor.
  function LegacyPrompt(s: E.SessionState,c: Choice,fOrders: bool): SaveTrace {
    if c in {Save,Submit} then DirectSave(s,c==Submit,fOrders).(fLeave:=true)
    else SaveTrace(s,true,false,false,None)
  }

  function LogBytes(commands: seq<S.Record>): nat
    decreases |commands|
  { if |commands|==0 then 0 else 2+|commands[0].rgb|+LogBytes(commands[1..]) }
  predicate FLogWarning(cbExisting: nat,cbPayload: nat) { cbExisting+cbPayload+2>32000 }
  function Append(s: E.SessionState,r: S.Record,fLogOff: bool): E.SessionState {
    if fLogOff then s else s.(commands:=s.commands+[r],fDirty:=true)
  }
  // This is a proposed safety boundary, stronger than WriteMemRt's warning.
  predicate FBoundedLog(commands: seq<S.Record>) {
    LogBytes(commands)<=32000 && forall r <- commands :: |r.rgb|<=1023
  }
  // Replay attempts every record, even following a false result. Command state
  // effects/coalescing remain S01Orders; this projects only the aggregate result.
  function ReplaySucceeded(results: seq<bool>): bool
    decreases |results|
  { |results|==0 || (results[0] && ReplaySucceeded(results[1..])) }

  datatype LogDisposition = Ignore | Replay | Corrupt
  function LoadedLog(fOpened: bool,fReadError: bool,fHeader285: bool,
    lidFile: U32,turnFile: Turn,genFile: int,s: E.SessionState,gen: int,
    fComplete: bool): LogDisposition {
    if !fOpened then Ignore else if fReadError then Corrupt
    else if !fHeader285 || lidFile!=s.game || turnFile!=s.turn || genFile!=gen then Ignore
    else if !fComplete then Corrupt else Replay
  }
  // FLoadLogFile does not itself check BOF player/kind/done; the stronger
  // typed boundary reuses S06 Matches and separately checks generation.
  predicate FOrdersForSession(s: E.SessionState,e: OrderEnvelope,genFile: int,gen: int) {
    Matches(e,s.game,s.player,s.turn) && genFile==gen
  }
  predicate FLoadCompletes(fUniverse: bool,fSnapshot: bool,fPassword: bool,
    log: LogDisposition,fReplay: bool) {
    fUniverse && fSnapshot && fPassword && log!=Corrupt && (log==Replay ==> fReplay)
  }
  function PasswordAllowed(salt: int,last: int,fAi: bool,fDefaultPresent: bool,
    defaultSalt: int,fValidate: bool,fPromptAccepted: bool): bool {
    salt==0 || salt==last || fAi || (fDefaultPresent && defaultSalt==salt) ||
      (!fValidate && fPromptAccepted)
  }
  function PasswordDelay(cFailures: nat): int {
    if cFailures<10 then 1000 else if cFailures<100 then 5000 else 10000
  }
  predicate FCachePassword(fAccepted: bool,fHotSeat: bool) { fAccepted && !fHotSeat }
  predicate FChangePassword(fLoaded: bool,fPlayer: bool,fSingle: bool,salt: int,fAuthenticated: bool) {
    fLoaded && fPlayer && !(fSingle && salt<=0) && fAuthenticated
  }
  predicate FPasswordConfirmed(fReadFirst: bool,fReadSecond: bool,hashFirst: int,hashSecond: int) {
    fReadFirst && fReadSecond && hashFirst==hashSecond
  }
  predicate FNewTurn(fOpened: bool,current: Turn,incoming: Turn) { fOpened && incoming>current }
  datatype ContinueAction = Unavailable | OpenRemembered | ChoosePlayer
  function Continue(fGame: bool,fTurn: bool): ContinueAction {
    if !fGame then Unavailable else if fTurn then OpenRemembered else ChoosePlayer
  }
  predicate FSelectPlayer(iplr: int,available: set<PlayerId>) { 0<=iplr<16 && iplr in available }
  function PlayerSuffix(iplr: PlayerId): int { iplr+1 }

  datatype ExportStage = CancelBeforeSave | SaveFailed | SavedCopyFailed | Exported
  function ExportFlags(s: E.SessionState,stage: ExportStage): E.SessionState {
    if stage in {SavedCopyFailed,Exported} then s.(fDirty:=false) else s
  }
  // File contents/restoration, including recovery failure, use the existing
  // publication relation rather than promising an atomic multi-file save.
  predicate FPublication(a: X.Publication,e: X.Event,b: X.Publication) { X.FTransition(a,e,b) }
  predicate FImport(prior: S.ManagedGame,next: S.ManagedGame,incoming: S.Bof,
    iplr: S.PlayerId,choice: X.ImportChoice) { X.FImportEffect(prior,next,incoming,iplr,choice) }
  datatype ImportAction = Identical | First | Later | Conflict
  function ClassifyImport(fExisting: bool,fEqual: bool,current: Turn,incoming: Turn): ImportAction {
    if !fExisting then First else if fEqual then Identical else if incoming>current then Later else Conflict
  }
  // Existing-turn menu path: history is attempted BEFORE the discard prompt.
  datatype NewTurnAction = Stay | Reload | SubmitAndWait | Inapplicable
  function WaitAction(fLoaded: bool,fPlayer: bool,fSingle: bool,fNew: bool,
    fDirty: bool,fAcceptDiscard: bool): NewTurnAction {
    if !fLoaded || !fPlayer || fSingle then Inapplicable
    else if !fNew then SubmitAndWait else if fDirty && !fAcceptDiscard then Stay else Reload
  }
  predicate FHistoryBeforeReloadPrompt(fLoaded: bool,fPlayer: bool,fSingle: bool,fNew: bool) {
    fLoaded && fPlayer && !fSingle && fNew
  }
  // Timer outcomes are externally scheduled; no fairness or eventual-arrival claim.
  function WaitTimerMilliseconds(): int { 10000 }
  function ReadyTimerMilliseconds(): int { 1000 }

  lemma CancelPreservesSession(s: E.SessionState,o: bool,h: bool)
    requires s.fDirty
    ensures GuardedLeave(true,s,Cancel,o,h)==SaveTrace(s,false,false,false,None)
  {}
  lemma GuardedFailureStays(s: E.SessionState,c: Choice,o: bool,h: bool)
    requires s.fDirty && c in {Save,Submit} && !(o && h)
    ensures !GuardedLeave(true,s,c,o,h).fLeave
    ensures GuardedLeave(true,s,c,o,h).state==s
    ensures !o ==> !GuardedLeave(true,s,c,o,h).fHistoryAttempt
  {}
  lemma SaveNeedsBoth(s: E.SessionState,c: Choice,o: bool,h: bool)
    requires s.fDirty && c in {Save,Submit}
    ensures GuardedLeave(true,s,c,o,h).fLeave <==> o && h
  {}
  lemma ExportPreservesIntent(s: E.SessionState,stage: ExportStage)
    ensures ExportFlags(s,stage).fSubmitted==s.fSubmitted
    ensures ExportFlags(s,stage).turn==s.turn && ExportFlags(s,stage).player==s.player
  {}
  lemma LoggingDisabledPreserves(s: E.SessionState,r: S.Record)
    ensures Append(s,r,true)==s
  {}
  lemma LaterNeverSameYear(fOpened: bool,t: Turn)
    ensures !FNewTurn(fOpened,t,t)
  {}
  lemma WrongPlayerRejected(s: E.SessionState,e: OrderEnvelope,g: int)
    requires e.iplr!=s.player
    ensures !FOrdersForSession(s,e,g,g)
  {}
  lemma SessionVectors() {
    var s := E.SessionState(42,10,1,true,false,false,false,[],0);
    assert DialogChoice(-2)==Cancel && DialogChoice(-1)==Submit;
    assert !GuardedLeave(true,s,Submit,true,false).fLeave;
    assert GuardedLeave(true,s,Submit,true,false).completedDone==Some(true);
    assert !GuardedLeave(true,s,Submit,true,false).state.fSubmitted;
    assert GuardedLeave(true,s,Save,false,true).fHistoryAttempt==false;
    assert DirectSave(s,true,false).fHistoryAttempt;
    assert !DirectSave(s,true,true).state.fDirty;
    assert LegacyPrompt(s,Cancel,false).fLeave;
    assert !FLogWarning(31998,0) && FLogWarning(31999,0);
    assert !ReplaySucceeded([true,false,true]);
    assert LoadedLog(true,false,true,42,9,1,s,1,true)==Ignore;
    assert LoadedLog(true,false,true,42,10,1,s,1,false)==Corrupt;
    assert LoadedLog(true,false,true,42,10,1,s,1,true)==Replay;
    assert !PasswordAllowed(9,8,false,false,9,true,true);
    assert PasswordDelay(9)==1000 && PasswordDelay(10)==5000;
    assert PasswordDelay(99)==5000 && PasswordDelay(100)==10000;
    assert !FCachePassword(true,true);
    assert !FChangePassword(true,true,true,0,true);
    assert Continue(true,false)==ChoosePlayer;
    assert ClassifyImport(true,false,10,12)==Later;
    assert ClassifyImport(true,false,10,10)==Conflict;
    assert ClassifyImport(false,false,10,9)==First;
    assert !ExportFlags(s,SavedCopyFailed).fDirty;
    assert WaitAction(true,true,false,true,true,false)==Stay;
    assert FHistoryBeforeReloadPrompt(true,true,false,true);
  }
}
