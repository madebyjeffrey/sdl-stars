include "S04-arithmetic.dfy"
include "S01-library-names.dfy"
module S04Platform {
  datatype Copy = Copied(count: nat) | Malformed | TooSmall
  function Utf8Copy(bytes: nat,capacity: nat,valid: bool): Copy {
    if capacity==0 then TooSmall else if !valid then Malformed else if bytes>=capacity then TooSmall else Copied(bytes)
  }
  predicate PathFits(bytes: nat,capacity: nat,valid: bool) { valid && bytes>0 && bytes<capacity }
  function TickDelta(now: nat,thenTick: nat): nat { (now-thenTick)%4294967296 }
  function Attempts(): nat { 64 }
  predicate DirectoryChanged(createdOrRemoved: bool,error: bool) { createdOrRemoved && !error }
  lemma CopyHasTerminator(b: nat,c: nat,v: bool)
    ensures Utf8Copy(b,c,v).Copied? ==> Utf8Copy(b,c,v).count<c
  {}
  lemma {:isolate_assertions} AcceptanceVectors()
    ensures Utf8Copy(3,4,true)==Copied(3) && Utf8Copy(4,4,true)==TooSmall
    ensures Utf8Copy(1,0,false)==TooSmall && Utf8Copy(1,2,false)==Malformed
    ensures TickDelta(5,4294967290)==11 && !DirectoryChanged(false,false)
  {}
}
