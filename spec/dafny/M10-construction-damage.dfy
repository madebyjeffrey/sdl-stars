include "M09-builder.dfy"
include "S06-shared-state.dfy"

// FBuildObject's fleet-capacity merge and CreateShip. This is not the
// separate player Merge2Fleets algorithm or a combat damage model.
module M10ConstructionDamage {
  import S = S01
  import C = S01Codecs
  import N = M09Builder
  import G = SharedGameState
  datatype Slot = Hole | Fleet(owner: S.I16,firstOrder: G.Point,ships: S.I16,damage: S.U16)
  datatype Found = None | At(index: nat)
  function Candidate(rows: seq<Slot>,owner: S.I16,point: G.Point,built: S.I16): Found
    ensures Candidate(rows,owner,point,built).At? ==> Candidate(rows,owner,point,built).index<|rows|
    decreases |rows|
  {
    if |rows|==0 || rows[0].Hole? || rows[0].owner>owner then None else
    if rows[0].owner>=owner && rows[0].firstOrder==point && 32766-built>rows[0].ships then At(0) else
    var next:=Candidate(rows[1..],owner,point,built);
    if next.None? then None else At(next.index+1)
  }
  function Div(n: int,d: int): int requires d!=0 { if d>0 then C.DivC(n,d) else -C.DivC(n,-d) }
  datatype Damage = Adjusted(word: S.U16) | UndefinedDamage(word: S.U16,reason: string)
  function Rebalance(priorValue: S.I16,added: S.I16,word: S.U16,hp: S.U16): Damage {
    var share:=word%128;var damage:=word/128;
    if priorValue==0 || damage==0 then Adjusted(0) else
    var initial:=N.Signed16(C.DivC(share*priorValue,100));
    var affected:=if initial==0 then 1 else initial;
    var per:=C.DivC(hp*damage,10);
    if !N.I32(per*affected) then UndefinedDamage(word,"original damage product") else
    var original:=C.DivC(per*affected,50);
    var total:=N.Signed16(priorValue+added);
    if total==0 then UndefinedDamage(word,"ship-share division") else
    var pct: nat:=Div(affected*100,total)%128;
    var revisedShare:=if pct==0 then 1 else pct;
    var prefix: S.U16:=revisedShare+128*damage;
    var next:=N.Signed16(C.DivC(revisedShare*total,100));
    var nextAffected:=if next==0 then 1 else next;
    if !N.I32(original*5) || !N.I32(Div(original*5,nextAffected)*100) || hp==0
    then UndefinedDamage(prefix,"damage renormalization") else
    var revisedDamage:=C.DivC(Div(original*5,nextAffected)*100,hp)%512;
    Adjusted(revisedShare+128*revisedDamage)
  }
  datatype Created = Created(ships: int,built: S.U32,exist: S.U32)
  datatype ShipOutcome = NoBase | UnavailableDesign | LostAtCapacity | MergeInto(index: nat) | AllocateFleet
  function ShipPlan(base: bool,free: bool,available: bool,fleetCount: nat,rows: seq<Slot>,owner: S.I16,point: G.Point,built: S.I16): ShipOutcome
    requires fleetCount<4096
  {
    if !base then NoBase else if free || !available then UnavailableDesign else
    if fleetCount!=512 then AllocateFleet else
    var candidate:=Candidate(rows,owner,point,built);
    if candidate.None? then LostAtCapacity else MergeInto(candidate.index)
  }
  // 0/1=one routed stopped/moving; 2/3=multiple routed stopped/moving;
  // 4=one default orders, 5=multiple/default (including nonpositive direct calls).
  function ShipNotice(route: bool,built: S.I16,nextWarp: nat): nat {
    if route then (if built==1 then (if nextWarp==0 then 0 else 1) else (if nextWarp==0 then 2 else 3))
    else if built==1 then 4 else 5
  }
  function CreateStack(priorValue: S.I16,added: S.I16,built: S.U32,exist: S.U32): Created
    ensures -32768<=CreateStack(priorValue,added,built,exist).ships<32768
  { Created(N.Signed16(priorValue+added),(built+added)%4294967296,(exist+added)%4294967296) }
  lemma CandidateStopsAtHole(rows: seq<Slot>,owner: S.I16,point: G.Point,built: S.I16)
    requires |rows|>0 && rows[0]==Hole
    ensures Candidate(rows,owner,point,built)==None
  {}
  lemma {:isolate_assertions} Vectors() {
    assert C.Pow2(4)==16 && C.Pow2(8)==256 && C.Pow2(12)==4096;
    assert C.Pow2(15)==32768 && C.Pow2(16)==65536;
    var point:=G.Point(10,20);
    assert Candidate([Fleet(0,point,32765,0)],0,point,1)==None;
    assert Candidate([Fleet(0,point,32764,0)],0,point,1)==At(0);
    assert Candidate([Fleet(1,point,0,0),Fleet(0,point,0,0)],0,point,1)==None;
    assert CreateStack(32767,1,4294967295,4294967295)==Created(-32768,0,0);
    assert Rebalance(10,1,0,0)==Adjusted(0);
  }
}
