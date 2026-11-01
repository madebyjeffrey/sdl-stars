include "M02-race-save.dfy"

module M02RaceAllocation {
  import S = S01
  import X = S01External
  import C = S01Codecs
  import N = S01LibraryNames
  import U = S04Utf8Prefix
  import F = M02RaceSave
  type Path = S.Bytes
  datatype Store = Store(files: map<Path,S.Bytes>,directories: set<Path>)
  function At(s: Store,p: Path): F.File { if p in s.files then F.Present(s.files[p]) else F.Absent }
  function Put(s: Store,p: Path,f: F.File): Store {
    if f.Present? then s.(files:=s.files[p:=f.bytes]) else s.(files:=map q | q in s.files && q!=p :: s.files[q])
  }
  function RemoveDirectory(s: Store,p: Path,removed: bool): Store {
    if removed then s.(directories:=s.directories-{p}) else s
  }
  function Decimal(n: nat): S.Bytes
    ensures 0<|Decimal(n)|
    decreases n
  { if n<10 then [C.Byte(48+n)] else Decimal(n/10)+[C.Byte(48+n%10)] }
  function Next(n: S.U32): S.U32 { ((n+1)%4294967296) as S.U32 }
  function Advance(n: S.U32,steps: nat): S.U32
    decreases steps
  { if steps==0 then n else Advance(Next(n),steps-1) }
  function Join(root: Path,sep: S.U8,name: S.Bytes): Path { root+[sep]+name }
  function StageName(root: Path,sep: S.U8,tick: S.U32,counter: S.U32): Path {
    Join(root,sep,[46,115,116,97,114,115,45,114,97,99,101,45,115,116,97,103,101,45]+Decimal(tick)+[45]+Decimal(counter))
  }
  function LastSeparator(path: Path,sep: S.U8): nat
    ensures LastSeparator(path,sep)<=|path|
    decreases |path|
  { if |path|==0 then 0 else if path[|path|-1]==sep then |path| else LastSeparator(path[..|path|-1],sep) }
  function ExportName(target: Path,sep: S.U8,tick: S.U32,counter: S.U32): Path {
    target[..LastSeparator(target,sep)]+[46,115,116,97,114,115,45,114,97,99,101,45,101,120,112,111,114,116,45]+
      Decimal(tick)+[45]+Decimal(counter)+[46,116,109,112]
  }
  function Fits(path: Path,capacity: nat,error: bool): bool { !error && 0<|path|<capacity }
  datatype CopyPhase = SourceOpenFailure | TargetOpenFailure | Contents
  datatype Copy = Copy(phase: CopyPhase,ok: bool,destination: F.File,existsError: bool)
  function Wanted(s: Store,source: Path,target: Path,exclusive: bool): S.Bytes
    requires source in s.files
  { if source==target && !exclusive then [] else s.files[source] }
  predicate ValidCopy(s: Store,source: Path,target: Path,exclusive: bool,r: Copy) {
    if r.phase==SourceOpenFailure then !r.ok && r.destination==At(s,target) else
    source in s.files &&
    (if r.phase==TargetOpenFailure then !r.ok &&
      (r.destination==At(s,target) || ((!exclusive || target !in s.files) && target !in s.directories && r.destination==F.Present([]))) else
     target !in s.directories && (!exclusive || target !in s.files) &&
     (if r.ok then r.destination==F.Present(Wanted(s,source,target,exclusive)) else
      r.destination.Absent? || X.FDirectFailure(Wanted(s,source,target,exclusive),r.destination.bytes)))
  }
  function Copied(s: Store,source: Path,target: Path,exclusive: bool,r: Copy): Store
    requires ValidCopy(s,source,target,exclusive,r)
  { Put(s,target,r.destination) }
  datatype StageReply = StageReply(directoryFormatError: bool,made: bool,observedDirectory: bool,fileFormatError: bool,removed: bool)
  datatype Event = Tick | DirectoryFormat(counter: S.U32,path: Path) | Make(path: Path,ok: bool) | Exists(path: Path,ok: bool)
    | FileFormat(path: Path) | Remove(path: Path,ok: bool) | ExportFormat(counter: S.U32,path: Path)
    | ClearErrno | CopyCall(source: Path,target: Path,ok: bool,existsError: bool)
  datatype Status = Returned(ok: bool) | Pending | InvalidEvidence
  datatype Allocation = Allocation(status: Status,store: Store,counter: S.U32,used: nat,path: Path,file: Path,events: seq<Event>)
  predicate EmptyBelow(s: Store,directory: Path,sep: S.U8) {
    var prefix:=directory+[sep];
    (forall p <- s.files.Keys :: |p|<|prefix| || p[..|prefix|]!=prefix) &&
    (forall p <- s.directories :: |p|<|prefix| || p[..|prefix|]!=prefix)
  }
  function StageLoop(s: Store,root: Path,sep: S.U8,tick: S.U32,counter: S.U32,
    dirCapacity: nat,fileCapacity: nat,left: nat,replies: seq<StageReply>): Allocation
    ensures StageLoop(s,root,sep,tick,counter,dirCapacity,fileCapacity,left,replies).used<=|replies|
    ensures StageLoop(s,root,sep,tick,counter,dirCapacity,fileCapacity,left,replies).used<=left
    ensures StageLoop(s,root,sep,tick,counter,dirCapacity,fileCapacity,left,replies).counter==
      Advance(counter,StageLoop(s,root,sep,tick,counter,dirCapacity,fileCapacity,left,replies).used)
    ensures StageLoop(s,root,sep,tick,counter,dirCapacity,fileCapacity,left,replies).store.files==s.files
    ensures StageLoop(s,root,sep,tick,counter,dirCapacity,fileCapacity,left,replies).status==Returned(true) ==>
      StageLoop(s,root,sep,tick,counter,dirCapacity,fileCapacity,left,replies).path in
        StageLoop(s,root,sep,tick,counter,dirCapacity,fileCapacity,left,replies).store.directories &&
      StageLoop(s,root,sep,tick,counter,dirCapacity,fileCapacity,left,replies).file !in s.files
    decreases left
  {
    if left==0 then Allocation(Returned(false),s,counter,0,[],[],[]) else
    if |replies|==0 then Allocation(Pending,s,counter,0,[],[],[]) else
    var r:=replies[0];var directory:=StageName(root,sep,tick,counter);var next:=Next(counter);
    var prefix:=[DirectoryFormat(counter,directory)];
    if !Fits(directory,dirCapacity,r.directoryFormatError) then Allocation(Returned(false),s,next,1,directory,[],prefix) else
    if r.made && (directory in s.files || directory in s.directories || !EmptyBelow(s,directory,sep)) then
      Allocation(InvalidEvidence,s,next,1,directory,[],prefix) else
    if !r.made then
      if r.observedDirectory && directory !in s.directories then Allocation(InvalidEvidence,s,next,1,directory,[],prefix) else
      if !r.observedDirectory then Allocation(Returned(false),s,next,1,directory,[],prefix+[Make(directory,false),Exists(directory,false)]) else
      var result:=StageLoop(s,root,sep,tick,next,dirCapacity,fileCapacity,left-1,replies[1..]);
      result.(used:=result.used+1,events:=prefix+[Make(directory,false),Exists(directory,true)]+result.events)
    else
      var created:=s.(directories:=s.directories+{directory});var file:=Join(directory,sep,[100,101,115,105,103,110,46,114,49]);
      var calls:=prefix+[Make(directory,true),FileFormat(file)];
      if !Fits(file,fileCapacity,r.fileFormatError) then
        Allocation(Returned(false),RemoveDirectory(created,directory,r.removed),next,1,directory,file,calls+[Remove(directory,r.removed)])
      else Allocation(Returned(true),created,next,1,directory,file,calls)
  }
  function Stage(s: Store,root: Path,sep: S.U8,tick: S.U32,counter: S.U32,dirCapacity: nat,fileCapacity: nat,replies: seq<StageReply>): Allocation {
    var r:=StageLoop(s,root,sep,tick,counter,dirCapacity,fileCapacity,1000,replies);r.(events:=[Tick]+r.events)
  }
  datatype ExportReply = ExportReply(formatError: bool,copy: Copy)
  function ExportLoop(s: Store,source: Path,target: Path,sep: S.U8,tick: S.U32,counter: S.U32,
    capacity: nat,left: nat,replies: seq<ExportReply>): Allocation
    ensures ExportLoop(s,source,target,sep,tick,counter,capacity,left,replies).used<=|replies|
    ensures ExportLoop(s,source,target,sep,tick,counter,capacity,left,replies).used<=left
    ensures ExportLoop(s,source,target,sep,tick,counter,capacity,left,replies).counter==
      Advance(counter,ExportLoop(s,source,target,sep,tick,counter,capacity,left,replies).used)
    decreases left
  {
    if left==0 then Allocation(Returned(false),s,counter,0,[],[],[]) else
    if |replies|==0 then Allocation(Pending,s,counter,0,[],[],[]) else
    var r:=replies[0];var path:=ExportName(target,sep,tick,counter);var next:=Next(counter);
    var calls:=[ExportFormat(counter,path)];
    if !Fits(path,capacity,r.formatError) then Allocation(Returned(false),s,next,1,[],path,calls) else
    if !ValidCopy(s,source,path,true,r.copy) then Allocation(InvalidEvidence,s,next,1,[],path,calls) else
    var copied:=Copied(s,source,path,true,r.copy);var allCalls:=calls+[ClearErrno,CopyCall(source,path,r.copy.ok,r.copy.existsError)];
    if r.copy.ok then Allocation(Returned(true),copied,next,1,[],path,allCalls) else
    if !r.copy.existsError then Allocation(Returned(false),copied,next,1,[],path,allCalls) else
    var result:=ExportLoop(copied,source,target,sep,tick,next,capacity,left-1,replies[1..]);
    result.(used:=result.used+1,events:=allCalls+result.events)
  }
  function Export(s: Store,source: Path,target: Path,sep: S.U8,tick: S.U32,counter: S.U32,capacity: nat,replies: seq<ExportReply>): Allocation {
    if LastSeparator(target,sep)>2147483647 then Allocation(Returned(false),s,counter,0,[],[],[]) else
    var r:=ExportLoop(s,source,target,sep,tick,counter,capacity,1000,replies);r.(events:=[Tick]+r.events)
  }
  function SuggestStem(name: S.Bytes): S.Bytes { U.Prefix(N.DropSuffix(name),239) }
  datatype Suggestion = Suggestion(name: S.Bytes,number: nat,calls: seq<S.Bytes>)
  function SuggestLoop(stem: S.Bytes,available: set<nat>,at: nat): Suggestion
    requires 1<=at<=1000
    ensures at<=SuggestLoop(stem,available,at).number<=1000
    ensures SuggestLoop(stem,available,at).number<1000 ==>
      SuggestLoop(stem,available,at).number in available &&
      SuggestLoop(stem,available,at).name==stem+[46,114]+Decimal(SuggestLoop(stem,available,at).number)
    ensures SuggestLoop(stem,available,at).number==1000 ==> SuggestLoop(stem,available,at).name==[]
    ensures forall j: nat | at<=j<SuggestLoop(stem,available,at).number :: j !in available
    ensures |SuggestLoop(stem,available,at).calls|==
      (if SuggestLoop(stem,available,at).number==1000 then 1000-at else SuggestLoop(stem,available,at).number-at+1)
    decreases 1000-at
  {
    if at==1000 then Suggestion([],1000,[]) else
    var name:=stem+[46,114]+Decimal(at);
    if at in available then Suggestion(name,at,[name]) else
    var next:=SuggestLoop(stem,available,at+1);next.(calls:=[name]+next.calls)
  }
  function Suggest(name: S.Bytes,available: set<nat>): Suggestion { SuggestLoop(SuggestStem(name),available,1) }
  lemma CounterWrap() ensures Next(4294967295)==0 {}
  lemma FileCopyFrame(s: Store,source: Path,target: Path,exclusive: bool,r: Copy,p: Path)
    requires ValidCopy(s,source,target,exclusive,r) && p!=target
    ensures At(Copied(s,source,target,exclusive,r),p)==At(s,p)
  {}
  lemma FormatConsumesCounter(s: Store,root: Path,sep: S.U8,tick: S.U32,counter: S.U32,r: StageReply)
    requires r.directoryFormatError
    ensures StageLoop(s,root,sep,tick,counter,512,512,1,[r]).counter==Next(counter)
    ensures StageLoop(s,root,sep,tick,counter,512,512,1,[r]).status==Returned(false)
  {}
  lemma Bounded(s: Store,root: Path,source: Path,target: Path,sep: S.U8,tick: S.U32,counter: S.U32,
    dirCapacity: nat,fileCapacity: nat,stage: seq<StageReply>,exports: seq<ExportReply>)
    ensures Stage(s,root,sep,tick,counter,dirCapacity,fileCapacity,stage).used<=1000
    ensures Export(s,source,target,sep,tick,counter,fileCapacity,exports).used<=1000
    ensures Stage(s,root,sep,tick,counter,dirCapacity,fileCapacity,stage).store.files==s.files
  {}
  lemma ExclusiveSourceFrame(s: Store,source: Path,target: Path,r: Copy)
    requires ValidCopy(s,source,target,true,r)
    ensures At(Copied(s,source,target,true,r),source)==At(s,source)
  {}
  function SaveAllocation(i: F.Inputs,r: Allocation): F.Inputs
    requires r.status==Returned(true) && r.path in r.store.directories && r.file !in r.store.files
    ensures SaveAllocation(i,r).stageReady && SaveAllocation(i,r).allocationDirectory
    ensures SaveAllocation(i,r).allocationFile==F.Absent
  { i.(stageReady:=true,allocationDirectory:=true,allocationFile:=At(r.store,r.file)) }
  lemma StageAdmitsSave(s: Store,root: Path,sep: S.U8,tick: S.U32,counter: S.U32,
    dirCapacity: nat,fileCapacity: nat,replies: seq<StageReply>,i: F.Inputs)
    requires Stage(s,root,sep,tick,counter,dirCapacity,fileCapacity,replies).status==Returned(true)
    ensures SaveAllocation(i,Stage(s,root,sep,tick,counter,dirCapacity,fileCapacity,replies)).stageReady
    ensures SaveAllocation(i,Stage(s,root,sep,tick,counter,dirCapacity,fileCapacity,replies)).allocationDirectory
    ensures SaveAllocation(i,Stage(s,root,sep,tick,counter,dirCapacity,fileCapacity,replies)).allocationFile==F.Absent
  {}
  lemma NameSearchHasNoFinalBudget() {
    var stem:=seq(236,i=>65 as S.U8);
    assert |stem+[46,114]+Decimal(999)|==241;
    assert !N.FRaceName(stem,Decimal(999),14,256);
  }
}
