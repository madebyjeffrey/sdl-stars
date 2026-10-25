include "S01-codecs.dfy"

// native encoding.c CchUtf8Prefix; includes invalid-input early stopping.
module S04Utf8Prefix {
  import opened S01
  import C = S01Codecs
  function Count(rgb: Bytes,capacity: nat): nat
    ensures Count(rgb,capacity)<=|rgb| && Count(rgb,capacity)<=capacity
    decreases |rgb|
  {
    var cb := CbCodePoint(rgb,0);
    if cb==0 || cb>capacity then 0 else cb+Count(rgb[cb..],capacity-cb)
  }
  function Prefix(rgb: Bytes,capacity: nat): Bytes { rgb[..Count(rgb,capacity)] }
  predicate ByteString(s: string) { forall i | 0<=i<|s| :: 0<=s[i] as int<256 }
  function BytesOf(s: string): Bytes
    requires ByteString(s)
    ensures |BytesOf(s)|==|s|
  { seq(|s|,i requires 0<=i<|s| => C.Byte(s[i] as int)) }
  function StringOf(rgb: Bytes): string
    ensures |StringOf(rgb)|==|rgb| && ByteString(StringOf(rgb))
  { seq(|rgb|,i requires 0<=i<|rgb| => rgb[i] as char) }
  opaque function Bound(s: string,capacity: nat): string
    requires ByteString(s)
    ensures |Bound(s,capacity)|<=capacity && ByteString(Bound(s,capacity))
  { StringOf(Prefix(BytesOf(s),capacity)) }
  lemma ShorterHead(rgb: Bytes,n: nat)
    requires CbCodePoint(rgb,0)<=n<=|rgb|
    ensures CbCodePoint(rgb[..n],0)==CbCodePoint(rgb,0)
  {}
  lemma PrefixValid(rgb: Bytes,capacity: nat)
    ensures FUtf8(Prefix(rgb,capacity))
    decreases |rgb|
  {
    var cb := CbCodePoint(rgb,0);
    if cb!=0 && cb<=capacity {
      PrefixValid(rgb[cb..],capacity-cb);
      ShorterHead(rgb,Count(rgb,capacity));
      assert Prefix(rgb,capacity)[cb..]==Prefix(rgb[cb..],capacity-cb);
    }
  }
  lemma AsciiCount(rgb: Bytes,capacity: nat)
    requires forall i | 0<=i<|rgb| :: 1<=rgb[i]<128
    ensures Count(rgb,capacity)==(if |rgb|<capacity then |rgb| else capacity)
    decreases |rgb|
  {
    if |rgb|>0 && capacity>0 {
      assert 1<=rgb[0]<128;
      assert CbCodePoint(rgb,0)==1;
      AsciiCount(rgb[1..],capacity-1);
    }
  }
  lemma AsciiBound(s: string,capacity: nat)
    requires forall i | 0<=i<|s| :: 1<=s[i] as int<128
    ensures Bound(s,capacity)==s[..if |s|<capacity then |s| else capacity]
    ensures |s|<=capacity ==> Bound(s,capacity)==s
  {
    reveal Bound();
    var rgb := BytesOf(s);
    assert forall i | 0<=i<|s| :: rgb[i]==s[i] as int;
    assert forall i | 0<=i<|rgb| :: 1<=rgb[i]<128;
    AsciiCount(rgb,capacity);
    var count := if |s|<capacity then |s| else capacity;
    assert StringOf(rgb[..count])==s[..count];
    if |s|<=capacity { assert s[..count]==s; }
  }
  lemma {:isolate_assertions} PrefixVectors() {
    assert Count([65,195,169,66],2)==1;
    assert Prefix([65,195,169,66],3)==[65,195,169];
    assert Count([65,255,66],8)==1;
    assert Count([65,0,66],8)==1;
    assert Count([240,159,152,128],3)==0;
    assert Count([240,159,152,128],4)==4;
    assert !FUtf8([0]) && FUtf8([195,169]);
    AsciiBound("abc",2);
    assert Bound("abc",2)=="ab";
  }
}
