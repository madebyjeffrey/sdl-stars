include "M08-native-economy.dfy"

// research.c GetTechLevelCost / CostOfDevelopingItem, native signed-byte
// levels/choices and unsigned stored spending. Pointer/alias adapters separate.
module M12DevelopmentQuotes {
  import E = M08NativeEconomy
  import M = M03Scoring
  import C = S01Codecs
  import S = S01
  import A = S04Arithmetic
  function Total(levels: seq<E.I8>): int
    ensures -128*|levels|<=Total(levels)<=127*|levels|
    decreases |levels|
  { if |levels|==0 then 0 else levels[0]+Total(levels[1..]) }
  function TechCost(levels: seq<E.I8>,level: int,choice: E.I8,slow: bool): E.Number
    requires |levels|==6
  {
    if level<0 || level>26 then E.Undefined("technology cost table index") else
    var base:=E.Word(10*Total(levels))+M.TechCosts()[level];
    // Native arithmetic right shift is floor /4, including negative bases.
    var priced:=if choice<1 then 2*base-base/4 else if choice==1 then base else C.DivC(base,2);
    E.Result(priced*(if slow then 2 else 1),"technology cost arithmetic")
  }
  lemma CostDefined(levels: seq<E.I8>,level: int,choice: E.I8,slow: bool)
    requires |levels|==6 && 0<=level<=26
    ensures TechCost(levels,level,choice,slow).Value?
    ensures -26880<=TechCost(levels,level,choice,slow).value<=323120
  {
    E.WordPreserves(10*Total(levels));
    assert 0<=M.TechCosts()[level]<=84700;
  }
  lemma CostProjection(levels: seq<E.I8>,level: nat,choice: E.I8,slow: bool)
    requires |levels|==6 && level<=26 && 0<=choice<=2
    requires forall i | 0<=i<6 :: levels[i]>=0
    ensures Total(levels)>=0
    ensures TechCost(levels,level,choice,slow).Value?
    ensures TechCost(levels,level,choice,slow).value==M.ResearchCost(Total(levels) as nat,level,choice,slow)
  {
    NonnegativeTotal(levels);E.WordPreserves(10*Total(levels));CostDefined(levels,level,choice,slow);
  }
  lemma NonnegativeTotal(levels: seq<E.I8>)
    requires forall i | 0<=i<|levels| :: levels[i]>=0
    ensures Total(levels)>=0
    decreases |levels|
  { if |levels|>0 { NonnegativeTotal(levels[1..]); } }
  datatype Status = Done | Unreachable | Undefined(reason: string)
  datatype Event = Quote(field: nat,level: int,cost: E.Number) | Raise(field: nat,value: E.I8) | Restore
  datatype State = State(levels: seq<E.I8>,cost: E.I32,current: E.I32,events: seq<Event>)
  datatype Outcome = Outcome(status: Status,state: State)
  function RaiseTo(s: State,field: nat,target: E.I8,choices: seq<E.I8>,slow: bool): Outcome
    requires |s.levels|==6 && field<6 && |choices|==6 && target<=26
    ensures |RaiseTo(s,field,target,choices,slow).state.levels|==6
    ensures !RaiseTo(s,field,target,choices,slow).status.Unreachable?
    ensures RaiseTo(s,field,target,choices,slow).state.cost==s.cost
    ensures forall j | 0<=j<6 && j!=field :: RaiseTo(s,field,target,choices,slow).state.levels[j]==s.levels[j]
    ensures RaiseTo(s,field,target,choices,slow).status.Done? ==> RaiseTo(s,field,target,choices,slow).state.levels[field]>=target
    decreases A.Max(0,(target as int)-s.levels[field])
  {
    if target<=s.levels[field] then Outcome(Done,s) else
    var level: int:=s.levels[field]+1;
    var quote:=TechCost(s.levels,level,choices[field],slow);
    var queried:=s.(events:=s.events+[Quote(field,level,quote)]);
    if quote.Undefined? then Outcome(Undefined(quote.reason),queried) else
    var current: int:=(s.current as int)+quote.value;
    if !E.Fits32(current) then Outcome(Undefined("development field addition"),queried) else
    var raised:=queried.(levels:=s.levels[field:=level as E.I8],current:=current as E.I32,
      events:=queried.events+[Raise(field,level as E.I8)]);
    RaiseTo(raised,field,target,choices,slow)
  }
  function Fields(s: State,field: nat,targets: seq<E.I8>,spent: seq<S.U32>,choices: seq<E.I8>,slow: bool): Outcome
    requires |s.levels|==6 && field<=6 && |targets|==6 && |spent|==6 && |choices|==6
    requires forall i | 0<=i<6 :: targets[i]<=26
    ensures |Fields(s,field,targets,spent,choices,slow).state.levels|==6
    ensures !Fields(s,field,targets,spent,choices,slow).status.Unreachable?
    ensures Fields(s,field,targets,spent,choices,slow).state.cost>=s.cost
    decreases 6-field
  {
    if field==6 then Outcome(Done,s) else
    if targets[field]<=s.levels[field] then Fields(s,field+1,targets,spent,choices,slow) else
    var stored:=E.Signed32(spent[field]);
    var scaled: int:=stored*(if slow then 2 else 1);
    if !E.Fits32(scaled) then Outcome(Undefined("slow stored-spending multiplication"),s) else
    if !E.Fits32(-scaled) then Outcome(Undefined("development spent negation"),s) else
    var raised:=RaiseTo(s.(current:=(-scaled) as E.I32),field,targets[field],choices,slow);
    if !raised.status.Done? then raised else
    var total: int:=(s.cost as int)+A.Max(0,raised.state.current);
    if !E.Fits32(total) then Outcome(Undefined("development total addition"),raised.state) else
    Fields(raised.state.(cost:=total as E.I32),field+1,targets,spent,choices,slow)
  }
  function {:isolate_assertions} Development(levels: seq<E.I8>,targets: seq<E.I8>,spent: seq<S.U32>,choices: seq<E.I8>,slow: bool): Outcome
    requires |levels|==6 && |targets|==6 && |spent|==6 && |choices|==6
    ensures |Development(levels,targets,spent,choices,slow).state.levels|==6
    ensures Development(levels,targets,spent,choices,slow).status.Done? ==> Development(levels,targets,spent,choices,slow).state.levels==levels
    ensures Development(levels,targets,spent,choices,slow).status.Done? ==> Development(levels,targets,spent,choices,slow).state.cost>=0
    ensures Development(levels,targets,spent,choices,slow).status.Unreachable? ==> Development(levels,targets,spent,choices,slow).state.levels==levels
    ensures Development(levels,targets,spent,choices,slow).status.Unreachable? ==> Development(levels,targets,spent,choices,slow).state.cost==-1 && Development(levels,targets,spent,choices,slow).state.events==[]
  {
    var start:=State(levels,0,0,[]);
    if exists i | 0<=i<6 :: targets[i]>26 then Outcome(Unreachable,start.(cost:=-1)) else
    var result:=Fields(start,0,targets,spent,choices,slow);
    if !result.status.Done? then result else
    Outcome(Done,result.state.(levels:=levels,events:=result.state.events+[Restore]))
  }
  lemma {:isolate_assertions} Vectors() {
    var zero: seq<E.I8>:=[0,0,0,0,0,0];
    E.WordPreserves(0);E.WordPreserves(10);
    assert TechCost(zero,1,0,true)==E.Value(176);
    assert TechCost(zero,1,1,false)==E.Value(50);
    assert TechCost([1,0,0,0,0,0],1,1,false)==E.Value(60);
    assert TechCost(zero,27,1,false).Undefined?;
    assert Development(zero,[27,0,0,0,0,0],[0,0,0,0,0,0],[1,1,1,1,1,1],false).status.Unreachable?;
  }
}
