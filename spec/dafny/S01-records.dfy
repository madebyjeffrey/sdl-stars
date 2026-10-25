include "S01-codecs.dfy"

// Exact payload extents and field extraction; strict well-formed-output rules.
// These predicates do not claim that the existing C readers enforce all rules.
module S01Records {
  import opened S01
  import opened S01Codecs
  datatype Field = Field(sz: string, ib: nat, cb: nat, lo: nat, bits: nat, fSigned: bool)
  function Value(rgb: Bytes, field: Field): int
    requires field.ib + field.cb <= |rgb| && field.bits > 0
  {
    var n := Bits(SliceWord(rgb, field.ib, field.cb), field.lo, field.bits);
    if field.fSigned then Signed(n, field.bits) else n
  }
  predicate FLayout(rgb: Bytes, fields: seq<Field>) {
    forall f <- fields :: f.bits > 0 && f.lo + f.bits <= 8 * f.cb && f.ib + f.cb <= |rgb|
  }
  function W(rgb: Bytes, ib: nat): U16
    requires ib + 2 <= |rgb|
  { Word(rgb, ib) }
  function D(rgb: Bytes, ib: nat): nat
    requires ib + 4 <= |rgb|
  { SliceWord(rgb, ib, 4) }
  predicate FZero(rgb: Bytes) { forall b <- rgb :: b == 0 }
  predicate FFixedText(rgb: Bytes) {
    FirstNul(rgb) < |rgb| && FUtf8(rgb[..FirstNul(rgb)])
  }
  predicate FBofBytes(rgb: Bytes, bof: Bof) {
    |rgb| == 16 && rgb[..4] == [74,51,74,51] && D(rgb,4) == bof.lidGame &&
    W(rgb,8) == bof.wVersion && W(rgb,10) == bof.turn &&
    Signed(W(rgb,12) % 32,5) == bof.iPlayer && W(rgb,12) / 32 == bof.salt &&
    W(rgb,14) % 256 == Dt(bof.kind) && Bits(W(rgb,14),8,1) == (if bof.fDone then 1 else 0) &&
    Bits(W(rgb,14),10,1) == (if bof.fMulti then 1 else 0) && W(rgb,14) / 8192 == bof.wGen
  }
  predicate FWriterBof(rgb: Bytes, bof: Bof) {
    FBofBytes(rgb,bof) && FCurrentBof(bof) && bof.wVersion == 10912 && !bof.fMulti &&
    Bits(W(rgb,14),12,1) == 0 && (bof.kind != Host ==> Bits(W(rgb,14),11,1) == 0)
  }
  predicate FGameBytes(rgb: Bytes, game: Game) {
    |rgb| == 64 && FGame(game) && D(rgb,0) == game.lid &&
    W(rgb,4) == game.mdSize && W(rgb,6) == game.mdDensity &&
    W(rgb,8) == game.cPlayer && W(rgb,10) == game.cPlanMax &&
    W(rgb,12) == game.mdStartDist && rgb[20..32] == game.rgvc &&
    FFixedText(rgb[32..]) && rgb[32..32 + FirstNul(rgb[32..])] == game.rgbTitle
  }
  function StarWord(star: StarPack): nat
    requires FStarPack(star)
  { star.dx + 1024 * star.y + 4194304 * star.idName }
  function StarX(rgstar: seq<StarPack>): nat
    decreases |rgstar|
  { if |rgstar| == 0 then 1000 else StarX(rgstar[..|rgstar|-1]) + rgstar[|rgstar|-1].dx }

  predicate FNames(rgb: Bytes, fSingle: bool, fUtf8: bool) {
    var name := ReadText(rgb,31,fUtf8);
    name.TextValue? && name.cbUsed <= |rgb| &&
    (if fSingle then name.cbUsed == |rgb|
     else var names := ReadText(rgb[name.cbUsed..],31,fUtf8);
          names.TextValue? && name.cbUsed + names.cbUsed == |rgb|)
  }
  function PlayerNamesOffset(rgb: Bytes): nat
    requires |rgb| >= 8
  { if W(rgb,6) % 8 != 7 then 8 else if |rgb| < 113 then |rgb| else 113 + (rgb[112] as int) }
  predicate FPlayer(rgb: Bytes, fLegacy: bool, fSingle: bool) {
    |rgb| >= 8 &&
    (W(rgb,6) % 8 == 7 ==> |rgb| >= 113 && (rgb[112] as int) <= 16) &&
    PlayerNamesOffset(rgb) <= |rgb| &&
    FNames(rgb[PlayerNamesOffset(rgb)..], fSingle, !fLegacy)
  }
  // Reconstruct PLAYER for checksum: padding is zero, learned bit is cleared.
  function PlayerImage(rgb: Bytes, name: Bytes, names: Bytes): Bytes
    requires |rgb| >= 113 && W(rgb,6) % 8 == 7 && (rgb[112] as int) <= 16
    requires 113 + (rgb[112] as int) <= |rgb| && |name| <= 31 && |names| <= 31
    ensures |PlayerImage(rgb,name,names)| == 192
  {
    rgb[..84] + LE(W(rgb,84) - Bits(W(rgb,84),3,1)*8,2) + rgb[86..112] +
    rgb[113..113 + (rgb[112] as int)] + Zeros(16-rgb[112]) +
    name + Zeros(32-|name|) + names + Zeros(32-|names|)
  }
  predicate FRacePayload(rgb: Bytes, checksum: U16, fLegacy: bool, fSingle: bool) {
    FPlayer(rgb,fLegacy,fSingle) && |rgb| >= 113 && W(rgb,6)%8 == 7 && (rgb[112] as int) <= 16 &&
    var name := ReadText(rgb[PlayerNamesOffset(rgb)..],31,!fLegacy);
    name.TextValue? && PlayerNamesOffset(rgb) + name.cbUsed <= |rgb| &&
    var names := if fSingle then TextValue([],0)
      else ReadText(rgb[PlayerNamesOffset(rgb) + name.cbUsed..],31,!fLegacy);
    names.TextValue? && RaceChecksum(PlayerImage(rgb,name.rgbText,names.rgbText)) == checksum
  }
  // Return -1 on a truncated/malformed sparse sequence, otherwise next offset.
  function SparseEnd(rgb: Bytes, ib: nat, c: nat, cbMask: nat, fLevels: bool): int
    requires cbMask in {1,2}
  {
    if ib + cbMask > |rgb| then -1
    else var mask := SliceWord(rgb,ib,cbMask);
      if fLevels && (exists i | 0 <= i < c :: Bits(mask,2*i,2) > 1) then -1
      else var end := ib + cbMask + PackedSize(mask,c);
        if end <= |rgb| then end else -1
  }
  function PlanetEnd(rgb: Bytes, rt: RecordTag, fHistory: bool): int {
    if |rgb| < 4 || rt !in {13,14} then -1
    else
      var flags := W(rgb,2); var det := flags % 128;
      var owner := Signed(W(rgb,0)/2048,5);
      var env := if det <= 1 then 4 else SparseEnd(rgb,4,3,1,true);
      if env < 0 then -1
      else if det > 1 && (env + 6 + 3*Bits(flags,10,1) > |rgb| ||
        !(forall i | env+3 <= i < env+6+3*Bits(flags,10,1) :: rgb[i] <= 100)) then -1
      else
        var ib := if det <= 1 then 4 else env+6+3*Bits(flags,10,1)+(if owner != -1 then 2 else 0);
        if ib > |rgb| then -1
        else
          var cargo := if det > 3 && Bits(flags,13,1)==1 then SparseEnd(rgb,ib,4,1,false) else ib;
          if cargo < 0 then -1
          else var end := cargo +
            (if rt==14 then Bits(flags,9,1)+(if fHistory then 2 else 0)
             else 8*Bits(flags,11,1)+(if owner != -1 then 4*Bits(flags,9,1)+2*Bits(flags,14,1) else 0));
            if end <= |rgb| then end else -1
  }
  predicate FPlanet(rgb: Bytes, rt: RecordTag, fHistory: bool) {
    PlanetEnd(rgb,rt,fHistory) == |rgb|
  }
  predicate FQueue(rgb: Bytes) { |rgb| % 4 == 0 && |rgb| <= 1023 }
  predicate FDesign(rgb: Bytes) {
    |rgb| >= 6 && Bits(W(rgb,0),10,5) <= 25 &&
    (if W(rgb,0)%256 == 7 then |rgb| >= 17 && (rgb[6] as int) <= 16 &&
       17 + 4*(rgb[6] as int) <= |rgb| && FExactName(rgb[17 + 4*(rgb[6] as int)..])
     else FExactName(rgb[6..]))
  }
  function FleetTail(rgb: Bytes): int {
    if |rgb| < 14 then -1
    else var ib := 14 + PopCount(W(rgb,12))*(if Bits(W(rgb,4),11,1)==1 then 1 else 2);
      if ib > |rgb| then -1
      else if W(rgb,4)%256 >= 4 then SparseEnd(rgb,ib,5,2,false) else ib
  }
  predicate FFleet(rgb: Bytes, rt: RecordTag) {
    var ib := FleetTail(rgb);
    ib >= 0 &&
    (if W(rgb,4)%256 < 7 then rt==17 && ib+8==|rgb|
     else rt==16 && W(rgb,4)%256==7 && ib+2<=|rgb| && ib+4+2*PopCount(W(rgb,ib))==|rgb|)
  }
  function DamageRead(w: U16): nat {
    w%128 + 128*(if w/128 >= 500 then 499 else w/128)
  }
  predicate FWaypoint(rgb: Bytes, rt: RecordTag) {
    ((rt==20 && |rgb|==8 && W(rgb,6)%16==0) ||
     (rt==19 && |rgb|==18 && W(rgb,6)%16<10)) &&
    Bits(W(rgb,6),8,4) in {0,1,2,4,8} &&
    (W(rgb,6)%16==1 ==> forall i | 0<=i<5 :: W(rgb,8+2*i)/4096<=9)
  }
  predicate FPlan(rgb: Bytes, fLog: bool) {
    |rgb|>=2 &&
    (if fLog && Bits(W(rgb,0),14,1)==1 then |rgb|==2
     else (|rgb|>=4 && FExactName(rgb[4..]) &&
       Bits(W(rgb,0),8,4) <= (if fLog then 6 else 5) &&
       W(rgb,2)%16 <= (if fLog then 8 else 7) && Bits(W(rgb,2),4,4) <= (if fLog then 8 else 7)))
  }
  function CatalogEnd(rgb: Bytes, args: seq<nat>): int
    decreases |rgb|
  {
    if |rgb|==0 then 0 else if |rgb|<4 then -1
    else var id := W(rgb,0)%512;
      if id>=|args| || id==511 || args[id]>7 then -1
      else var cb := 4+args[id]+PopCount(Bits(W(rgb,0),9,args[id]));
        if cb>|rgb| then -1 else var tail := CatalogEnd(rgb[cb..],args);
          if tail<0 then -1 else cb+tail
  }
  predicate FPlayerMessage(rgb: Bytes, fWriter: bool) {
    |rgb|>=12 && (!fWriter || D(rgb,0)==0) &&
    var len := Signed(W(rgb,10),16);
    |rgb|==12+(if len<0 then -len else len) &&
    (if len<0 then FFixedText(rgb[12..]) && FirstNul(rgb[12..])<=1000
     else var text := Decompress(rgb[12..]);
       text.BytesValue? && |text.rgb|<=1000 && FUtf8(text.rgb))
  }
  function EventsEnd(rgb: Bytes): int
    decreases |rgb|
  {
    if |rgb|==0 then 0 else if |rgb|<6 then -1
    else var count := Signed(W(rgb,2),16);
      if count<0 || 6+8*count>|rgb| then -1
      else var cb := 6+8*count; var rest := EventsEnd(rgb[cb..]);
        if rest<0 then -1 else cb+rest
  }
  predicate FBattle(rgb: Bytes) {
    |rgb|>=14 && W(rgb,6)==|rgb| && 14+29*(rgb[3] as int)<=|rgb| &&
    EventsEnd(rgb[14+29*(rgb[3] as int)..])==|rgb|-14-29*(rgb[3] as int)
  }
  function Join(rgrec: seq<Record>): Bytes
    decreases |rgrec|
  { if |rgrec|==0 then [] else rgrec[0].rgb+Join(rgrec[1..]) }
  predicate FBattleChunks(rgrec: seq<Record>) {
    |rgrec|>0 && FTagged(rgrec[0],31) && FTaggedSequence(rgrec[1..],39) && FBattle(Join(rgrec))
  }
  predicate FThing(rgb: Bytes) { |rgb|==18 && W(rgb,0)/8192<4 }
  function ThingPlayerView(rgb: Bytes, iPlayer: PlayerId): Bytes
    requires FThing(rgb)
    ensures |ThingPlayerView(rgb,iPlayer)|==18
  {
    var kind := W(rgb,0)/8192; var mask := Pow2(iPlayer) % 65536;
    if kind==0 then rgb[..10]+LE((W(rgb,10) as bv16 & mask as bv16) as int,2)+rgb[12..14]+
      LE((W(rgb,14) as bv16 & mask as bv16) as int,2)+rgb[16..]
    else if kind==2 then rgb[..8]+LE((W(rgb,8) as bv16 & mask as bv16) as int,2)+
      LE((W(rgb,10) as bv16 & mask as bv16) as int,2)+rgb[12..]
    else if kind==3 then rgb[..12]+LE((W(rgb,12) as bv16 & mask as bv16) as int,2)+[0,0]+rgb[16..]
    else rgb
  }
  datatype AiKind = BaseHistory | CyberHistory | EmptyHistory
  predicate FAiHistory(rgb: Bytes, kind: AiKind, cPlanMax: nat) {
    |rgb|>=2 && W(rgb,0)==|rgb| &&
    (match kind
     case EmptyHistory => |rgb|==2
     case CyberHistory => cPlanMax<=999 && |rgb|==2+2*cPlanMax
     case BaseHistory => (|rgb|>=4 && W(rgb,2)<=64 && |rgb|==4+20*W(rgb,2) &&
       forall i | 0<=i<W(rgb,2) :: W(rgb,6+20*i)<=8))
  }
  function Chunks(rgb: Bytes, rt: RecordTag): seq<Record>
    decreases |rgb|
  {
    if |rgb|==0 then [] else var cb:= if |rgb|>1023 then 1023 else |rgb|;
      [Record(rt,rgb[..cb])]+Chunks(rgb[cb..],rt)
  }

  datatype ArrayField = ArrayField(sz: string,ib: nat,cbElement: nat,count: nat,fSigned: bool)
  function ArrayValues(rgb: Bytes,field: ArrayField): seq<int>
    requires field.cbElement>0 && field.ib+field.cbElement*field.count<=|rgb|
    decreases field.count
  {
    if field.count==0 then []
    else var u:=SliceWord(rgb,field.ib,field.cbElement);
      [if field.fSigned then Signed(u,8*field.cbElement) else u]+
      ArrayValues(rgb,field.(ib:=field.ib+field.cbElement,count:=field.count-1))
  }
  function PlayerArrays(): seq<ArrayField> {
    [ArrayField("rgEnvCenter",16,1,3,true),ArrayField("rgEnvMin",19,1,3,true),
     ArrayField("rgEnvMax",22,1,3,true),ArrayField("rgTech",26,1,6,true),
     ArrayField("rgResSpent",32,4,6,false),ArrayField("rgAttr",62,1,16,true)]
  }
  predicate FPlayerObserved(rgb: Bytes,fLegacy: bool,fSingle: bool) {
    |rgb|>=8 && (W(rgb,6)%8==7 ==> |rgb|>=113 && (!fLegacy || rgb[112]<=16)) &&
    PlayerNamesOffset(rgb)<=|rgb| &&
    var first:=ReadText(rgb[PlayerNamesOffset(rgb)..],31,!fLegacy);
    first.TextValue? && PlayerNamesOffset(rgb)+first.cbUsed<=|rgb| &&
    var second:=if fSingle then TextValue([],0)
      else ReadText(rgb[PlayerNamesOffset(rgb)+first.cbUsed..],31,!fLegacy);
    second.TextValue? &&
    (!fLegacy || PlayerNamesOffset(rgb)+first.cbUsed+second.cbUsed==|rgb|)
  }
  function RepairBitmap(ibmp: U8,iFirst: U8): nat {
    if iFirst<=ibmp<iFirst+4 then ibmp else (((ibmp as bv8) & 3) | (iFirst as bv8)) as int
  }
  predicate FKeepFullDesign(fIncomingFull: bool,fPriorFree: bool,fPriorFull: bool,
    oldHull: U8,newHull: U8,oldBitmap: U8,newBitmap: U8) {
    !fIncomingFull && !fPriorFree && fPriorFull && oldHull==newHull && oldBitmap==newBitmap
  }
  datatype Slot = Slot(category: U16,item: U8,count: U8)
  function RepairSlot(slot: Slot,allowed: U16,capacity: U8,fTechUnavailable: bool,
    fGift: bool,fHost: bool,fEngineSlot: bool,engineCategory: U16,quickJump5: U8): Slot {
    var invalid:=((slot.category as bv16)&(allowed as bv16))==0 ||
      (fTechUnavailable && !fGift && !fHost) || slot.count>capacity;
    var repaired:=if slot.count>0 && invalid then slot.(count:=0) else slot;
    if fEngineSlot && repaired.count==0 then Slot(engineCategory,quickJump5,capacity) else repaired
  }
  predicate FEvent(rgb: Bytes) {
    |rgb|>=6 && Signed(W(rgb,2),16)>=0 && |rgb|==6+8*W(rgb,2)
  }
  function TokenChunks(rgb: Bytes): seq<Record>
    requires |rgb|%29==0
    decreases |rgb|
  {
    if |rgb|==0 then [] else var cb:=if |rgb|>1015 then 1015 else |rgb|;
      [Record(39,rgb[..cb])]+TokenChunks(rgb[cb..])
  }
  function EventChunks(events: seq<Bytes>,acc: Bytes): seq<Record>
    requires |acc|<1024 && (forall e <- events :: FEvent(e))
    decreases |events|,|acc|
  {
    if |events|==0 then (if |acc|==0 then [] else [Record(39,acc)])
    else if |events[0]|>=1024 then
      (if |acc|==0 then [] else [Record(39,acc)])+Chunks(events[0],39)+EventChunks(events[1..],[])
    else if |acc|+|events[0]|<1024 then EventChunks(events[1..],acc+events[0])
    else [Record(39,acc)]+EventChunks(events,[])
  }
  function JoinBytes(parts: seq<Bytes>): Bytes
    decreases |parts|
  { if |parts|==0 then [] else parts[0]+JoinBytes(parts[1..]) }
  function WriteBattleChunks(rgb: Bytes,events: seq<Bytes>): seq<Record>
    requires FBattle(rgb) && (forall e <- events :: FEvent(e))
    requires JoinBytes(events)==rgb[14+29*(rgb[3] as int)..]
  {
    if |rgb|<1024 then [Record(31,rgb)]
    else var cbTokens:=14+29*(rgb[3] as int);
      var first:=if cbTokens<1024 then cbTokens else 1000;
      [Record(31,rgb[..first])]+TokenChunks(rgb[first..cbTokens])+
      (if |rgb|-cbTokens<1024 then [Record(39,rgb[cbTokens..])] else EventChunks(events,[]))
  }

  // Field tables: offsets are relative to each named structure, not a native pointer.
  function BofFields(): seq<Field> {
    [Field("rgid",0,4,0,32,false),
     Field("lidGame",4,4,0,32,true),
     Field("wVersion",8,2,0,16,false),
     Field("turn",10,2,0,16,false),
     Field("iPlayer",12,2,0,5,true),
     Field("salt",12,2,5,11,false),
     Field("dt",14,2,0,8,false),
     Field("fDone",14,2,8,1,false),
     Field("fInUse",14,2,9,1,false),
     Field("fMulti",14,2,10,1,false),
     Field("fGameOverMan",14,2,11,1,false),
     Field("fCrippled",14,2,12,1,false),
     Field("wGen",14,2,13,3,false)]
  }
  function GameFields(): seq<Field> {
    [Field("lid",0,4,0,32,true),
     Field("mdSize",4,2,0,16,false),
     Field("mdDensity",6,2,0,16,false),
     Field("cPlayer",8,2,0,16,true),
     Field("cPlanMax",10,2,0,16,true),
     Field("mdStartDist",12,2,0,16,false),
     Field("fDirty",14,2,0,16,true),
     Field("fExtraFuel",16,2,0,1,false),
     Field("fSlowTech",16,2,1,1,false),
     Field("fSinglePlayer",16,2,2,1,false),
     Field("fTutorial",16,2,3,1,false),
     Field("fAiAlliance",16,2,4,1,false),
     Field("fBBS",16,2,5,1,false),
     Field("fVisibleScores",16,2,6,1,false),
     Field("fNoRandom",16,2,7,1,false),
     Field("fClumping",16,2,8,1,false),
     Field("wGen",16,2,9,3,false),
     Field("turn",18,2,0,16,false),
     Field("rgvc",20,12,0,96,false),
     Field("szTitle",32,32,0,256,false)]
  }
  function PlayerFields(): seq<Field> {
    [Field("iPlayer",0,1,0,8,true),
     Field("cShDef",1,1,0,8,true),
     Field("cPlanet",2,2,0,16,true),
     Field("cFleet",4,2,0,12,false),
     Field("cshdefSB",4,2,12,4,false),
     Field("det",6,2,0,3,false),
     Field("iPlrBmp",6,2,3,5,false),
     Field("fInclude",6,2,8,1,false),
     Field("mdPlayer",6,2,9,7,false),
     Field("fAi",6,2,9,1,false),
     Field("lvlAi",6,2,10,3,false),
     Field("idAi",6,2,13,3,false),
     Field("idPlanetHome",8,2,0,16,true),
     Field("wScore",10,2,0,16,false),
     Field("lSalt",12,4,0,32,true),
     Field("rgEnvCenter",16,3,0,24,false),
     Field("rgEnvMin",19,3,0,24,false),
     Field("rgEnvMax",22,3,0,24,false),
     Field("pctIdealGrowth",25,1,0,8,true),
     Field("rgTech",26,6,0,48,false),
     Field("rgResSpent",32,24,0,192,false),
     Field("pctResearch",56,1,0,8,true),
     Field("iTechCur",57,1,0,4,false),
     Field("iTechNext",57,1,4,4,true),
     Field("lResLastYear",58,4,0,32,true),
     Field("rgAttr",62,16,0,128,false),
     Field("grbitAttr",78,4,0,32,false),
     Field("grbitTrader",82,2,0,16,false),
     Field("fDead",84,2,0,1,false),
     Field("fCrippled",84,2,1,1,false),
     Field("fLearned",84,2,3,1,false),
     Field("fHacker",84,2,4,1,false),
     Field("zipprodq",86,26,0,208,false)]
  }
  function PlanetPrefixFields(): seq<Field> {
    [Field("id",0,2,0,11,true),
     Field("iPlayer",0,2,11,5,true),
     Field("det",2,2,0,7,false),
     Field("fHomeworld",2,2,7,1,false),
     Field("fInclude",2,2,8,1,false),
     Field("fStarbase",2,2,9,1,false),
     Field("fIncEVO",2,2,10,1,false),
     Field("fIncImp",2,2,11,1,false),
     Field("fIsArtifact",2,2,12,1,false),
     Field("fIncSurfMin",2,2,13,1,false),
     Field("fRouting",2,2,14,1,false),
     Field("fFirstYear",2,2,15,1,false)]
  }
  function ImprovementFields(): seq<Field> {
    [Field("iDeltaPop",0,4,0,8,false),
     Field("cMines",0,4,8,12,false),
     Field("cFactories",0,4,20,12,false),
     Field("cDefenses",4,4,0,12,false),
     Field("iScanner",4,4,12,5,false),
     Field("fArtifact",4,4,22,1,false),
     Field("fNoResearch",4,4,23,1,false)]
  }
  function StarbaseFields(): seq<Field> {
    [Field("isb",0,2,0,4,false),
     Field("pctDp",0,2,4,12,false),
     Field("idFling",2,2,0,10,false),
     Field("iWarpFling",2,2,10,4,false),
     Field("fNoHeal",2,2,14,1,false)]
  }
  function ProdFields(): seq<Field> {
    [Field("cItem",0,4,0,10,false),
     Field("iItem",0,4,10,7,false),
     Field("grobj",0,4,17,3,false),
     Field("pct",0,4,20,7,false)]
  }
  function TemplateFields(): seq<Field> {
    [Field("fNoResearch",0,1,0,8,false),
     Field("cpq",1,1,0,8,false),
     Field("rgprodq",2,24,0,192,false)]
  }
  function TemplateItemFields(): seq<Field> {
    [Field("mdIdle",0,2,0,6,false),
     Field("cQuan",0,2,6,10,false)]
  }
  function DesignPrefixFields(): seq<Field> {
    [Field("det",0,2,0,8,false),
     Field("fInclude",0,2,8,1,false),
     Field("fFree",0,2,9,1,false),
     Field("ishdef",0,2,10,5,false),
     Field("fGift",0,2,15,1,false),
     Field("ihuldef",2,1,0,8,false),
     Field("ibmp",3,1,0,8,false),
     Field("dpOrMass",4,2,0,16,false)]
  }
  function DesignFullFields(): seq<Field> {
    [Field("chsl",6,1,0,8,false),
     Field("turn",7,2,0,16,false),
     Field("cBuilt",9,4,0,32,false),
     Field("cExist",13,4,0,32,false)]
  }
  function HullSlotFields(): seq<Field> {
    [Field("grhst",0,2,0,16,false),
     Field("iItem",2,1,0,8,false),
     Field("cItem",3,1,0,8,false)]
  }
  function FleetFields(): seq<Field> {
    [Field("id",0,2,0,16,false),
     Field("ifl",0,2,0,9,false),
     Field("owner",0,2,9,4,false),
     Field("iPlayer",2,2,0,16,true),
     Field("det",4,2,0,8,false),
     Field("fInclude",4,2,8,1,false),
     Field("fRepOrders",4,2,9,1,false),
     Field("fDead",4,2,10,1,false),
     Field("fByteCounts",4,2,11,1,false),
     Field("fBombed",4,2,12,1,false),
     Field("fHereAllTurn",4,2,13,1,false),
     Field("fNoHeal",4,2,14,1,false),
     Field("fMark",4,2,15,1,false),
     Field("idPlanet",6,2,0,16,true),
     Field("x",8,2,0,16,true),
     Field("y",10,2,0,16,true),
     Field("grbitCsh",12,2,0,16,false)]
  }
  function DirectionFields(): seq<Field> {
    [Field("dirFltX",0,4,0,8,false),
     Field("dirFltY",0,4,8,8,false),
     Field("warp",0,4,16,4,false),
     Field("fValid",0,4,20,1,false),
     Field("fComposition",0,4,21,1,false),
     Field("fTargeted",0,4,22,1,false),
     Field("fSkipped",0,4,23,1,false),
     Field("wtMass",4,4,0,32,true)]
  }
  function DamageFields(): seq<Field> {
    [Field("pctSh",0,2,0,7,false),
     Field("pctDp",0,2,7,9,false)]
  }
  function WaypointFields(): seq<Field> {
    [Field("x",0,2,0,16,true),
     Field("y",2,2,0,16,true),
     Field("id",4,2,0,16,true),
     Field("grTask",6,2,0,4,false),
     Field("iWarp",6,2,4,4,false),
     Field("grobj",6,2,8,4,false),
     Field("fValidTask",6,2,12,1,false),
     Field("fNoAutoTrack",6,2,13,1,false)]
  }
  function ItemActionFields(): seq<Field> {
    [Field("cQuantity",0,2,0,12,false),
     Field("iAction",0,2,12,4,false)]
  }
  function PlanFields(): seq<Field> {
    [Field("iPlayer",0,2,0,4,false),
     Field("iplan",0,2,4,4,false),
     Field("mdTactic",0,2,8,4,false),
     Field("fDelete",0,2,14,1,false),
     Field("fDumpCargo",0,2,15,1,false),
     Field("mdTarget1",2,2,0,4,false),
     Field("mdTarget2",2,2,4,4,false),
     Field("iAttack",2,2,8,5,false)]
  }
  function CatalogFields(): seq<Field> {
    [Field("iMsg",0,2,0,9,false),
     Field("grWord",0,2,9,7,false),
     Field("wGoto",2,2,0,16,false)]
  }
  function PlayerMessageFields(): seq<Field> {
    [Field("legacyNext",0,4,0,32,false),
     Field("iPlayer",4,2,0,16,true),
     Field("iTo",6,2,0,16,true),
     Field("iInRe",8,2,0,16,true),
     Field("cLen",10,2,0,16,true)]
  }
  function BattleFields(): seq<Field> {
    [Field("id",0,2,0,16,false),
     Field("cPlayer",2,1,0,8,false),
     Field("ctok",3,1,0,8,false),
     Field("grfPlr",4,2,0,16,false),
     Field("cbData",6,2,0,16,false),
     Field("idPlanet",8,2,0,16,false),
     Field("x",10,2,0,16,true),
     Field("y",12,2,0,16,true)]
  }
  function TokenFields(): seq<Field> {
    [Field("id",0,2,0,16,false),
     Field("iPlayer",2,1,0,8,false),
     Field("grobj",3,1,0,8,false),
     Field("ishdef",4,1,0,8,false),
     Field("brc",5,1,0,8,false),
     Field("initBase",6,1,0,8,false),
     Field("initMin",7,1,0,8,false),
     Field("initMac",8,1,0,8,false),
     Field("target",9,1,0,8,false),
     Field("cloak",10,1,0,8,false),
     Field("jam",11,1,0,8,false),
     Field("computer",12,1,0,8,false),
     Field("capacitor",13,1,0,8,false),
     Field("beamDefense",14,1,0,8,false),
     Field("mass",15,2,0,16,false),
     Field("shields",17,2,0,16,false),
     Field("cShips",19,2,0,16,false),
     Field("dv",21,2,0,16,false),
     Field("target1",23,2,0,4,false),
     Field("target2",23,2,4,4,false),
     Field("tactic",23,2,8,4,false),
     Field("target0",23,2,12,4,false),
     Field("dxyLim",25,2,0,4,false),
     Field("dxyMax",25,2,4,4,false),
     Field("speed",25,2,8,4,false),
     Field("cTarget",25,2,12,4,false),
     Field("fActive",27,2,0,1,false),
     Field("fDetector",27,2,1,1,false),
     Field("fTorp",27,2,2,1,false),
     Field("fRegen",27,2,3,1,false),
     Field("fMoved",27,2,4,1,false),
     Field("disengage",27,2,5,5,false),
     Field("dwt",27,2,10,4,false),
     Field("cMoves",27,2,14,2,false)]
  }
  function EventFields(): seq<Field> {
    [Field("itok",0,1,0,8,false),
     Field("brcDest",1,1,0,8,false),
     Field("ctok",2,2,0,16,true),
     Field("round",4,2,0,4,false),
     Field("disengage",4,2,4,4,false),
     Field("attacker",4,2,8,8,false)]
  }
  function KillFields(): seq<Field> {
    [Field("itok",0,1,0,8,false),
     Field("grfWeapon",1,1,0,8,false),
     Field("cKilled",2,2,0,16,false),
     Field("shields",4,2,0,16,false),
     Field("dv",6,2,0,16,false)]
  }
  function ScoreFields(): seq<Field> {
    [Field("iPlayer",0,2,0,5,false),
     Field("fValid",0,2,5,1,false),
     Field("grVictory",0,2,6,8,false),
     Field("fWinner",0,2,14,1,false),
     Field("fHistory",0,2,15,1,false),
     Field("rankOrTurn",2,2,0,16,false),
     Field("lScore",4,4,0,32,true),
     Field("lResources",8,4,0,32,true),
     Field("cPlanet",12,2,0,16,true),
     Field("cStarbase",14,2,0,16,true),
     Field("cShip1",16,2,0,16,false),
     Field("cShip2",18,2,0,16,false),
     Field("cShip3",20,2,0,16,false),
     Field("cTech",22,2,0,16,true)]
  }
  function ThingFields(): seq<Field> {
    [Field("id",0,2,0,9,false),
     Field("iPlayer",0,2,9,4,false),
     Field("type",0,2,13,3,false),
     Field("x",2,2,0,16,true),
     Field("y",4,2,0,16,true),
     Field("payload",6,10,0,80,false),
     Field("turn",16,2,0,16,false)]
  }
  function MineFields(): seq<Field> {
    [Field("cMines",0,4,0,32,true),
     Field("grKnown",4,2,0,16,false),
     Field("iMine",6,1,0,8,false),
     Field("fDetonate",7,1,0,8,false),
     Field("grVisible",8,2,0,16,false)]
  }
  function PacketFields(): seq<Field> {
    [Field("idDest",0,2,0,10,false),
     Field("warp",0,2,10,4,false),
     Field("fMoved",0,2,14,1,false),
     Field("fInclude",0,2,15,1,false),
     Field("iron",2,2,0,16,true),
     Field("bora",4,2,0,16,true),
     Field("germ",6,2,0,16,true),
     Field("wtMax",8,2,0,14,false),
     Field("decay",8,2,14,2,false)]
  }
  function WormholeFields(): seq<Field> {
    [Field("stability",0,2,0,2,false),
     Field("cLastMove",0,2,2,10,false),
     Field("fDestKnown",0,2,12,1,false),
     Field("fInclude",0,2,13,1,false),
     Field("grKnown",2,2,0,16,false),
     Field("grTraversed",4,2,0,16,false),
     Field("idPartner",6,2,0,16,false)]
  }
  function TraderFields(): seq<Field> {
    [Field("xDest",0,2,0,16,true),
     Field("yDest",2,2,0,16,true),
     Field("warp",4,2,0,4,false),
     Field("fInclude",4,2,4,1,false),
     Field("grKnown",6,2,0,16,false),
     Field("grReward",8,2,0,16,false)]
  }
  function AiBaseFields(): seq<Field> {
    [Field("idPlanet",0,2,0,16,true),
     Field("cFreighter",2,2,0,16,true),
     Field("rgidFleet",4,16,0,128,false)]
  }
  function CyberFields(): seq<Field> {
    [Field("dirLast",0,2,0,3,false),
     Field("fColony",0,2,3,1,false),
     Field("fPacket",0,2,4,1,false),
     Field("target",0,2,5,2,false),
     Field("fScan",0,2,7,1,false)]
  }

  lemma RecordVectors()
    ensures FQueue([]) && FQueue([0,0,0,0]) && !FQueue([0])
    ensures DamageRead(65535) == 63999
    ensures FStarPack(StarPack(5,1100,0))
    ensures StarX([StarPack(5,1100,0),StarPack(7,1200,1)]) == 1012
    ensures FThing(Zeros(18)) && !FThing(Zeros(17))
  {}
}
