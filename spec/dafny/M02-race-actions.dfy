include "M02-race-allocation.dfy"

// Ordinary returning-service domain. S01 owns parser/credential extraction;
// S04 owns native path identities and I/O refinement, S05 modal reentrancy.
module M02RaceActions {
  import S = S01
  import L = M02Library
  import K = S01LibraryNames
  import N = M02NativeRaceState
  import A = M02RaceAllocation
  import F = M02RaceSave
  datatype State = State(native: N.State,version: S.U16,store: A.Store,counter: S.U32)
  datatype Read = Read(value: S.I16,native: N.State,version: S.U16)
  predicate ReadFrame(s: State,r: Read) {
    N.Shape(r.native) && r.native.path==s.native.path && r.native.newFlag==s.native.newFlag
  }
  function ReadState(s: State,r: Read): State { s.(native:=r.native,version:=r.version) }
  predicate Filename(name: S.Bytes,root: S.Bytes) {
    var dot:=K.LastDot(name);
    0<dot && dot+2<|name| && name[dot+1] in {82,114} &&
      K.FRaceName(name[..dot],name[dot+2..],|root|,256)
  }
  datatype Event = Init | Name | SourcePath(path: S.Bytes) | Capture | Parse(path: S.Bytes)
    | Suggest(calls: seq<S.Bytes>) | TargetPath(path: S.Bytes) | Copy(source: S.Bytes,target: S.Bytes)
    | Remove(path: S.Bytes,okay: bool) | Restore | Window | Alert
    | Picker | Compare | Allocate(events: seq<A.Event>) | Replace(source: S.Bytes,target: S.Bytes,okay: bool)
    | Size(path: S.Bytes) | Confirm(name: S.Bytes) | ClearBinding
  datatype Status = Returned(okay: bool) | Pending | InvalidEvidence
  datatype Result = Result(status: Status,state: State,calls: seq<Event>)
  function Done(saved: N.Snapshot,s: State,okay: bool,calls: seq<Event>,notify: bool,window: bool): Result {
    Result(Returned(okay),s.(native:=N.Restore(saved,s.native,true,true)),
      calls+[Restore]+(if notify then [Window]+(if window then [Alert] else []) else []))
  }
  predicate RemoveFrame(s: A.Store,path: S.Bytes,okay: bool) { !okay || path in s.files }
  function Removed(s: A.Store,path: S.Bytes,okay: bool): A.Store {
    if okay then A.Put(s,path,F.Absent) else s
  }
  predicate ReplaceFrame(s: A.Store,source: S.Bytes,target: S.Bytes,okay: bool) {
    !okay || (source in s.files && target !in s.directories)
  }
  function Replaced(s: A.Store,source: S.Bytes,target: S.Bytes,okay: bool): A.Store
    requires ReplaceFrame(s,source,target,okay)
  {
    if !okay then s else
    var file:=A.At(s,source);A.Put(A.Put(s,source,F.Absent),target,file)
  }
  datatype DuplicateInputs = DuplicateInputs(init: bool,source: Read,available: set<nat>,
    formatError: bool,copy: A.Copy,target: Read,removed: bool,window: bool)
  function Duplicate(s: State,root: S.Bytes,sep: S.U8,name: S.Bytes,i: DuplicateInputs): Result
    requires N.Shape(s.native)
  {
    if !i.init then Result(Returned(false),s,[Init]) else
    if !Filename(name,root) then Result(Returned(false),s,[Init,Name]) else
    var source:=A.Join(root,sep,name);var saved:=N.Capture(s.native);
    var prefix:=[Init,Name,SourcePath(source),Capture,Parse(source)];
    if !ReadFrame(s,i.source) then Result(InvalidEvidence,s,prefix) else
    var read:=ReadState(s,i.source);
    if i.source.value<=0 then Done(saved,read,false,prefix,false,i.window) else
    var suggestion:=A.Suggest(K.DropSuffix(name),i.available);
    var calls:=prefix+[Suggest(suggestion.calls)];
    if |suggestion.name|==0 then Done(saved,read,false,calls,true,i.window) else
    var target:=A.Join(root,sep,suggestion.name);var formatted:=calls+[TargetPath(target)];
    if i.formatError then Done(saved,read,false,formatted,true,i.window) else
    var copiedCalls:=formatted+[Copy(source,target)];
    if !A.ValidCopy(read.store,source,target,false,i.copy) then Result(InvalidEvidence,read,copiedCalls) else
    var copied:=read.(store:=A.Copied(read.store,source,target,false,i.copy));
    if !i.copy.ok then Done(saved,copied,false,copiedCalls,true,i.window) else
    var parsedCalls:=copiedCalls+[Parse(target)];
    if !ReadFrame(copied,i.target) then Result(InvalidEvidence,copied,parsedCalls) else
    var parsed:=ReadState(copied,i.target);
    if i.target.value>0 then Done(saved,parsed,true,parsedCalls,false,i.window) else
    if !RemoveFrame(parsed.store,target,i.removed) then Result(InvalidEvidence,parsed,parsedCalls) else
    var removed:=parsed.(store:=Removed(parsed.store,target,i.removed));
    Done(saved,removed,false,parsedCalls+[Remove(target,i.removed)],true,i.window)
  }
  datatype PathChoice = Direct(path: S.Bytes) | Pick
  datatype ExportInputs = ExportInputs(init: bool,picked: bool,pickerPath: S.Bytes,source: Read,
    sourceKey: S.Bytes,targetKey: S.Bytes,replies: seq<A.ExportReply>,target: Read,
    replaced: bool,removed: bool,window: bool)
  function Export(s: State,root: S.Bytes,sep: S.U8,name: S.Bytes,target: PathChoice,tick: S.U32,i: ExportInputs): Result
    requires N.Shape(s.native)
  {
    if !i.init then Result(Returned(false),s,[Init]) else
    if !Filename(name,root) then Result(Returned(false),s,[Init,Name]) else
    var source:=A.Join(root,sep,name);var calls:=[Init,Name,SourcePath(source)]+(if target.Pick? then [Picker] else []);
    if target.Pick? && !i.picked then Result(Returned(false),s,calls) else
    if target.Pick? && |i.pickerPath|>=512 then Result(InvalidEvidence,s,calls) else
    var path:=if target.Pick? then i.pickerPath else target.path;var saved:=N.Capture(s.native);
    var parsedCalls:=calls+[Capture,Parse(source)];
    if !ReadFrame(s,i.source) then Result(InvalidEvidence,s,parsedCalls) else
    var read:=ReadState(s,i.source);
    if i.source.value<=0 then Done(saved,read,false,parsedCalls,true,i.window) else
    var compared:=parsedCalls+[Compare];
    if !K.FComparisonKey(source,i.sourceKey) || !K.FComparisonKey(path,i.targetKey) then Result(InvalidEvidence,read,compared) else
    if i.sourceKey==i.targetKey then Done(saved,read,false,compared,true,i.window) else
    var allocation:=A.Export(read.store,source,path,sep,tick,read.counter,512,i.replies);
    var staged:=read.(store:=allocation.store,counter:=allocation.counter);
    var allocatedCalls:=compared+[Allocate(allocation.events)];
    if allocation.status==A.Pending then Result(Pending,staged,allocatedCalls) else
    if allocation.status==A.InvalidEvidence then Result(InvalidEvidence,staged,allocatedCalls) else
    if !allocation.status.ok then Done(saved,staged,false,allocatedCalls,true,i.window) else
    ExportStaged(saved,staged,allocation.file,path,i,allocatedCalls)
  }
  function ExportStaged(saved: N.Snapshot,s: State,temp: S.Bytes,target: S.Bytes,i: ExportInputs,calls: seq<Event>): Result
    ensures ExportStaged(saved,s,temp,target,i,calls).status.Returned? ==>
      N.Capture(ExportStaged(saved,s,temp,target,i,calls).state.native)==saved
  {
    var parsedCalls:=calls+[Parse(temp)];
    if !ReadFrame(s,i.target) then Result(InvalidEvidence,s,parsedCalls) else
    var parsed:=ReadState(s,i.target);
    if i.target.value<=0 then ExportFailed(saved,parsed,temp,i,parsedCalls) else
    if !ReplaceFrame(parsed.store,temp,target,i.replaced) then Result(InvalidEvidence,parsed,parsedCalls) else
    var replaced:=parsed.(store:=Replaced(parsed.store,temp,target,i.replaced));
    var replacedCalls:=parsedCalls+[Replace(temp,target,i.replaced)];
    if !i.replaced then ExportFailed(saved,replaced,temp,i,replacedCalls) else
    Done(saved,replaced,true,replacedCalls,false,i.window)
  }
  function ExportFailed(saved: N.Snapshot,s: State,temp: S.Bytes,i: ExportInputs,calls: seq<Event>): Result
    ensures ExportFailed(saved,s,temp,i,calls).status.Returned? ==>
      N.Capture(ExportFailed(saved,s,temp,i,calls).state.native)==saved
  {
    if !RemoveFrame(s.store,temp,i.removed) then Result(InvalidEvidence,s,calls) else
    Done(saved,s.(store:=Removed(s.store,temp,i.removed)),false,calls+[Remove(temp,i.removed)],true,i.window)
  }
  datatype DeleteInputs = DeleteInputs(init: bool,sizeRead: bool,size: int,answer: int,removed: bool)
  function Delete(s: State,root: S.Bytes,sep: S.U8,name: S.Bytes,i: DeleteInputs): Result
    requires N.Shape(s.native)
  {
    if !i.init then Result(Returned(false),s,[Init]) else
    if !Filename(name,root) then Result(Returned(false),s,[Init,Name]) else
    var path:=A.Join(root,sep,name);var calls:=[Init,Name,TargetPath(path),Size(path)];
    if i.sizeRead && (path !in s.store.files || i.size!=|s.store.files[path]|) then Result(InvalidEvidence,s,calls) else
    if !i.sizeRead || i.size<=0 then Result(Returned(false),s,calls) else
    var prompted:=calls+[Confirm(name)];
    if i.answer!=6 then Result(Returned(false),s,prompted) else
    if !RemoveFrame(s.store,path,i.removed) then Result(InvalidEvidence,s,prompted) else
    var removed:=s.(store:=Removed(s.store,path,i.removed));var removedCalls:=prompted+[Remove(path,i.removed)];
    if !i.removed then Result(Returned(false),removed,removedCalls+[Alert]) else
    if N.Text(s.native.path)!=path then Result(Returned(true),removed,removedCalls) else
    Result(Returned(true),removed.(native:=s.native.(path:=s.native.path[0:=0])),removedCalls+[ClearBinding])
  }
  lemma DoneRestoresOwned(saved: N.State,s: State,okay: bool,calls: seq<Event>,notify: bool,window: bool)
    ensures Done(N.Capture(saved),s,okay,calls,notify,window).state.native==saved.(suggestion:=s.native.suggestion)
    ensures Done(N.Capture(saved),s,okay,calls,notify,window).state.version==s.version
    ensures Done(N.Capture(saved),s,okay,calls,notify,window).state.counter==s.counter
  {}
  lemma DuplicateRestores(s: State,root: S.Bytes,sep: S.U8,name: S.Bytes,i: DuplicateInputs)
    requires N.Shape(s.native)
    ensures Duplicate(s,root,sep,name,i).status.Returned? ==>
      N.Capture(Duplicate(s,root,sep,name,i).state.native)==N.Capture(s.native)
  {}
  lemma DeleteFrames(s: State,root: S.Bytes,sep: S.U8,name: S.Bytes,i: DeleteInputs,p: S.Bytes)
    requires N.Shape(s.native) && p!=A.Join(root,sep,name)
    ensures A.At(Delete(s,root,sep,name,i).state.store,p)==A.At(s.store,p)
    ensures Delete(s,root,sep,name,i).state.native.race==s.native.race
    ensures Delete(s,root,sep,name,i).state.native.password==s.native.password
    ensures Delete(s,root,sep,name,i).state.native.cache==s.native.cache
    ensures Delete(s,root,sep,name,i).state.native.newFlag==s.native.newFlag
    ensures Delete(s,root,sep,name,i).state.native.suggestion==s.native.suggestion
    ensures Delete(s,root,sep,name,i).state.native.path[1..]==s.native.path[1..]
    ensures Delete(s,root,sep,name,i).state.version==s.version && Delete(s,root,sep,name,i).state.counter==s.counter
  {}
  lemma ExportRestores(s: State,root: S.Bytes,sep: S.U8,name: S.Bytes,target: PathChoice,tick: S.U32,i: ExportInputs)
    requires N.Shape(s.native)
    ensures Export(s,root,sep,name,target,tick,i).status.Returned? ==>
      N.Capture(Export(s,root,sep,name,target,tick,i).state.native)==N.Capture(s.native)
  {}
  lemma DeclinePreserves(s: State,root: S.Bytes,sep: S.U8,name: S.Bytes,i: DeleteInputs)
    requires N.Shape(s.native) && i.answer!=6
    ensures Delete(s,root,sep,name,i).state==s
  {}
  lemma ReplacementFailureFrame(s: A.Store,source: S.Bytes,target: S.Bytes)
    ensures Replaced(s,source,target,false)==s
  {}
  lemma ReplacementSelf(s: A.Store,path: S.Bytes)
    requires path in s.files && path !in s.directories
    ensures Replaced(s,path,path,true)==s
  {}
  lemma FailedAliasValidationRetainsFile(saved: N.Snapshot,s: State,path: S.Bytes,i: ExportInputs)
    requires path in s.store.files && ReadFrame(s,i.target) && i.target.value<=0 && !i.removed
    ensures ExportStaged(saved,s,path,path,i,[]).status==Returned(false)
    ensures ExportStaged(saved,s,path,path,i,[]).state.store==s.store
    ensures A.At(ExportStaged(saved,s,path,path,i,[]).state.store,path).Present?
  {}
}
