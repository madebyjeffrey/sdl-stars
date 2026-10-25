include "S01-files.dfy"
module S01External {
  import opened S01
  import opened S01Codecs
  import opened S01Records
  import opened S01Orders
  import opened S01Files
  datatype Metadata = Metadata(version: nat, rgbName: Bytes, iPlayer: int,
                                time: nat, rgbExchange: Bytes)
  predicate FMetadata(m: Metadata,cbName: nat,cbExchange: nat) {
    m.version in {1,2} && 0<|m.rgbName|<cbName && -1<=m.iPlayer<16 &&
    (if m.version==1 then m.rgbExchange==m.rgbName else 0<|m.rgbExchange|<cbExchange)
  }
  function Decimal(n: nat): Bytes
    decreases n
  { if n<10 then [Byte(48+n)] else Decimal(n/10)+[Byte(48+n%10)] }
  function SignedDecimal(n: int): Bytes { if n<0 then [45]+Decimal(-n) else Decimal(n) }
  function MetadataBytes(m: Metadata): Bytes {
    Decimal(m.version)+[10]+m.rgbName+[10]+SignedDecimal(m.iPlayer)+[10]+Decimal(m.time)+[10]+
    (if m.version==2 then m.rgbExchange+[10] else [])
  }
  function Lower(b: U8): U8 { if 65<=b<=90 then Byte((b as int)+32) else b }
  function LowerBytes(b: Bytes): Bytes
    decreases |b|
  { if |b|==0 then [] else [Lower(b[0])]+LowerBytes(b[1..]) }
  function Number(b: Bytes): nat
    decreases |b|
  { if |b|==0 then 0 else 10*Number(b[..|b|-1])+(b[|b|-1] as int)%16 }
  predicate FDigits(b: Bytes) { |b|>0 && forall c <- b :: 48<=c<=57 }
  predicate FPlayerSuffix(b: Bytes) {
    3<=|b|<=4 && b[0]==46 && Lower(b[1]) in {109,104,120} && FDigits(b[2..]) &&
    b[2]!=48 && 1<=Number(b[2..])<=16
  }
  predicate FFamilySuffix(b: Bytes) {
    LowerBytes(b) in {[46,120,121],[46,104,115,116]} || FPlayerSuffix(b)
  }
  // Components are already split on slash, with a directory's final slash removed.
  predicate FSafeComponent(part: Bytes) {
    |part|>0 && part!=[46] && part!=[46,46] &&
    (forall c <- part :: c>=32 && c !in {47,92,58})
  }
  predicate FSafeComponents(parts: seq<Bytes>) {
    |parts|>0 && (forall part <- parts :: FSafeComponent(part))
  }
  function JoinPath(parts: seq<Bytes>): Bytes
    decreases |parts|
  { if |parts|==0 then [] else parts[0]+(if |parts|==1 then [] else [47]+JoinPath(parts[1..])) }
  predicate FZipName(rgb: Bytes,parts: seq<Bytes>,fDirectory: bool,fUtf8: bool) {
    0<|rgb|<512 && rgb==JoinPath(parts)+(if fDirectory then [47] else []) &&
    FSafeComponents(parts) && (if fUtf8 then FUtf8(rgb) else forall b <- rgb :: b<128)
  }
  opaque function CrcBits(crc: bv32,c: nat): bv32
    decreases c
  { if c==0 then crc else CrcBits((crc >> 1) ^ (if crc & 1 == 1 then 0xedb88320 else 0),c-1) }
  opaque function CrcBytes(rgb: Bytes,crc: bv32): bv32
    decreases |rgb|
  { if |rgb|==0 then crc else CrcBytes(rgb[1..],CrcBits(crc ^ (rgb[0] as bv32),8)) }
  function Crc32(rgb: Bytes): U32 { (CrcBytes(rgb,0xffffffff) ^ 0xffffffff) as int }
  datatype ZipEntry = ZipEntry(rgbName: Bytes,parts: seq<Bytes>,fDirectory: bool,
    flags: U16,mdCompression: U16,disk: U16,fSymlink: bool,fReparse: bool,
    cbCompressed: U32,cbPlain: U32,crc: U32,ibLocal: U32,cbExtra: U16,
    localSignature: U32,localFlags: U16,localMethod: U16,localName: Bytes,
    rgbCompressed: Bytes,rgbPlain: Bytes,cbConsumed: nat,cbProduced: nat,fStreamEnd: bool)
  predicate FZipEntry(e: ZipEntry,ibCentral: nat) {
    e.disk==0 && e.mdCompression in {0,8} && e.flags%2==0 && !e.fSymlink && !e.fReparse &&
    FZipName(e.rgbName,e.parts,e.fDirectory,Bits(e.flags,11,1)==1) &&
    e.cbPlain<=67108864 &&
      e.localSignature==67324752 && e.localFlags==e.flags && e.localMethod==e.mdCompression &&
      e.localName==e.rgbName && e.ibLocal+30+|e.localName|+e.cbExtra+e.cbCompressed<=ibCentral &&
      (e.fDirectory || (|e.rgbCompressed|==e.cbCompressed && |e.rgbPlain|==e.cbPlain && Crc32(e.rgbPlain)==e.crc &&
      (if e.mdCompression==0 then e.cbCompressed==e.cbPlain && e.rgbCompressed==e.rgbPlain
       else e.fStreamEnd && e.cbConsumed==e.cbCompressed && e.cbProduced==e.cbPlain)))
  }
  function PlainTotal(entries: seq<ZipEntry>): nat
    decreases |entries|
  { if |entries|==0 then 0 else (entries[0].cbPlain as int)+PlainTotal(entries[1..]) }
  datatype Archive = Archive(cbFile: nat,ibEocd: nat,cbComment: nat,disk: nat,centralDisk: nat,
    cDisk: nat,cEntries: nat,ibCentral: nat,cbCentral: nat,entries: seq<ZipEntry>)
  predicate FArchive(a: Archive) {
    a.ibEocd+22+a.cbComment==a.cbFile && a.disk==0 && a.centralDisk==0 &&
    a.cDisk==a.cEntries && a.cEntries==|a.entries| && a.cEntries<=4096 &&
    a.ibCentral+a.cbCentral<=a.ibEocd && PlainTotal(a.entries)<=536870912 &&
    (forall e <- a.entries :: FZipEntry(e,a.ibCentral)) &&
    (forall i,j | 0<=i<j<|a.entries| :: !a.entries[i].fDirectory && !a.entries[j].fDirectory ==>
      a.entries[i].parts!=a.entries[j].parts)
  }
  datatype ZipOutput = ZipOutput(entries: seq<ZipEntry>,dosDate: U16,dosTime: U16,rgbComment: Bytes)
  predicate FZipOutput(output: ZipOutput) {
    |output.entries|<=65535 && output.dosDate==0 && output.dosTime==0 && |output.rgbComment|==0 &&
    forall e <- output.entries :: FZipWriter(e)
  }
  // Export subset; import accepts a wider method/extra/comment range.
  predicate FZipWriter(e: ZipEntry) {
    !e.fSymlink && e.flags==2048 && e.mdCompression==0 && e.disk==0 &&
    e.cbCompressed==e.cbPlain && e.cbPlain==|e.rgbPlain| && e.crc==Crc32(e.rgbPlain) && e.cbExtra==0
  }

  datatype BatchPlayer = RacePath(rgbPath: Bytes) | AiPlayer(idAi: nat,level: nat)
  datatype VictoryLine = VictoryLine(enabled: bool,values: seq<nat>)
  datatype Batch = Batch(rgbTitle: Bytes,size: nat,density: nat,distance: nat,
    seed: seq<U32>,flags: seq<bool>,players: seq<BatchPlayer>,victory: seq<VictoryLine>,
    required: nat,minimumYears: nat,rgbBase: Bytes)
  predicate FVictory(v: VictoryLine,i: nat) {
    i<7 && (!v.enabled ||
      (if i==1 then |v.values|==2 && 8<=v.values[0]<=26 && 2<=v.values[1]<=6
       else (|v.values|==1 &&
       (if i==0 then 20<=v.values[0]<=100 else if i==2 then 1000<=v.values[0]<=20000
        else if i==3 then 20<=v.values[0]<=300 else if i==4 then 10<=v.values[0]<=500
        else if i==5 then 10<=v.values[0]<=300 else 30<=v.values[0]<=900))))
  }
  predicate FBatch(b: Batch,cbWork: nat,cbBase: nat) {
    0<|b.rgbTitle|<=31 && FUtf8(b.rgbTitle) && b.size<=4 && b.density<=3 && b.distance<=3 &&
    |b.seed|<=1 && |b.flags|==7 && 1<=|b.players|<=16 && b.players[0].RacePath? &&
    (forall p <- b.players :: if p.RacePath? then 0<|p.rgbPath|<cbWork && FUtf8(p.rgbPath)
      else p.idAi<=6 && p.level<=4) && |b.victory|==7 &&
    (forall i | 0<=i<7 :: FVictory(b.victory[i],i)) &&
    b.required<=7 && (b.required>0 ==> 30<=b.minimumYears<=500) &&
    0<|b.rgbBase|<cbBase && FUtf8(b.rgbBase)
  }
  predicate FDefinitionBytes(b: Bytes) {
    |b|<16000 && FUtf8(if |b|>=3 && b[..3]==[239,187,191] then b[3..] else b)
  }
  predicate FNumericLine(b: Bytes) { forall c <- b :: c==32 || 48<=c<=57 }
  function DigitStep(n: U32,b: U8): U32
    requires 48<=b<=57
  { (10*(n as int)+(b as int)-48)%4294967296 }
  datatype Numbers = BadNumbers | NumberValues(values: seq<U32>)
  // Observed CParseNumbers: invalid suffixes after cMax numbers are ignored.
  function ParseNumbers(b: Bytes,cMax: nat,acc: U32,fHaveDigits: bool): Numbers
    decreases |b|
  {
    if cMax==0 then NumberValues([])
    else if |b|==0 || b[0]==0 then NumberValues(if fHaveDigits then [acc] else [])
    else if b[0]!=32 && !(48<=b[0]<=57) then BadNumbers
    else if b[0]!=32 then ParseNumbers(b[1..],cMax,DigitStep(acc,b[0]),true)
    else if !fHaveDigits then ParseNumbers(b[1..],cMax,0,false)
    else var rest:=ParseNumbers(b[1..],cMax-1,0,false);
      if rest.BadNumbers? then BadNumbers else NumberValues([acc]+rest.values)
  }
  datatype Line = Line(rgb: Bytes,ending: Bytes)
  predicate FLine(line: Line) {
    line.ending in {[10],[13],[13,10]} && 10 !in line.rgb && 13 !in line.rgb && 0 !in line.rgb
  }
  function DefinitionLines(lines: seq<Line>,fBom: bool): Bytes
    decreases |lines|
  {
    (if fBom then [239,187,191] else [])+
    (if |lines|==0 then [] else lines[0].rgb+lines[0].ending+DefinitionLines(lines[1..],false))
  }
  function TrimLeadingSpaces(b: Bytes): Bytes
    decreases |b|
  { if |b|>0 && b[0]==32 then TrimLeadingSpaces(b[1..]) else b }
  function StripXY(b: Bytes): Bytes {
    if |b|>=3 && b[|b|-3..]==[46,120,121] then b[..|b|-3] else b
  }
  // Exact table order, independent of M15's calculated cell values.
  datatype ReportKind = MapReport | PlanetReport | FleetReport
  function Columns(kind: ReportKind,fPlayer: bool): seq<string> {
    match kind
    case MapReport => ["#","X","Y","Name"]
    case PlanetReport => ["Planet Name","Owner","Starbase Type","Report Age","Population","Value",
      "Production Queue","Mines","Factories","Def %","S Iron","S Bora","S Germ","Iron MR","Bora MR",
      "Germ MR","Iron MC","Bora MC","Germ MC","Resources"]+
      (if fPlayer then ["Grav","Temp","Rad","GravOrig","TempOrig","RadOrig","Terra","Cap","Scan","Pen",
       "Driver","Warp","Route","GateRange","GateMass","PctDmg"] else [])
    case FleetReport => ["Fleet Name","X","Y","Planet","Destination","Battle Plan","Ship Cnt","Iron",
      "Bora","Germ","Col","Fuel"]+(if fPlayer then ["Owner","ETA","Warp","Mass","Cloak","Scan","Pen",
      "Task","Mining","Sweep","Laying","Terra","Unarmed","Scout","Warship","Utility","Bomber"] else [])
  }
  function ReportRow(cells: seq<Bytes>): Bytes
    decreases |cells|
  { if |cells|==0 then [13,10] else cells[0]+(if |cells|==1 then [13,10] else [9]+ReportRow(cells[1..])) }

  datatype Phase = Idle | Staged | PriorHeld | Published | OrdersRetired | Restored | RecoveryRequired | Cancelled
  datatype Publication = Publication(phase: Phase,prior: Bytes,candidate: Bytes,current: Bytes,
    fPriorAvailable: bool,fOrdersPresent: bool)
  datatype Event = Stage | HoldPrior | Publish | RetireOrders | Fail | Restore | FailRestore | Cancel
  predicate FTransition(a: Publication,e: Event,b: Publication) {
    b.prior==a.prior && b.candidate==a.candidate &&
    (match e
     case Stage => a.phase==Idle && b==a.(phase:=Staged)
     case HoldPrior => a.phase==Staged && b==a.(phase:=PriorHeld,fPriorAvailable:=true)
     case Publish => a.phase==PriorHeld && b==a.(phase:=Published,current:=a.candidate)
     case RetireOrders => a.phase==Published && b==a.(phase:=OrdersRetired,fOrdersPresent:=false)
     case Fail => a.phase in {Staged,PriorHeld,Published} && b==a.(phase:=RecoveryRequired)
     case Restore => a.fPriorAvailable && a.phase==RecoveryRequired &&
       b==a.(phase:=Restored,current:=a.prior)
     case FailRestore => a.phase==RecoveryRequired && b==a
     case Cancel => a.phase in {Idle,Staged} && b==a.(phase:=Cancelled))
  }
  // Direct core writes are not atomic: partial bytes remain on failure.
  predicate FDirectFailure(wanted: Bytes,stored: Bytes) { |stored|<=|wanted| && stored==wanted[..|stored|] }
  datatype SaveResult = SaveResult(orders: Bytes,history: Bytes,fDirty: bool,fSubmitted: bool,fExported: bool)
  predicate FExportFailureAfterSave(prior: SaveResult,result: SaveResult,newOrders: Bytes,newHistory: Bytes) {
    result.orders==newOrders && result.history==newHistory && !result.fExported &&
    result.fSubmitted==prior.fSubmitted
  }
  datatype ImportChoice = SameBytes | FirstPlayer | LaterTurn | ReplaceConflict | SeparateCopy | CancelImport
  predicate FImportEffect(prior: ManagedGame,next: ManagedGame,incoming: Bof,iPlayer: PlayerId,choice: ImportChoice) {
    next.lidGame==prior.lidGame && next.histories==prior.histories &&
    (if choice in {SameBytes,SeparateCopy,CancelImport} then next==prior
     else if choice==LaterTurn then FLaterTurn(prior,incoming,iPlayer) && next==PublishLaterTurn(prior,incoming,iPlayer)
     else (next.turns==prior.turns[iPlayer:=incoming] && next.orders==prior.orders &&
       (if choice==FirstPlayer then iPlayer !in prior.turns && next.backups==prior.backups
        else iPlayer in prior.turns && |next.backups|>|prior.backups|)))
  }
  lemma ExternalVectors()
    ensures FPlayerSuffix([46,109,49,54]) && !FPlayerSuffix([46,109,48,49])
    ensures !FSafeComponents([[46,46],[120]])
    ensures !FSafeComponents([[97,92,98]])
    ensures ReportRow([[97],[],[98]])==[97,9,9,98,13,10]
    ensures StripXY([97,46,120,121])==[97] && StripXY([97,46,88,89])==[97,46,88,89]
  {
    var parent: Bytes := [46,46];
    var leaf: Bytes := [120];
    var paths: seq<Bytes> := [parent,leaf];
    assert parent in paths;
    assert !FSafeComponent(parent);
    assert !FSafeComponents(paths);
    var unsafe: Bytes := [97,92,98];
    assert 92 in unsafe;
    assert !FSafeComponent(unsafe);
    assert unsafe in [unsafe];
    assert !FSafeComponents([unsafe]);
  }
}
