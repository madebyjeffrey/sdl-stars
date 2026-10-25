include "M13-relations.dfy"
include "S04-arithmetic.dfy"
module M14BattleReview {
  import opened S04Arithmetic
  predicate CanCopy(count: nat) { count<15 }
  predicate CanRenameOrDelete(index: nat) { index>0 }
  function AttackCode(row: nat,self: nat,fSingle: bool): nat { if fSingle then 3 else if row>=self+4 then row+1 else row }
  function Commit<T>(prior: T,working: T,dirty: bool): T { if dirty then working else prior }
  function AfterDelete(plan: nat,removed: nat): nat
    requires removed>0
  { if plan>=removed then plan-1 else plan }
  function Step(current: int,delta: int,last: nat): int { Clamp(current+delta,-1,last) }
  function Quantum(ctrl: bool,shift: bool): nat { if ctrl then 100 else if shift then 10 else 1 }
  function TimerMillis(speed: nat): int requires speed<=4 { 570-120*speed }
  lemma ReplayBound(c: int,d: int,last: nat) ensures -1<=Step(c,d,last)<=last {}
  lemma {:isolate_assertions} AcceptanceVectors()
    ensures !CanCopy(15) && CanCopy(14) && !CanRenameOrDelete(0)
    ensures AttackCode(6,2,false)==7 && AttackCode(0,2,true)==3
    ensures Commit(10,20,true)==20 && Step(2,100,10)==10 && Step(2,-10,10)==-1
    ensures TimerMillis(4)==90 && Quantum(true,true)==100
  {}
}
