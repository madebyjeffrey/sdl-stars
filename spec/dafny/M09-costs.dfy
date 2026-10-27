include "M09-production.dfy"
include "S01-orders.dfy"

// GetProductionCosts numeric caller contract. Quote words come from M11's
// actual pricing calls in source order; pointer/catalog/quote production is
// a separate adapter, not an assertion that GetTruePartCost is total/pure.
module M09Costs {
  import S = S01
  import C = S01Codecs
  import O = S01Orders
  import P = M09Production
  import A = S04Arithmetic
  type I8 = n: int | -128<=n<128
  function U16(n: int): S.U16 { n%65536 }
  function U32(n: int): S.U32 { n%4294967296 }
  function I16(n: int): int { C.Signed(n,16) }
  function Div(n: int): int { C.DivC(n,10) }
  datatype Slot = Slot(kind: S.U16,item: S.U8,count: S.U8,quote: seq<S.U16>)
  predicate SlotFits(s: Slot) { |s.quote|==4 }
  datatype Design = Design(free: bool,hull: S.U16,total: seq<S.U16>,slots: seq<Slot>,used: S.U8)
  predicate DesignFits(d: Design) { |d.total|==4 && |d.slots|==16 && forall s <- d.slots :: SlotFits(s) }
  datatype Inputs = Inputs(ships: seq<Design>,bases: seq<Design>,base: bool,current: nat,
    bare: seq<S.U16>,colony: I8,pricing: I8,cheap: bool,isb: bool,tt: bool,alchemy: bool,tutorial: bool,
    factory: I8,mine: I8,defOre: seq<S.I16>,defRes: S.U16,genesis: seq<S.U16>,scanners: seq<seq<S.U16>>)
  predicate InputFits(x: Inputs) {
    |x.ships|==16 && |x.bases|==10 && x.current<16 &&
    (forall d <- x.ships+x.bases :: DesignFits(d)) && |x.bare|==4 && |x.defOre|==3 &&
    |x.genesis|==4 && |x.scanners|==9 && (forall q <- x.scanners :: |q|==4)
  }
  function ChangedHull(newCost: S.U16,oldCost: S.U16): S.U16 {
    var half:=newCost/2;
    var upgrade:=I16(newCost-C.DivC(I16(oldCost),2));
    U16(A.Max(half,upgrade))
  }
  // Stored signed words are multiplied and narrowed, then the 3/7 or 2/8
  // products are explicitly narrowed AGAIN before signed /10 in the source.
  function SlotCost(oldWord: S.U16,newWord: S.U16,oldCount: S.U8,newCount: S.U8,sameKind: bool,sameItem: bool): int {
    var oldCost:=I16(I16(oldWord)*oldCount);
    var newCost:=I16(I16(newWord)*newCount);
    I16(if !sameKind then A.Max(Div(I16(3*newCost)),newCost-Div(I16(7*oldCost)))
      else if !sameItem then A.Max(Div(I16(2*newCost)),newCost-Div(I16(8*oldCost)))
      else A.Max(0,newCost-oldCost))
  }
  function Credit(remaining: S.U16,oldWord: S.U16,newWord: S.U16,oldCount: S.U8,newCount: S.U8,sameKind: bool,sameItem: bool): S.U16
    ensures Credit(remaining,oldWord,newWord,oldCount,newCount,sameKind,sameItem)<=remaining
  {
    var next:=I16(I16(newWord)*newCount);
    var retained:=SlotCost(oldWord,newWord,oldCount,newCount,sameKind,sameItem);
    var credit:=U16(next-retained);
    U16(remaining-A.Min(remaining,credit))
  }
  lemma NativeCreditEquivalent(remaining: S.U16,next: int,retained: int)
    ensures U16(remaining-(if remaining>=U16(next-retained) then next-retained else remaining))==
      remaining-A.Min(remaining,U16(next-retained))
  {
    if remaining>=U16(next-retained) {
      assert U16(remaining-(next-retained))==U16(remaining-U16(next-retained));
    }
  }
  function Slots(cost: seq<S.U16>,priorSlots: seq<Slot>,next: seq<Slot>,i: nat,n: nat): seq<S.U16>
    requires |cost|==4 && |priorSlots|==16 && |next|==16 && i<=n<=16
    requires forall s <- priorSlots+next :: SlotFits(s)
    ensures |Slots(cost,priorSlots,next,i,n)|==4
    ensures forall j | 0<=j<4 :: Slots(cost,priorSlots,next,i,n)[j]<=cost[j]
    decreases n-i
  {
    if i==n then cost else
      var a:=priorSlots[i];var b:=next[i];
      assert a in priorSlots+next && b in priorSlots+next;
      assert SlotFits(a) && SlotFits(b);
      var revised:=if a.count==0 || b.count==0 then cost else
        seq(4,j requires 0<=j<4 => Credit(cost[j],a.quote[j],b.quote[j],a.count,b.count,a.kind==b.kind,a.item==b.item));
      Slots(revised,priorSlots,next,i+1,n)
  }
  function Discounted(cost: S.U16,base: bool,discount: bool): S.U32 {
    if !base then cost else U32(P.BaseCost(cost,discount))
  }
  datatype Result = Quoted(values: seq<S.U32>) | InvalidDesign | InvalidCurrentBase | InvalidSlots
  function Ship(x: Inputs,item: nat): Result
    requires InputFits(x)
    ensures Ship(x,item).Quoted? ==> |Ship(x,item).values|==4
  {
    var base:=item>=16;
    var index:=if base then item-16 else item;
    if (!base && index>=16) || (base && index>=10) then InvalidDesign else
    var design:=if base then x.bases[index] else x.ships[index];
    assert design in x.ships+x.bases;
    assert DesignFits(design);
    if design.free then Quoted([0,0,0,0]) else
    if base && x.base && x.current>=10 then InvalidCurrentBase else
    var prior:=if base && x.base then x.bases[x.current] else design;
    assert prior in x.ships+x.bases;
    assert DesignFits(prior);
    if base && x.base && prior.hull==design.hull && prior.used>16 then InvalidSlots else
    var initial:=if !base || !x.base then design.total
      else if prior.hull!=design.hull then seq(4,j requires 0<=j<4 => ChangedHull(design.total[j],prior.total[j]))
      else
        var unhulled:=seq(4,j requires 0<=j<4 => U16(design.total[j]-I16(x.bare[j])));
        Slots(unhulled,prior.slots,design.slots,0,prior.used);
    Quoted(seq(4,j requires 0<=j<4 => Discounted(initial[j],base,x.isb || x.pricing==8)))
  }
  function Planetary(x: Inputs,item: nat,prior: seq<S.U32>): seq<S.U32>
    requires InputFits(x) && |prior|==4
    ensures |Planetary(x,item,prior)|==4
  {
    if item in {1,7} then
      var c:=if x.cheap then 1 else 0;
      if x.tutorial then [U32(2-c),U32(2-c),U32(2-c),U32(x.factory)] else [0,0,U32(4-c),U32(x.factory)]
    else if item in {0,8} then [0,0,0,U32(x.mine)]
    else if item in {2,9} then
      var raw: seq<S.U32> := [U32(x.defOre[0]),U32(x.defOre[1]),U32(x.defOre[2]),x.defRes];
      if x.pricing==4 then seq(4,j requires 0<=j<4 => U32(U32((raw[j] as int)*3)/5)) else raw
    else if item in {3,11} then [0,0,0,U32(P.AlchemyCost(x.alchemy))]
    else if item in {4,5,12} then [0,0,0,U32(P.TerraformCost(x.tt,x.colony==3))]
    else if item==13 then seq(4,j requires 0<=j<4 => U32(x.genesis[j]))
    else if item in {14,15,16} then
      var ore:=if x.colony==6 then 70 else if x.colony==7 then 120 else 110;
      [if item==14 then ore else 0,if item==15 then ore else 0,if item==16 then ore else 0,if x.colony==6 then 5 else 10]
    else if item in {6,17} then
      var ore:=if x.colony==6 then 25 else if x.colony==7 then 48 else 44;
      [ore,ore,ore,if x.colony==6 then 5 else 10]
    else if 18<=item<=27 then
      var index:=if item==27 then 0 else item-18;
      seq(4,j requires 0<=j<4 => U32(x.scanners[index][j]))
    else prior
  }
  function Scale(values: seq<S.U32>,count: nat): seq<S.U32>
    requires |values|==4
    ensures |Scale(values,count)|==4
  { [U32((values[0] as int)*count),U32((values[1] as int)*count),
     U32((values[2] as int)*count),U32((values[3] as int)*count)] }
  lemma ScaleIdentity(values: seq<S.U32>)
    requires |values|==4
    ensures Scale(values,1)==values
    ensures Scale(values,0)==[0,0,0,0]
  { assert values==[values[0],values[1],values[2],values[3]]; }
  function Cost(x: Inputs,row: O.Prod,prior: seq<S.U32>,onlyOne: bool): Result
    requires InputFits(x) && O.FProd(row) && |prior|==4
  {
    var unit:=if row.grobj==2 then Ship(x,row.iItem) else Quoted(Planetary(x,row.iItem,prior));
    if !unit.Quoted? then unit else
    Quoted(Scale(unit.values,if onlyOne then 1 else row.cItem))
  }
  lemma {:isolate_assertions} {:fuel Slots,3} {:fuel Cost,3} {:fuel Ship,3} {:fuel Planetary,3} Vectors() {
    assert C.Pow2(4)==16 && C.Pow2(8)==256 && C.Pow2(12)==4096;
    assert C.Pow2(15)==32768 && C.Pow2(16)==65536;
    assert ChangedHull(100,80)==P.ChangedHullCost(100,80);
    assert ChangedHull(65535,65535)==32767 && P.ChangedHullCost(65535,65535)==32768;
    assert SlotCost(80,100,1,1,false,false)==44;
    assert Credit(1000,80,100,1,1,false,false)==944;
    assert Credit(40,80,100,1,1,false,false)==0;
    assert Discounted(101,true,true)==41;
    assert Scale([4294967295,2147483648,17,29],1023)==[4294966273,2147483648,17391,29667];
    var empty:=Slot(0,0,0,[0,0,0,0]);
    var d:=Design(false,1,[100,200,300,400],seq(16,i=>empty),0);
    var x:=Inputs(seq(16,i=>d),seq(10,i=>d),false,0,[0,0,0,0],6,4,false,false,false,false,false,10,3,[10,20,30],100,[7,8,9,10],seq(9,i=>[20,30,40,50]));
    assert InputFits(x);
    assert Planetary(x,14,[9,9,9,9])==[70,0,0,5];
    assert Planetary(x,2,[9,9,9,9])==[6,12,18,60];
    assert Planetary(x,10,[9,9,9,9])==[9,9,9,9];
    assert Scale([9,9,9,9],3)==[27,27,27,27];
    assert x.ships[0]==d && !d.free && d.total==[100,200,300,400];
    assert Ship(x,0)==Quoted(seq(4,j requires 0<=j<4 => Discounted(d.total[j],false,false)));
    assert (seq(4,j requires 0<=j<4 => Discounted(d.total[j],false,false)))==[100,200,300,400];
    assert Ship(x,0)==Quoted([100,200,300,400]);
    ScaleIdentity([100,200,300,400]);
    assert Cost(x,O.Prod(3,10,1,0),[9,9,9,9],false)==Quoted([27,27,27,27]);
    assert Cost(x,O.Prod(0,0,2,0),[9,9,9,9],true)==Quoted([100,200,300,400]);
    assert Ship(x,26)==InvalidDesign;
  }
}
