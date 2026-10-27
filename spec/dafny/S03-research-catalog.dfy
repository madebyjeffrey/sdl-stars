include "S03-catalogs.dfy"

// Fresh generated dataids.h, ascending HullSlotType bit order; hstHull ends at
// ihuldefOrbitalFort, while hstSBHull uses isbhullCount. Native fixture binds
// these literals to the generated enum values and actual FLookupPart bounds.
module S03ResearchCatalog {
  import S = S01
  function Masks(): seq<S.U16> { [1,2,4,8,16,32,64,128,256,512,1024,2048,4096,8192,16384,32768] }
  function Limits(): seq<nat>
    ensures |Limits()|==16
    ensures forall i | 0<=i<16 :: Limits()[i]<256
  { [16,16,10,12,24,12,15,8,10,16,5,17,11,20,32,15] }
  function Calls(limits: seq<nat>,kind: nat): nat
    requires kind<=|limits|
    decreases |limits|-kind
  { if kind==|limits| then 0 else limits[kind]+1+Calls(limits,kind+1) }
  lemma CurrentCalls()
    ensures Calls(Limits(),0)==255
  {
    assert Calls(Limits(),15)==16;
    assert Calls(Limits(),14)==49;
    assert Calls(Limits(),13)==70;
    assert Calls(Limits(),12)==82;
    assert Calls(Limits(),11)==100;
    assert Calls(Limits(),10)==106;
    assert Calls(Limits(),9)==123;
    assert Calls(Limits(),8)==134;
    assert Calls(Limits(),7)==143;
    assert Calls(Limits(),6)==159;
    assert Calls(Limits(),5)==172;
    assert Calls(Limits(),4)==197;
    assert Calls(Limits(),3)==210;
    assert Calls(Limits(),2)==221;
    assert Calls(Limits(),1)==238;
    assert Calls(Limits(),0)==255;
  }
}
