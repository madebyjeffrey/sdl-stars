include "M02-native-race-state.dfy"

// FSaveLibraryRace ordinary/escaping controller. Serialized bytes and native
// filesystem results are supplied; no complete-write or crash guarantee follows.
module M02RaceSave {
  import S = S01
  import C = S01Codecs
  import N = M02NativeRaceState
  import L = M02Library
  datatype File = Absent | Present(bytes: S.Bytes)
  datatype Files = Files(target: File,stage: File,directory: bool,open: bool)
  datatype State = State(native: N.State,candidate: S.Bytes,files: Files)
  predicate Shape(s: State) { N.Shape(s.native) && N.Comparable(s.candidate) }
  datatype Operation = Init | Name | PathFormat | Allocate | Create | Player | Checksum(value: S.U16)
    | End | Close(failed: bool) | Replace | RemoveFile | RemoveDirectory | Window | Alert | Bind
  datatype Status = Returned(value: bool) | Escaped(at: Operation) | InvalidEvidence
  datatype Result = Result(status: Status,state: State,calls: seq<Operation>)
  datatype Transfer = Transfer(wanted: S.Bytes,written: S.Bytes,escaped: bool)
  predicate ValidTransfer(t: Transfer) {
    |t.written|<=|t.wanted| && t.written==t.wanted[..|t.written|] && (!t.escaped ==> t.written==t.wanted)
  }
  datatype Inputs = Inputs(init: bool,name: bool,stageReady: bool,allocationFile: File,allocationDirectory: bool,created: bool,createFile: File,createOpen: bool,
    player: Transfer,ending: Transfer,closeFailed: bool,replaced: bool,removeFile: bool,removeDirectory: bool,
    window: bool,aliasCandidate: bool)
  function Append(file: File,bytes: S.Bytes): File
    requires file.Present?
  { Present(file.bytes+bytes) }
  predicate ValidCleanup(s: State,i: Inputs) {
    !i.removeDirectory || i.removeFile || s.files.stage.Absent?
  }
  function Clean(s: State,i: Inputs): State
    requires ValidCleanup(s,i)
  {
    s.(files:=s.files.(stage:=if i.removeFile then Absent else s.files.stage,
      directory:=if i.removeDirectory then false else s.files.directory))
  }
  function Finish(s: State,i: Inputs,path: S.Bytes,name: S.Bytes): Result
    requires Shape(s) && |path|<512 && s.files.stage.Present? && !s.files.open
  {
    if !i.replaced && !ValidCleanup(s,i) then Result(InvalidEvidence,s,[Replace]) else
    if !i.replaced then
      Result(Returned(false),Clean(s,i),[Replace,RemoveFile,RemoveDirectory,Window]+(if i.window then [Alert] else [])) else
    var files:=Files(s.files.stage,Absent,if i.removeDirectory then false else s.files.directory,false);
    Result(Returned(true),s.(native:=N.Bind(s.native,path,name),files:=files),[Replace,RemoveDirectory,Bind])
  }
  function Eof(checksum: S.U16): S.Bytes { C.LE(S.Header(0,2),2)+C.LE(checksum,2) }
  function Writes(s: State,i: Inputs,path: S.Bytes,name: S.Bytes): Result
    requires Shape(s) && |path|<512 && s.files.stage.Present?
    requires !i.aliasCandidate || s.candidate==s.native.race
  {
    if !ValidTransfer(i.player) then Result(InvalidEvidence,s,[]) else
    var candidate:=N.BeforeWrite(s.candidate);
    var native:=if i.aliasCandidate then s.native.(race:=candidate) else s.native;
    var playerState:=s.(native:=native,candidate:=candidate,files:=s.files.(stage:=Append(s.files.stage,i.player.written)));
    if i.player.escaped then Result(Escaped(Player),playerState,[Player]) else
    var checksum:=C.RaceChecksum(candidate);
    if !ValidTransfer(i.ending) || i.ending.wanted!=Eof(checksum) then
      Result(InvalidEvidence,playerState,[Player,Checksum(checksum)]) else
    var endState:=playerState.(files:=playerState.files.(stage:=Append(playerState.files.stage,i.ending.written)));
    if i.ending.escaped then Result(Escaped(End),endState,[Player,Checksum(checksum),End]) else
    var closed:=endState.(files:=endState.files.(open:=false));
    var result:=Finish(closed,i,path,name);
    result.(calls:=[Player,Checksum(checksum),End,Close(i.closeFailed)]+result.calls)
  }
  function Save(s: State,i: Inputs,path: S.Bytes,name: S.Bytes): Result
    requires Shape(s) && |path|<512
    requires !i.aliasCandidate || s.candidate==s.native.race
  {
    if !i.init then Result(Returned(false),s,[Init]) else
    if !i.name then Result(Returned(false),s,[Init,Name]) else
    var stageState:=s.(files:=s.files.(stage:=i.allocationFile,directory:=i.allocationDirectory));
    if !i.stageReady then Result(Returned(false),stageState,[Init,Name,PathFormat,Allocate]) else
    if !stageState.files.directory || !stageState.files.stage.Absent? then Result(InvalidEvidence,stageState,[Init,Name,PathFormat,Allocate]) else
    var created:=stageState.(files:=stageState.files.(stage:=i.createFile,open:=i.createOpen));
    if !i.created && !ValidCleanup(created,i) then Result(InvalidEvidence,created,[Init,Name,PathFormat,Allocate,Create]) else
    if !i.created then Result(Returned(false),Clean(created,i),[Init,Name,PathFormat,Allocate,Create,RemoveFile,RemoveDirectory]) else
    if !created.files.stage.Present? || !created.files.open then Result(InvalidEvidence,created,[Init,Name,PathFormat,Allocate,Create]) else
    var result:=Writes(created,i,path,name);
    result.(calls:=[Init,Name,PathFormat,Allocate,Create]+result.calls)
  }
  // Scan is canonical L.Scan folded in native enumeration order. Readable names
  // and native case-comparison keys are already extracted by M02/S01/S04.
  function Fold(candidates: seq<L.Candidate>,comparisons: seq<int>,stem: S.Bytes,race: S.Bytes,design: L.Design,v: L.Versions): L.Versions
    requires |candidates|==|comparisons|
    decreases |candidates|
  {
    if |candidates|==0 then v else
    Fold(candidates[1..],comparisons[1..],stem,race,design,L.Scan(v,candidates[0],stem,race,design,comparisons[0]))
  }
  function Existing(name: S.Bytes,read: bool,working: S.Bytes,stored: S.Bytes): L.Existing
    requires N.Comparable(working) && (|name|==0 || !read || N.Comparable(stored))
  {
    if |name|==0 then L.NoExisting else if !read then L.UnreadableExisting else
    L.ReadableExisting(name,N.Equal(working,stored),C.SliceWord(stored,12,4)!=0)
  }
  lemma ExistingUnreadableFirst(same: L.Found,choice: L.Choice,maximum: nat,fNew: bool,auth: bool,valid: bool,free: bool)
    ensures L.Plan(L.UnreadableExisting,same,choice,maximum,fNew,auth,valid,free)==L.Failed
  {}
  lemma FinishFailurePreservesTarget(s: State,i: Inputs,path: S.Bytes,name: S.Bytes)
    requires Shape(s) && |path|<512 && s.files.stage.Present? && !s.files.open && !i.replaced && ValidCleanup(s,i)
    ensures Finish(s,i,path,name).state.files.target==s.files.target
    ensures Finish(s,i,path,name).state.native==s.native
  {}
  lemma {:isolate_assertions} CloseFailureIgnored(s: State,i: Inputs,path: S.Bytes,name: S.Bytes)
    requires Shape(s) && |path|<512 && s.files.stage.Present?
    requires !i.aliasCandidate || s.candidate==s.native.race
    ensures Writes(s,i,path,name).status==Writes(s,i.(closeFailed:=!i.closeFailed),path,name).status
    ensures Writes(s,i,path,name).state==Writes(s,i.(closeFailed:=!i.closeFailed),path,name).state
  { hide C.RaceChecksum; hide Eof; }
  lemma EscapeSkipsPublication(s: State,i: Inputs,path: S.Bytes,name: S.Bytes)
    requires Shape(s) && |path|<512 && s.files.stage.Present?
    requires !i.aliasCandidate || s.candidate==s.native.race
    requires ValidTransfer(i.player) && ValidTransfer(i.ending) && i.player.escaped
    ensures Writes(s,i,path,name).status==Escaped(Player)
    ensures Replace !in Writes(s,i,path,name).calls && RemoveFile !in Writes(s,i,path,name).calls
    ensures Writes(s,i,path,name).state.files.target==s.files.target
  {}
  lemma SuccessMilestones(s: State,i: Inputs,path: S.Bytes,name: S.Bytes)
    requires Shape(s) && |path|<512 && (!i.aliasCandidate || s.candidate==s.native.race)
    ensures Save(s,i,path,name).status==Returned(true) ==>
      i.init && i.name && i.stageReady && i.created && !i.player.escaped && !i.ending.escaped && i.replaced &&
      !Save(s,i,path,name).state.files.open && Save(s,i,path,name).state.files.stage.Absent? &&
      Save(s,i,path,name).state.native.newFlag==0
  {}
  lemma {:isolate_assertions} FailureTargetFrame(s: State,i: Inputs,path: S.Bytes,name: S.Bytes)
    requires Shape(s) && |path|<512 && (!i.aliasCandidate || s.candidate==s.native.race)
    ensures Save(s,i,path,name).status!=Returned(true) ==> Save(s,i,path,name).state.files.target==s.files.target
    ensures Save(s,i,path,name).status!=Returned(true) ==> Save(s,i,path,name).state.native.path==s.native.path
  { hide C.RaceChecksum; hide Eof; }
  lemma CloseWarningVector() {
    var zero:=seq(192,i=>0 as S.U8);
    var native:=N.State(zero,C.Zeros(16),C.Zeros(16),C.Zeros(512),1,C.Zeros(256));
    var state:=State(native,zero,Files(Present([9]),Absent,false,false));
    var io:=Transfer([2],[2],false);var eof:=Eof(C.RaceChecksum(zero));var end:=Transfer(eof,eof,false);
    var inputs:=Inputs(true,true,true,Absent,true,true,Present([1]),true,io,end,true,true,true,true,false,false);
    var result:=Save(state,inputs,[80],[65]);
    assert result.status==Returned(true) && result.state.files.target==Present([1,2]+eof);
    assert !result.state.files.open && result.state.native.newFlag==0;
  }
  lemma FoldMaximum(candidates: seq<L.Candidate>,comparisons: seq<int>,stem: S.Bytes,race: S.Bytes,design: L.Design,v: L.Versions)
    requires |candidates|==|comparisons|
    ensures Fold(candidates,comparisons,stem,race,design,v).maximum>=v.maximum
    decreases |candidates|
  {
    if |candidates|>0 {
      var next:=L.Scan(v,candidates[0],stem,race,design,comparisons[0]);
      FoldMaximum(candidates[1..],comparisons[1..],stem,race,design,next);
    }
  }
}
