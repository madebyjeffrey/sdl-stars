include "M19-resolution.dfy"
include "M10-mining-capacity.dfy"

// mine.c EstMineralsMined: all three minerals and ordered AR remote calls.
// Draws and returning helper values are supplied traces, not global purity.
module M19Mining {
  import E = M08NativeEconomy
  import R = M19Resolution
  import S = S01
  import C = S01Codecs
  import A = S04Arithmetic
  datatype Ore = Ore(stock: E.I32,concentration: S.U8,level: S.U8)
  datatype Depleted = Depleted(concentration: S.U8,level: S.U8,iterations: nat)
  function Deplete(conc: S.U8,level: S.U8,demand: nat): Depleted
    ensures Deplete(conc,level,demand).concentration<=conc
    ensures conc>=1 ==> Deplete(conc,level,demand).concentration>=1
    ensures Deplete(conc,level,demand).iterations<=conc
    ensures Deplete(conc,level,demand).iterations<=(if conc==0 then 0 else conc-1)
    ensures conc>1 && demand>0 && Deplete(conc,level,demand).concentration==conc ==>
      Deplete(conc,level,demand).level<(if level==0 then 256 else level)
    decreases conc
  {
    if demand==0 || conc<=1 then Depleted(conc,level,0) else
    var normalized:=if level==0 then 256 else level;
    var band:=R.DepletionBand(conc);
    var left:=R.DepletionLeft(level,conc);
    if left<=demand then
      var tail:=Deplete((conc-1) as S.U8,0,demand-left);
      Depleted(tail.concentration,tail.level,tail.iterations+1)
    else
      var length:=12500/band;
      var raw: int:=(left-demand)*256/length;
      var floored:=if raw<1 then 1 else raw;
      var next:=if floored>=normalized then normalized-1 else floored;
      Depleted((if next==0 then conc-1 else conc) as S.U8,next as S.U8,1)
  }
  datatype State = State(ores: seq<Ore>,quantities: seq<E.I32>,draws: seq<S.U8>,consumed: nat,
    marked: set<nat>,events: seq<string>)
  predicate Shape(s: State) {
    |s.ores|==3 && |s.quantities|==3 && s.consumed<=|s.draws| && forall i | 0<=i<|s.draws| :: s.draws[i]<100
  }
  datatype Status = Done | Pending(reason: string) | Undefined(reason: string)
  datatype Outcome = Outcome(status: Status,state: State)
  function Event(s: State,event: string): State
    requires Shape(s) ensures Shape(Event(s,event))
  { s.(events:=s.events+[event]) }
  function Quantities(s: State,q: seq<E.I32>): State
    requires Shape(s) && |q|==3 ensures Shape(Quantities(s,q))
    ensures Quantities(s,q).ores==s.ores
  { s.(quantities:=q) }
  function UsesDraw(scaled: E.I32,generating: bool): bool { generating && scaled-C.DivC(scaled,100)*100!=0 }
  function NativeYield(scaled: E.I32,generating: bool,draw: nat): E.I32
    requires draw<100
  {
    var whole:=C.DivC(scaled,100);var fraction:=scaled-whole*100;
    (whole+(if generating && fraction!=0 && draw<fraction then 1 else 0)) as E.I32
  }
  function Axis(s: State,at: nat,mines: E.I32,efficiency: E.I8,home: bool,remote: bool,ar: bool,apply: bool,generating: bool): Outcome
    requires Shape(s) && at<3
    ensures Shape(Axis(s,at,mines,efficiency,home,remote,ar,apply,generating).state)
    ensures !apply ==> Axis(s,at,mines,efficiency,home,remote,ar,apply,generating).state.ores==s.ores
    ensures Axis(s,at,mines,efficiency,home,remote,ar,apply,generating).state.marked==s.marked
    ensures !generating ==> Axis(s,at,mines,efficiency,home,remote,ar,apply,generating).state.consumed==s.consumed
  {
    var ore:=s.ores[at];var conc:=R.Concentration(ore.concentration,home,remote,ar);
    var actual:=mines*conc;
    if !E.Fits32(actual) then Outcome(Undefined("mine/concentration product"),s) else
    var product:=if remote then actual else actual*efficiency;
    if !E.Fits32(product) then Outcome(Undefined("mine efficiency product"),s) else
    var scaled:=if remote then product else C.DivC(product,10);
    var needsDraw:=UsesDraw(scaled as E.I32,generating);
    if needsDraw && s.consumed==|s.draws| then Outcome(Pending("Random(100) result"),s) else
    var next:=if needsDraw then Event(s.(consumed:=s.consumed+1),"Random(100)") else s;
    var quantity:=NativeYield(scaled as E.I32,generating,if needsDraw then s.draws[s.consumed] else 0);
    var published:=Event(Quantities(next,next.quantities[at:=quantity as E.I32]),"quantity");
    if !apply then Outcome(Done,published) else
    var stock: int:=(ore.stock as int)+quantity;
    if !E.Fits32(stock) then Outcome(Undefined("mineral stock addition"),published) else
    var depleted:=Deplete(ore.concentration,ore.level,A.Max(0,C.DivC(actual,100)) as nat);
    var changed:=Ore(stock as E.I32,depleted.concentration,depleted.level);
    Outcome(Done,Event(published.(ores:=published.ores[at:=changed]),"stock/depletion"))
  }
  function Axes(s: State,at: nat,mines: E.I32,efficiency: E.I8,home: bool,remote: bool,ar: bool,apply: bool,generating: bool): Outcome
    requires Shape(s) && at<=3
    ensures Shape(Axes(s,at,mines,efficiency,home,remote,ar,apply,generating).state)
    ensures !apply ==> Axes(s,at,mines,efficiency,home,remote,ar,apply,generating).state.ores==s.ores
    ensures Axes(s,at,mines,efficiency,home,remote,ar,apply,generating).state.marked==s.marked
    ensures !generating ==> Axes(s,at,mines,efficiency,home,remote,ar,apply,generating).state.consumed==s.consumed
    decreases 3-at
  {
    if at==3 then Outcome(Done,s) else
    var result:=Axis(s,at,mines,efficiency,home,remote,ar,apply,generating);
    if !result.status.Done? then result else Axes(result.state,at+1,mines,efficiency,home,remote,ar,apply,generating)
  }
  datatype FirstOrder = MissingOrder | Order(task: nat)
  datatype Fleet = Fleet(planet: S.I16,player: S.I16,dead: bool,orders: S.I16,first: FirstOrder,mines: E.Number)
  datatype Slot = Hole | Present(fleet: Fleet)
  predicate Candidate(f: Fleet,planet: S.I16,player: S.I16) { f.planet==planet && f.player==player && !f.dead && f.orders<=1 }
  function AddOutputs(s: State,child: seq<E.I32>,at: nat): Outcome
    requires Shape(s) && |child|==3 && at<=3
    ensures Shape(AddOutputs(s,child,at).state)
    ensures AddOutputs(s,child,at).state.ores==s.ores
    decreases 3-at
  {
    if at==3 then Outcome(Done,s) else
    var sum: int:=(s.quantities[at] as int)+child[at];
    if !E.Fits32(sum) then Outcome(Undefined("remote quantity addition"),s) else
    AddOutputs(Event(Quantities(s,s.quantities[at:=sum as E.I32]),"remote quantity"),child,at+1)
  }
  function Remotes(s: State,rows: seq<Slot>,at: nat,planet: S.I16,player: S.I16,home: bool,apply: bool,generating: bool): Outcome
    requires Shape(s) && at<=|rows|
    ensures Shape(Remotes(s,rows,at,planet,player,home,apply,generating).state)
    ensures !apply ==> Remotes(s,rows,at,planet,player,home,apply,generating).state.ores==s.ores
    ensures !apply ==> Remotes(s,rows,at,planet,player,home,apply,generating).state.marked==s.marked
    ensures !generating ==> Remotes(s,rows,at,planet,player,home,apply,generating).state.consumed==s.consumed
    decreases |rows|-at
  {
    if at==|rows| || rows[at].Hole? then Outcome(Done,s) else
    var fleet:=rows[at].fleet;
    if !Candidate(fleet,planet,player) then Remotes(s,rows,at+1,planet,player,home,apply,generating) else
    if fleet.first.MissingOrder? then Outcome(Undefined("first waypoint dereference"),s) else
    if fleet.first.task!=3 then Remotes(s,rows,at+1,planet,player,home,apply,generating) else
    var queried:=Event(s,"fleet mining capacity");
    if fleet.mines.Undefined? then Outcome(Undefined(fleet.mines.reason),queried) else
    if fleet.mines.value<=0 then Remotes(queried,rows,at+1,planet,player,home,apply,generating) else
    var child:=Axes(Event(Quantities(queried,[0,0,0]),"primary"),0,fleet.mines.value,10,home,true,true,apply,generating);
    var restored:=Quantities(child.state,s.quantities);
    if !child.status.Done? then Outcome(child.status,restored) else
    var combined:=AddOutputs(restored,child.state.quantities,0);
    if !combined.status.Done? then combined else
    var marked:=if apply then Event(combined.state.(marked:=combined.state.marked+{at}),"clear here-all-turn") else combined.state;
    Remotes(marked,rows,at+1,planet,player,home,apply,generating)
  }
  function Estimate(s: State,count: E.I32,owned: bool,pop: E.I32,ar: bool,operating: E.Number,efficiency: E.I8,
    home: bool,apply: bool,generating: bool,planet: S.I16,player: S.I16,rows: seq<Slot>): Outcome
    requires Shape(s)
    requires count==-1 && owned && pop!=0 && operating.Value? ==> -32768<=operating.value<32768
    ensures Shape(Estimate(s,count,owned,pop,ar,operating,efficiency,home,apply,generating,planet,player,rows).state)
    ensures !apply ==> Estimate(s,count,owned,pop,ar,operating,efficiency,home,apply,generating,planet,player,rows).state.ores==s.ores
    ensures !apply ==> Estimate(s,count,owned,pop,ar,operating,efficiency,home,apply,generating,planet,player,rows).state.marked==s.marked
    ensures !generating ==> Estimate(s,count,owned,pop,ar,operating,efficiency,home,apply,generating,planet,player,rows).state.consumed==s.consumed
  {
    var entered:=if owned then Event(s,"primary") else s;
    if count==-1 && (!owned || pop==0) then Outcome(Done,Quantities(entered,[-1,-1,-1])) else
    var queried:=if count==-1 then Event(entered,"operating mines") else entered;
    if count==-1 && operating.Undefined? then Outcome(Undefined(operating.reason),queried) else
    var ready:=if count==-1 && !ar then Event(queried,"mining efficiency") else queried;
    var local:=Axes(ready,0,if count==-1 then operating.value else count,if count==-1 && !ar then efficiency else 10,
      home,count!=-1,ar && owned,apply,generating);
    if !local.status.Done? || !ar || !owned || count!=-1 then local else
    Remotes(local.state,rows,0,planet,player,home,apply,generating)
  }
  function FromOperating(s: State,owned: bool,pop: E.I32,ar: bool,calculation: E.Calculation,trace: A.Evaluation,value: int,
    efficiency: E.I8,home: bool,apply: bool,generating: bool,planet: S.I16,player: S.I16,rows: seq<Slot>): Outcome
    requires Shape(s)
    requires owned && pop!=0 && calculation.Integer? && calculation.result.Value? ==> -32768<=calculation.result.value<32768
    requires owned && pop!=0 && calculation.Floating? ==> A.FFloatInt(calculation.expression,trace,value)
    ensures Shape(FromOperating(s,owned,pop,ar,calculation,trace,value,efficiency,home,apply,generating,planet,player,rows).state)
  {
    var mines:=if !owned || pop==0 then E.Undefined("uncalled")
      else if calculation.Integer? then calculation.result else E.FinishFloat(calculation,trace,value);
    Estimate(s,-1,owned,pop,ar,mines,efficiency,home,apply,generating,planet,player,rows)
  }
  lemma DepletionArithmetic(conc: S.U8,level: S.U8,demand: nat)
    requires conc>1 && R.DepletionLeft(level,conc)>demand
    ensures 0<R.DepletionBand(conc)<=100
    ensures 0<12500/R.DepletionBand(conc)
    ensures (R.DepletionLeft(level,conc)-demand)*256<2147483648
  {}
  lemma YieldProjection(scaled: E.I32,generating: bool,draw: nat)
    requires scaled>=0 && draw<100
    ensures NativeYield(scaled,generating,draw)==R.Yield(scaled as nat,generating,draw)
    ensures UsesDraw(scaled,generating)==R.MiningDraws(scaled as nat,generating)
  { reveal R.Yield(); }
  lemma EligibilityProjection(f: Fleet,planet: S.I16,player: S.I16)
    requires f.orders>=0 && f.first.Order? && f.mines.Value?
    ensures R.RemoteEligible(R.MiningFleet(f.planet,f.player,f.dead,f.orders as nat,f.first.task,f.mines.value),planet,player)==
      (Candidate(f,planet,player) && f.first.task==3 && f.mines.value>0)
  {}
  lemma {:isolate_assertions} Vectors() {
    assert Deplete(100,1,1)==Depleted(99,253,2);
    assert Deplete(100,0,125)==Depleted(99,0,1);
    assert Deplete(1,0,2147483647)==Depleted(1,0,0);
    assert UsesDraw(-1,true) && NativeYield(-1,true,0)==0;
    var s:=State([Ore(0,1,0),Ore(0,1,0),Ore(0,1,0)],[7,8,9],[0,99,0],0,{},[]);
    assert Shape(s);
    assert Estimate(s,-1,false,1,false,E.Undefined("uncalled"),10,false,true,true,0,-1,[]).state.quantities==[-1,-1,-1];
    assert Axes(s,0,1,-1,false,false,false,true,true).state.consumed==0;
  }
}
