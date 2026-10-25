include "M03-scoring.dfy"
include "M11-design.dfy"
module M12Research {
  import opened S04Arithmetic
  datatype Policy = Policy(field: nat,next: nat,percent: nat)
  predicate Fits(p: Policy) { p.field<6 && p.next<8 && p.percent<=100 }
  function NextChoice(row: nat): nat requires row<8 { if row==0 then 6 else if row<=6 then row-1 else 7 }
  function ChoiceRow(next: nat): nat requires next<8 { if next==6 then 0 else if next<6 then next+1 else 7 }
  function Close(working: Policy): Policy { working }
  function PrimaryPool(pool: nat,fGeneral: bool): nat { if fGeneral then (pool+1)/2 else pool }
  function OtherPool(pool: nat,fGeneral: bool): nat { if fGeneral then (pool*3+19)/20 else 0 }
  predicate CanAdvance(level: nat,fCrippled: bool) { level<26 && (!fCrippled || level<10) }
  function Lowest(levels: seq<nat>,i: nat,best: nat): nat
    requires 0<=best<i<=|levels| decreases |levels|-i
  { if i==|levels| then best else Lowest(levels,i+1,if levels[i]<levels[best] then i else best) }
  lemma ChoiceRoundTrip(n: nat) requires n<8 ensures NextChoice(ChoiceRow(n))==n {}
  lemma {:isolate_assertions} AcceptanceVectors()
    ensures NextChoice(0)==6 && NextChoice(6)==5 && NextChoice(7)==7
    ensures PrimaryPool(101,true)==51 && OtherPool(101,true)==16
    ensures !CanAdvance(10,true) && CanAdvance(25,false) && !CanAdvance(26,false)
    ensures Lowest([4,2,2,5,6,3],1,0)==1
  {}
}
