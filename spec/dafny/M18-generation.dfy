include "S04-arithmetic.dfy"
include "M17-host.dfy"
module M18Generation {
  datatype Phase = PreOrders | NormalizeRaces | UnmarkMines | MoveObjectsBefore | MoveFleets | RestoreHomeworlds
    | DecayObjects | BreedTransit | Produce | MoveObjectsAfter | Fuel | PostOrders | Sweep | Heal | AutoTerra | RemoteTerra
    | ValidateRoutes | Guesses | Backups | IncrementYear | Score | DesignCaches | RotateFiles | WriteFiles | Cleanup
  function Schedule(): seq<Phase> {
    [PreOrders,NormalizeRaces,UnmarkMines,MoveObjectsBefore,MoveFleets,RestoreHomeworlds,
    DecayObjects,BreedTransit,Produce,MoveObjectsAfter,Fuel,PostOrders,Sweep,Heal,AutoTerra,RemoteTerra,
    ValidateRoutes,Guesses,Backups,IncrementYear,Score,DesignCaches,RotateFiles,WriteFiles,Cleanup]
  }
  // Projection of successful top-level control flow. Individual phase outcomes
  // are external; no arbitrary phase state is certified by this cursor.
  function Next(cursor: nat,phase: Phase): nat requires cursor<|Schedule()| {
    if Schedule()[cursor]==phase then cursor+1 else cursor
  }
  datatype OrderStage = Battle | Things | Orders1 | Drop | Research | Orders2 | Transfers
  function Orders(post: bool): seq<OrderStage> {
    (if post then [Battle] else [])+[Things,Orders1,Drop,Research,Orders2]+(if post then [] else [Transfers])
  }
  function Swap<T>(xs: seq<T>,i: nat,j: nat): seq<T> requires i<|xs| && j<|xs| { xs[i:=xs[j]][j:=xs[i]] }
  lemma SwapPermutation<T>(xs: seq<T>,i: nat,j: nat)
    requires i<|xs| && j<|xs| ensures multiset(Swap(xs,i,j))==multiset(xs)
  {}
  lemma {:isolate_assertions} AcceptanceVectors()
    ensures Schedule()[8]==Produce && Schedule()[9]==MoveObjectsAfter
    ensures Schedule()[19]==IncrementYear && Schedule()[20]==Score
    ensures Next(8,Produce)==9 && Next(8,Heal)==8
    ensures Orders(false)[0]==Things && Orders(true)[0]==Battle
  {}
}
