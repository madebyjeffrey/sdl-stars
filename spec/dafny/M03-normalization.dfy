include "M03-setup.dfy"

// createui.c normalization. Pricing and catalogs are explicit checked inputs;
// calls that generate random races occur later in M05, not here.
module M03Normalization {
  import opened S04Arithmetic
  import U = M03Setup
  import Race = M04Race
  import R = SharedGameState
  import S = S01
  datatype Player = Player(race: Race.Race,fAI: bool,kind: int,level: int)
  datatype State = State(players: seq<Player>,rng: R.Rng)
  function Substitute(p: Player,points: int,preset: Race.Race): Player
    requires Race.FShape(p.race) && Race.FShape(preset)
    ensures Race.FShape(Substitute(p,points,preset).race)
  {
    if p.fAI then p else if points<0 then Player(preset.(fHacker:=true),false,0,0)
    else p.(race:=Race.Bounds(p.race,true))
  }
  function MissingNames(p: Player,rng: R.Rng,names: seq<S.Bytes>): (Player,R.Rng)
    requires |names|==24
  { var d := if |p.race.singular|==0 then Take(rng,24) else DrawResult(rng,0);
    var singular := if |p.race.singular|==0 then names[d.value] else p.race.singular;
    (p.(race:=p.race.(singular:=singular,plural:=if |p.race.plural|==0 then singular+[115] else p.race.plural)),d.rng) }
  function InitialPass(s: State,i: nat,points: seq<int>,preset: Race.Race,names: seq<S.Bytes>): State
    requires i<=|s.players| && |points|==|s.players| && |names|==24
    requires Race.FShape(preset) && forall p <- s.players :: Race.FShape(p.race)
    ensures |InitialPass(s,i,points,preset,names).players|==|s.players|
    decreases |s.players|-i
  {
    if i==|s.players| then s else
    var p := Substitute(s.players[i],points[i],preset); var result := MissingNames(p,s.rng,names);
    InitialPass(State(s.players[i:=result.0],result.1),i+1,points,preset,names)
  }
  // A probe trace is finite only when an unused catalog name is reached; no
  // artificial retry ceiling is introduced in the observed loop.
  predicate FNameProbe(players: seq<Player>,names: seq<S.Bytes>,trace: seq<int>)
    decreases |trace|
  {
    |names|==24 && |trace|>0 && 0<=trace[0]<24 &&
    (if |trace|==1 then (forall p <- players :: p.race.singular!=names[trace[0]]) else
      (exists p <- players :: p.race.singular==names[trace[0]]) &&
      trace[1]==(trace[0]+1)%24 && FNameProbe(players,names,trace[1..]))
  }
  lemma NameProbeLast(players: seq<Player>,names: seq<S.Bytes>,trace: seq<int>)
    requires FNameProbe(players,names,trace)
    ensures 0<=trace[|trace|-1]<24
    decreases |trace|
  { if |trace|>1 { NameProbeLast(players,names,trace[1..]); } }
  predicate FDuplicateName(s: State,next: State,i: nat,names: seq<S.Bytes>,probe: seq<int>) {
    i<|s.players| &&
    (if exists j | 0<=j<i :: s.players[j].race.singular==s.players[i].race.singular then
      FNameProbe(s.players,names,probe) && 0<=probe[|probe|-1]<24 && probe[0]==Take(s.rng,24).value &&
      next==State(s.players[i:=s.players[i].(race:=s.players[i].race.(singular:=names[probe[|probe|-1]],plural:=names[probe[|probe|-1]]+[115]))],Take(s.rng,24).rng)
    else next==s)
  }
  function Portraits(players: seq<Player>): seq<int> {
    seq(|players|,i requires 0<=i<|players| => if players[i].fAI || players[i].race.portrait<0 || players[i].race.portrait>=32 then -1 else players[i].race.portrait)
  }
  predicate FFirstDuplicate(xs: seq<int>,i: nat,j: nat) {
    j<i<|xs| && xs[i]!=-1 && xs[j]==xs[i] && forall k | 0<=k<j :: xs[k]!=xs[i]
  }
  function DropPortrait(xs: seq<int>,i: nat,j: nat,rng: R.Rng): (seq<int>,R.Rng)
    requires FFirstDuplicate(xs,i,j)
  { var d := Take(rng,2); (xs[if d.value==0 then j else i := -1],d.rng) }
  predicate FFillPortrait(xs: seq<int>,next: seq<int>,i: nat,rng: R.Rng,after: R.Rng,probe: seq<int>) {
    i<|xs| && (if xs[i]!=-1 then next==xs && after==rng else
      |probe|>0 && (forall p <- probe :: 0<=p<32) && probe[0]==Take(rng,32).value && after==Take(rng,32).rng &&
      (forall k | 0<=k<|probe|-1 :: probe[k] in xs && probe[k+1]==(probe[k]+1)%32) &&
      probe[|probe|-1] !in xs && next==xs[i:=probe[|probe|-1]])
  }
  lemma NormalizationVectors() {
    assert Portraits([])==[];
    assert FNameProbe([],seq(24,i=>[115]),[0]);
    assert !FNameProbe([],seq(24,i=>[115]),[0,1]);
    assert FFirstDuplicate([4,3,4],2,0);
    assert !FFirstDuplicate([4,3,4],2,1);
  }
}
