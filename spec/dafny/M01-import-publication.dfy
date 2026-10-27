include "M01-library.dfy"

// Source controllers after admission/choice. Filesystem replies are explicit
// S04 outcomes, not assertions of crash atomicity or complete helper refinement.
module M01ImportPublication {
  import S = S01
  import G = SharedGameState
  import L = M01Library
  type Path = nat
  datatype Artifact = Artifact(directory: bool,cells: map<nat,S.Bytes>)
  type Store = map<Path,Artifact>
  datatype Operation = Probe(tag: nat,path: Path) | Format(tag: nat)
    | Copy(source: Path,target: Path) | Replace(source: Path,target: Path)
    | Remove(path: Path,tree: bool) | Build(tag: nat,path: Path) | Allocate(tag: nat,path: Path)
  datatype Program = Return(value: S.I16) | Notice(message: nat,next: Program) | ClearPending(next: Program)
    | Ask(operation: Operation,yes: Program,no: Program)
  datatype Reply = Reply(ok: bool,residue: G.Option<Artifact>,environment: Store)
  datatype Event = Called(operation: Operation,ok: bool) | Notified(message: nat) | PendingCleared
  datatype Status = Returned(value: S.I16) | Pending(operation: Operation) | InvalidEvidence
  datatype Result = Result(status: Status,store: Store,used: nat,events: seq<Event>)
  function Overlay(before: Store,changes: Store): Store { before+changes }
  function Erase(store: Store,path: Path): Store { map p | p in store && p!=path :: store[p] }
  function Residue(store: Store,path: Path,value: G.Option<Artifact>): Store {
    if value.Some? then store[path:=value.value] else store
  }
  // A failed helper can leave a partial stage/cleanup result. Its extent is
  // recorded at that path; it cannot silently rewrite unrelated paths here.
  function Valid(store: Store,op: Operation,r: Reply): bool {
    match op
    case Copy(s,t) => (r.ok && s in store && !store[s].directory && t !in store) ||
      (!r.ok && (r.residue.None? || (s in store && !store[s].directory && t !in store && !r.residue.value.directory &&
        store[s].cells.Keys=={0} && r.residue.value.cells.Keys=={0} &&
        |r.residue.value.cells[0]|<=|store[s].cells[0]| && r.residue.value.cells[0]==store[s].cells[0][..|r.residue.value.cells[0]|])))
    case Replace(s,t) => !r.ok || s in store
    case Remove(p,tree) => r.residue.None? || (!r.ok && p in store &&
      r.residue.value.directory==store[p].directory &&
      (forall k <- r.residue.value.cells.Keys :: k in store[p].cells && r.residue.value.cells[k]==store[p].cells[k]))
    case Build(_,p) => r.residue.Some? && r.residue.value.directory
    case Allocate(_,p) => !r.ok || p !in store
    case Probe(tag,p) => r.residue.None? && (!r.ok || tag !in {1,2,3,5} ||
      (p in store && store[p].directory==(tag in {3,5})))
    case Format(_) => r.residue.None?
  }
  function Apply(store: Store,op: Operation,r: Reply): Store
    requires Valid(store,op,r)
  {
    match op
    case Copy(s,t) => if r.ok then store[t:=store[s]] else Residue(store,t,r.residue)
    case Replace(s,t) => if !r.ok || s==t then store else Erase(store,s)[t:=store[s]]
    case Remove(p,_) => if r.ok then Erase(store,p) else Residue(store,p,r.residue)
    case Build(_,p) => store[p:=r.residue.value]
    case Allocate(_,p) => if r.ok then store[p:=Artifact(true,map[])] else store
    case _ => store
  }
  function Run(program: Program,store: Store,replies: seq<Reply>): Result
    ensures Run(program,store,replies).used<=|replies|
    decreases program
  {
    match program
    case Return(v) => Result(Returned(v),store,0,[])
    case Notice(m,next) => var result:=Run(next,store,replies);result.(events:=[Notified(m)]+result.events)
    case ClearPending(next) => var result:=Run(next,store,replies);result.(events:=[PendingCleared]+result.events)
    case Ask(op,yes,no) =>
      if |replies|==0 then Result(Pending(op),store,0,[]) else
      var r:=replies[0];var seen:=Overlay(store,r.environment);
      if !Valid(seen,op,r) then Result(InvalidEvidence,seen,1,[]) else
      var result:=Run(if r.ok then yes else no,Apply(seen,op,r),replies[1..]);
      result.(used:=result.used+1,events:=[Called(op,r.ok)]+result.events)
  }
  function Ignore(op: Operation,next: Program): Program { Ask(op,next,next) }
  function CheckFormat(tag: nat,next: Program): Program { Ask(Format(tag),next,Return(0)) }
  // Numeric tags distinguish actual source calls; notices are normalized IDs.
  // 1 stage/header, 2 install, 3 preserve, 4 restore, 5 retire, 6 replace,
  // 7 cleanup warning, 8 separate copy, 9 family import failure.
  datatype Paths = Paths(source: Path,target: Path,orders: Path,stage: Path,previous: Path,
    directory: Path,hold: Path,pending: Path)
  function StageFailure(p: Paths): Program { Ignore(Remove(p.stage,false),Notice(1,Return(0))) }
  function InstallFailure(p: Paths,prior: bool): Program {
    Ignore(Remove(p.stage,false),if prior then Ignore(Remove(p.previous,false),Notice(2,Return(0))) else Notice(2,Return(0)))
  }
  function RetireFailed(p: Paths): Program {
    Ask(Replace(p.previous,p.target),Ignore(Remove(p.previous,false),Notice(5,Return(0))),Notice(4,Return(0)))
  }
  function InstallPrior(p: Paths,ordersSeen: bool): Program {
    Ask(Copy(p.target,p.previous),
      Ask(Replace(p.stage,p.target),
        if ordersSeen then Ask(Remove(p.orders,false),Ignore(Remove(p.previous,false),Return(1)),RetireFailed(p))
        else Ignore(Remove(p.previous,false),Return(1)),InstallFailure(p,true)),
      Ignore(Remove(p.stage,false),Notice(3,Return(0))))
  }
  function Later(p: Paths,hasPrior: bool): Program {
    var installed:=if !hasPrior then Ask(Replace(p.stage,p.target),Return(1),InstallFailure(p,false)) else
      Ask(Probe(1,p.orders),InstallPrior(p,true),InstallPrior(p,false));
    var stage:=Ask(Copy(p.source,p.stage),Ask(Probe(2,p.stage),installed,StageFailure(p)),StageFailure(p));
    CheckFormat(1,CheckFormat(2,CheckFormat(3,stage)))
  }
  function ReplaceDone(p: Paths,held: bool,restoreFailed: bool): Program {
    var message:=if restoreFailed then 4 else 6;
    var end:=if held then Ask(Replace(p.hold,p.directory),Notice(6,Return(0)),Notice(4,Return(0))) else Notice(message,Return(0));
    Ask(Probe(3,p.stage),Ignore(Remove(p.stage,true),end),end)
  }
  function ConflictPublish(p: Paths): Program {
    Ask(Replace(p.stage,p.directory),
      Ask(Remove(p.hold,true),Return(1),Notice(7,Return(1))),
      Ask(Replace(p.hold,p.directory),ReplaceDone(p,false,false),ReplaceDone(p,true,true)))
  }
  function ConflictReplace(p: Paths): Program {
    var done:=ReplaceDone(p,false,false);
    var hold:=Ask(Replace(p.directory,p.hold),ConflictPublish(p),done);
    var prepare:=Ask(Build(1,p.stage),Ask(Build(2,p.stage),Ask(Build(3,p.stage),
      Ask(Probe(4,p.hold),hold,done),done),done),done);
    CheckFormat(4,Ask(Probe(5,p.directory),Ask(Allocate(1,p.stage),prepare,Return(0)),Return(0)))
  }
  function SeparateCopy(p: Paths): Program {
    var fail:=Ask(Probe(3,p.stage),Ignore(Remove(p.stage,true),Notice(8,Return(0))),Notice(8,Return(0)));
    var staged:=Ask(Build(4,p.stage),Ask(Build(3,p.stage),Ask(Replace(p.stage,p.target),Return(1),fail),fail),fail);
    CheckFormat(4,Ask(Probe(5,p.directory),Ask(Probe(6,p.target),CheckFormat(5,
      Ask(Probe(7,p.target),Ask(Probe(8,p.target),Ask(Allocate(1,p.stage),staged,Return(0)),Return(0)),Return(0))),Return(0)),Return(0)))
  }
  // New family after successful family inspection/name choice. The reservation
  // and exchange-name writes already happened and are not rolled back here.
  function FamilyPublish(p: Paths): Program {
    var fail:=Ignore(Remove(p.stage,true),Ignore(Remove(p.pending,false),ClearPending(Notice(9,Return(0)))));
    Ask(Allocate(2,p.stage),Ask(Build(5,p.stage),Ask(Build(6,p.stage),Ask(Build(7,p.stage),
      Ask(Remove(p.pending,false),Ask(Replace(p.stage,p.pending),ClearPending(Return(1)),fail),fail),fail),fail),fail),
      Ignore(Remove(p.pending,false),ClearPending(Return(0))))
  }
  function ReturnedTrue(result: Result): bool { result.status==Returned(1) }
  function Writes(op: Operation): set<Path> {
    match op
    case Copy(_,t) => {t} case Replace(s,t) => {s,t} case Remove(p,_) => {p}
    case Build(_,p) => {p} case Allocate(_,p) => {p} case _ => {}
  }
  function Changed(events: seq<Event>): set<Path>
    decreases |events|
  { if |events|==0 then {} else (if events[0].Called? then Writes(events[0].operation) else {})+Changed(events[1..]) }
  predicate Quiet(replies: seq<Reply>) { forall r <- replies :: r.environment==map[] }
  lemma ApplyFrame(store: Store,op: Operation,r: Reply,path: Path)
    requires Valid(store,op,r) && path !in Writes(op)
    ensures (path in Apply(store,op,r))==(path in store)
    ensures path in store ==> Apply(store,op,r)[path]==store[path]
  {}
  lemma RunFrame(program: Program,store: Store,replies: seq<Reply>,path: Path)
    requires Quiet(replies)
    ensures path !in Changed(Run(program,store,replies).events) ==>
      ((path in Run(program,store,replies).store)==(path in store) &&
       (path in store ==> Run(program,store,replies).store[path]==store[path]))
    decreases program
  {
    match program
    case Return(_) =>
    case Notice(_,next) => RunFrame(next,store,replies,path);
    case ClearPending(next) => RunFrame(next,store,replies,path);
    case Ask(op,yes,no) =>
      if |replies|>0 {
        var r:=replies[0];assert r.environment==map[];
        if Valid(store,op,r) {
          var tail:=Run(if r.ok then yes else no,Apply(store,op,r),replies[1..]);
          RunFrame(if r.ok then yes else no,Apply(store,op,r),replies[1..],path);
          if path !in Changed(Run(program,store,replies).events) { ApplyFrame(store,op,r,path); }
        }
      }
  }
  predicate Clears(program: Program)
    decreases program
  {
    match program
    case Return(_) => false case ClearPending(_) => true case Notice(_,next) => Clears(next)
    case Ask(_,yes,no) => Clears(yes) && Clears(no)
  }
  lemma ClearOnReturn(program: Program,store: Store,replies: seq<Reply>)
    requires Clears(program)
    ensures Run(program,store,replies).status.Returned? ==> PendingCleared in Run(program,store,replies).events
    decreases program
  {
    match program
    case Return(_) =>
    case ClearPending(_) =>
    case Notice(_,next) => ClearOnReturn(next,store,replies);
    case Ask(op,yes,no) =>
      if |replies|>0 {
        var r:=replies[0];var seen:=Overlay(store,r.environment);
        if Valid(seen,op,r) { ClearOnReturn(if r.ok then yes else no,Apply(seen,op,r),replies[1..]); }
      }
  }
  function Simple(ok: bool): Reply { Reply(ok,G.None,map[]) }
  lemma CopySuccess(store: Store,s: Path,t: Path)
    requires Valid(store,Copy(s,t),Simple(true))
    ensures Apply(store,Copy(s,t),Simple(true))[t]==store[s]
    ensures s in Apply(store,Copy(s,t),Simple(true))
  {}
  lemma RestoreSuccess(store: Store,p: Paths)
    requires p.previous in store && p.previous!=p.target
    ensures Apply(store,Replace(p.previous,p.target),Simple(true))[p.target]==store[p.previous]
    ensures p.previous !in Apply(store,Replace(p.previous,p.target),Simple(true))
  {}
  lemma NoResponseIsNotSuccess(program: Program,store: Store)
    requires program.Ask?
    ensures !ReturnedTrue(Run(program,store,[]))
  {}
  lemma FirstDoesNotRetire(p: Paths)
    ensures Later(p,false)==CheckFormat(1,CheckFormat(2,CheckFormat(3,
      Ask(Copy(p.source,p.stage),Ask(Probe(2,p.stage),Ask(Replace(p.stage,p.target),Return(1),InstallFailure(p,false)),StageFailure(p)),StageFailure(p)))))
  {}
  lemma Vectors() {
    var p:=Paths(0,1,2,3,4,5,6,7);var incoming:=Artifact(false,map[0:=[2]]);var prior:=Artifact(false,map[0:=[1]]);
    var store:=map[0:=incoming,1:=prior,2:=prior];
    var prefix:=[Simple(true),Simple(true),Simple(true),Simple(true),Simple(true),Simple(false),Simple(true),Simple(true),Simple(true)];
    assert Run(Later(p,true),store,prefix).status==Returned(1);
    assert 2 in Run(Later(p,true),store,prefix).store; // failed stat is not absence
    assert Run(Later(p,true),store,prefix).store[1]==incoming;
  }
  lemma CollisionCleanup() {
    var p:=Paths(0,1,2,3,4,5,6,7);var data:=Artifact(false,map[0:=[1]]);
    var store:=map[0:=data,3:=data];
    var replies:=[Simple(true),Simple(true),Simple(true),Simple(false),Simple(true)];
    assert Run(Later(p,false),store,replies).status==Returned(0);
    assert 3 !in Run(Later(p,false),store,replies).store;
  }
  lemma {:isolate_assertions} RetirementOutcomes() {
    var p:=Paths(0,1,2,3,4,5,6,7);
    var incoming:=Artifact(false,map[0:=[2]]);var prior:=Artifact(false,map[0:=[1]]);
    // Checkpoint after incoming installation and failed orders removal.
    var store:=map[0:=incoming,1:=incoming,2:=prior,4:=prior];
    var failed:=Run(RetireFailed(p),store,[Simple(false)]);
    assert failed.status==Returned(0) && failed.store[1]==incoming && failed.store[4]==prior;
    assert Notified(4) in failed.events && failed.store[2]==prior;
    var restored:=Run(RetireFailed(p),store,[Simple(true),Simple(false)]);
    assert restored.status==Returned(0) && restored.store[1]==prior && 4 !in restored.store;
    assert Notified(5) in restored.events && restored.store[2]==prior;
  }
  lemma {:isolate_assertions} {:fuel Run,8} RetriedRestore() {
    var p:=Paths(0,1,2,3,4,5,6,7);
    var prior:=Artifact(true,map[0:=[1],1:=[3]]);var staged:=Artifact(true,map[0:=[2]]);
    // Checkpoint after the original directory has moved to hold.
    var replies:=[Simple(false),Simple(false),Simple(true),Simple(true),Simple(true)];
    var result:=Run(ConflictPublish(p),map[3:=staged,6:=prior],replies);
    assert result.status==Returned(0) && result.store[5]==prior && 6 !in result.store && 3 !in result.store;
    assert Notified(6) in result.events && Notified(4) !in result.events;
  }
  lemma {:isolate_assertions} PublishedWithRemnant() {
    var p:=Paths(0,1,2,3,4,5,6,7);
    var prior:=Artifact(true,map[0:=[1],1:=[3]]);var staged:=Artifact(true,map[0:=[2]]);
    var remnant:=Artifact(true,map[0:=[1]]);
    var replies:=[Simple(true),Reply(false,G.Some(remnant),map[])];
    var result:=Run(ConflictPublish(p),map[3:=staged,6:=prior],replies);
    assert ReturnedTrue(result) && result.store[5]==staged && result.store[6]==remnant;
    assert Notified(7) in result.events;
  }
  lemma {:fuel Clears,20} FamilyClearsPending(p: Paths,store: Store,replies: seq<Reply>)
    ensures Run(FamilyPublish(p),store,replies).status.Returned? ==>
      PendingCleared in Run(FamilyPublish(p),store,replies).events
  { ClearOnReturn(FamilyPublish(p),store,replies); }
}
