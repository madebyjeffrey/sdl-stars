include "S02-state.dfy"
include "S06-lookups.dfy"
include "M15-reports.dfy"

// RestoreSelection control flow with explicit callback boundaries. Each Step
// after a callback receives the actual post-callback state/world; no purity or
// unchanged-game assumption is imposed on tutorial/UI callbacks.
module M07Restoration {
  import S = S02State
  import L = S06Lookups
  import M = M15Reports
  datatype World = World(player: int,gameId: int,turn: int,home: int,maximum: nat,
    planets: seq<L.Entry>,fleets: seq<L.Slot>,counts: seq<nat>,
    filters: seq<bool>,inbound: nat,showFiltered: bool,tutorial: bool)
  predicate WorldFits(w: World) {
    0<=w.player<16 && |w.counts|==16 && L.CountBefore(w.counts,16)<=32767 &&
    |w.planets|<=32767 && |w.fleets|<=32767 && w.maximum<=32767 &&
    |w.filters|+w.inbound<=32767
  }
  datatype Saved = Saved(selection: S.Selection,gameId: int,turn: int,message: int)
  datatype State = State(saved: Saved,cursor: int,gotoVcr: bool,tutorChange: bool)
  datatype Choice = NoChoice | PlanetChoice(id: int) | FleetChoice(id: int)
  function OwnedPlanet(w: World,id: int): bool {
    var found := L.Planet(w.planets,w.maximum,id);
    found.Located? && w.planets[found.index].owner==w.player
  }
  function FirstPlanet(rows: seq<L.Entry>,player: int): Choice
    decreases |rows|
  {
    if |rows|==0 then NoChoice else if rows[0].owner==player then PlanetChoice(rows[0].id)
    else FirstPlanet(rows[1..],player)
  }
  function FirstFleet(rows: seq<L.Slot>,player: int): Choice
    decreases |rows|
  {
    if |rows|==0 || rows[0].Hole? then NoChoice
    else if rows[0].value.owner==player then FleetChoice(rows[0].value.id)
    else FirstFleet(rows[1..],player)
  }
  function Fallback(w: World): Choice {
    var home := L.Planet(w.planets,w.maximum,w.home);
    if home.Located? && w.planets[home.index].owner==w.player then PlanetChoice(w.planets[home.index].id)
    else
      var planet := FirstPlanet(w.planets,w.player);
      if planet!=NoChoice then planet else FirstFleet(w.fleets,w.player)
  }
  datatype Phase = Start | ClearInitial | CheckMessage | AfterTitle | CheckTutorial | CheckFallback | ClearFinal | Done
  datatype Event = Quiet | SelectFleet(id: int) | SelectPlanet(id: int)
    | FindSomething(choice: Choice) | SetTitle | InvalidateMessage | AdvanceTutorial
    | Complete | UndefinedLookup
  datatype Transition = Transition(state: State,phase: Phase,event: Event)
  datatype PostOpenEvent = SuspendDraw | Restore | Refit | ResumeDraw | InvalidateScanner | UpdateScanner
  function PostOpen(scannerExists: bool): seq<PostOpenEvent> {
    if scannerExists then [SuspendDraw,Restore,Refit,ResumeDraw,InvalidateScanner,UpdateScanner] else []
  }
  function Rank(p: Phase): nat {
    match p
    case Start => 7 case ClearInitial => 6 case CheckMessage => 5
    case AfterTitle => 4 case CheckTutorial => 3 case CheckFallback => 2
    case ClearFinal => 1 case Done => 0
  }
  function SetSelection(s: State,selection: S.Selection): State {
    s.(saved:=s.saved.(selection:=selection))
  }
  function Clear(s: State): State { SetSelection(s,s.saved.selection.(kind:=0)) }
  function MessageSeed(saved: int,total: nat): int
    requires saved>0
    ensures -1<=MessageSeed(saved,total)<total
  { if saved-1>=total then -1 else saved-1 }
  function Step(s: State,w: World,p: Phase): Transition
    requires WorldFits(w)
    ensures p!=Done ==> Rank(Step(s,w,p).phase)<Rank(p)
  {
    var sel := s.saved.selection;
    match p
    case Start =>
      if sel.player!=w.player || s.saved.gameId!=w.gameId then
        Transition(SetSelection(s,sel.(kind:=1,objectId:=w.home)),CheckFallback,Quiet)
      else if sel.kind==2 then
        var fleet := L.Fleet(w.fleets,w.counts,sel.objectId);
        if fleet.NullAccess? then Transition(s,Done,UndefinedLookup)
        else if fleet.Located? then Transition(s,ClearInitial,SelectFleet(sel.objectId))
        else Transition(SetSelection(s,sel.(kind:=1,objectId:=w.home)),CheckMessage,Quiet)
      else if sel.kind==1 then
        if OwnedPlanet(w,sel.objectId) then Transition(s,ClearInitial,SelectPlanet(sel.objectId))
        else Transition(SetSelection(s,sel.(objectId:=w.home)),CheckMessage,Quiet)
      else Transition(s,CheckMessage,Quiet)
    case ClearInitial => Transition(Clear(s),CheckMessage,Quiet)
    case CheckMessage =>
      if s.saved.turn==w.turn && s.saved.message>0 then
        var cursor := M.Next(w.filters,w.inbound,MessageSeed(s.saved.message,|w.filters|+w.inbound),w.showFiltered,false);
        Transition(s.(cursor:=cursor,gotoVcr:=false),AfterTitle,SetTitle)
      else Transition(s,CheckFallback,Quiet)
    case AfterTitle => Transition(s,CheckTutorial,InvalidateMessage)
    case CheckTutorial =>
      if w.tutorial then Transition(s.(tutorChange:=true),CheckFallback,AdvanceTutorial)
      else Transition(s,CheckFallback,Quiet)
    case CheckFallback =>
      if sel.kind==0 then Transition(s,Done,Complete)
      else if sel.objectId!=-1 && OwnedPlanet(w,sel.objectId) then
        Transition(s,ClearFinal,SelectPlanet(sel.objectId))
      else Transition(s,ClearFinal,FindSomething(Fallback(w)))
    case ClearFinal => Transition(Clear(s),Done,Complete)
    case Done => Transition(s,Done,Complete)
  }
  lemma ClearAfterCallback(s: State,w: World)
    requires WorldFits(w)
    ensures Step(s,w,ClearInitial).state.saved.selection.kind==0
    ensures Step(s,w,ClearInitial).state.saved.selection.objectId==s.saved.selection.objectId
    ensures Step(s,w,ClearFinal).state.saved.selection.kind==0
    ensures Step(s,w,ClearFinal).state.cursor==s.cursor
  {}
  lemma MismatchSkipsMessages(s: State,w: World)
    requires WorldFits(w)
    requires s.saved.selection.player!=w.player || s.saved.gameId!=w.gameId
    ensures Step(s,w,Start).phase==CheckFallback
    ensures Step(s,w,Start).state.cursor==s.cursor
    ensures Step(s,w,Start).state.saved.selection.objectId==w.home
  {}
  lemma FallbackStopsAtHole(tail: seq<L.Slot>,player: int)
    ensures FirstFleet([L.Hole]+tail,player)==NoChoice
  {}
  lemma FleetTailAfterHoleIgnored(prefix: seq<L.Slot>,tail: seq<L.Slot>,player: int)
    ensures FirstFleet(prefix+[L.Hole]+tail,player)==FirstFleet(prefix+[L.Hole],player)
    decreases |prefix|
  {
    if |prefix|>0 {
      assert (prefix+[L.Hole]+tail)[0]==prefix[0];
      assert (prefix+[L.Hole])[0]==prefix[0];
      assert (prefix+[L.Hole]+tail)[1..]==prefix[1..]+[L.Hole]+tail;
      assert (prefix+[L.Hole])[1..]==prefix[1..]+[L.Hole];
      FleetTailAfterHoleIgnored(prefix[1..],tail,player);
    }
  }
  lemma OwnedHomeWins(w: World)
    requires OwnedPlanet(w,w.home)
    ensures Fallback(w)==PlanetChoice(w.planets[L.Planet(w.planets,w.maximum,w.home).index].id)
  {}
  lemma {:isolate_assertions} {:fuel L.CountBefore,18} RestorationVectors() {
    var counts := [1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0];
    var w := World(0,7,2400,2,10,[L.Entry(2,0),L.Entry(5,1)],
      [L.Present(L.Entry(0,0))],counts,[false,true,false],1,false,true);
    assert WorldFits(w);
    var s := State(Saved(S.Selection(2,0,0),7,2400,1),3,true,false);
    assert Step(s,w,Start).event==SelectFleet(0);
    assert Step(s,w,ClearInitial).state.saved.selection.kind==0;
    assert Step(s,w,CheckMessage).state.cursor==2;
    assert !Step(s,w,CheckMessage).state.gotoVcr;
    assert Step(s,w,AfterTitle).event==InvalidateMessage;
    assert Step(s,w,CheckTutorial).event==AdvanceTutorial;
    assert Step(s,w,CheckTutorial).state.tutorChange;
    assert Fallback(w)==PlanetChoice(2);
    var other := SetSelection(s,S.Selection(4,0,5));
    assert Step(other,w,Start).phase==CheckMessage;
    assert Step(other,w,CheckFallback).event==FindSomething(PlanetChoice(2));
    var noPlanets := w.(planets:=[],home:=-1,fleets:=[L.Hole,L.Present(L.Entry(0,0))]);
    assert Fallback(noPlanets)==NoChoice;
    assert MessageSeed(99,4)==-1 && M.Next(w.filters,1,-1,false,false)==0;
    assert PostOpen(false)==[] && PostOpen(true)[..2]==[SuspendDraw,Restore];
  }
}
