include "M09-presets.dfy"
include "M09-production.dfy"

module M09Editor {
  import R = M09Presets
  import O = S01Orders
  import P = M09Production
  import C = S01Codecs
  import A = S04Arithmetic
  predicate Rows(q: seq<R.Row>) { forall i | 0<=i<|q| :: R.Fits(q[i]) }
  function Entry(count: nat,item: nat,kind: nat): R.Row
    requires item<128 && kind<8
    ensures R.Fits(Entry(count,item,kind))
  { R.Row(O.Prod(count%1024,item,kind,0),0) }
  function Count(r: R.Row,n: nat): R.Row
    requires R.Fits(r) && n<1024
    ensures R.Fits(Count(r,n))
  { r.(value:=r.value.(cItem:=n)) }
  datatype Design = Design(free: bool,gift: bool,mass: nat)
  function Ships(ds: seq<Design>,index: nat,base: bool,capacity: nat): seq<R.Row>
    requires |ds|==16 && index<=16
    ensures |Ships(ds,index,base,capacity)|<=16-index && Rows(Ships(ds,index,base,capacity))
    decreases 16-index
  {
    if index==16 then [] else
      (if base && capacity!=0 && !ds[index].free && !ds[index].gift && ds[index].mass<=capacity
       then [Entry(1023,index,2)] else [])+Ships(ds,index+1,base,capacity)
  }
  function Bases(ds: seq<Design>,index: nat,base: bool,current: nat): seq<R.Row>
    requires |ds|==10 && index<=10
    ensures |Bases(ds,index,base,current)|<=10-index && Rows(Bases(ds,index,base,current))
    decreases 10-index
  {
    if index==10 then [] else
      (if !ds[index].free && !ds[index].gift && (current!=index || !base) then [Entry(1,index+16,2)] else [])+
      Bases(ds,index+1,base,current)
  }
  function Installation(maximum: int,built: nat,item: nat,capped: bool): seq<R.Row>
    requires item<128
    ensures |Installation(maximum,built,item,capped)|<=1 && Rows(Installation(maximum,built,item,capped))
  {
    var remaining:=C.Signed(maximum-built,16);
    if remaining<=0 then [] else [Entry(if capped then A.Min(remaining,1020) else remaining,item,1)]
  }
  function Automatic(primary: int,index: nat): seq<R.Row>
    requires index<=7
    ensures |Automatic(primary,index)|<=7-index && Rows(Automatic(primary,index))
    decreases 7-index
  { if index==7 then [] else (if R.Allowed(index,primary) then [Entry(1023,index,1)] else [])+Automatic(primary,index+1) }
  datatype Inputs = Inputs(ships: seq<Design>,bases: seq<Design>,base: bool,current: nat,capacity: nat,
    genesis: bool,warp: int,maxFactory: int,factories: nat,maxMine: int,mines: nat,maxDefense: int,defenses: nat,
    scanner: nat,terraform: int,primary: int)
  predicate InputFits(x: Inputs) {
    |x.ships|==16 && |x.bases|==10 && x.current<16 && (!x.base || x.current<10) && x.capacity<65536 && x.scanner<32 &&
    (forall d <- x.ships :: d.mass<65536) &&
    -32768<=x.warp<32768 && -32768<=x.terraform<32768 &&
    -32768<=x.maxFactory<32768 && -32768<=x.maxMine<32768 && -32768<=x.maxDefense<32768 &&
    x.factories<4096 && x.mines<4096 && x.defenses<4096
  }
  function Catalog(x: Inputs): seq<R.Row>
    requires InputFits(x)
    ensures |Catalog(x)|<=44 && Rows(Catalog(x))
  {
    Ships(x.ships,0,x.base,x.capacity)+Bases(x.bases,0,x.base,x.current)+
    (if x.genesis then [Entry(1,13,1)] else [])+
    (if x.warp>0 then [Entry(1023,14,1),Entry(1023,15,1),Entry(1023,16,1),Entry(1023,17,1)] else [])+
    Installation(x.maxFactory,x.factories,7,true)+Installation(x.maxMine,x.mines,8,true)+
    Installation(x.maxDefense,x.defenses,9,false)+[Entry(1023,11,1)]+
    (if x.scanner==31 && x.primary!=8 then [Entry(1,27,1)] else [])+
    (if x.terraform>0 then [Entry(x.terraform,12,1)] else [])+Automatic(x.primary,0)
  }
  function PhysicalCatalog(x: Inputs): seq<R.Row>
    requires InputFits(x)
    ensures |PhysicalCatalog(x)|==64 && Rows(PhysicalCatalog(x))
  { Catalog(x)+seq(64-|Catalog(x)|,i=>R.ZeroRow()) }
  datatype Initial = Initial(capacity: nat,rows: seq<R.Row>,savedResearch: bool)
  function AllocateInitial(hasQueue: bool,prior: seq<R.Row>,research: bool): Initial
    requires |prior|<=255
  { Initial(if hasQueue then |prior| else 2,if hasQueue then prior else [],research) }
  datatype Sanitation = Sanitized(buffer: seq<R.Row>,count: nat,catalog: seq<R.Row>)
    | OverlappingCopy(buffer: seq<R.Row>,count: nat,catalog: seq<R.Row>,offset: nat,rows: nat)
    | OutsideBuffer
  function Sanitize(buffer: seq<R.Row>,count: nat,catalog: seq<R.Row>,active: nat,index: nat,offset: nat): Sanitation
    requires count<=|buffer| && active<=|catalog| && Rows(buffer) && Rows(catalog)
    decreases if index<count then count-index else 0
  {
    if index>=count then Sanitized(buffer,count,catalog)
    else if offset>=|buffer| then OutsideBuffer else
    var src:=R.Find(catalog,buffer[offset].value,0,active);
    if src==active then
      if index+1<count then
        if count-index-1>1 then OverlappingCopy(buffer,count,catalog,offset,count-index-1)
        else if offset+1>=|buffer| then OutsideBuffer
        else Sanitize(buffer[offset:=buffer[offset+1]],count-1,catalog,active,index,offset+1)
      else Sanitize(buffer,count-1,catalog,active,index+1,offset+1)
    else
      var taken:=A.Min(catalog[src].value.cItem,buffer[offset].value.cItem);
      var updated:=buffer[offset:=Count(buffer[offset],taken)];
      var remaining:=if catalog[src].value.cItem==1023 then catalog
        else catalog[src:=Count(catalog[src],catalog[src].value.cItem-taken)];
      Sanitize(updated,count,remaining,active,index+1,offset+1)
  }
  function Visible(catalog: seq<R.Row>,active: nat,index: nat,rank: nat): nat
    requires index<=active<=|catalog|
    ensures index<=Visible(catalog,active,index,rank)<=active
    decreases active-index
  {
    if index==active then active else
    if catalog[index].value.cItem!=0 then
      if rank==0 then index else Visible(catalog,active,index+1,rank-1)
    else Visible(catalog,active,index+1,rank)
  }
  function Destination(count: nat,selection: int): int
    requires -1<=selection<=count
    ensures -1<=Destination(count,selection)<=count
  { if selection<0 then (if count==0 then 0 else count-1) else selection-1 }
  datatype Place = Merge(index: nat) | Insert(index: nat)
  function Locate(q: seq<R.Row>,selection: int,value: O.Prod): Place
    requires -1<=selection<=|q|
    ensures Locate(q,selection,value).index<=|q|
    ensures Locate(q,selection,value).Merge? ==> Locate(q,selection,value).index<|q|
  {
    var dst:=Destination(|q|,selection);
    if dst>=|q| then Insert(dst) else
    if dst>=0 && O.FSameItem(q[dst].value,value) then Merge(dst) else
    var next:=dst+1;
    if next<|q| && O.FSameItem(q[next].value,value) then Merge(next) else Insert(next)
  }
  datatype Effects = Effects(queueFill: bool,sourceFill: bool,selection: int,redraw: bool,beep: bool)
  datatype Command = Changed(queue: seq<R.Row>,catalog: seq<R.Row>,capacity: nat,effects: Effects)
    | CatalogOutside | ReallocationEscape(catalog: seq<R.Row>)
  function Add(q: seq<R.Row>,catalog: seq<R.Row>,active: nat,capacity: nat,sourceSelection: int,selection: int,
               shift: bool,ctrl: bool,reallocationOK: bool): Command
    requires |q|<=255 && capacity<=255 && active<=|catalog| && Rows(q) && Rows(catalog)
    requires sourceSelection>=-1 && -1<=selection<=|q|
  {
    if sourceSelection<0 then Changed(q,catalog,capacity,Effects(false,false,-1,false,false)) else
    var src:=Visible(catalog,active,0,sourceSelection);
    if src==|catalog| then CatalogOutside else
    var count:=if !shift && !ctrl then 1 else A.Min(catalog[src].value.cItem,P.Quantum(shift,ctrl));
    var prod:=Count(catalog[src],count);
    var remaining:=if catalog[src].value.cItem==1023 then catalog else
      catalog[src:=Count(catalog[src],(catalog[src].value.cItem-count)%1024)];
    var place:=Locate(q,selection,prod.value);
    if place.Merge? then
      var prior:=q[place.index];
      var total:=P.AddCount(prior.value.cItem,count,prior.value.grobj==1 && prior.value.iItem==3);
      Changed(q[place.index:=Count(prior,total)],remaining,capacity,Effects(true,remaining[src].value.cItem==0,place.index+1,true,false))
    else if |q|>=40 then Changed(q,remaining,capacity,Effects(false,false,-1,true,true))
    else if |q|==capacity && !reallocationOK then ReallocationEscape(remaining)
    else
      Changed(q[..place.index]+[prod]+q[place.index..],remaining,if |q|==capacity then |q|+4 else capacity,
        Effects(true,remaining[src].value.cItem==0,place.index+1,true,false))
  }
  function Remove(q: seq<R.Row>,catalog: seq<R.Row>,active: nat,capacity: nat,selection: int,shift: bool,ctrl: bool): Command
    requires |q|<=255 && capacity<=255 && active<=|catalog| && Rows(q) && Rows(catalog)
    requires -1<=selection<=|q|
  {
    if selection<=0 then Changed(q,catalog,capacity,Effects(false,false,-1,false,false)) else
    var i:=selection-1; var prod:=q[i]; var src:=R.Find(catalog,prod.value,0,active);
    if src==|catalog| then CatalogOutside else
    var n:=A.Min(prod.value.cItem,if prod.value.grobj==1 && prod.value.iItem==3 then 1020 else P.Quantum(shift,ctrl));
    var remaining:=catalog[src:=R.ReturnQuantity(catalog[src],n)];
    var value:=prod.value.cItem-n;
    var rows:=if value==0 then q[..i]+q[i+1..] else q[i:=Count(prod,value)];
    var selected:=if value!=0 || i+1<|q| then i+1 else if i>0 then i else -1;
    Changed(rows,remaining,capacity,Effects(true,catalog[src].value.cItem==0,selected,true,false))
  }
  function Move(q: seq<R.Row>,selection: int,down: bool): seq<R.Row>
    requires -1<=selection<=|q|
  {
    var left:=if down then selection-1 else selection-2;
    if left<0 || left+1>=|q| then q else q[left:=q[left+1]][left+1:=q[left]]
  }
  lemma MovePreserves(q: seq<R.Row>,selection: int,down: bool)
    requires -1<=selection<=|q|
    ensures multiset(Move(q,selection,down))==multiset(q)
  {
    var left:=if down then selection-1 else selection-2;
    if 0<=left && left+1<|q| {
      assert q==q[..left]+[q[left],q[left+1]]+q[left+2..];
      assert Move(q,selection,down)==q[..left]+[q[left+1],q[left]]+q[left+2..];
    }
  }
  lemma {:isolate_assertions} {:fuel C.Pow2,18} {:fuel Sanitize,4} Vectors() {
    var a:=Entry(10,7,1); var b:=Entry(1,8,1); var c:=Entry(1,9,1);
    assert Rows([a,b,c]);
    assert Locate([a,b],1,c.value)==Insert(1);
    assert Locate([a,b],0,c.value)==Insert(0);
    assert Locate([a,b],1,b.value)==Merge(1);
    assert Locate([a,b],-1,a.value)==Insert(2);
    assert Installation(1024,0,9,false)==[Entry(0,9,1)];
    assert Installation(32767,4095,7,true)==[Entry(1020,7,1)];
    assert Installation(-32768,1,7,true)==[Entry(1020,7,1)];
    var changed:=Sanitize([a,b],2,[b],1,0,0);
    assert changed.Sanitized? && changed.count==1 && changed.buffer[0]==b;
    var large:=Count(b,5);
    assert Sanitize([a,large],2,[b],1,0,0).buffer==[large,b];
    assert Sanitize([a,b,c],3,[b,c],2,0,0).OverlappingCopy?;
    assert Add([a,b],[c],1,4,0,1,false,false,true).queue==[a,c,b];
    assert Add([a,b],[b],1,4,0,1,false,false,true).queue[1].value.cItem==2;
    var full:=seq(40,i=>a);
    assert Add(full,[c],1,40,0,0,false,false,true).effects.beep;
    assert Add(full,[c],1,40,0,0,false,false,true).catalog[0].value.cItem==0;
    assert !Add(full,[c],1,40,0,0,false,false,true).effects.sourceFill;
    assert Add([], [a],1,0,0,-1,false,false,false).ReallocationEscape?;
    assert Add([], [R.ZeroRow()],0,0,0,-1,false,false,true).catalog[0].value.cItem==1023;
    var alchemy:=Entry(1023,3,1);
    assert Add([], [alchemy],1,0,0,-1,true,true,true).queue[0].value.cItem==1020;
    assert Add([Entry(1020,3,1)], [alchemy],1,1,0,1,true,true,true).queue[0].value.cItem==1;
    assert Remove([alchemy],[alchemy],1,1,1,false,false).queue[0].value.cItem==3;
    assert Remove([b],[b],1,1,1,false,false).queue==[];
    assert Remove([b],[b],1,1,1,false,false).effects.selection==-1;
    assert AllocateInitial(false,[],true)==Initial(2,[],true);
  }
}
