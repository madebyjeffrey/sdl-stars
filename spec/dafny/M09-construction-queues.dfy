include "M09-host-production.dfy"

// build.c KillQueuedShips / KillQueuedMassPackets. Keep physical source writes
// during compaction; merely filtering logical rows loses inactive-tail effects.
module M09ConstructionQueues {
  import H = M09HostProduction
  import F = M09Forecast
  import R = M09Presets
  import Q = S06PlanetEditor
  import S = S01
  predicate Keep(row: R.Row,ships: bool) {
    if ships then row.value.grobj!=2 || row.value.iItem>=16
    else row.value.grobj!=1 || row.value.iItem<14 || row.value.iItem>17
  }
  datatype Compacted = Compacted(rows: seq<S.U32>,used: nat)
  function Compact(rows: seq<S.U32>,count: nat,source: nat,destination: nat,ships: bool): Compacted
    requires destination<=source<=count<=|rows|
    ensures |Compact(rows,count,source,destination,ships).rows|==|rows|
    ensures destination<=Compact(rows,count,source,destination,ships).used<=count
    decreases count-source
  {
    if source==count then Compacted(rows,destination) else
    var row:=H.Decode(rows[source]);
    if !Keep(row,ships) then Compact(rows,count,source+1,destination,ships) else
    var changed:=if ships && row.value.grobj==2 then row.(value:=row.value.(pct:=0)) else row;
    var written:=rows[source:=F.Word(changed)];
    var copied:=if source==destination then written else written[destination:=written[source]];
    Compact(copied,count,source+1,destination+1,ships)
  }
  datatype Result = Unchanged(queue: H.Queue,refresh: bool) | Processed(queue: H.Queue,refresh: bool) | OutsideAllocation
  function Invalidate(queue: H.Queue,ships: bool,selectedPlanetMatches: bool): Result {
    if queue.NoQueue? || queue.block.count==0 then Unchanged(queue,false) else
    var b:=queue.block;
    if b.count>|b.rows| then OutsideAllocation else
    var compact:=Compact(b.rows,b.count,0,0,ships);
    if compact.used==0 then Processed(H.NoQueue,selectedPlanetMatches) else
    Processed(H.Allocated(b.(rows:=compact.rows,count:=compact.used as S.U8)),selectedPlanetMatches)
  }
  lemma {:isolate_assertions} Vectors() {
    var ship:=R.Row(R.O.Prod(2,0,2,50),31);
    var base:=R.Row(R.O.Prod(1,16,2,59),30);
    var rows: seq<S.U32>:=[F.Word(ship),F.Word(base),77];
    H.PackedRoundTrip(ship);H.PackedRoundTrip(base);
    H.PackedRoundTrip(base.(value:=base.value.(pct:=0)));
    var out:=Compact(rows,2,0,0,true);
    assert out.used==1;
    assert H.Decode(out.rows[0])==base.(value:=base.value.(pct:=0));
    assert H.Decode(out.rows[1])==base.(value:=base.value.(pct:=0));
    assert out.rows[2]==77;
  }
}
