include "S06-shared-state.dfy"
module M16Tutorial {
  function Page(fragment: nat): nat { fragment/8+1 }
  function Advance(fragment: nat,fTaskDone: bool,fTurnDone: bool): nat { if fTaskDone && !fTurnDone then fragment+8 else fragment }
  predicate Finished(fragment: nat,fFinished: bool) { fragment>=640 || fFinished }
  predicate ExitWithoutPrompt(turn: nat) { turn>=30 }
  datatype Help = Index | Context(id: nat) | Unsupported
  // document means the validated packaged HTML body was successfully exported
  // with one selected-context marker; the legacy HLP is not a runtime input.
  predicate CanOpen(request: Help,contexts: set<nat>,document: bool,assets: bool,stamp: bool,launch: bool) {
    document && assets && stamp && launch && (request.Index? || (request.Context? && request.id in contexts))
  }
  lemma PendingTaskStays(n: nat,t: bool) ensures Advance(n,false,t)==n {}
  lemma {:isolate_assertions} AcceptanceVectors()
    ensures Page(0)==1 && Page(632)==80 && Advance(0,true,false)==8
    ensures Advance(0,true,true)==0 && Finished(640,false)
    ensures !CanOpen(Context(13002),{4501,1070},true,true,true,true)
    ensures CanOpen(Index,{},true,true,true,true) && !CanOpen(Index,{},true,false,true,true)
  {}
}
