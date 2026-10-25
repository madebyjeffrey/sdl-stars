include "S06-shared-state.dfy"
module M13Relations {
  type Relation = n: int | 0<=n<=2 witness 0
  function PlayerAt(row: nat,self: nat): nat { if row>=self then row+1 else row }
  function Set(rels: seq<Relation>,other: nat,value: Relation): seq<Relation>
    requires other<|rels| { rels[other:=value] }
  // FAttackPlayer uses the attacker's directed relation, not mutual friendship.
  predicate Attack(policy: nat,other: nat,relation: Relation) {
    if policy==0 then false else if policy==3 then true else if policy==2 then relation!=1
    else if policy==1 then relation==2 else other==policy-4
  }
  lemma SelfExcluded(row: nat,self: nat) ensures PlayerAt(row,self)!=self {}
  lemma OtherRelationsPreserved(rs: seq<Relation>,who: nat,v: Relation,j: nat)
    requires who<|rs| && j<|rs| && j!=who ensures Set(rs,who,v)[j]==rs[j] {}
  lemma {:isolate_assertions} AcceptanceVectors()
    ensures PlayerAt(2,2)==3 && PlayerAt(1,2)==1
    ensures !Attack(2,3,1) && Attack(2,3,0) && !Attack(1,3,0) && Attack(1,3,2)
    ensures Attack(7,3,1) && !Attack(7,2,2)
  {}
}
