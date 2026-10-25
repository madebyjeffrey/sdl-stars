include "S02-profile.dfy"
include "S04-utf8-prefix.dfy"
module S02ProfileIO {
  import opened S01
  import C = S01Codecs
  import P = S02Profile
  import T = S04Utf8Prefix
  datatype OptionalBytes = Null | Text(rgb: Bytes)
  predicate ValidText(value: OptionalBytes) { value.Null? || FUtf8(value.rgb) }
  predicate WriteArguments(section: OptionalBytes,key: OptionalBytes,value: OptionalBytes) {
    section.Text? && |section.rgb|>0 && ValidText(section) && ValidText(key) && ValidText(value) &&
    10 !in section.rgb && 13 !in section.rgb && 93 !in section.rgb &&
    (key.Null? || (61 !in key.rgb && 10 !in key.rgb && 13 !in key.rgb)) &&
    (value.Null? || (10 !in value.rgb && 13 !in value.rgb))
  }
  datatype RequestCheck = NoCacheFlush | InvalidArguments | AcceptedArguments
  function CheckRequest(section: OptionalBytes,key: OptionalBytes,value: OptionalBytes): RequestCheck {
    if section.Null? && key.Null? && value.Null? then NoCacheFlush
    else if WriteArguments(section,key,value) then AcceptedArguments else InvalidArguments
  }
  datatype Buffer = Buffer(bytes: Bytes,count: nat)
  function Put(buffer: Bytes,offset: nat,value: Bytes): Bytes
    requires offset+|value|<=|buffer|
    ensures |Put(buffer,offset,value)|==|buffer|
  { buffer[..offset]+value+buffer[offset+|value|..] }
  function Scalar(value: Bytes,prior: Bytes,hasBuffer: bool,pathOK: bool): Buffer
    ensures |Scalar(value,prior,hasBuffer,pathOK).bytes|==|prior|
    ensures |prior|>0 ==> Scalar(value,prior,hasBuffer,pathOK).count<|prior|
  {
    if !hasBuffer || |prior|==0 then Buffer(prior,0) else
      var initialized := prior[0:=0];
      if !pathOK then Buffer(initialized,0) else
      var prefix := T.Prefix(value,|prior|-1);
      Buffer(Put(initialized,0,prefix+[0]),|prefix|)
  }
  function ListLoop(names: seq<Bytes>,buffer: Bytes,used: nat): Buffer
    requires used<=|buffer|
    ensures |ListLoop(names,buffer,used).bytes|==|buffer|
    ensures ListLoop(names,buffer,used).count<=|buffer|
    decreases |names|
  {
    if |names|==0 then Buffer(buffer,if |buffer|>=2 && used==|buffer|-1 then |buffer|-2 else used)
    else if |names[0]|==0 then ListLoop(names[1..],buffer,used)
    else if |buffer|<2 then Buffer(buffer,used)
    else if used>|buffer|-2 then Buffer(buffer,|buffer|-2)
    else
      var prefix := T.Prefix(names[0],|buffer|-2-used);
      var next := used+|prefix|;
      var copied := Put(buffer,used,prefix);
      if |prefix|<|names[0]| || next+1>=|buffer| then
        Buffer(Put(copied,next,C.Zeros(|buffer|-next)),|buffer|-2)
      else ListLoop(names[1..],Put(copied,next,[0,0]),next+1)
  }
  function List(names: seq<Bytes>,prior: Bytes,hasBuffer: bool,pathOK: bool): Buffer
    ensures |List(names,prior,hasBuffer,pathOK).bytes|==|prior|
  {
    if !hasBuffer || |prior|==0 then Buffer(prior,0) else
    var initialized := if |prior|>1 then Put(prior,0,[0,0]) else prior[0:=0];
    if !pathOK then Buffer(initialized,0) else ListLoop(names,initialized,0)
  }
  // Names are projections of the existing parsed rows, not a set: duplicates stay.
  function SectionNames(lines: seq<P.Line>): seq<string> decreases |lines| {
    if |lines|==0 then [] else
    (if lines[0].Section? && |lines[0].name|>0 then [lines[0].name] else [])+SectionNames(lines[1..])
  }
  function KeysInBlock(lines: seq<P.Line>): seq<string> decreases |lines| {
    if |lines|==0 || lines[0].Section? then [] else
    (if lines[0].Key? && |lines[0].name|>0 then [lines[0].name] else [])+KeysInBlock(lines[1..])
  }
  function KeyNames(lines: seq<P.Line>,section: string): seq<string> decreases |lines| {
    if |lines|==0 then [] else if lines[0].Section? && P.Fold(lines[0].name)==P.Fold(section)
    then KeysInBlock(lines[1..]) else KeyNames(lines[1..],section)
  }
  function Rows(raw: seq<Bytes>): Bytes decreases |raw| {
    if |raw|==0 then [] else raw[0]+(if |raw|>1 && |raw[0]|>0 && raw[0][|raw[0]|-1]!=10 then [10] else [])+Rows(raw[1..])
  }
  function Serialized(raw: seq<Bytes>): Bytes { [239,187,191]+Rows(raw) }
  datatype Stage = Allocate | Temporary | Mode | Stream | Bom | RowsStage | Flush | Sync | Close | Rename | Done
  function Stages(posix: bool,targetExists: bool): seq<Stage> {
    [Allocate,Temporary]+(if posix && targetExists then [Mode] else [])+[Stream,Bom,RowsStage,Flush,Sync,Close,Rename]
  }
  datatype SaveResult = SaveResult(success: bool,published: bool,temporaryMayRemain: bool,cleanupAttempt: bool,errorAssigned: bool)
  function Failed(stage: Stage,cleanupOK: bool): SaveResult
    requires stage!=Done
  {
    // Before mkstemp success there is no newly created file; unlink still runs
    // on a mkstemp failure but not a temporary-name allocation failure.
    SaveResult(false,false,stage!=Allocate && stage!=Temporary && !cleanupOK,
      stage!=Allocate,stage!=Bom)
  }
  function Save(stages: seq<Stage>,firstFailure: Stage,cleanupOK: bool): SaveResult
    requires firstFailure==Done || firstFailure in stages
  { if firstFailure==Done then SaveResult(true,true,false,false,false) else Failed(firstFailure,cleanupOK) }
  datatype Execution = Execution(result: SaveResult,attempted: seq<Stage>)
  function Execute(stages: seq<Stage>,outcomes: map<Stage,bool>,cleanupOK: bool): Execution
    requires forall stage <- stages :: stage!=Done && stage in outcomes
    ensures |Execute(stages,outcomes,cleanupOK).attempted|<=|stages|
    ensures Execute(stages,outcomes,cleanupOK).result.success == (forall stage <- stages :: outcomes[stage])
    decreases |stages|
  {
    if |stages|==0 then Execution(SaveResult(true,true,false,false,false),[]) else
    if !outcomes[stages[0]] then Execution(Failed(stages[0],cleanupOK),[stages[0]]) else
    var tail := Execute(stages[1..],outcomes,cleanupOK);
    Execution(tail.result,[stages[0]]+tail.attempted)
  }
  // Effects are classified for one call without competing writers. Native success
  // results/errno are inputs; no crash durability or cross-process exclusion claim.
  lemma FailedDoesNotPublish(s: Stage,cleanupOK: bool)
    requires s!=Done
    ensures !Failed(s,cleanupOK).published && !Failed(s,cleanupOK).success
  {}
  lemma {:isolate_assertions} BufferVectors() {
    assert Scalar([65,195,169],[9,9,9],true,true)==Buffer([65,0,9],1);
    assert Scalar([65],[9,9],false,true)==Buffer([9,9],0);
    assert Scalar([65],[9,9],true,false)==Buffer([0,9],0);
    assert List([[65,66]],[9,9,9,9],true,true)==Buffer([65,66,0,0],2);
    assert C.Zeros(1)==[0];
    assert C.Zeros(2)==[0,0];
    assert C.Zeros(3)==[0,0,0];
    assert List([[195,169]],[9,9,9],true,true)==Buffer([0,0,0],1);
    assert List([[65],[66]],[9,9,9,9,9],true,true)==Buffer([65,0,66,0,0],3);
    assert List([],[9,9,9],true,true)==Buffer([0,0,9],0);
    assert WriteArguments(Text([65]),Text([]),Text([]));
    assert !WriteArguments(Text([65,93]),Null,Null);
    assert !WriteArguments(Text([65]),Text([61]),Text([66]));
    assert Serialized([[65],[66,10],[]])==[239,187,191,65,10,66,10];
    assert !Failed(Bom,true).errorAssigned && Failed(Sync,false).temporaryMayRemain;
    assert Failed(Temporary,false).cleanupAttempt && !Failed(Temporary,false).temporaryMayRemain;
    assert Save(Stages(false,false),Done,false).published;
    assert CheckRequest(Null,Null,Null)==NoCacheFlush;
    assert CheckRequest(Null,Text([65]),Null)==InvalidArguments;
    assert Execute([Flush,Sync,Close,Rename],map[Flush:=true,Sync:=false,Close:=true,Rename:=true],true).attempted==[Flush,Sync];
  }
}
