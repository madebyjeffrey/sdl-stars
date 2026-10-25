include "S01-codecs.dfy"
module S03Catalogs {
  predicate AppendOnly<T(==)>(prior: seq<T>,next: seq<T>) { |next|>=|prior| && next[..|prior|]==prior }
  function PartId(index: nat): nat { index+1 }
  function BaseHull(index: nat): nat { index+32 }
  function TextChunk(id: nat): nat { id/64 }
  function TextOffset(id: nat): nat { id%64 }
  function Nibbles(rank: nat): seq<nat> { seq(rank/15,i=>15)+[rank%15] }
  predicate TextLength(n: nat) { n<=255 }
  predicate StringFits(bytes: nat,capacity: nat) { bytes<capacity }
  predicate BooleanField(name: string) { |name|>0 && name[0]=='f' }
  predicate Consecutive(ids: seq<nat>) { forall i | 0<=i<|ids|-1 :: ids[i+1]==ids[i]+1 }
  lemma PrefixStable<T>(a: seq<T>,b: seq<T>,i: nat)
    requires AppendOnly(a,b) && i<|a| ensures b[i]==a[i] {}
  lemma {:isolate_assertions} AcceptanceVectors()
    ensures AppendOnly<int>([1,2],[1,2,3]) && !AppendOnly<int>([1,2],[2,1,3])
    ensures PartId(0)==1 && BaseHull(0)==32 && TextChunk(65)==1 && TextOffset(65)==1
    ensures Nibbles(30)==[15,15,0] && !TextLength(256) && !StringFits(32,32)
    ensures BooleanField("fFree") && !BooleanField("count")
  {
    assert [2,1,3][..2][0]==2;
    assert [1,2][0]==1;
    assert [2,1,3][..2]!=[1,2];
    assert !AppendOnly<int>([1,2],[2,1,3]);
  }
}
