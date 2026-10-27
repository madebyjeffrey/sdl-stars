include "M12-catalog-termination.dfy"
include "M11-native-availability.dfy"

// Construct actual decision replies from a shaped catalog/context instead of
// assuming a Complete supplied trace. Native row/frame extraction remains a
// separate refinement obligation; announcement effects remain canonical Scan.
module M12EquipmentLookup {
  import A = M11NativeAvailability
  import T = M12CatalogTermination
  import N = M12ResearchAnnouncements
  import K = S03ResearchCatalog
  import S = S01
  function {:isolate_assertions} AppendTrace(c: A.Context,rows: seq<seq<A.PartRow>>,field: nat,
    kind: nat,item: nat,prefix: seq<N.Reply>): seq<N.Reply>
    requires A.Shape(c) && A.CatalogShape(rows) && field<6 && kind<=16
    requires kind<16 ==> item<=K.Limits()[kind]
    ensures |AppendTrace(c,rows,field,kind,item,prefix)|==|prefix|+T.Remaining(K.Limits(),kind,item)
    ensures AppendTrace(c,rows,field,kind,item,prefix)[..|prefix|]==prefix
    ensures T.Complete(AppendTrace(c,rows,field,kind,item,prefix),|prefix|,K.Limits(),kind,item)
    decreases T.Remaining(K.Limits(),kind,item)
  {
    if kind==16 then prefix else
    A.MaskKindAt(kind,0);
    var result:=A.LookupRow(c,rows,K.Masks()[kind],item as S.U8,A.Prior(0));
    var need:=if item<K.Limits()[kind] then rows[kind][item].need[field] else 0;
    var reply:=N.Reply(kind,item as S.U8,result.status,need,result.pointer.Row?,true);
    if item==K.Limits()[kind] then AppendTrace(c,rows,field,kind+1,0,prefix+[reply])
    else AppendTrace(c,rows,field,kind,item+1,prefix+[reply])
  }
  function ScanCatalog(player: nat,c: A.Context,rows: seq<seq<A.PartRow>>,field: nat,
    prefix: seq<N.Reply>,events: seq<N.Event>): N.Result
    requires player<16 && A.Shape(c) && A.CatalogShape(rows) && field<6
    ensures ScanCatalog(player,c,rows,field,prefix,events).status.Done?
    ensures ScanCatalog(player,c,rows,field,prefix,events).cursor==|prefix|+255
  {
    var trace:=AppendTrace(c,rows,field,0,0,prefix);
    T.CurrentBound(player,field,c.levels[field],A.Bit(c,1),trace,|prefix|,events);
    N.Scan(player,field,c.levels[field],A.Bit(c,1),0,0,trace,|prefix|,events)
  }
}
