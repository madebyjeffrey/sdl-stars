include "M12-research-announcements.dfy"
include "S03-research-catalog.dfy"

module M12CatalogTermination {
  import N = M12ResearchAnnouncements
  import E = M08NativeEconomy
  import K = S03ResearchCatalog
  import S = S01
  predicate Limits(limits: seq<nat>) { |limits|==16 && forall i | 0<=i<16 :: limits[i]<256 }
  function Remaining(limits: seq<nat>,kind: nat,item: nat): nat
    requires Limits(limits) && kind<=16 && (kind<16 ==> item<=limits[kind])
    decreases 16-kind
  { if kind==16 then 0 else limits[kind]-item+1+Remaining(limits,kind+1,0) }
  predicate Complete(trace: seq<N.Reply>,cursor: nat,limits: seq<nat>,kind: nat,item: nat)
    requires Limits(limits) && cursor<=|trace| && kind<=16 && (kind<16 ==> item<=limits[kind])
    decreases |trace|-cursor
  {
    kind==16 || (cursor<|trace| &&
      trace[cursor].kind==kind && trace[cursor].item==item && trace[cursor].returned &&
      (trace[cursor].availability==0 <==> item==limits[kind]) &&
      (trace[cursor].availability==1 ==> trace[cursor].pointer) &&
      (if item==limits[kind] then Complete(trace,cursor+1,limits,kind+1,0)
       else Complete(trace,cursor+1,limits,kind,item+1)))
  }
  lemma ScanCompletes(player: nat,field: nat,level: E.I8,tt: bool,trace: seq<N.Reply>,cursor: nat,
    limits: seq<nat>,kind: nat,item: nat,events: seq<N.Event>)
    requires player<16 && field<6 && Limits(limits) && cursor<=|trace| && kind<=16
    requires kind<16 ==> item<=limits[kind]
    requires Complete(trace,cursor,limits,kind,item)
    ensures N.Scan(player,field,level,tt,kind,(if kind==16 then 0 else item) as S.I16,trace,cursor,events).status.Done?
    ensures N.Scan(player,field,level,tt,kind,(if kind==16 then 0 else item) as S.I16,trace,cursor,events).cursor==cursor+Remaining(limits,kind,item)
    decreases |trace|-cursor
  {
    if kind<16 {
      var issued:=events+[N.Lookup(player,kind,item as S.U8)];
      if item==limits[kind] {
        ScanCompletes(player,field,level,tt,trace,cursor+1,limits,kind+1,0,issued);
      } else {
        E.WordPreserves(item+1);
        var next:=N.AfterLookup(player,field,level,tt,kind,item as S.I16,trace[cursor],issued);
        ScanCompletes(player,field,level,tt,trace,cursor+1,limits,kind,item+1,next);
      }
    }
  }
  lemma RemainingAtZero(limits: seq<nat>,kind: nat)
    requires Limits(limits) && kind<=16
    ensures Remaining(limits,kind,0)==K.Calls(limits,kind)
    decreases 16-kind
  {
    if kind<16 { RemainingAtZero(limits,kind+1); }
  }
  lemma CurrentBound(player: nat,field: nat,level: E.I8,tt: bool,trace: seq<N.Reply>,cursor: nat,events: seq<N.Event>)
    requires player<16 && field<6 && cursor<=|trace|
    requires Complete(trace,cursor,K.Limits(),0,0)
    ensures N.Scan(player,field,level,tt,0,0,trace,cursor,events).status.Done?
    ensures N.Scan(player,field,level,tt,0,0,trace,cursor,events).cursor==cursor+255
  {
    ScanCompletes(player,field,level,tt,trace,cursor,K.Limits(),0,0,events);
    RemainingAtZero(K.Limits(),0);
    K.CurrentCalls();
  }
}
