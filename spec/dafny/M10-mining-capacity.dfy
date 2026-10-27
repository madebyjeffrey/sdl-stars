include "M08-native-economy.dfy"

// ship2.c CMineFromLpfl. Slot abilities are explicit returning FLookupPart
// projections; availability, native pointers and lookup failure frames are separate.
module M10MiningCapacity {
  import E = M08NativeEconomy
  import S = S01
  import C = S01Codecs
  datatype Slot = Slot(kind: S.U16,item: S.U8,count: S.U8,ability: S.I16,returned: bool)
  datatype Design = Design(count: S.I16,extent: S.U8,slots: seq<Slot>)
  function SlotSum(slots: seq<Slot>,extent: nat,at: nat,total: E.I32): E.Number
    requires at<=extent
    decreases extent-at
  {
    if at==extent then E.Value(total) else if at>=|slots| || at>=16 then E.Undefined("hull slot extent") else
    var slot:=slots[at];
    if slot.kind!=128 || slot.item>6 then SlotSum(slots,extent,at+1,total) else
    if !slot.returned then E.Undefined("mining part pointer/result") else
    // Compound addition promotes to uint32; assignment preserves low 32 bits.
    SlotSum(slots,extent,at+1,E.Signed32(total+(slot.count as int)*slot.ability))
  }
  function DesignSum(designs: seq<Design>,at: nat,total: E.I32): E.Number
    requires at<=16
    decreases 16-at
  {
    if at==16 then E.Value(if total<4000 then total else 4000) else
    if at>=|designs| then E.Undefined("sixteen design/count slots") else
    var design:=designs[at];
    if design.count<=0 then DesignSum(designs,at+1,total) else
    var slots:=SlotSum(design.slots,design.extent,0,0);
    if slots.Undefined? then slots else
    var product:=slots.value*design.count;
    if !E.Fits32(product) then E.Undefined("fleet mining multiplication") else
    DesignSum(designs,at+1,E.Signed32(total+product))
  }
  function Capacity(designs: seq<Design>): E.Number { DesignSum(designs,0,0) }
  lemma CapacityCap(designs: seq<Design>)
    ensures Capacity(designs).Value? ==> Capacity(designs).value<=4000
  { DesignCap(designs,0,0); }
  lemma DesignCap(designs: seq<Design>,at: nat,total: E.I32)
    requires at<=16
    ensures DesignSum(designs,at,total).Value? ==> DesignSum(designs,at,total).value<=4000
    decreases 16-at
  {
    if at<16 && at<|designs| {
      var d:=designs[at];
      if d.count<=0 { DesignCap(designs,at+1,total); } else {
        var slots:=SlotSum(d.slots,d.extent,0,0);
        if slots.Value? && E.Fits32(slots.value*d.count) { DesignCap(designs,at+1,E.Signed32(total+slots.value*d.count)); }
      }
    }
  }
  lemma {:isolate_assertions} Vectors() {
    assert C.Pow2(4)==16 && C.Pow2(8)==256 && C.Pow2(12)==4096 && C.Pow2(16)==65536;
    assert C.Pow2(20)==1048576 && C.Pow2(24)==16777216 && C.Pow2(28)==268435456;
    assert C.Pow2(31)==2147483648 && C.Pow2(32)==4294967296;
    assert SlotSum([Slot(128,0,2,5,true)],1,0,0)==E.Value(10);
    assert SlotSum([Slot(128,7,2,5,false)],1,0,0)==E.Value(0);
    assert SlotSum([],17,0,0).Undefined?;
    assert SlotSum([Slot(128,0,1,5,false)],1,0,0).Undefined?;
  }
}
