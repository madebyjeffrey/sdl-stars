include "M09-presets.dfy"
include "M09-production.dfy"

// CBuildProdItem, turn2.c. One transition per native control/array operation;
// faults mark the first undefined arithmetic/read, not a runtime error result.
module M09Builder {
  import R = M09Presets
  import O = S01Orders
  import C = S01Codecs
  import P = M09Production
  predicate I32(n: int) { -2147483648<=n<2147483648 }
  function Signed32(n: int): int
    ensures I32(Signed32(n))
  {
    assert C.Pow2(4)==16 && C.Pow2(8)==256 && C.Pow2(12)==4096 && C.Pow2(16)==65536;
    assert C.Pow2(20)==1048576 && C.Pow2(24)==16777216 && C.Pow2(28)==268435456;
    assert C.Pow2(31)==2147483648 && C.Pow2(32)==4294967296;
    C.Signed(n,32)
  }
  function Signed16(n: int): int
    ensures -32768<=Signed16(n)<32768
  {
    assert C.Pow2(4)==16 && C.Pow2(8)==256 && C.Pow2(12)==4096;
    assert C.Pow2(15)==32768 && C.Pow2(16)==65536;
    C.Signed(n,16)
  }
  datatype Partial = Absent | Empty | Value(row: R.Row)
  datatype Need = Uninitialized | Needed(amount: int)
  datatype Context = Context(alchemy: bool,cheapAlchemy: bool,calcOnly: bool,generating: bool)
  // Supplied native helper returns; adapters must establish call-time values.
  datatype Limits = Limits(operable: int,installed: nat,terraform: int,growth: int,desirability: int,warp: int,destination: int)
  predicate LimitsFit(l: Limits) {
    -32768<=l.operable<32768 && l.installed<4096 && -32768<=l.terraform<32768 &&
    I32(l.growth) && -32768<=l.desirability<32768 && -32768<=l.warp<32768
  }
  datatype Prepared = Prepared(row: R.Row,automatic: nat,manualItem: nat)
  function Prepare(row: R.Row,l: Limits): Prepared
    requires R.Fits(row) && LimitsFit(l)
    ensures R.Fits(Prepare(row,l).row) && Prepare(row,l).automatic<=1 && Prepare(row,l).manualItem<128
    ensures Prepare(row,l).row.value.cItem<=1023
  {
    if !R.Automatic(row) then Prepared(row,0,0) else
    var item:=row.value.iItem;
    var raw:=if item in {0,1,2} then Signed16(l.operable-l.installed)
      else if item in {4,5} then (if l.terraform>0 && item==4 && l.growth>=0 && l.desirability>0 then 0 else l.terraform)
      else if item==6 && (l.warp==0 || l.destination==0) then 0 else 1000;
    var cap:=if raw<0 then 0 else raw;
    var n:=if row.value.cItem>cap || item==3 then cap%1024 else row.value.cItem;
    var manual:=if item==0 then 8 else if item==1 then 7 else if item==2 then 9 else if item==3 then 11 else if item==6 then 17 else 12;
    Prepared(row.(value:=row.value.(cItem:=n)),1,manual)
  }
  // 0=quote, 1/2/3=mine/factory/defense capacity, 4=terraform,
  // 5=growth, 6=desirability, 7=packet warp. Omitted calls are not evaluated.
  function Calls(row: R.Row,l: Limits): seq<nat>
    requires R.Fits(row) && LimitsFit(l)
  {
    [0]+(if !R.Automatic(row) then [] else
      if row.value.iItem in {0,1,2} then [row.value.iItem+1] else
      if row.value.iItem in {4,5} then [4]+(if l.terraform>0 && row.value.iItem==4
        then [5]+(if l.growth>=0 then [6] else []) else []) else
      if row.value.iItem==6 then [7] else [])
  }
  datatype Phase = Paid | Afford | Progress | Charge | Alchemize | AddMineral
    | PartialAlchemy | Complete | FinalAlchemy | FinalMineral | Publish | Done | Undefined(reason: string)
  datatype State = State(caller: R.Row,prod: R.Row,partial: Partial,cost: seq<int>,paid: seq<int>,resources: seq<int>,
    built: nat,conversions: int,automatic: nat,manualItem: nat,initial: nat,pct: int,need: Need,
    mineralBlocked: bool,resourceBlocked: bool,index: nat,amount: int,status: P.Status,message: bool,phase: Phase)
  predicate Fits(s: State) {
    R.Fits(s.caller) && R.Fits(s.prod) && (s.partial.Value? ==> R.Fits(s.partial.row)) &&
    |s.cost|==4 && |s.paid|==4 && |s.resources|==4 &&
    (forall n <- s.cost+s.paid+s.resources :: I32(n)) &&
    s.automatic<=2 && s.manualItem<128 && s.index<=4 && s.built+s.prod.value.cItem<=1023 &&
    -32768<=s.conversions<32768 && s.initial<128 && (s.need.Needed? ==> I32(s.need.amount)) &&
    (s.phase==AddMineral ==> s.need.Needed? && s.index<=3) &&
    (s.phase==FinalMineral ==> s.index<=3)
  }
  function Start(row: R.Row,partial: Partial,cost: seq<int>,resources: seq<int>,limits: Limits): State
    requires R.Fits(row) && (partial.Value? ==> R.Fits(partial.row)) && LimitsFit(limits)
    requires |cost|==4 && |resources|==4 && forall n <- cost+resources :: I32(n)
    ensures Fits(Start(row,partial,cost,resources,limits))
  {
    var p:=Prepare(row,limits);
    State(row,p.row,partial,cost,[0,0,0,0],resources,0,0,p.automatic,p.manualItem,row.value.pct,100,
      Uninitialized,false,false,0,0,P.BlockedSame,false,Paid)
  }
  function Fault(s: State,why: string): State { s.(phase:=Undefined(why)) }
  // The C additions/multiplications are signed BEFORE any explicit unsigned
  // cast. Check each intermediate in source order; never assign it C meaning
  // after signed overflow. Compound unsigned resource subtraction wraps.
  function {:isolate_assertions} Step(s: State,x: Context): State
    requires Fits(s)
    ensures Fits(Step(s,x))
  {
    var i:=s.index;
    match s.phase
    case Paid =>
      if i==4 then s.(index:=0,phase:=Afford) else
      var product:=s.cost[i]*s.prod.value.pct;
      if !I32(product) then Fault(s,"initial paid multiplication") else
      s.(paid:=s.paid[i:=product%4294967296/100],index:=i+1)
    case Afford =>
      if s.prod.value.cItem==0 then s.(index:=0,phase:=FinalAlchemy) else
      if i==4 then s.(built:=s.built+1,prod:=s.prod.(value:=s.prod.value.(cItem:=s.prod.value.cItem-1,pct:=0)),index:=0,phase:=Complete) else
      var due:=s.cost[i]-s.paid[i];
      if !I32(due) then Fault(s,"remaining cost subtraction") else
      if due<=s.resources[i] then s.(index:=i+1) else
      s.(index:=0,pct:=100,mineralBlocked:=false,resourceBlocked:=false,phase:=Progress)
    case Progress =>
      if i==4 then
        (if s.mineralBlocked && s.automatic!=0 then
          (if x.alchemy then s.(phase:=Alchemize) else s.(automatic:=2,phase:=FinalAlchemy,index:=0))
         else s.(index:=0,phase:=Charge))
      else if s.cost[i]<=0 then s.(index:=i+1) else
      var sum:=s.resources[i]+s.paid[i];
      var next:=sum+1;
      // rgRes>=cost bypasses both products and the additions in C.
      if s.resources[i]<s.cost[i] && (!I32(sum) || !I32(next) || !I32(sum*100) || !I32(next*100))
      then Fault(s,"progress rounding arithmetic") else
      var low:=if s.resources[i]>=s.cost[i] then 100 else C.DivC(sum*100,s.cost[i]);
      var high:=if s.resources[i]>=s.cost[i] then 100 else C.DivC(next*100,s.cost[i])-1;
      var pct:=if low<=high then high else low;
      if pct>=s.pct then s.(index:=i+1) else
      var due:=s.cost[i]-s.paid[i];var needed:=due-s.resources[i];
      if !I32(due) || !I32(needed) then Fault(s,"mineral need subtraction") else
      s.(pct:=pct,need:=Needed(needed),mineralBlocked:=s.mineralBlocked || i!=3,
        resourceBlocked:=s.resourceBlocked || i==3,index:=i+1)
    case Charge =>
      if i==4 then
        var revised:=s.(prod:=s.prod.(value:=s.prod.value.(pct:=s.pct%128)));
        if !x.alchemy || s.resourceBlocked then revised.(index:=0,phase:=FinalAlchemy)
        else revised.(phase:=Alchemize)
      else
      var product:=s.cost[i]*s.pct;
      var due:=C.DivC(product,100)-s.paid[i];
      if !I32(product) || !I32(due) || !I32(s.resources[i]-due) || !I32(s.paid[i]+due)
      then Fault(s,"partial payment arithmetic") else
      s.(resources:=s.resources[i:=s.resources[i]-due],paid:=s.paid[i:=s.paid[i]+due],index:=i+1)
    case Alchemize =>
      if s.need.Uninitialized? then Fault(s,"uninitialized lMinNeeded") else
      var price:=P.AlchemyCost(x.cheapAlchemy);
      var affordable:=C.DivC(s.resources[3],price);
      var amount:=if affordable>s.need.amount then s.need.amount else affordable;
      if amount>0 then s.(amount:=amount,index:=0,phase:=AddMineral)
      else if amount==s.need.amount then s.(index:=0,phase:=Afford)
      else s.(phase:=PartialAlchemy)
    case AddMineral =>
      if i<3 then
        if !I32(s.resources[i]+s.amount) then Fault(s,"alchemy mineral addition") else
        s.(resources:=s.resources[i:=s.resources[i]+s.amount],index:=i+1)
      else
      var product:=P.AlchemyCost(x.cheapAlchemy)*s.amount;
      if !I32(product) then Fault(s,"alchemy price multiplication") else
      var resources:=s.resources[3:=Signed32(s.resources[3]-product%4294967296)];
      var count:=Signed16(s.conversions+s.amount%65536);
      s.(resources:=resources,conversions:=count,index:=0,phase:=if s.amount==s.need.amount then Afford else PartialAlchemy)
    case PartialAlchemy =>
      if s.resources[3]<=0 || s.partial.Absent? then s.(index:=0,phase:=FinalAlchemy) else
      var n:=s.resources[3];var price:=P.AlchemyCost(x.cheapAlchemy);
      // memset precedes rounding; retain that partial mutation on a fault.
      var revised:=s.(partial:=Value(R.Row(O.Prod(1,11,1,0),0)));
      if !I32(n*100) || !I32(n+1) || !I32((n+1)*100) then Fault(revised,"alchemy partial rounding") else
      var a:=C.DivC(n*100,price);var b:=C.DivC((n+1)*100,price)-1;
      var pct:=if a>b then a else b;
      var changed:=revised.(partial:=Value(R.Row(O.Prod(1,11,1,pct%128),0)));
      var payment:=C.DivC(pct*price,100);
      if !I32(pct*price) || !I32(n-payment) then Fault(changed,"alchemy partial payment") else
      changed.(resources:=s.resources[3:=n-payment],index:=0,phase:=FinalAlchemy)
    case Complete =>
      if i==4 then s.(index:=0,phase:=Afford) else
      var due:=s.cost[i]-s.paid[i];
      if !I32(due) || !I32(s.resources[i]-due) then Fault(s,"completion payment") else
      s.(resources:=s.resources[i:=s.resources[i]-due],paid:=s.paid[i:=0],index:=i+1)
    case FinalAlchemy =>
      if s.built>0 && s.prod.value.grobj==1 && s.prod.value.iItem in {3,11}
      then s.(conversions:=Signed16(s.conversions+s.built),index:=0,phase:=FinalMineral)
      else s.(phase:=Publish)
    case FinalMineral =>
      if i==3 then s.(phase:=Publish) else
      if !I32(s.resources[i]+s.built) then Fault(s,"completed alchemy mineral addition") else
      s.(resources:=s.resources[i:=s.resources[i]+s.built],index:=i+1)
    case Publish =>
      var status:=P.Result(s.automatic,s.built,s.prod.value.cItem,s.initial,s.prod.value.pct);
      var partial:=if s.automatic!=0 && !s.partial.Absent? &&
        (s.partial.Empty? || s.partial.row.value.cItem==0) && s.prod.value.pct>0
        then Value(s.prod.(value:=s.prod.value.(cItem:=1,iItem:=s.manualItem))) else s.partial;
      s.(caller:=if !x.calcOnly && s.automatic==0 then s.prod else s.caller,
        partial:=partial,status:=status,message:=P.AlchemyMessage(s.conversions,x.calcOnly,x.generating),phase:=Done)
    case Done => s
    case Undefined(_) => s
  }
  // A bounded trace is explicit about uncompleted execution; it is not a
  // fabricated total-termination theorem for arbitrary signed native inputs.
  function Run(s: State,x: Context,fuel: nat): State
    requires Fits(s)
    ensures Fits(Run(s,x,fuel))
    decreases fuel
  { if fuel==0 || s.phase.Done? || s.phase.Undefined? then s else Run(Step(s,x),x,fuel-1) }
  predicate WithoutAlchemy(s: State) { s.phase !in {Alchemize,AddMineral,PartialAlchemy} }
  function Rank(s: State): nat
    requires s.index<=4
  {
    if s.phase.Done? || s.phase.Undefined? then 0 else
    100*s.prod.value.cItem+(match s.phase
      case Paid => 100-s.index case Afford => 80-s.index
      case Progress => 70-s.index case Charge => 60-s.index
      case Complete => 90-s.index case FinalAlchemy => 10
      case FinalMineral => 8-s.index case Publish => 1
      case _ => 0)
  }
  lemma {:isolate_assertions} NoAlchemyStepProgress(s: State,x: Context)
    requires Fits(s) && WithoutAlchemy(s) && !x.alchemy && !s.phase.Done? && !s.phase.Undefined?
    ensures WithoutAlchemy(Step(s,x)) && Rank(Step(s,x))<Rank(s)
  { }
  function FinishNoAlchemy(s: State,x: Context): State
    requires Fits(s) && WithoutAlchemy(s) && !x.alchemy
    ensures Fits(FinishNoAlchemy(s,x))
    ensures FinishNoAlchemy(s,x).phase.Done? || FinishNoAlchemy(s,x).phase.Undefined?
    decreases Rank(s)
  {
    if s.phase.Done? || s.phase.Undefined? then s else
    NoAlchemyStepProgress(s,x);
    FinishNoAlchemy(Step(s,x),x)
  }
  lemma CompletedCountBound(s: State,x: Context)
    requires Fits(s)
    ensures Step(s,x).built<=1023
    ensures s.phase.Done? ==> Step(s,x)==s
  {}
  lemma PaidChargeBalance(s: State,x: Context)
    requires Fits(s) && s.phase==Charge && s.index<4
    ensures !Step(s,x).phase.Undefined? ==>
      Step(s,x).resources[s.index]+Step(s,x).paid[s.index]==s.resources[s.index]+s.paid[s.index]
  {}
  lemma AutomaticRetainsCaller(s: State,x: Context)
    requires Fits(s) && s.phase==Publish && s.automatic!=0
    ensures Step(s,x).caller==s.caller
  {}
  lemma CompletePaymentBalance(s: State,x: Context)
    requires Fits(s) && s.phase==Complete && s.index<4
    ensures !Step(s,x).phase.Undefined? ==>
      Step(s,x).resources[s.index]+Step(s,x).paid[s.index]==s.resources[s.index]+s.paid[s.index]-s.cost[s.index]
  {}
  lemma AutoBlockSkipsPayment(s: State,x: Context)
    requires Fits(s) && s.phase==Progress && s.index==4 && s.mineralBlocked && s.automatic!=0 && !x.alchemy
    ensures Step(s,x).automatic==2 && Step(s,x).phase==FinalAlchemy
    ensures Step(s,x).resources==s.resources && Step(s,x).paid==s.paid && Step(s,x).prod==s.prod
  {}
  lemma MissingNeedBoundary(s: State,x: Context)
    requires Fits(s) && s.phase==Alchemize && s.need==Uninitialized
    ensures Step(s,x).phase==Undefined("uninitialized lMinNeeded")
  {}
  lemma AutoPartialRetainsUnused(s: State,x: Context)
    requires Fits(s) && s.phase==Publish && s.automatic!=0 && s.partial==Empty && s.prod.value.pct>0
    ensures Step(s,x).partial.Value? && Step(s,x).partial.row.unused==s.prod.unused
    ensures Step(s,x).partial.row.value.cItem==1 && Step(s,x).partial.row.value.iItem==s.manualItem
  {}
  lemma {:isolate_assertions} Vectors()
    ensures Prepare(R.Row(O.Prod(20,0,1,0),31),Limits(12,5,0,0,0,0,0)).row.value.cItem==7
    ensures Prepare(R.Row(O.Prod(1,3,1,0),0),Limits(0,0,0,0,0,0,0)).row.value.cItem==1000
    ensures Prepare(R.Row(O.Prod(5,4,1,0),0),Limits(0,0,5,0,1,0,0)).row.value.cItem==0
    ensures Prepare(R.Row(O.Prod(5,4,1,0),0),Limits(0,0,5,-1,1,0,0)).row.value.cItem==5
    ensures Prepare(R.Row(O.Prod(1,0,1,0),0),Limits(-32768,1,0,0,0,0,0)).row.value.cItem==1
    ensures P.DimensionPercent(10,5,0)==59
    ensures Calls(R.Row(O.Prod(5,4,1,0),0),Limits(0,0,5,-1,1,0,0))==[0,4,5]
    ensures Calls(R.Row(O.Prod(5,4,1,0),0),Limits(0,0,5,0,1,0,0))==[0,4,5,6]
  {
    assert C.Pow2(4)==16 && C.Pow2(8)==256 && C.Pow2(12)==4096;
    assert C.Pow2(15)==32768 && C.Pow2(16)==65536;
  }
}
