include "M04-wizard-controller.dfy"

// WM_COMMAND arms of all six RaceWizardDlg callbacks. Native controls, catalog
// text and drawing/pricing post-state remain explicit adapter obligations.
module M04PageCommands {
  import S = S01
  import C = S01Codecs
  import N = M02NativeRaceState
  import R = M04Race
  import W = M04WizardController
  import A = S04Arithmetic
  function Put(r: S.Bytes,at: nat,value: S.Bytes): S.Bytes
    requires |r|==192 && at+|value|<=192
    ensures |Put(r,at,value)|==192
  { r[..at]+value+r[at+|value|..] }
  function Portrait(r: S.Bytes): nat
    requires |r|==192
    ensures Portrait(r)<32
  { r[6]/8 }
  function SetPortrait(r: S.Bytes,n: nat): S.Bytes
    requires |r|==192
    ensures |SetPortrait(r,n)|==192
  { r[6:=(r[6]%8+8*(n%32)) as S.U8] }
  function Stat(r: S.Bytes,i: nat): int
    requires |r|==192 && i<16
  { C.Signed(r[62+i],8) }
  function SetStat(r: S.Bytes,i: nat,value: int): S.Bytes
    requires |r|==192 && i<16
    ensures |SetStat(r,i,value)|==192
  { r[62+i:=C.Byte(A.Clamp(value,R.MinAttr()[i],R.MaxAttr()[i]))] }
  function SetTrait(r: S.Bytes,bit: nat,on: bool): S.Bytes
    requires |r|==192 && bit<32
    ensures |SetTrait(r,bit,on)|==192
  {
    var at:=78+bit/8;var mask: bv8:=(1 as bv8) << (bit%8);
    var prior:=r[at] as bv8;
    r[at:=(if on then prior | mask else prior & !mask) as int]
  }
  function AR(r: S.Bytes): S.Bytes
    requires |r|==192
    ensures |AR(r)|==192
  {
    var a:=SetStat(SetStat(SetStat(r,1,10),2,10),3,10);
    var b:=SetStat(SetStat(SetStat(a,4,10),5,5),6,10);
    SetTrait(b,31,false)
  }
  function Narrow(raw: int): int { C.Signed(raw,16) }
  function First(checked: seq<bool>): nat
    ensures First(checked)<=|checked|
    decreases |checked|
  { if |checked|==0 || checked[0] then 0 else 1+First(checked[1..]) }
  function MatchName(name: S.Bytes,names: seq<S.Bytes>): nat
    ensures MatchName(name,names)<=|names|
    decreases |names|
  { if |names|==0 || name==names[0] then 0 else 1+MatchName(name,names[1..]) }
  function Nav(id: S.U16): int {
    if id==2 then 0 else if id==1070 then 1 else if id==1071 then 2 else if id==1072 then 3 else -1
  }
  datatype State = State(native: N.State,readOnly: S.I16,description: S.I16,
    editName: S.Bytes,editPlural: S.Bytes,combo: int,nextEnabled: bool)
  predicate Shape(s: State) { N.Shape(s.native) }
  datatype Input = Input(id: S.U16,notification: S.U16,check: int,radios: seq<bool>,
    name: S.Bytes,plural: S.Bytes,password: S.Bytes,rejected: bool,combo: int,
    defaultNames: seq<S.Bytes>,autoPlural: S.Bytes,drawn: N.State)
  datatype Event = Help(page: W.Page) | Sticky | End(code: nat) | ReadRadios
    | ReplacePreset(index: nat) | ClearRandom | ReadName | ReadPlural | ReadLeftover
    | ReadPassword | PasswordAlert | ReadCombo | SetLeftover | Hash | RestorePortrait
    | ZeroName | ZeroPlural | ReadButtonText | CompareDefault(index: nat) | ReadDefault(index: nat)
    | SetNameText | MakePlural | SetPluralText | InvalidatePoints | SetCombo(value: int)
    | EnableNext(enabled: bool) | SetPortraitImage | InvalidatePortrait
    | ReadCheck | SetImmunity(axis: nat) | SetDiscount | SetPrimary(index: nat)
    | ResetAR | SetLesser(index: nat) | SetResearch(index: nat,value: int) | SetHigherTech
    | InvalidateDescription | Draw(page: W.Page,mask: int,before: N.State)
  datatype Status = Returned(handled: bool,closed: bool,code: int) | InvalidEvidence
  datatype Result = Result(status: Status,state: State,calls: seq<Event>)
  predicate Text(text: S.Bytes,capacity: nat) { |text|<capacity && 0 !in text }
  predicate Presets(rows: seq<S.Bytes>) { |rows|==7 && forall r <- rows :: |r|==192 }
  function Keep(s: State,calls: seq<Event>): Result { Result(Returned(false,false,-1),s,calls) }
  function Invalid(s: State,calls: seq<Event>): Result { Result(InvalidEvidence,s,calls) }
  function Names(r: S.Bytes,name: S.Bytes,plural: S.Bytes,zero: bool): S.Bytes
    requires |r|==192 && Text(name,32) && Text(plural,32)
    ensures |Names(r,name,plural,zero)|==192
  {
    var a:=if zero then C.Zeros(32) else r[128..160];
    var b:=if zero then C.Zeros(32) else r[160..192];
    Put(Put(r,128,N.CopyString(a,name)),160,N.CopyString(b,plural))
  }
  function Commit(s: State,x: Input,presets: seq<S.Bytes>,code: nat): Result
    requires Shape(s) && Presets(presets) && code<4
    ensures Shape(Commit(s,x,presets,code).state)
  {
    if code==0 then Result(Returned(true,true,0),s,[Sticky,End(0)]) else
    if |x.radios|!=8 || !Text(x.name,32) || !Text(x.plural,32) then Invalid(s,[]) else
    var portrait:=Portrait(s.native.race);var selected:=First(x.radios);
    var r:=if selected<7 then presets[selected] else SetTrait(s.native.race,30,false);
    var chosen:=[ReadRadios]+(if selected<7 then [ReplacePreset(selected)] else [ClearRandom]);
    var named:=Names(r,x.name,x.plural,false);
    var prefix:=chosen+[ReadName,ReadPlural,ReadLeftover,ReadPassword];
    var password:=if x.rejected then s.native.password[0:=0] else
      if Text(x.password,16) then N.CopyString(s.native.password,x.password) else s.native.password;
    var native:=s.native.(race:=named,password:=password);
    if x.rejected then Result(Returned(true,false,-1),s.(native:=native),prefix+[PasswordAlert]) else
    if !Text(x.password,16) then Invalid(s.(native:=native),prefix) else
    var allotted:=SetStat(named,7,Narrow(x.combo));
    var salted:=Put(allotted,12,C.LE(W.Salt(x.password),4));
    var final:=s.(native:=native.(race:=SetPortrait(salted,portrait)));
    Result(Returned(true,true,code),final,prefix+[ReadCombo,SetLeftover,Hash,RestorePortrait,Sticky,End(code)])
  }
  function DefaultClick(s: State,x: Input,presets: seq<S.Bytes>): Result
    requires Shape(s) && Presets(presets) && 271<=x.id<=278
    ensures Shape(DefaultClick(s,x,presets).state)
  {
    if !Text(x.name,32) || !Text(x.plural,32) || |x.defaultNames|!=7 ||
      !(forall n <- x.defaultNames :: 0 !in n) then Invalid(s,[]) else
    var r:=Names(s.native.race,x.name,x.plural,true);
    var prefix:=[ZeroName,ReadName,ZeroPlural,ReadPlural,ReadButtonText];
    var matched:=MatchName(x.name,x.defaultNames);
    var scanned:=seq(if matched<7 then matched+1 else 7,i requires 0<=i<(if matched<7 then matched+1 else 7) => CompareDefault(i));
    var replace:=matched<7 && x.id<278;
    if replace && (!Text(x.defaultNames[x.id-271],32) || !Text(x.autoPlural,32)) then
      Invalid(s.(native:=s.native.(race:=r)),prefix+scanned) else
    var name:=if replace then x.defaultNames[x.id-271] else x.name;
    var plural:=if replace then x.autoPlural else x.plural;
    var named:=if replace then Names(r,name,plural,true) else r;
    var source:=if x.id<=277 then presets[x.id-271] else named;
    var combo:=Stat(source,7);var enabled:=x.id!=277;
    var final:=s.(native:=s.native.(race:=SetPortrait(named,Portrait(source))),
      editName:=name,editPlural:=plural,combo:=combo,nextEnabled:=enabled);
    Keep(final,prefix+scanned+(if replace then [ZeroName,ReadDefault(x.id-271),SetNameText,ZeroPlural,MakePlural,SetPluralText] else [])+
      [InvalidatePoints,ReadLeftover,SetCombo(combo),EnableNext(enabled),SetPortraitImage,InvalidatePortrait])
  }
  // Drawing invokes the existing CAdvantagePoints owner and can normalize the
  // race. Its explicit full focused post-state is not an identity receipt.
  predicate DrawFrame(before: N.State,after: N.State) {
    N.Shape(after) && after.password==before.password && after.cache==before.cache &&
    after.path==before.path && after.newFlag==before.newFlag && after.suggestion==before.suggestion
  }
  function Drawing(s: State,x: Input,page: W.Page,mask: int,calls: seq<Event>): Result
    requires Shape(s)
    ensures Shape(Drawing(s,x,page,mask,calls).state)
  {
    if !DrawFrame(s.native,x.drawn) then Invalid(s,calls) else
    Keep(s.(native:=x.drawn),calls+[Draw(page,mask,s.native)])
  }
  function Command(s: State,page: W.Page,x: Input,presets: seq<S.Bytes>): Result
    requires Shape(s) && Presets(presets)
    ensures Shape(Command(s,page,x,presets).state)
  {
    if x.id==118 then Result(Returned(true,false,-1),s,[Help(page)]) else
    var nav:=Nav(x.id);
    if nav>=0 then
      if page==1 then Commit(s,x,presets,nav) else Result(Returned(true,true,nav),s,[Sticky,End(nav)])
    else if page==1 then
      if x.notification==0 && 271<=x.id<=278 then DefaultClick(s,x,presets) else Keep(s,[])
    else if page==4 && 291<=x.id<=293 then
      var axis:=x.id-291;var immune:=Narrow(x.check)==1;
      var r:=s.native.race[16+axis:=if immune then 255 else 50]
        [19+axis:=if immune then 255 else 20][22+axis:=if immune then 255 else 80];
      Drawing(s.(native:=s.native.(race:=r)),x,page,Narrow([1,2,4][axis]+65280),[ReadCheck,SetImmunity(axis)])
    else if page==5 && x.id==291 then
      var r:=SetTrait(s.native.race,31,Narrow(x.check)!=0);
      Drawing(s.(native:=s.native.(race:=r)),x,page,99,[ReadCheck,SetDiscount])
    else if page==2 && x.notification==0 && 271<=x.id<=280 then
      var index:=x.id-271;var r:=SetStat(s.native.race,14,index);
      var isAR:=Stat(r,14)==8;var changed:=if isAR then AR(r) else r;
      Keep(s.(native:=s.native.(race:=changed)),[SetPrimary(index)]+(if isAR then [ResetAR] else [])+[InvalidatePoints,InvalidateDescription])
    else if page==3 && 291<=x.id<=304 then
      var index:=x.id-291;var r:=SetTrait(s.native.race,index,Narrow(x.check)!=0);
      Keep(s.(native:=s.native.(race:=r),description:=index),[ReadCheck,SetLesser(index),InvalidatePoints,InvalidateDescription])
    else if page==6 && x.notification==0 && 271<=x.id<=288 then
      var index:=x.id-271;var stat:=index/3+8;var value:=index%3;
      Keep(s.(native:=s.native.(race:=SetStat(s.native.race,stat,value))),[SetResearch(stat,value),InvalidatePoints])
    else if page==6 && x.id==291 then
      Keep(s.(native:=s.native.(race:=SetTrait(s.native.race,29,Narrow(x.check)!=0))),[ReadCheck,SetHigherTech,InvalidatePoints])
    else Keep(s,[])
  }
  lemma CancelKeepsState(s: State,page: W.Page,x: Input,presets: seq<S.Bytes>)
    requires Shape(s) && Presets(presets) && x.id==2
    ensures Command(s,page,x,presets).state==s
    ensures Command(s,page,x,presets).status==Returned(true,true,0)
  {}
  lemma RejectedPasswordDoesNotClose(s: State,x: Input,presets: seq<S.Bytes>,code: nat)
    requires Shape(s) && Presets(presets) && 0<code<4 && |x.radios|==8
    requires Text(x.name,32) && Text(x.plural,32) && x.rejected
    ensures Commit(s,x,presets,code).status==Returned(true,false,-1)
    ensures Commit(s,x,presets,code).state.native.password[0]==0
    ensures Commit(s,x,presets,code).state.native.password[1..]==s.native.password[1..]
    ensures Hash !in Commit(s,x,presets,code).calls
  {}
  lemma NonClickRadioIgnored(s: State,page: W.Page,x: Input,presets: seq<S.Bytes>)
    requires Shape(s) && Presets(presets) && x.notification!=0
    requires (page==2 && 271<=x.id<=280) || (page==6 && 271<=x.id<=288)
    ensures Command(s,page,x,presets).state==s
    ensures Command(s,page,x,presets).status==Returned(false,false,-1)
  {}
  lemma RejectedPresetKeepsPrefix(s: State,x: Input,presets: seq<S.Bytes>,code: nat)
    requires Shape(s) && Presets(presets) && 0<code<4 && |x.radios|==8
    requires First(x.radios)<7 && Text(x.name,32) && Text(x.plural,32) && x.rejected
    ensures Commit(s,x,presets,code).state.native.race[..128]==presets[First(x.radios)][..128]
    ensures RestorePortrait !in Commit(s,x,presets,code).calls
  {}
  lemma SuccessfulCommitRestoresPortrait(s: State,x: Input,presets: seq<S.Bytes>,code: nat)
    requires Shape(s) && Presets(presets) && 0<code<4 && |x.radios|==8
    requires Text(x.name,32) && Text(x.plural,32) && !x.rejected && Text(x.password,16)
    ensures Commit(s,x,presets,code).status==Returned(true,true,code)
    ensures Portrait(Commit(s,x,presets,code).state.native.race)==Portrait(s.native.race)
    ensures Commit(s,x,presets,code).state.native.password[..|x.password|+1]==x.password+[0]
    ensures Commit(s,x,presets,code).state.native.path==s.native.path
  {}
  lemma {:fuel C.Pow2,18} DrawingMaskVectors() {
    assert Narrow(65281)==-255;
    assert Narrow(65282)==-254;
    assert Narrow(65284)==-252;
  }
}
