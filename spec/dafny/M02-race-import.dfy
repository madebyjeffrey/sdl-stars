include "M02-race-actions.dfy"

// Import/name-choice/wrapper control. Native parser, dialog and wizard receipts
// have explicit frames; models do not approve the GAP-08 legacy policy split.
module M02RaceImport {
  import S = S01
  import K = S01LibraryNames
  import N = M02NativeRaceState
  import A = M02RaceAllocation
  import R = M02RaceActions
  import F = M02RaceSave
  datatype Override = NoOverride | Named(name: S.Bytes)
  datatype State = State(context: R.State,library: S.Bytes,override: Override,dialog: S.I16,saveOpen: bool)
  predicate Shape(s: State) { N.Shape(s.context.native) && |s.library|==256 }
  function Take(text: S.Bytes,limit: nat): S.Bytes { text[..if |text|<limit then |text| else limit] }
  function Format(buffer: S.Bytes,text: S.Bytes): S.Bytes
    requires 0<|buffer|
    ensures |Format(buffer,text)|==|buffer|
  { N.CopyString(buffer,Take(text,|buffer|-1)) }
  function Basename(path: S.Bytes,sep: S.U8): S.Bytes { path[A.LastSeparator(path,sep)..] }
  function Hint(source: S.Bytes): S.Bytes {
    var a:=A.LastSeparator(source,47);var b:=A.LastSeparator(source,92);
    var stem:=Take(source[if a>b then a else b..],255);
    Take(K.DropSuffix(stem)+[46,114,49],255)
  }
  function Minor(version: S.U16): nat { (version/32)%128 }
  function NameWrites(buffer: S.Bytes,names: seq<S.Bytes>): S.Bytes
    requires |buffer|==256
    ensures |NameWrites(buffer,names)|==256
    decreases |names|
  { if |names|==0 then buffer else NameWrites(Format(buffer,names[0]),names[1..]) }
  datatype Event = Picker | SourceLength | Capture | Parse(path: S.Bytes) | HintName(name: S.Bytes)
    | Init | Name | Available | SetName | Suggest(calls: seq<S.Bytes>) | DialogOn | Dialog | DialogOff
    | Target(path: S.Bytes) | Message | Allocate(events: seq<A.Event>) | Copy(source: S.Bytes,target: S.Bytes)
    | Replace(source: S.Bytes,target: S.Bytes,okay: bool) | RemoveFile(path: S.Bytes,okay: bool)
    | RemoveDirectory(path: S.Bytes,okay: bool) | Restore | Bind
    | Save(stageEvents: seq<A.Event>,saveCalls: seq<F.Operation>) | SetOverride | ClearOverride
    | EditName(name: S.Bytes) | LoadCalls(loadCalls: seq<R.Event>) | Wizard | RestoreEdit | IgnoreEditResult
  datatype Status = Returned(okay: bool) | Escaped | Pending | InvalidEvidence
  datatype Result = Result(status: Status,state: State,calls: seq<Event>)
  function Failed(saved: N.Snapshot,s: State,calls: seq<Event>): Result {
    Result(Returned(false),s.(context:=s.context.(native:=N.Restore(saved,s.context.native,true,true))),calls+[Restore])
  }
  datatype Naming = Naming(init: bool,available: set<nat>,answer: int,name: S.Bytes,formatError: bool,freeAfter: bool)
  datatype Choice = Choice(status: Status,state: State,path: S.Bytes,calls: seq<Event>)
  function Choose(s: State,root: S.Bytes,sep: S.U8,hint: S.Bytes,i: Naming): Choice
    requires Shape(s)
    ensures Shape(Choose(s,root,sep,hint,i).state)
    ensures Choose(s,root,sep,hint,i).status==Returned(true) ==> |Choose(s,root,sep,hint,i).path|<512
  {
    if !i.init then Choice(Returned(false),s,[],[Init]) else
    var copied:=Format(s.library,hint);var suggested:=A.Suggest(N.Text(copied),i.available);
    var names:=NameWrites(copied,suggested.calls);
    var named:=s.(library:=if |suggested.name|==0 then names[0:=0] else names);
    var calls:=[Init,SetName,Suggest(suggested.calls)];
    if |suggested.name|==0 then Choice(Returned(false),named,[],calls) else
    var dialog:=named.(dialog:=0);var prompted:=calls+[DialogOn,Dialog,DialogOff];
    if i.answer!=1 then Choice(Returned(false),dialog,[],prompted) else
    if !R.Filename(i.name,root) then Choice(InvalidEvidence,dialog,[],prompted) else
    var accepted:=dialog.(library:=Format(dialog.library,i.name));var path:=A.Join(root,sep,i.name);
    if |path|>=512 then Choice(InvalidEvidence,accepted,path,prompted) else
    var formatted:=prompted+[Target(path)];
    if i.formatError then Choice(Returned(false),accepted,path,formatted+[Message]) else
    if !i.freeAfter then Choice(Returned(false),accepted,path,formatted+[Available,Message]) else
    Choice(Returned(true),accepted,path,formatted+[Available])
  }
  datatype Inputs = Inputs(picked: bool,pickerPath: S.Bytes,source: R.Read,
    init: bool,freeOverride: bool,naming: Naming,stages: seq<A.StageReply>,copy: A.Copy,
    replaced: bool,removeStage: bool,removeDirectory: bool,target: R.Read,removeTarget: bool,
    save: F.Inputs)
  // Both ordinary FWasRaceFile exits call StreamClose; this is logical hf,
  // not a guarantee that an underlying failed close released its descriptor.
  function ReadState(s: State,r: R.Read): State { s.(context:=R.ReadState(s.context,r),saveOpen:=false) }
  function RemoveDir(s: State,path: S.Bytes,okay: bool): State {
    s.(context:=s.context.(store:=A.RemoveDirectory(s.context.store,path,okay)))
  }
  predicate DirectoryRemoval(s: State,path: S.Bytes,sep: S.U8,okay: bool) {
    !okay || (path in s.context.store.directories && A.EmptyBelow(s.context.store,path,sep))
  }
  function Import(s: State,root: S.Bytes,sep: S.U8,source: R.PathChoice,tick: S.U32,i: Inputs): Result
    requires Shape(s)
    ensures Import(s,root,sep,source,tick,i).status==Returned(false) ==>
      N.Capture(Import(s,root,sep,source,tick,i).state.context.native)==N.Capture(s.context.native)
  {
    var picked:=if source.Pick? then [Picker] else [];
    if source.Pick? && !i.picked then Result(Returned(false),s,picked) else
    var path:=if source.Pick? then i.pickerPath else source.path;
    var calls:=picked+[SourceLength];
    if |path|>=512 then Result(Returned(false),s,calls) else
    var saved:=N.Capture(s.context.native);var parsedCalls:=calls+[Capture,Parse(path)];
    if !R.ReadFrame(s.context,i.source) then Result(InvalidEvidence,s,parsedCalls) else
    var parsed:=ReadState(s,i.source);
    if i.source.value<=0 then Failed(saved,parsed,parsedCalls) else
    var legacy:=Minor(i.source.version)<85;var hint:=Hint(path);var hinted:=parsedCalls+[HintName(hint)];
    if s.override.Named? then
      if !i.init then Failed(saved,parsed,hinted+[Init]) else
      if !R.Filename(s.override.name,root) then Failed(saved,parsed,hinted+[Init,Name]) else
      if !i.freeOverride then Failed(saved,parsed,hinted+[Init,Name,Available]) else
      var name:=s.override.name;var target:=A.Join(root,sep,name);
      var named:=parsed.(library:=Format(parsed.library,name));
      if |target|>=512 then Result(InvalidEvidence,named,hinted) else
      NamedImport(saved,named,root,sep,path,target,legacy,tick,i,hinted+[Init,Name,Available,SetName,Target(target)])
    else
      var chosen:=Choose(parsed,root,sep,hint,i.naming);var namedCalls:=hinted+chosen.calls;
      if chosen.status==InvalidEvidence then Result(InvalidEvidence,chosen.state,namedCalls) else
      if chosen.status!=Returned(true) then Failed(saved,chosen.state,namedCalls) else
      NamedImport(saved,chosen.state,root,sep,path,chosen.path,legacy,tick,i,namedCalls)
  }
  function NamedImport(saved: N.Snapshot,s: State,root: S.Bytes,sep: S.U8,source: S.Bytes,target: S.Bytes,
    legacy: bool,tick: S.U32,i: Inputs,calls: seq<Event>): Result
    requires Shape(s) && |target|<512
    ensures NamedImport(saved,s,root,sep,source,target,legacy,tick,i,calls).status==Returned(false) ==>
      N.Capture(NamedImport(saved,s,root,sep,source,target,legacy,tick,i,calls).state.context.native)==saved
  {
    if legacy then
      var result:=LegacySave(s,root,sep,target,tick,i);
      if result.status==Returned(false) then Failed(saved,result.state,calls+result.calls) else
      result.(calls:=calls+result.calls)
    else
      var allocation:=A.Stage(s.context.store,root,sep,tick,s.context.counter,512,512,i.stages);
      var staged:=s.(context:=s.context.(store:=allocation.store,counter:=allocation.counter));
      var allocatedCalls:=calls+[Allocate(allocation.events)];
      if allocation.status==A.Pending then Result(Pending,staged,allocatedCalls) else
      if allocation.status==A.InvalidEvidence then Result(InvalidEvidence,staged,allocatedCalls) else
      if !allocation.status.ok then Failed(saved,staged,allocatedCalls) else
      CopyCurrent(saved,staged,source,target,allocation.path,allocation.file,sep,i,allocatedCalls)
  }
  function CopyCurrent(saved: N.Snapshot,s: State,source: S.Bytes,target: S.Bytes,directory: S.Bytes,stage: S.Bytes,
    sep: S.U8,i: Inputs,calls: seq<Event>): Result
    requires Shape(s) && |target|<512
    ensures CopyCurrent(saved,s,source,target,directory,stage,sep,i,calls).status==Returned(false) ==>
      N.Capture(CopyCurrent(saved,s,source,target,directory,stage,sep,i,calls).state.context.native)==saved
  {
    var copiedCalls:=calls+[Copy(source,stage)];
    if !A.ValidCopy(s.context.store,source,stage,false,i.copy) then Result(InvalidEvidence,s,copiedCalls) else
    var copied:=s.(context:=s.context.(store:=A.Copied(s.context.store,source,stage,false,i.copy)));
    if !i.copy.ok then CurrentFailure(saved,copied,directory,stage,sep,i,copiedCalls) else
    if !R.ReplaceFrame(copied.context.store,stage,target,i.replaced) then Result(InvalidEvidence,copied,copiedCalls) else
    var published:=copied.(context:=copied.context.(store:=R.Replaced(copied.context.store,stage,target,i.replaced)));
    var publishedCalls:=copiedCalls+[Replace(stage,target,i.replaced)];
    if !i.replaced then CurrentFailure(saved,published,directory,stage,sep,i,publishedCalls) else
    if !DirectoryRemoval(published,directory,sep,i.removeDirectory) then Result(InvalidEvidence,published,publishedCalls) else
    var removed:=RemoveDir(published,directory,i.removeDirectory);
    ValidatePublished(saved,removed,target,sep,i,publishedCalls+[RemoveDirectory(directory,i.removeDirectory)])
  }
  function ValidatePublished(saved: N.Snapshot,s: State,target: S.Bytes,sep: S.U8,i: Inputs,calls: seq<Event>): Result
    requires Shape(s) && |target|<512
    ensures ValidatePublished(saved,s,target,sep,i,calls).status==Returned(false) ==>
      N.Capture(ValidatePublished(saved,s,target,sep,i,calls).state.context.native)==saved
  {
    var parsedCalls:=calls+[Parse(target)];
    if !R.ReadFrame(s.context,i.target) then Result(InvalidEvidence,s,parsedCalls) else
    var parsed:=ReadState(s,i.target);
    if i.target.value<=0 then
      if !R.RemoveFrame(parsed.context.store,target,i.removeTarget) then Result(InvalidEvidence,parsed,parsedCalls) else
      var deleted:=parsed.(context:=parsed.context.(store:=R.Removed(parsed.context.store,target,i.removeTarget)));
      Failed(saved,deleted,parsedCalls+[RemoveFile(target,i.removeTarget)])
    else
      var native:=N.SetFile(parsed.context.native.(path:=N.CopyString(parsed.context.native.path,target)),Basename(target,sep));
      Result(Returned(true),parsed.(context:=parsed.context.(native:=native)),parsedCalls+[Bind])
  }
  function CurrentFailure(saved: N.Snapshot,s: State,directory: S.Bytes,stage: S.Bytes,sep: S.U8,i: Inputs,calls: seq<Event>): Result
    ensures CurrentFailure(saved,s,directory,stage,sep,i,calls).status==Returned(false) ==>
      N.Capture(CurrentFailure(saved,s,directory,stage,sep,i,calls).state.context.native)==saved
  {
    if !R.RemoveFrame(s.context.store,stage,i.removeStage) then Result(InvalidEvidence,s,calls) else
    var removed:=s.(context:=s.context.(store:=R.Removed(s.context.store,stage,i.removeStage)));
    if !DirectoryRemoval(removed,directory,sep,i.removeDirectory) then Result(InvalidEvidence,removed,calls) else
    Failed(saved,RemoveDir(removed,directory,i.removeDirectory),calls+[RemoveFile(stage,i.removeStage),RemoveDirectory(directory,i.removeDirectory)])
  }
  // Stage allocation and the existing save controller are composed rather than
  // treating legacy-save success as an unconstrained boolean.
  function {:isolate_assertions} LegacySave(s: State,root: S.Bytes,sep: S.U8,target: S.Bytes,tick: S.U32,i: Inputs): Result
    requires Shape(s) && |target|<512
  {
    if !N.Comparable(s.context.native.race) then Result(InvalidEvidence,s,[]) else
    var name:=N.Text(s.library);var ready:=i.save.init && R.Filename(name,root);
    var a:=if ready then A.Stage(s.context.store,root,sep,tick,s.context.counter,512,512,i.stages)
      else A.Allocation(A.Returned(false),s.context.store,s.context.counter,0,[],[],[]);
    var staged:=s.(context:=s.context.(store:=a.store,counter:=a.counter));
    if a.status==A.Pending then Result(Pending,staged,[Save(a.events,[F.Init,F.Name,F.PathFormat,F.Allocate])]) else
    if a.status==A.InvalidEvidence then Result(InvalidEvidence,staged,[]) else
    var stageReady:=ready && a.status.ok;
    if stageReady && a.file==target then Result(InvalidEvidence,staged,[]) else
    var input:=i.save.(name:=R.Filename(name,root),aliasCandidate:=true,stageReady:=stageReady,
      allocationFile:=A.At(a.store,a.file),allocationDirectory:=a.path in a.store.directories);
    var files:=F.Files(A.At(s.context.store,target),F.Absent,false,false);
    var result:=F.Save(F.State(s.context.native,s.context.native.race,files),input,target,name);
    var store:=if !stageReady then a.store else
      A.RemoveDirectory(A.Put(A.Put(a.store,target,result.state.files.target),a.file,result.state.files.stage),a.path,!result.state.files.directory);
    var state:=staged.(context:=staged.context.(native:=result.state.native,store:=store),saveOpen:=result.state.files.open);
    var calls:=[Save(a.events,result.calls)];
    match result.status
      case InvalidEvidence => Result(InvalidEvidence,state,calls)
      case Escaped(_) => Result(Escaped,state,calls)
      case Returned(okay) => Result(Returned(okay),state,calls)
  }
  function As(s: State,root: S.Bytes,sep: S.U8,source: R.PathChoice,name: Override,tick: S.U32,i: Inputs): Result
    requires Shape(s)
  {
    var result:=Import(s.(override:=name),root,sep,source,tick,i);
    if result.status.Returned? then result.(state:=result.state.(override:=NoOverride),calls:=[SetOverride]+result.calls+[ClearOverride])
    else result.(calls:=[SetOverride]+result.calls)
  }
  datatype EditInputs = EditInputs(init: bool,read: R.Read,window: bool,finished: bool,escaped: bool,wizard: State)
  function Load(s: State,root: S.Bytes,sep: S.U8,name: S.Bytes,i: EditInputs): Result
    requires Shape(s)
    ensures Load(s,root,sep,name,i).status.Returned? ==> Shape(Load(s,root,sep,name,i).state)
  {
    if !i.init then Result(Returned(false),s,[LoadCalls([R.Init])]) else
    if !R.Filename(name,root) then Result(Returned(false),s,[LoadCalls([R.Init,R.Name])]) else
    var path:=A.Join(root,sep,name);var saved:=N.Capture(s.context.native);
    var calls:=[R.Init,R.Name,R.SourcePath(path),R.Capture,R.Parse(path)];
    if !R.ReadFrame(s.context,i.read) || |path|>=512 then Result(InvalidEvidence,s,[LoadCalls(calls)]) else
    var read:=ReadState(s,i.read);
    if i.read.value<=0 then
      var result:=R.Done(saved,read.context,false,calls,true,i.window);
      Result(Returned(false),read.(context:=result.state),[LoadCalls(result.calls)])
    else Result(Returned(true),read.(context:=read.context.(native:=N.Bind(read.context.native,path,name))),[LoadCalls(calls),Bind])
  }
  function Edit(s: State,root: S.Bytes,sep: S.U8,name: S.Bytes,i: EditInputs): Result
    requires Shape(s)
  {
    var saved:=N.Capture(s.context.native);var loaded:=Load(s,root,sep,name,i);
    if loaded.status!=Returned(true) then loaded.(calls:=[Capture]+loaded.calls) else
    var calls:=[Capture]+loaded.calls+[Wizard];
    if !Shape(i.wizard) || i.wizard.override!=s.override then Result(InvalidEvidence,loaded.state,calls) else
    if i.escaped then Result(Escaped,i.wizard,calls) else
    var native:=N.Restore(saved,i.wizard.context.native,!i.finished,true);
    Result(Returned(i.finished),i.wizard.(context:=i.wizard.context.(native:=native)),calls+[RestoreEdit])
  }
  function AndEdit(s: State,root: S.Bytes,sep: S.U8,source: R.PathChoice,tick: S.U32,i: Inputs,edit: EditInputs): Result
    requires Shape(s)
  {
    var imported:=Import(s,root,sep,source,tick,i);
    AfterImport(imported,root,sep,edit)
  }
  function AfterImport(imported: Result,root: S.Bytes,sep: S.U8,edit: EditInputs): Result {
    if imported.status!=Returned(true) then imported else
    if !Shape(imported.state) then Result(InvalidEvidence,imported.state,imported.calls) else
    var name:=Basename(N.Text(imported.state.context.native.path),sep);
    var edited:=Edit(imported.state,root,sep,name,edit);
    var calls:=imported.calls+[EditName(name)]+edited.calls;
    if edited.status.Returned? then Result(Returned(true),edited.state,calls+[IgnoreEditResult]) else
    edited.(calls:=calls)
  }
  lemma FailureRestores(saved: N.State,s: State,calls: seq<Event>)
    ensures N.Capture(Failed(N.Capture(saved),s,calls).state.context.native)==N.Capture(saved)
    ensures Failed(N.Capture(saved),s,calls).state.context.native.suggestion==s.context.native.suggestion
    ensures Failed(N.Capture(saved),s,calls).state.library==s.library
    ensures Failed(N.Capture(saved),s,calls).state.context.store==s.context.store
  {}
  lemma AsClearsOrdinary(s: State,root: S.Bytes,sep: S.U8,source: R.PathChoice,name: Override,tick: S.U32,i: Inputs)
    requires Shape(s)
    ensures As(s,root,sep,source,name,tick,i).status.Returned? ==> As(s,root,sep,source,name,tick,i).state.override==NoOverride
  {}
  lemma ImportFalseRestores(s: State,root: S.Bytes,sep: S.U8,source: R.PathChoice,tick: S.U32,i: Inputs)
    requires Shape(s)
    ensures Import(s,root,sep,source,tick,i).status==Returned(false) ==>
      N.Capture(Import(s,root,sep,source,tick,i).state.context.native)==N.Capture(s.context.native)
  {}
  lemma EditKeepsBinding(s: State,root: S.Bytes,sep: S.U8,name: S.Bytes,i: EditInputs)
    requires Shape(s)
    ensures Edit(s,root,sep,name,i).status.Returned? ==>
      Edit(s,root,sep,name,i).state.context.native.path==s.context.native.path &&
      Edit(s,root,sep,name,i).state.context.native.newFlag==s.context.native.newFlag
  {}
  lemma NewFlagOnCurrentSuccess(saved: N.Snapshot,s: State,source: S.Bytes,target: S.Bytes,directory: S.Bytes,stage: S.Bytes,
    sep: S.U8,i: Inputs,calls: seq<Event>)
    requires Shape(s) && |target|<512
    ensures CopyCurrent(saved,s,source,target,directory,stage,sep,i,calls).status==Returned(true) ==>
      CopyCurrent(saved,s,source,target,directory,stage,sep,i,calls).state.context.native.newFlag==s.context.native.newFlag
  {}
  lemma PublishedRejectionRetainsFile(saved: N.Snapshot,s: State,target: S.Bytes,sep: S.U8,i: Inputs,calls: seq<Event>)
    requires Shape(s) && |target|<512 && target in s.context.store.files
    requires R.ReadFrame(s.context,i.target) && i.target.value<=0 && !i.removeTarget
    ensures ValidatePublished(saved,s,target,sep,i,calls).status==Returned(false)
    ensures ValidatePublished(saved,s,target,sep,i,calls).state.context.store==s.context.store
  {}
  lemma EditReturnDoesNotUndoImport(imported: Result,root: S.Bytes,sep: S.U8,edit: EditInputs)
    requires imported.status==Returned(true) && Shape(imported.state)
    requires Edit(imported.state,root,sep,Basename(N.Text(imported.state.context.native.path),sep),edit).status.Returned?
    ensures AfterImport(imported,root,sep,edit).status==Returned(true)
    ensures AfterImport(imported,root,sep,edit).state==Edit(imported.state,root,sep,Basename(N.Text(imported.state.context.native.path),sep),edit).state
  {}
}
