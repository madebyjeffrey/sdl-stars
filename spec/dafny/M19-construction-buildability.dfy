include "M19-construction-effects.dfy"
include "M11-design-buildability.dfy"

// FBuildObject's fleet-branch entry -> actual design availability handoff.
// Ready means continue to object application, not that an object was created.
// Native read-domain outcomes retain prefixes and add no runtime validation.
module M19ConstructionBuildability {
  import B = M11DesignBuildability
  import A = M11NativeAvailability
  import E = M08NativeEconomy
  import C = S01Codecs
  import R = M19ConstructionEffects
  import S = S01
  import G = SharedGameState
  import I = S03LookupIdentities
  import K = S03ResearchCatalog
  datatype Entry = Entry(free: bool,design: B.Design,storedCargo: S.U16)
  datatype Roster = Missing | Present(entries: seq<Entry>)
  predicate RostersFit(ships: seq<Roster>,bases: seq<Roster>) {
    |ships|==16 && |bases|==16 &&
    (forall p | 0<=p<16 ::
      (ships[p].Present? ==> |ships[p].entries|==16 && (forall e <- ships[p].entries :: B.Shape(e.design))) &&
      (bases[p].Present? ==> |bases[p].entries|==10 && (forall e <- bases[p].entries :: B.Shape(e.design))))
  }
  datatype HullAddress = Outside | ShipHull(index: nat) | BaseHull(index: nat)
  function HullMetadata(hull: S.U16): HullAddress {
    var id:=E.Word(hull);
    if id>=I.ihuldefOrbitalFort then
      (if id-I.ihuldefOrbitalFort<K.Limits()[10] then BaseHull((id-I.ihuldefOrbitalFort) as nat) else Outside)
    else if id<0 then Outside else ShipHull(id as nat)
  }
  datatype Notice = Notice(offset: nat,cargo: S.I16)
  function BaseNotice(storedCargo: S.U16,catalogCargo: S.U16): Notice
    ensures BaseNotice(storedCargo,catalogCargo).offset<3
  { Notice(R.BaseMessage(storedCargo),E.Word(catalogCargo)) }
  datatype Event = RosterRead(base: bool,owner: S.I16) | DesignRead(base: bool,rawIndex: int)
    | Availability(index: nat,base: bool) | FailureNotice(index: nat) | HullRead(hull: S.U16)
  datatype Stage = Rejected | ShipReady(index: nat) | BaseReady(index: nat,hull: HullAddress) | Undefined(reason: string)
  datatype Result = Result(stage: Stage,global: S.I16,check: G.Option<B.Outcome>,events: seq<Event>)
  function {:isolate_assertions} Gate(item: S.I16,hasBase: bool,owner: S.I16,saved: S.I16,
    ships: seq<Roster>,bases: seq<Roster>,players: seq<A.Context>,rows: seq<seq<A.PartRow>>): Result
    requires RostersFit(ships,bases) && B.PlayersFit(players) && A.CatalogShape(rows)
    ensures Gate(item,hasBase,owner,saved,ships,bases,players,rows).check.Some? ==>
      Gate(item,hasBase,owner,saved,ships,bases,players,rows).check.value==B.Check(
        (if item>=16 then bases[owner].entries[item-16].design else ships[owner].entries[item].design),players,rows,owner,saved)
    ensures Gate(item,hasBase,owner,saved,ships,bases,players,rows).check.None? ==> Gate(item,hasBase,owner,saved,ships,bases,players,rows).global==saved
    ensures Gate(item,hasBase,owner,saved,ships,bases,players,rows).check.Some? && Gate(item,hasBase,owner,saved,ships,bases,players,rows).check.value.status.Done? ==>
      Gate(item,hasBase,owner,saved,ships,bases,players,rows).global==saved
    ensures Gate(item,hasBase,owner,saved,ships,bases,players,rows).stage.ShipReady? || Gate(item,hasBase,owner,saved,ships,bases,players,rows).stage.BaseReady? ==>
      Gate(item,hasBase,owner,saved,ships,bases,players,rows).check.Some? &&
      Gate(item,hasBase,owner,saved,ships,bases,players,rows).check.value.status==B.Done(true)
  {
    if item<16 && !hasBase then Result(Rejected,saved,G.None,[]) else
    var base:=item>=16;var index:=if base then (item as int)-16 else item as int;
    var events:=[RosterRead(base,owner)];
    if owner<0 || owner>=16 then Result(Undefined("design roster owner"),saved,G.None,events) else
    var roster:=if base then bases[owner] else ships[owner];
    if roster.Missing? then Result(Undefined("design roster pointer"),saved,G.None,events) else
    var accessed:=events+[DesignRead(base,index)];
    if index<0 || index>=|roster.entries| then Result(Undefined("design slot read"),saved,G.None,accessed) else
    var entry:=roster.entries[index];var notice:=if base then [] else [FailureNotice(index as nat)];
    if entry.free then Result(Rejected,saved,G.None,accessed+notice) else
    var queried:=accessed+[Availability(index as nat,base)];
    var checked:=B.Check(entry.design,players,rows,owner,saved);
    if checked.status.Undefined? then Result(Undefined(checked.status.reason),checked.global,G.Some(checked),queried) else
    if !checked.status.buildable then Result(Rejected,checked.global,G.Some(checked),queried+notice) else
    if !base then Result(ShipReady(index as nat),checked.global,G.Some(checked),queried) else
    var hull:=HullMetadata(entry.design.hull);var read:=queried+[HullRead(entry.design.hull)];
    if hull.Outside? then Result(Undefined("base notification hull read"),checked.global,G.Some(checked),read) else
    Result(BaseReady(index as nat,hull),checked.global,G.Some(checked),read)
  }
  lemma NoBaseSkipsOwner(item: S.I16,owner: S.I16,saved: S.I16,ships: seq<Roster>,bases: seq<Roster>,players: seq<A.Context>,rows: seq<seq<A.PartRow>>)
    requires item<16 && RostersFit(ships,bases) && B.PlayersFit(players) && A.CatalogShape(rows)
    ensures Gate(item,false,owner,saved,ships,bases,players,rows)==Result(Rejected,saved,G.None,[])
  {}
  lemma FreeSkipsCheck(item: S.I16,hasBase: bool,owner: S.I16,saved: S.I16,
    ships: seq<Roster>,bases: seq<Roster>,players: seq<A.Context>,rows: seq<seq<A.PartRow>>)
    requires RostersFit(ships,bases) && B.PlayersFit(players) && A.CatalogShape(rows) && 0<=owner<16
    requires (if item>=16 then bases[owner].Present? && item<26 && bases[owner].entries[item-16].free
      else hasBase && 0<=item && ships[owner].Present? && ships[owner].entries[item].free)
    ensures Gate(item,hasBase,owner,saved,ships,bases,players,rows).stage==Rejected
    ensures Gate(item,hasBase,owner,saved,ships,bases,players,rows).check.None?
    ensures Gate(item,hasBase,owner,saved,ships,bases,players,rows).global==saved
  {}
  lemma ReadyUsesRoute(item: S.I16,hasBase: bool,owner: S.I16,saved: S.I16,
    ships: seq<Roster>,bases: seq<Roster>,players: seq<A.Context>,rows: seq<seq<A.PartRow>>)
    requires RostersFit(ships,bases) && B.PlayersFit(players) && A.CatalogShape(rows)
    ensures Gate(item,hasBase,owner,saved,ships,bases,players,rows).stage.ShipReady? ==>
      R.Route(2,item,hasBase)==R.Ship(Gate(item,hasBase,owner,saved,ships,bases,players,rows).stage.index)
    ensures Gate(item,hasBase,owner,saved,ships,bases,players,rows).stage.BaseReady? ==>
      R.Route(2,item,hasBase)==R.Base(Gate(item,hasBase,owner,saved,ships,bases,players,rows).stage.index)
  {}
  lemma DecodedHullSafe(hull: S.U16)
    requires hull<I.ihuldefOrbitalFort+K.Limits()[10]
    ensures !HullMetadata(hull).Outside?
  { E.WordPreserves(hull); }
  lemma {:isolate_assertions} MetadataVectors() {
    E.WordPreserves(0);E.WordPreserves(32);E.WordPreserves(36);E.WordPreserves(37);E.WordPreserves(288);
    assert C.Pow2(4)==16 && C.Pow2(8)==256 && C.Pow2(12)==4096 && C.Pow2(15)==32768 && C.Pow2(16)==65536;
    assert E.Word(32768)==-32768;
    assert HullMetadata(0)==ShipHull(0) && HullMetadata(32)==BaseHull(0) && HullMetadata(36)==BaseHull(4);
    assert HullMetadata(37)==Outside && HullMetadata(288)==Outside && HullMetadata(32768)==Outside;
    assert B.HullRequest(288).item==0 && B.HullRequest(288).mask==1024;
    assert BaseNotice(0,0)==Notice(0,0) && BaseNotice(1,65535)==Notice(1,-1) && BaseNotice(65535,0)==Notice(2,0);
  }
}
