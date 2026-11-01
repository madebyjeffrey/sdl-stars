include "M02-race-import.dfy"

// RaceCreationWizard returning-service controller. Page bodies, native pricing,
// credential/save and modal callback extraction retain their owning contracts.
module M04WizardController {
  import S = S01
  import C = S01Codecs
  import N = M02NativeRaceState
  import I = M02RaceImport
  import R = M04Race
  type I32 = n: int | -2147483648<=n<=2147483647 witness 0
  type Page = p: int | 1<=p<=6 witness 1
  datatype Dispatch = Open(page: Page) | Finish | Cancel
  function Code(result: int): S.U16 { (result%65536) as S.U16 }
  function Route(page: Page,result: int): Dispatch {
    var code:=Code(result);
    if code==0 || code==65535 then Cancel else
    if page>1 && code==1 then Open(page-1) else
    if code==3 || page==6 then Finish else Open(page+1)
  }
  function DialogNumber(page: Page): int { R.DialogOrder()[page-1] }
  function Retry(panel: S.I16): Page { if 1<=panel<=6 then panel else 1 }
  datatype RectLife = NoFrame | LocalFrame | ExpiredFrame
  datatype State = State(context: I.State,panel: S.I16,readOnly: S.I16,parent: nat,
    rects: RectLife,sequenceOwned: bool,dontWrite: S.I16,salt: I32,last: I32)
  datatype Stage = PageDialog(page: Page) | Price | Password | Save | End(okay: bool)
  datatype Service = DialogService | PriceService | PasswordService | SaveService | EndService
  datatype Reply = Reply(service: Service,context: I.State,panel: S.I16,code: int,points: S.I16,
    okay: bool,salt: I32,last: I32,escaped: bool)
  datatype Event = StackRectangles(count: nat) | SetMode | Parent | BeginSequence(owned: bool)
    | SetPanel(page: Page) | MakeProc(dialog: int) | Dialog(dialog: int) | FreeProc(dialog: int)
    | ReadOnly | Calculate | PointAlert(debt: nat) | Hash | SetSaltLast | CheckPassword
    | HasLibrary | LibrarySave | StandaloneSave(requested: S.Bytes,aliasCache: bool,suggested: S.Bytes)
    | EndSequence | Return(okay: bool)
  datatype Status = Returned(okay: bool) | Pending | Escaped | InvalidEvidence
  datatype Result = Result(status: Status,state: State,used: nat,calls: seq<Event>)
  function Complete(s: State,okay: bool): Result {
    Result(Returned(okay),s.(rects:=ExpiredFrame),0,[Return(okay)])
  }
  function Escape(s: State,calls: seq<Event>): Result { Result(Escaped,s.(rects:=ExpiredFrame),1,calls) }
  function Apply(s: State,r: Reply): State { s.(context:=r.context,panel:=r.panel,salt:=r.salt,last:=r.last) }
  predicate Valid(s: State) { I.Shape(s.context) }
  function Close(s: State,okay: bool,replies: seq<Reply>): Result
    ensures Close(s,okay,replies).used<=|replies|
  {
    if !s.sequenceOwned then Complete(s,okay) else
    if |replies|==0 then Result(Pending,s,0,[EndSequence]) else
    var r:=replies[0];
    if r.service!=EndService || !I.Shape(r.context) then Result(InvalidEvidence,s,1,[EndSequence]) else
    var after:=Apply(s,r);
    if r.escaped then Escape(after,[EndSequence]) else
    var result:=Complete(after,okay);result.(used:=1,calls:=[EndSequence]+result.calls)
  }
  lemma {:fuel C.Pow2,34} SaltRange(password: S.Bytes)
    requires 0 !in password
    ensures -2147483648<=R.Credential(password)<=2147483647
  { assert C.Pow2(32)==4294967296 && C.Pow2(31)==2147483648; }
  function Salt(password: S.Bytes): I32
    requires 0 !in password
  { SaltRange(password); R.Credential(password) }
  function Prefix(stage: Stage): seq<Event> {
    match stage
    case PageDialog(p) => [SetPanel(p),MakeProc(DialogNumber(p)),Dialog(DialogNumber(p))]
    case Price => [ReadOnly,Calculate]
    case Password => [CheckPassword]
    case Save => [HasLibrary]
    case End(_) => [EndSequence]
  }
  function Expected(stage: Stage): Service {
    match stage
    case PageDialog(_) => DialogService
    case Price => PriceService
    case Password => PasswordService
    case Save => SaveService
    case End(_) => EndService
  }
  function Run(s: State,stage: Stage,replies: seq<Reply>): Result
    requires Valid(s)
    ensures Run(s,stage,replies).used<=|replies|
    decreases |replies|
  {
    if stage.End? then Close(s,stage.okay,replies) else
    if stage==Price && s.readOnly!=0 then
      var result:=Close(s,false,replies);result.(calls:=[ReadOnly]+result.calls)
    else if stage==Save && s.dontWrite!=0 then Close(s,true,replies) else
    var current:=if stage.PageDialog? then s.(panel:=stage.page) else s;
    var prefix:=Prefix(stage);
    var hasLibrary:=s.context.context.native.newFlag!=0 || s.context.context.native.path[0]!=0;
    var cached:=N.Text(s.context.context.native.cache);var full:=N.Text(s.context.context.native.suggestion);
    if stage==Save && !hasLibrary && (|cached|>=16 || |full|>=256) then Result(InvalidEvidence,current,0,prefix) else
    var request:=if |cached|==0 then [115,116,97,114,115,46,114,49] else cached;
    var aliasCache:=|cached|>0;
    var saveCall:=if stage!=Save then [] else if hasLibrary then [LibrarySave] else
      [StandaloneSave(request,aliasCache,N.Suggested(s.context.context.native,aliasCache,request))];
    var calls:=prefix+saveCall;
    if |replies|==0 then Result(Pending,current,0,calls) else
    var r:=replies[0];
    if r.service!=Expected(stage) || !I.Shape(r.context) then Result(InvalidEvidence,current,1,calls) else
    var after:=Apply(current,r);
    if r.escaped then Escape(after,calls) else
    if stage.PageDialog? then
      var freed:=calls+[FreeProc(DialogNumber(stage.page))];
      var route:=Route(stage.page,r.code);
      var result:=if route.Cancel? then Close(after,false,replies[1..]) else
        Run(after,if route.Finish? then Price else PageDialog(route.page),replies[1..]);
      result.(used:=result.used+1,calls:=freed+result.calls)
    else if stage==Price then
      if r.points<0 then
        var result:=Run(after,PageDialog(Retry(after.panel)),replies[1..]);
        result.(used:=result.used+1,calls:=calls+[PointAlert((-(r.points as int)) as nat)]+result.calls)
      else
        var password:=N.Text(after.context.context.native.password);
        if |password|>=16 || 0 in password then Result(InvalidEvidence,after,1,calls) else
        var hashed:=after.(salt:=Salt(password),last:=-5);
        var result:=Run(hashed,Password,replies[1..]);
        result.(used:=result.used+1,calls:=calls+[Hash,SetSaltLast]+result.calls)
    else if stage==Password then
      var result:=Run(after,if r.okay then Save else PageDialog(1),replies[1..]);
      result.(used:=result.used+1,calls:=calls+result.calls)
    else
      var result:=if r.okay then Close(after,true,replies[1..]) else Run(after,PageDialog(1),replies[1..]);
      result.(used:=result.used+1,calls:=calls+result.calls)
  }
  function Begin(context: I.State,readOnly: S.I16,dontWrite: S.I16,parent: nat,sequenceOwned: bool,
    salt: I32,last: I32,replies: seq<Reply>): Result
    requires I.Shape(context)
  {
    var initial:=State(context,1,readOnly,parent,LocalFrame,sequenceOwned,dontWrite,salt,last);
    var result:=Run(initial,PageDialog(1),replies);
    result.(calls:=[StackRectangles(17),SetMode,Parent,BeginSequence(sequenceOwned)]+result.calls)
  }
  lemma ReadOnlyFinish(s: State,replies: seq<Reply>)
    requires Valid(s) && s.readOnly!=0
    ensures Run(s,Price,replies).status==Close(s,false,replies).status
    ensures Calculate !in Run(s,Price,replies).calls
  {}
  lemma NoWriteSkipsSave(s: State,replies: seq<Reply>)
    requires Valid(s) && s.dontWrite!=0
    ensures Run(s,Save,replies).status==Close(s,true,replies).status
    ensures LibrarySave !in Run(s,Save,replies).calls
  {}
  lemma ReturnExpiresFrame(s: State,okay: bool)
    ensures Complete(s,okay).state.rects==ExpiredFrame
    ensures Complete(s,okay).state.context==s.context
  {}
  lemma RouteVectors() {
    assert Route(1,1)==Open(2) && Route(6,2)==Finish;
    assert Route(4,-1)==Cancel && Route(4,65536)==Cancel;
    assert Route(4,65537)==Open(3) && Route(4,65539)==Finish;
    assert Retry(-1)==1 && Retry(7)==1 && Retry(6)==6;
  }
}
