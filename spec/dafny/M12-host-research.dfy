include "M12-research.dfy"
include "M12-research-announcements.dfy"

// Complete UpdateResearchStatus numeric/control transitions. Run exposes a
// finite prefix rather than silently assuming arbitrary helper termination.
module M12HostResearch {
  import E = M08NativeEconomy
  import Q = M12DevelopmentQuotes
  import N = M12ResearchAnnouncements
  import R = M12Research
  import S = S01
  import C = S01Codecs
  type Next = n: int | -8<=n<8
  datatype Player = Player(levels: seq<E.I8>,spent: seq<S.U32>,choices: seq<E.I8>,now: C.Nibble,next: Next,
    pool: E.I32,primary: E.I8,general: bool,tt: bool,dead: bool,crippled: bool)
  predicate PlayerShape(p: Player) { |p.levels|==6 && |p.spent|==6 && |p.choices|==6 }
  datatype Phase = StartPlayer | StartPass | Load | PrimaryPool | OtherPools | Check | Announce | Redirect | Store
    | NextField | EndPlayer | GrantPlayer | Grants | GrantMessage | EndGrants
  datatype Status = Running | Done | Pending | Escaped | Undefined(reason: string)
  datatype State = State(players: seq<Player>,slow: bool,originPool: bool,phase: Phase,status: Status,
    player: nat,field: nat,other: nat,usingPool: bool,general: nat,redo: bool,current: S.I16,next: S.I16,
    change: bool,noticeNext: S.I16,spent: E.I32,alive: nat,totals: seq<E.I32>,grantRedo: bool,
    global: S.I16,trace: seq<N.Reply>,cursor: nat,events: seq<N.Event>)
  predicate Shape(s: State) {
    |s.players|<=16 && (forall i | 0<=i<|s.players| :: PlayerShape(s.players[i])) &&
    s.player<=|s.players| && s.field<=6 && s.other<=6 && s.general<=2 && s.alive<=16 &&
    |s.totals|==6 && s.cursor<=|s.trace| &&
    (s.phase !in {StartPlayer,GrantPlayer,EndGrants} ==> s.player<|s.players|) &&
    (s.phase !in {StartPlayer,StartPass,EndPlayer,GrantPlayer,Grants,EndGrants} ==> s.field<6) &&
    (s.phase==StartPlayer ==> s.alive<=s.player) &&
    (s.phase !in {StartPlayer,GrantPlayer,Grants,GrantMessage,EndGrants} ==> s.alive<=s.player+1) &&
    (s.phase in {GrantPlayer,Grants,GrantMessage,EndGrants} || s.status==Done ==> s.global==-1) &&
    (s.phase in {Grants,GrantMessage} ==> s.originPool && s.alive>1) &&
    (s.status==Running && s.phase==PrimaryPool ==> s.usingPool && s.general<2 && s.field==s.current) &&
    (s.grantRedo ==> s.originPool)
  }
  opaque function LevelTotal(players: seq<Player>): int
    decreases |players|
  { if |players|==0 then 0 else Q.Total(players[0].levels)+LevelTotal(players[1..]) }
  lemma LevelTotalUpdate(players: seq<Player>,at: nat,p: Player)
    requires at<|players|
    ensures LevelTotal(players[at:=p])==LevelTotal(players)-Q.Total(players[at].levels)+Q.Total(p.levels)
    decreases |players|
  {
    reveal LevelTotal();
    if at>0 {
      assert players[at:=p][1..]==players[1..][at-1:=p];
      LevelTotalUpdate(players[1..],at-1,p);
    } else { assert players[at:=p][1..]==players[1..]; }
  }
  lemma FieldTotalUpdate(levels: seq<E.I8>,at: nat,value: E.I8)
    requires at<|levels|
    ensures Q.Total(levels[at:=value])==Q.Total(levels)-levels[at]+value
    decreases |levels|
  {
    if at>0 {
      assert levels[at:=value][1..]==levels[1..][at-1:=value];
      FieldTotalUpdate(levels[1..],at-1,value);
    } else { assert levels[at:=value][1..]==levels[1..]; }
  }
  function Begin(players: seq<Player>,slow: bool,pool: bool,trace: seq<N.Reply>,initialGlobal: S.I16): State
    requires |players|<=16 && forall i | 0<=i<|players| :: PlayerShape(players[i])
    ensures Shape(Begin(players,slow,pool,trace,initialGlobal))
  { State(players,slow,pool,StartPlayer,Running,0,0,0,pool,0,false,0,0,false,0,0,0,[0,0,0,0,0,0],false,initialGlobal,trace,0,[]) }
  function Fail(s: State,reason: string): State
    requires Shape(s.(status:=Undefined(reason))) ensures Shape(Fail(s,reason))
  { s.(status:=Undefined(reason)) }
  function Put(s: State,p: Player): State
    requires Shape(s) && s.player<|s.players| && PlayerShape(p)
    ensures Shape(Put(s,p))
    ensures |Put(s,p).players|==|s.players|
    ensures LevelTotal(Put(s,p).players)==LevelTotal(s.players)-Q.Total(s.players[s.player].levels)+Q.Total(p.levels)
  { LevelTotalUpdate(s.players,s.player,p);s.(players:=s.players[s.player:=p]) }
  function Progress(p: Player,field: nat,amount: int,add: bool): Player
    requires PlayerShape(p) && field<6
    ensures PlayerShape(Progress(p,field,amount,add))
    ensures Progress(p,field,amount,add).levels==p.levels
  { p.(spent:=p.spent[field:=((if add then p.spent[field] as int else 0)+amount)%4294967296]) }
  function Lowest(levels: seq<E.I8>,at: nat,best: nat): nat
    requires |levels|==6 && best<at<=6
    ensures Lowest(levels,at,best)<6
    decreases 6-at
  { if at==6 then best else Lowest(levels,at+1,if levels[at]<(levels[best] as int)%256 then at else best) }
  function Eligible(level: E.I8,crippled: bool): bool {
    if level<0 then true else R.CanAdvance(level as nat,crippled)
  }
  function StoreAmount(spent: E.I32,slow: bool): E.Number {
    if !slow then E.Value(spent) else if !E.Fits32((spent as int)+1) then E.Undefined("slow spending round addition")
    else E.Value(((spent as int)+1)/2 as E.I32)
  }
  function OtherAmount(pool: E.I32): E.Number {
    if !E.Fits32((pool as int)*3) then E.Undefined("general research pool multiplication") else
    E.Value(C.DivC(E.Signed32((pool as int)*3+19),20) as E.I32)
  }
  function {:isolate_assertions} Step(s: State): State
    requires Shape(s)
    ensures Shape(Step(s))
    ensures NormalLevels(s) ==> NormalLevels(Step(s))
    ensures |Step(s).players|==|s.players|
    ensures LevelTotal(Step(s).players)==LevelTotal(s.players)+
      (if s.status==Running && s.phase==Check && Step(s).phase==Announce && Step(s).status==Running then 1 else 0)
  {
    if s.status!=Running then s else
    if s.phase==StartPlayer then
      if s.player==|s.players| then s.(phase:=GrantPlayer,player:=0,field:=0,global:=-1,events:=s.events+[N.Global(-1)]) else
      var p:=s.players[s.player];
      s.(phase:=StartPass,usingPool:=s.originPool,general:=if p.general then 1 else 0,current:=p.now,next:=p.next,
        global:=s.player as S.I16,alive:=s.alive+(if p.dead then 0 else 1),
        events:=s.events+[N.Trait(s.player,"Generalized Research"),N.Global(s.player as S.I16)])
    else if s.phase==EndPlayer then s.(phase:=StartPlayer,player:=s.player+1)
    else if s.phase==EndGrants then
      if s.grantRedo then s.(originPool:=false,phase:=StartPlayer,player:=0,field:=0,alive:=0,grantRedo:=false,events:=s.events+[N.Recurse])
      else s.(status:=Done)
    else if s.phase==GrantPlayer then
      if !s.originPool || s.alive<=1 || s.player==|s.players| then s.(phase:=EndGrants) else
      var read:=s.(events:=s.events+[N.Trait(s.player,"Primary")]);
      if s.players[s.player].primary==1 then read.(phase:=Grants,field:=0) else read.(player:=s.player+1)
    else if s.phase==Grants then
      if s.field==6 then s.(phase:=GrantPlayer,player:=s.player+1,field:=0) else
      var p:=s.players[s.player];
      if s.totals[s.field]<=0 then s.(field:=s.field+1) else
      var amount:=C.DivC(C.DivC(s.totals[s.field],s.alive),2);
      if amount<=1 then s.(field:=s.field+1) else
      s.(phase:=GrantMessage,spent:=amount as E.I32,grantRedo:=true,
        events:=s.events+[N.Message(s.player,N.Shared,-2,s.field as S.I16,E.Word(amount),0)])
    else if s.player>=|s.players| then Fail(s,"research player/control adapter") else
    if s.phase==StartPass then s.(phase:=Load,field:=0,redo:=false) else
    if s.phase==NextField then
      if s.field==5 then s.(phase:=if s.redo then StartPass else EndPlayer) else s.(phase:=Load,field:=s.field+1) else
    if s.field>=6 then Fail(s,"research field/control adapter") else
    var p:=s.players[s.player];
    match s.phase
    case Load =>
      var spent:=E.Signed32(p.spent[s.field]);var scaled: int:=spent*(if s.slow then 2 else 1);
      if !E.Fits32(scaled) then Fail(s,"slow stored research multiplication") else
      s.(phase:=if s.field==s.current && s.usingPool && s.general<2 then PrimaryPool else Check,spent:=scaled as E.I32,change:=false)
    case PrimaryPool =>
      var marked:=if s.general!=0 then s.(general:=2,redo:=true) else s;
      if s.general!=0 && !E.Fits32((p.pool as int)+1) then Fail(marked,"general primary round addition") else
      var amount:=if s.general==0 then p.pool else C.DivC((p.pool as int)+1,2);
      var spent: int:=(s.spent as int)+amount;
      if !E.Fits32(spent) then Fail(marked,"primary research addition") else
      var added:=marked.(spent:=spent as E.I32);
      var total: int:=(s.totals[s.field] as int)+amount;
      if !E.Fits32(total) then Fail(added,"shared primary field total") else
      added.(totals:=s.totals[s.field:=total as E.I32],phase:=if s.general==0 then Check else OtherPools,other:=0)
    case OtherPools =>
      if s.other==6 then s.(phase:=Check) else if s.other==s.field then s.(other:=s.other+1) else
      var amount:=OtherAmount(p.pool);
      if amount.Undefined? then Fail(s,amount.reason) else
      var stored:=if s.slow then C.DivC(amount.value,2) else amount.value;
      var credited:=Put(s,Progress(p,s.other,stored,true));
      var total: int:=(s.totals[s.other] as int)+amount.value;
      if !E.Fits32(total) then Fail(credited,"shared secondary field total") else
      credited.(totals:=s.totals[s.other:=total as E.I32],other:=s.other+1)
    case Check =>
      if !Eligible(p.levels[s.field],p.crippled) then s.(phase:=NextField) else
      var level: int:=p.levels[s.field]+1;
      var quote:=Q.TechCost(p.levels,level,p.choices[s.field],s.slow);
      var queried:=s.(events:=s.events+[N.Cost(s.player,s.field,level,quote)]);
      if quote.Undefined? then Fail(queried,quote.reason) else
      if quote.value>s.spent then queried.(phase:=Store) else
      var leftover: int:=(s.spent as int)-quote.value;
      if !E.Fits32(leftover) then Fail(queried,"research cost subtraction") else
      FieldTotalUpdate(p.levels,s.field,level as E.I8);
      var changed:=p.(levels:=p.levels[s.field:=level as E.I8]);
      var next:=if level==26 && s.next==6 then 7 else s.next;
      var change:=s.current==s.field && next!=6;
      var target:=if !change then s.current else if next==7 then Lowest(changed.levels,1,0) else next;
      Put(queried,changed).(phase:=Announce,spent:=leftover as E.I32,next:=next,change:=change,noticeNext:=target,
        events:=queried.events+[N.Message(s.player,if s.general==0 then N.Ordinary else N.General,-2,level as S.I16,s.field as S.I16,E.Word(target))])
    case Announce =>
      var result:=N.Scan(s.player,s.field,p.levels[s.field],p.tt,0,0,s.trace,s.cursor,s.events);
      var recorded:=s.(cursor:=result.cursor,events:=result.events);
      if result.status.Done? then recorded.(phase:=Redirect)
      else recorded.(status:=if result.status.Pending? then Pending else if result.status.Escaped? then Escaped else Undefined(result.status.reason))
    case Redirect =>
      if !((s.usingPool || s.change || s.next==7) && s.next!=6 && s.field==s.current) then s.(phase:=Check) else
      var next:=if s.next==7 then Lowest(p.levels,1,0) else s.next;
      var changed:=if s.next==7 then p.(now:=(next%16) as C.Nibble) else p.(now:=(next%16) as C.Nibble,next:=6);
      var cleared:=Put(s,Progress(changed,s.field,0,false)).(current:=next,next:=if s.next==7 then 7 else 6);
      var stored:=StoreAmount(s.spent,s.slow);
      if stored.Undefined? then Fail(cleared,stored.reason) else
      if next<0 || next>=6 then Fail(cleared,"redirected research field index") else
      Put(cleared,Progress(cleared.players[s.player],next as nat,stored.value,true)).(usingPool:=false,redo:=true,phase:=NextField)
    case Store =>
      var stored:=StoreAmount(s.spent,s.slow);
      if stored.Undefined? then Fail(s,stored.reason) else Put(s,Progress(p,s.field,stored.value,false)).(phase:=NextField)
    case GrantMessage =>
      var stored:=StoreAmount(s.spent,s.slow);
      if stored.Undefined? then Fail(s,stored.reason) else Put(s,Progress(p,s.field,stored.value,true)).(phase:=Grants,field:=s.field+1)
    case _ => Fail(s,"research phase/control adapter")
  }
  function Run(s: State,fuel: nat): State
    requires Shape(s)
    ensures Shape(Run(s,fuel))
    decreases fuel
  { if fuel==0 || s.status!=Running then s else Run(Step(s),fuel-1) }
  lemma TerminalStable(s: State,fuel: nat)
    requires Shape(s) && s.status!=Running
    ensures Run(s,fuel)==s
  {}
  predicate NormalLevels(s: State)
    requires Shape(s)
  {
    forall i,j | 0<=i<|s.players| && 0<=j<6 :: 0<=s.players[i].levels[j]<=26
  }
  lemma {:isolate_assertions} StepNormal(s: State)
    requires Shape(s) && NormalLevels(s)
    ensures NormalLevels(Step(s))
  {}
  lemma RunNormal(s: State,fuel: nat)
    requires Shape(s) && NormalLevels(s)
    ensures NormalLevels(Run(s,fuel))
    decreases fuel
  {
    if fuel>0 && s.status==Running { StepNormal(s);RunNormal(Step(s),fuel-1); }
  }
  function NaturalLevels(levels: seq<E.I8>): seq<nat>
    requires forall i | 0<=i<|levels| :: levels[i]>=0
  { seq(|levels|,(i: int) requires 0<=i<|levels| => levels[i] as nat) }
  lemma LowestProjection(levels: seq<E.I8>,at: nat,best: nat)
    requires |levels|==6 && best<at<=6 && forall i | 0<=i<6 :: levels[i]>=0
    ensures Lowest(levels,at,best)==R.Lowest(NaturalLevels(levels),at,best)
    decreases 6-at
  { if at<6 { LowestProjection(levels,at+1,if levels[at]<levels[best] then at else best); } }
  lemma PrimaryProjection(pool: E.I32)
    requires 0<=pool<2147483647
    ensures C.DivC((pool as int)+1,2)==R.PrimaryPool(pool as nat,true)
  {}
  lemma OtherProjection(pool: E.I32)
    requires 0<=pool && (pool as int)*3+19<=2147483647
    ensures OtherAmount(pool).Value? && OtherAmount(pool).value==R.OtherPool(pool as nat,true)
  {
    assert C.Pow2(4)==16 && C.Pow2(8)==256 && C.Pow2(12)==4096 && C.Pow2(16)==65536;
    assert C.Pow2(20)==1048576 && C.Pow2(24)==16777216 && C.Pow2(28)==268435456;
    assert C.Pow2(31)==2147483648 && C.Pow2(32)==4294967296;
  }
  lemma {:isolate_assertions} Vectors() {
    assert Lowest([4,2,2,5,6,3],1,0)==1;
    assert Lowest([-1,0,0,0,0,0],1,0)==1;
    assert StoreAmount(-2,true)==E.Value(-1);
    assert StoreAmount(2147483647,true).Undefined?;
  }
}
