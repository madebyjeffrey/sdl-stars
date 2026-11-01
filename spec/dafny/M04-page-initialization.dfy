include "M04-page-commands.dfy"
include "M01-library-name-dialog.dfy"

// Ordinary WM_INITDIALOG flow. UI requests do not assert native control success.
module M04PageInitialization {
  import S = S01
  import C = S01Codecs
  import N = M02NativeRaceState
  import P = M04PageCommands
  import W = M04WizardController
  import G = M01LibraryNameDialog
  function {:fuel C.Pow2,18} I16(n: int): S.I16 {
    assert C.Pow2(15)==32768 && C.Pow2(16)==65536;
    P.Narrow(n)
  }
  function Trait(r: S.Bytes,bit: nat): bool
    requires |r|==192 && bit<32
  { (r[78+bit/8] as bv8 & ((1 as bv8) << (bit%8)))!=0 }
  function Preset(r: S.Bytes,rows: seq<S.Bytes>,at: nat): nat
    requires |r|==192 && P.Presets(rows) && at<=7
    ensures at<=Preset(r,rows,at)<=7
    decreases 7-at
  {
    if at==7 then 7 else
    if P.SetPortrait(r,P.Portrait(rows[at]))[..128]==rows[at][..128] then at else Preset(r,rows,at+1)
  }
  function TitleId(base: S.U16,readOnly: S.I16): S.U16 { (base+(readOnly as int))%65536 }
  function Offset(r: G.Rect,dy: int): G.Rect { G.Rect(r.left,r.top+dy,r.right,r.bottom+dy) }
  datatype Geometry = Geometry(right: W.I32,font: S.I16,temperature: S.U16,unit: S.U16,
    growthLabel: S.U16,percent: S.U16)
  function Label(g: Geometry): S.I16 { I16(g.temperature+10) }
  function Middle(g: Geometry): S.I16 { I16(g.right-Label(g)-I16(g.unit+10)) }
  function Height(g: Geometry): int { C.DivC(I16(3*(g.font as int)),2) }
  predicate GeometrySafe(g: Geometry) {
    G.I32(g.right-Label(g)) && G.I32(g.right-Label(g)-I16(g.unit+10))
  }
  function Layout(g: Geometry): seq<G.Rect>
    ensures |Layout(g)|==17
  {
    var xLabel:=Label(g);var middle:=Middle(g);var dy:=Height(g);var top:=I16(3*(g.font as int));
    var base:=[G.Rect(xLabel,top,xLabel+dy,top+dy),
      G.Rect(xLabel+dy+6,top,xLabel+middle-dy-6,top+dy),
      G.Rect(xLabel+middle-dy,top,xLabel+middle,top+dy),
      G.Rect(xLabel,top+dy+4,3*dy+xLabel,2*dy+top+4),
      G.Rect(xLabel+middle-3*dy,top+dy+4,xLabel+middle,2*dy+top+4)];
    var row2:=seq(5,i requires 0<=i<5 => Offset(base[i],3*dy+C.DivC(g.font,2)));
    var row3:=seq(5,i requires 0<=i<5 => Offset(base[i],6*dy+g.font));
    var left:=xLabel+g.growthLabel+g.percent+4;
    var y:=9*dy+top-3+3*C.DivC(g.font,2);
    // Supported native compilers use arithmetic signed right shift here.
    var up:=G.Rect(left,y,left+15,g.font/2+y+3);
    base+row2+row3+[up,Offset(up,up.bottom-up.top-1)]
  }
  datatype State = State(command: P.State,rects: seq<G.Rect>,initialized: set<nat>,count: S.I16,habitatCache: S.I16)
  function Slots(): set<nat> { {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16} }
  predicate Shape(s: State) {
    P.Shape(s.command) && |s.rects|==17 && s.initialized<=Slots()
  }
  datatype Input = Input(tutorial: bool,player: S.I16,defaultName: S.Bytes,titleBase: S.U16,
    geometry: Geometry,growthFirst: bool)
  datatype Event = TitleLookup(id: nat) | TitleFormat(panel: S.I16) | SetTitle
    | SetText(id: nat,text: S.Bytes) | ReadDefaultName(capacity: nat) | Sticky
    | MatchPreset(index: nat,portrait: nat) | RestorePortrait | Radio(first: nat,last: nat,choice: int)
    | Limit(id: nat,bytes: nat) | AddLeftover(index: nat) | Combo(choice: int) | SetPassword(text: S.Bytes)
    | Disable(id: nat) | SetDescription(index: nat) | LesserLabel(index: nat) | Check(id: nat,on: bool)
    | SetHabitatCache(value: int) | GetDC | GetClient | SelectFont | MeasureTemperature | MeasureUnit
    | SetRectangles | PlaceImmunity(axis: nat,x: int,y: int,width: int,height: int)
    | MeasureGrowth | MeasurePercent | SetCount(value: int) | RestoreFont | ReleaseDC
    | HigherTechLabel(level: nat)
  datatype Status = Returned(handled: bool) | InvalidEvidence
  datatype Result = Result(status: Status,state: State,calls: seq<Event>)
  function Title(s: State,x: Input,panel: S.I16): seq<Event> {
    [TitleLookup(TitleId(x.titleBase,s.command.readOnly)),TitleFormat(panel),SetTitle]
  }
  function Finish(s: State,calls: seq<Event>): Result { Result(Returned(true),s,calls) }
  function Off(ids: seq<nat>): seq<Event> { seq(|ids|,i requires 0<=i<|ids| => Disable(ids[i])) }
  function Range(first: nat,count: nat): seq<nat> { seq(count,i requires 0<=i<count => first+i) }
  function Matches(r: S.Bytes,rows: seq<S.Bytes>,count: nat): seq<Event>
    requires |r|==192 && P.Presets(rows) && count<=7
  { seq(count,i requires 0<=i<count => MatchPreset(i,P.Portrait(rows[i]))) }
  function Identity(s: State,x: Input,rows: seq<S.Bytes>,panel: S.I16): Result
    requires Shape(s) && P.Presets(rows)
    ensures Shape(Identity(s,x,rows,panel).state)
  {
    var r:=s.command.native.race;
    if !N.Comparable(r) then Result(InvalidEvidence,s,Title(s,x,panel)) else
    var name:=N.Text(r[128..160]);var plural:=N.Text(r[160..]);
    var prefix:=Title(s,x,panel)+[SetText(268,name),SetText(2075,plural)];
    if |name|==0 && !P.Text(x.defaultName,16) then Result(InvalidEvidence,s,prefix) else
    var named:=if |name|!=0 then r else P.Put(r,128,N.CopyString(r[128..160],x.defaultName));
    var actualName:=if |name|==0 then x.defaultName else name;
    var initialized:=s.(command:=s.command.(native:=s.command.native.(race:=named),editName:=actualName,editPlural:=plural));
    var before:=prefix+(if |name|==0 then [ReadDefaultName(16),SetText(268,actualName)] else [])+[Sticky];
    var forced:=x.tutorial && x.player==0 && s.command.readOnly!=0;
    var selected:=if forced then 0 else Preset(named,rows,0);
    var matchCalls:=if forced then [] else Matches(named,rows,if selected<7 then selected+1 else 7)+[RestorePortrait];
    var controls:=[Radio(271,278,selected+271),Limit(268,15),Limit(2075,15),Limit(269,15)]+
      seq(5,i requires 0<=i<5 => AddLeftover(i));
    var choice:=P.Stat(named,7);var salted:=named[12..16]!=C.Zeros(4);
    var password:=N.Text(s.command.native.password);
    var calls:=before+matchCalls+controls+[Combo(choice)];
    if salted && |password|>=16 then Result(InvalidEvidence,initialized,calls) else
    var disabled:=if s.command.readOnly==0 then [] else Off([268,269,2075]+Range(271,8)+[2074]);
    Finish(initialized.(command:=initialized.command.(combo:=choice)),calls+(if salted then [SetPassword(password)] else [])+disabled)
  }
  function Initialize(s: State,page: W.Page,panel: S.I16,x: Input,rows: seq<S.Bytes>): Result
    requires Shape(s) && P.Presets(rows)
    ensures Shape(Initialize(s,page,panel,x,rows).state)
  {
    if page==1 then Identity(s,x,rows,panel) else
    var r:=s.command.native.race;var ro:=s.command.readOnly!=0;var title:=Title(s,x,panel);
    if page==2 then
      Finish(s,title+[Radio(271,280,P.Stat(r,14)+271)]+(if ro then Off(Range(271,10)) else [])+[Sticky])
    else if page==3 then
      var choices:=seq(14,i requires 0<=i<14 => [LesserLabel(i),Check(291+i,Trait(r,i))]+(if ro then [Disable(291+i)] else []));
      var flat:=Flatten(choices);
      Finish(s.(command:=s.command.(description:=0)),title+[SetDescription(0)]+flat+[Sticky])
    else if page==4 then
      var g:=x.geometry;var before:=title+[SetHabitatCache(-1),GetDC,GetClient,SelectFont,MeasureTemperature,MeasureUnit];
      var caching:=s.(habitatCache:=-1);
      if !GeometrySafe(g) then Result(InvalidEvidence,caching,before) else
      var rects:=Layout(g);var dy:=Height(g);var xLabel:=Label(g);var middle:=Middle(g);
      var placements:=seq(3,i requires 0<=i<3 =>
        [PlaceImmunity(i,3*dy+xLabel+6,rects[5*i+3].top,middle-6*dy-12,dy),Check(291+i,r[22+i]>=128)]);
      var after:=caching.(rects:=rects,initialized:=Slots(),count:=17);
      var measures:=if x.growthFirst then [MeasureGrowth,MeasurePercent] else [MeasurePercent,MeasureGrowth];
      Finish(after,before+[SetRectangles]+Flatten(placements)+measures+[SetCount(17),RestoreFont,ReleaseDC]+
        (if ro then Off(Range(291,3)) else [])+[Sticky])
    else if page==5 then
      Finish(s,title+[Check(291,Trait(r,31))]+(if ro || P.Stat(r,14)==8 then [Disable(291)] else [])+[Sticky])
    else
      var radios:=seq(6,i requires 0<=i<6 => Radio(3*i+271,3*i+273,3*i+271+P.Stat(r,i+8)));
      var level:=if P.Stat(r,14)==9 then 4 else 3;
      Finish(s,title+radios+(if ro then Off(Range(271,18)) else [])+
        [HigherTechLabel(level),Check(291,Trait(r,29))]+(if ro then [Disable(291)] else [])+[Sticky])
  }
  function Flatten(rows: seq<seq<Event>>): seq<Event>
    decreases |rows|
  { if |rows|==0 then [] else rows[0]+Flatten(rows[1..]) }
  lemma OtherPagesKeepRace(s: State,page: W.Page,panel: S.I16,x: Input,rows: seq<S.Bytes>)
    requires Shape(s) && P.Presets(rows) && page!=1
    ensures Initialize(s,page,panel,x,rows).state.command.native==s.command.native
  {}
  lemma NonHabitatKeepsRectangles(s: State,page: W.Page,panel: S.I16,x: Input,rows: seq<S.Bytes>)
    requires Shape(s) && P.Presets(rows) && page!=4
    ensures Initialize(s,page,panel,x,rows).state.rects==s.rects
    ensures Initialize(s,page,panel,x,rows).state.initialized==s.initialized
    ensures Initialize(s,page,panel,x,rows).state.count==s.count
  {}
  lemma IdentityKeepsPortrait(s: State,x: Input,rows: seq<S.Bytes>,panel: S.I16)
    requires Shape(s) && P.Presets(rows)
    ensures P.Portrait(Identity(s,x,rows,panel).state.command.native.race)==P.Portrait(s.command.native.race)
    ensures Identity(s,x,rows,panel).state.command.nextEnabled==s.command.nextEnabled
  {}
  lemma PresetIgnoresNames(a: S.Bytes,b: S.Bytes,rows: seq<S.Bytes>,at: nat)
    requires |a|==192 && |b|==192 && a[..128]==b[..128] && P.Presets(rows) && at<=7
    ensures Preset(a,rows,at)==Preset(b,rows,at)
    decreases 7-at
  {
    if at<7 {
      assert P.SetPortrait(a,P.Portrait(rows[at]))[..128]==P.SetPortrait(b,P.Portrait(rows[at]))[..128];
      PresetIgnoresNames(a,b,rows,at+1);
    }
  }
  lemma {:isolate_assertions} LayoutFits(g: Geometry)
    ensures forall i | 0<=i<17 :: G.Fits(Layout(g)[i])
  {
    forall i | 0<=i<17 ensures G.Fits(Layout(g)[i]) {}
  }
}
