include "M11-design.dfy"
include "M08-native-economy.dfy"
include "S03-lookup-identities.dfy"

// Native FLookupPart/TechStatus/FShouldPartBeHidden decisions. Catalog addresses
// are logical references, not serialized pointers. Actual C frames are qualified
// by the source fixture and remain distinct from a universal memory refinement.
module M11NativeAvailability {
  import D = M11Design
  import E = M08NativeEconomy
  import S = S01
  import C = S01Codecs
  import K = S03ResearchCatalog
  import I = S03LookupIdentities
  datatype Context = Context(headless: bool,primary: E.I8,flags: S.U32,trader: S.U16,levels: seq<E.I8>,current: nat)
  predicate Shape(c: Context) { |c.levels|==6 && c.current<16 }
  function Bit(c: Context,bit: nat): bool requires bit<32 { (c.flags/C.Pow2(bit))%2==1 }
  function Kind(mask: S.U16,at: nat): nat
    requires at<=16
    ensures at<=Kind(mask,at)<=16
    ensures Kind(mask,at)<16 ==> K.Masks()[Kind(mask,at)]==mask
    ensures forall j | at<=j<Kind(mask,at) :: K.Masks()[j]!=mask
    decreases 16-at
  { if at==16 || K.Masks()[at]==mask then at else Kind(mask,at+1) }
  function Missing(have: seq<E.I8>,need: seq<E.I8>): set<int>
    requires |have|==6 && |need|==6
  { set i: int | 0<=i<6 && have[i]<need[i] }
  function TechStatus(have: seq<E.I8>,need: seq<E.I8>,field: nat): S.I16
    requires |have|==6 && |need|==6 && field<16
    ensures 1<=TechStatus(have,need,field)<=256
  {
    var missing:=Missing(have,need);
    if missing=={} then 1 else
    if field<6 && missing=={field} then
      (if need[field]==have[field]+1 then 2 else (need[field] as int)-have[field]+1) as S.I16
    else 99
  }
  function TraderFlag(kind: nat,item: S.U8): S.U16
    requires kind<16
  {
    if kind==0 && item==I.iengineEnigmaPulsar then 512
    else if kind==2 && item==I.ishieldLangstonShell then 4
    else if kind==3 && item==I.iarmorMegaPolyShell then 8
    else if kind==4 && item==I.ibeamMultiContainedMunition then 128
    else if kind==5 && item==I.itorpAntiMatterTorpedo then 64
    else if kind==6 && item==I.ibombHushABoom then 32
    else if kind==7 && item==I.iminingAlienMiner then 16
    else if kind==11 && item==I.ispecialEMultiFunctionPod then 2
    else if kind==12 && item==I.ispecialMMultiCargoPod then 1
    else if kind==12 && item==I.ispecialMJumpGate then 2048
    else if kind==14 && item==I.ihuldefMiniMorph then 256
    else if kind==15 && item==I.iplanetaryGenesisDevice then 1024 else 0
  }
  function Hidden(c: Context,kind: nat,item: S.U8): bool
    requires kind<16
  {
    var flag:=TraderFlag(kind,item);
    !c.headless && flag!=0 && ((c.trader as bv16)&(flag as bv16))==0
  }
  // Getter/call tags: 0 major, 1 hidden helper, 2 technology helper, 100+bit race bit.
  datatype Decision = Decision(allowed: bool,calls: seq<nat>)
  function HiddenDecision(c: Context,kind: nat,item: S.U8,calls: seq<nat>): Decision
    requires kind<16
  { Decision(!Hidden(c,kind,item),calls+[1]) }
  function Policy(c: Context,kind: nat,item: S.U8,planetaryAbility: S.I16): Decision
    requires Shape(c) && kind<16
  {
    var ra:=if c.headless then 9 else c.primary as int;
    if c.headless && kind !in {2,3,4} then Decision(true,[]) else
    match kind
    case 0 =>
      if item==I.iengineSettlersDelight && ra!=0 then Decision(false,[]) else
      var scoop:=I.iengineSubGalacticFuelScoop<=item<=I.iengineGalaxyScoop || item==I.iengineRadiatingHydroRamScoop;
      var a:=if scoop then [107] else [];
      if scoop && Bit(c,7) then Decision(false,a) else
      var ife:=item in {I.iengineGalaxyScoop,I.iengineFuelMizer};var b:=a+(if ife then [100] else []);
      if ife && !Bit(c,0) then Decision(false,b) else
      var noRam:=item==I.iengineInterspace10;var d:=b+(if noRam then [107] else []);
      if noRam && !Bit(c,7) then Decision(false,d) else HiddenDecision(c,kind,item,d)
    case 1 =>
      var nas:=item in {I.iscannerFerretScanner,I.iscannerDolphinScanner,I.iscannerElephantScanner};
      var calls:=if nas then [110] else [];
      if nas && Bit(c,10) then Decision(false,calls) else
      Decision(!(item in {I.iscannerChameleonScanner,I.iscannerPickPocketScanner,I.iscannerRobberBaronScanner} && ra!=1),calls)
    case 2 =>
      if (item==I.ishieldShadowShield && ra!=1) || (item==I.ishieldCrobySharmor && ra!=4) then Decision(false,[]) else HiddenDecision(c,kind,item,[])
    case 3 =>
      if (item==I.iarmorDepletedNeutronium && ra!=1) || (item==I.iarmorFieldedKelarium && ra!=4) then Decision(false,[]) else HiddenDecision(c,kind,item,[])
    case 4 =>
      if (item==I.ibeamMiniGun && ra!=4) || (item in {I.ibeamBlunderbuss,I.ibeamGatlingNeutrinoCannon} && ra!=2) then Decision(false,[]) else HiddenDecision(c,kind,item,[])
    case 5 => HiddenDecision(c,kind,item,[])
    case 6 =>
      if (I.ibombSmartBomb<=item<=I.ibombAnnihilatorBomb && ra==4) || (item==I.ibombRetroBomb && ra!=3) then Decision(false,[]) else HiddenDecision(c,kind,item,[])
    case 7 =>
      var obrm:=item in {I.iminingRoboMiner,I.iminingRoboMaxiMiner,I.iminingRoboSuperMiner,I.iminingRoboMidgetMiner,I.iminingRoboUltraMiner};
      var a:=if obrm then [109] else [];
      if obrm && Bit(c,9) then Decision(false,a) else
      var arm:=item in {I.iminingRoboMidgetMiner,I.iminingRoboUltraMiner};var b:=a+(if arm then [102] else []);
      if arm && !Bit(c,2) then Decision(false,b) else
      if item==I.iminingOrbitalAdjuster && ra!=3 then Decision(false,b) else HiddenDecision(c,kind,item,b)
    case 8 =>
      var sd:=item in {I.iminesMineDispenser40,I.iminesMineDispenser80,I.iminesMineDispenser130,I.iminesHeavyDispenser50,I.iminesHeavyDispenser110,I.iminesHeavyDispenser200,I.iminesSpeedTrap30,I.iminesSpeedTrap50};
      Decision(!(sd && ra!=5) && !(item==I.iminesSpeedTrap20 && ra !in {4,5}) && !(item==I.iminesMineDispenser50 && ra==2),[])
    case 9 =>
      if I.ispecialSBMassDriver5<=item<=I.ispecialSBUltraDriver13 then Decision(item in {I.ispecialSBMassDriver7,I.ispecialSBUltraDriver10} || ra==6,[]) else
      if item<I.ispecialSBStargate100250 || item>I.ispecialSBStargateAnyAny then Decision(true,[]) else
      Decision(!(ra!=7 && (item==I.ispecialSBStargateAny300 || item>=I.ispecialSBStargate100Any)) && ra!=0,[])
    case 10 =>
      var isb:=item in {I.isbhullSpaceDock,I.isbhullUltraStation};var calls:=if isb then [103] else [];
      if isb && !Bit(c,3) then Decision(false,calls) else Decision(item!=I.isbhullDeathStar || ra==8,calls)
    case 11 =>
      if Hidden(c,kind,item) then Decision(false,[1]) else
      Decision(!((item in {I.ispecialETransportCloaking,I.ispecialEUltraStealthCloak} && ra!=1) ||
        (item==I.ispecialEEnergyDampener && ra!=5) || (item==I.ispecialEAntiMatterGenerator && ra!=7) ||
        (item==I.ispecialEFluxCapacitor && ra!=0) || (item in {I.ispecialEJammer10,I.ispecialEJammer50,I.ispecialETachyonDetector} && ra!=4)),[1])
    case 12 =>
      if Hidden(c,kind,item) then Decision(false,[1]) else
      Decision(!(item==I.ispecialMColonizationModule && ra==8) && (item!=I.ispecialMOrbitalConstructionModule || ra==8),[1])
    case 13 =>
      var tt:=I.iterraTotalTerraform3<=item<=I.iterraTotalTerraform30;
      Decision(!tt || Bit(c,1),if tt then [101] else [])
    case 14 =>
      if (item in {I.ihuldefMiniColonyShip,I.ihuldefMetaMorph} && ra!=0) || (item in {I.ihuldefFuelTransport,I.ihuldefSuperFreighter} && ra!=4) then Decision(false,[]) else
      var obrm:=item in {I.ihuldefMiner,I.ihuldefMaxiMiner,I.ihuldefMidgetMiner,I.ihuldefUltraMiner};var a:=if obrm then [109] else [];
      if obrm && Bit(c,9) then Decision(false,a) else
      var arm:=item in {I.ihuldefMidgetMiner,I.ihuldefMiner,I.ihuldefUltraMiner};var b:=a+(if arm then [102] else []);
      if arm && !Bit(c,2) then Decision(false,b) else
      if (item in {I.ihuldefDreadnought,I.ihuldefBattleCruiser} && ra!=2) || (item in {I.ihuldefRogue,I.ihuldefStealthBomber} && ra!=1) ||
        (item in {I.ihuldefMiniMineLayer,I.ihuldefSuperMineLayer} && ra!=5) then Decision(false,b) else HiddenDecision(c,kind,item,b)
    case 15 =>
      var scanner:=I.iplanetaryViewer50<=item<=I.iplanetarySnooper620X;
      var nas:=scanner && planetaryAbility<0;var calls:=if nas then [110] else [];
      if nas && Bit(c,10) then Decision(false,calls) else
      if (scanner && ra==8) || (I.iplanetarySDI<=item<=I.iplanetaryNeutronShield && ra==8) ||
        (I.iplanetaryLaserBattery<=item<=I.iplanetaryNeutronShield && ra==2) then Decision(false,calls) else HiddenDecision(c,kind,item,calls)
    case _ => Decision(false,[])
  }
  datatype Address = Prior(token: int) | Row(kind: nat,item: S.U8)
  datatype Result = Result(status: S.I16,pointer: Address,calls: seq<nat>)
  function Lookup(c: Context,mask: S.U16,item: S.U8,need: seq<E.I8>,planetaryAbility: S.I16,prior: Address): Result
    requires Shape(c) && |need|==6
    ensures -1<=Lookup(c,mask,item,need,planetaryAbility,prior).status<=256
    ensures Lookup(c,mask,item,need,planetaryAbility,prior).status==0 <==>
      (Kind(mask,0)==16 || item>=K.Limits()[Kind(mask,0)])
    ensures Lookup(c,mask,item,need,planetaryAbility,prior).status==0 ==> Lookup(c,mask,item,need,planetaryAbility,prior).pointer==prior
    ensures Lookup(c,mask,item,need,planetaryAbility,prior).status!=0 ==> Lookup(c,mask,item,need,planetaryAbility,prior).pointer==Row(Kind(mask,0),item)
  {
    var kind:=Kind(mask,0);var queried:=if c.headless then [] else [0];
    if kind==16 || item>=K.Limits()[kind] then Result(0,prior,queried) else
    var decision:=Policy(c,kind,item,planetaryAbility);var calls:=queried+decision.calls;
    if !decision.allowed then Result(-1,Row(kind,item),calls) else
    if c.headless then Result(1,Row(kind,item),calls) else
    Result(TechStatus(c.levels,need,c.current),Row(kind,item),calls+[2])
  }
  datatype PartRow = PartRow(need: seq<E.I8>,planetaryAbility: S.I16)
  predicate CatalogShape(rows: seq<seq<PartRow>>) {
    |rows|==16 && (forall k | 0<=k<16 :: (|rows[k]|==K.Limits()[k] &&
      (forall item | 0<=item<|rows[k]| :: |rows[k][item].need|==6)))
  }
  lemma MaskKindAt(kind: nat,at: nat)
    requires at<=kind<16
    ensures Kind(K.Masks()[kind],at)==kind
    decreases kind-at
  {
    if at<kind {
      assert K.Masks()[at]!=K.Masks()[kind];
      MaskKindAt(kind,at+1);
    }
  }
  function HiddenMask(c: Context,mask: S.U16,item: S.U8): bool {
    var kind:=Kind(mask,0);kind<16 && Hidden(c,kind,item)
  }
  function LookupRow(c: Context,rows: seq<seq<PartRow>>,mask: S.U16,item: S.U8,prior: Address): Result
    requires Shape(c) && CatalogShape(rows)
    ensures -1<=LookupRow(c,rows,mask,item,prior).status<=256
    ensures LookupRow(c,rows,mask,item,prior).status==0 <==>
      (Kind(mask,0)==16 || item>=K.Limits()[Kind(mask,0)])
    ensures LookupRow(c,rows,mask,item,prior).status==0 ==> LookupRow(c,rows,mask,item,prior).pointer==prior
    ensures LookupRow(c,rows,mask,item,prior).status!=0 ==> LookupRow(c,rows,mask,item,prior).pointer==Row(Kind(mask,0),item)
  {
    var kind:=Kind(mask,0);
    var row:=if kind<16 && item<K.Limits()[kind] then rows[kind][item] else PartRow([0,0,0,0,0,0],0);
    Lookup(c,mask,item,row.need,row.planetaryAbility,prior)
  }
  function Natural(xs: seq<E.I8>): seq<nat>
    requires forall i | 0<=i<|xs| :: xs[i]>=0
  { seq(|xs|,(i: int) requires 0<=i<|xs| => xs[i] as nat) }
  function Code(status: D.Availability): int {
    match status
    case Invalid => 0
    case Restricted => -1
    case Available => 1
    case NextTech => 2
    case Later(levels) => levels
    case OtherTech => 99
  }
  lemma TechProjection(have: seq<E.I8>,need: seq<E.I8>,field: nat)
    requires |have|==6 && |need|==6 && field<6
    requires forall i | 0<=i<6 :: have[i]>=0 && need[i]>=0
    ensures TechStatus(have,need,field)==Code(D.TechStatus(Natural(have),Natural(need),field))
  {
    assert Missing(have,need)==D.Missing(Natural(have),Natural(need));
  }
  lemma {:isolate_assertions} Vectors() {
    assert TechStatus([-128,0,0,0,0,0],[127,0,0,0,0,0],0)==256;
    assert TechStatus([0,0,0,0,0,0],[98,0,0,0,0,0],0)==99;
    assert TechStatus([0,0,0,0,0,0],[1,0,0,0,0,0],15)==99;
    assert TechStatus([0,0,0,0,0,0],[1,0,0,0,0,0],0)==2;
    assert Kind(3,0)==16;
    var c:=Context(true,0,0,0,[0,0,0,0,0,0],0);
    MaskKindAt(2,0);
    var restricted:=Lookup(c,4,I.ishieldShadowShield as S.U8,[0,0,0,0,0,0],0,Prior(42));
    assert restricted.status==-1 && restricted.pointer==Row(2,I.ishieldShadowShield as S.U8) && restricted.calls==[];
    var ordinary:=Lookup(c,4,0,[0,0,0,0,0,0],0,Prior(42));
    assert ordinary.status==1 && ordinary.calls==[1];
    var invalid:=Lookup(c.(headless:=false),3,0,[0,0,0,0,0,0],0,Prior(42));
    assert invalid.status==0 && invalid.pointer==Prior(42) && invalid.calls==[0];
    MaskKindAt(1,0);
    var nas:=Lookup(c.(headless:=false,flags:=1024),2,I.iscannerFerretScanner as S.U8,[0,0,0,0,0,0],0,Prior(0));
    assert nas.status==-1 && nas.calls==[0,110];
  }
}
