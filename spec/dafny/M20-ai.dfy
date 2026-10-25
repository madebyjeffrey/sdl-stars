include "S04-arithmetic.dfy"
module M20AI {
  datatype Engine = Robotoid | Turindrone | Cybertron | Macinti | Automitron | Rototill | Maid | Unknown
  // IDs are pinned to enums.h; no algorithm is assigned to the reserved value 6.
  function Dispatch(id: nat): Engine {
    if id==0 then Robotoid else if id==1 then Turindrone else if id==2 then Automitron else if id==3 then Rototill
    else if id==4 then Cybertron else if id==5 then Macinti else if id==7 then Maid else Unknown
  }
  datatype Entry = LoadFailure | SaveWithoutStrategy | RunStrategy
  function Enter(loaded: bool,dead: bool,fAllocated: bool): Entry {
    if !loaded then LoadFailure else if dead || !fAllocated then SaveWithoutStrategy else RunStrategy
  }
  function Mode(prior: nat,requested: nat): nat { if requested==65535 then prior else requested }
  function ShuffleDraws(owned: nat,tutorial: bool): nat { if tutorial || owned==0 then 0 else owned-1 }
  // IdRandomPlanetNearby reservoir step: RNG draw is supplied, not nondeterminism
  // silently substituted for the shared RNG sequence.
  function Reservoir(prior: int,candidate: int,draw: nat): int { if draw==0 then candidate else prior }
  lemma {:isolate_assertions} AcceptanceVectors()
    ensures Dispatch(0)==Robotoid && Dispatch(7)==Maid && Dispatch(6)==Unknown
    ensures Enter(false,false,true)==LoadFailure && Enter(true,true,true)==SaveWithoutStrategy
    ensures Mode(4,65535)==4 && ShuffleDraws(10,true)==0 && ShuffleDraws(10,false)==9
    ensures Reservoir(1,2,0)==2 && Reservoir(1,2,3)==1
  {}
}
