include "S01-library-names.dfy"
include "M06-session.dfy"

module M01Library {
  import opened S01
  import X = S01External
  import N = S01LibraryNames
  import M = M06Session
  // Fields are inspected outcomes, not assurances that every listing revalidates
  // the universe or that a metadata-presence check parses its contents.
  predicate FGamesRow(fName: bool,fMetadataRead: bool,fUniverseNonempty: bool,players: set<PlayerId>) {
    fName && fMetadataRead && fUniverseNonempty && players!={}
  }
  predicate FHostRow(fName: bool,fMetadataExists: bool,fHostValid: bool,fUniverseValid: bool,
    hostId: U32,universeId: U32) {
    fName && fMetadataExists && fHostValid && fUniverseValid && hostId==universeId
  }
  datatype Recent = Recent(name: Bytes,player: PlayerId,played: int,fTurn: bool)
  datatype MaybeRecent = NoRecent | SomeRecent(value: Recent)
  function Consider(priorChoice: MaybeRecent,next: Recent): MaybeRecent {
    if priorChoice.NoRecent? || next.played>priorChoice.value.played then SomeRecent(next) else priorChoice
  }
  function Latest(rows: seq<Recent>,priorChoice: MaybeRecent): MaybeRecent
    decreases |rows|
  { if |rows|==0 then priorChoice else Latest(rows[1..],Consider(priorChoice,rows[0])) }
  function Continue(rows: seq<Recent>,fDeleted: bool): M.ContinueAction {
    var r := Latest(rows,NoRecent);
    if fDeleted || r.NoRecent? then M.Unavailable else M.Continue(true,r.value.fTurn)
  }
  function PlayerYear(turn: U16): int { 2400+turn }
  predicate FNewFamily(fEnumerated: bool,fBad: bool,fXY: bool,fValid: bool,
    players: set<PlayerId>,fHost: bool) {
    fEnumerated && !fBad && fXY && fValid && (players!={} || fHost)
  }
  predicate FBackupDirectory(b: Bytes) {
    |b|>=6 && b[..6]==[98,97,99,107,117,112] &&
    (|b|==6 || (|b| in {7,8} && X.FDigits(b[6..])) ||
      (|b|==10 && b[6]==46 && X.FDigits(b[7..])))
  }
  function BackupSlot(occupied: set<nat>): nat { N.FirstFree(occupied,0,999) }
  function StageAttempts(): nat { 1000 }
  datatype Target = NewFamily | OnlyMatch | ChooseMatch | TargetOverflow
  function ImportTarget(count: nat): Target {
    if count==0 then NewFamily else if count==1 then OnlyMatch else if count<=64 then ChooseMatch else TargetOverflow
  }
  // Classifications and publication effects remain the existing M06/S01 rules.
  function TurnAction(fExisting: bool,fEqual: bool,prior: U16,next: U16): M.ImportAction {
    M.ClassifyImport(fExisting,fEqual,prior,next)
  }
  predicate FHostOrders(fInspected: bool,fPlayerExists: bool,host: Bof,universe: Bof,orders: Bof,p: PlayerId) {
    fInspected && fPlayerExists && host.kind==Host && universe.kind==Universe && orders.kind==Orders &&
    FCurrentBof(universe) && host.lidGame==universe.lidGame && host.lidGame!=0 &&
    FOrdersFor(orders,host,p)
  }
  datatype OrderChoice = RejectOrders | ReuseOrders | ConfirmOrders | StageOrders
  function OrdersAction(fValid: bool,fExists: bool,fEqual: bool): OrderChoice {
    if !fValid then RejectOrders else if fExists && fEqual then ReuseOrders
    else if fExists then ConfirmOrders else StageOrders
  }
  datatype Entry = Entry(name: Bytes,title: Bytes,exchange: Bytes,lid: U32,
    remembered: int,played: nat,turns: map<PlayerId,U16>)
  function Renamed(e: Entry,name: Bytes): Entry { e.(name:=name) }
  function Duplicated(e: Entry,name: Bytes): Entry { e.(name:=name,remembered:=-1,played:=0) }
  function ConflictCopy(e: Entry,name: Bytes,p: PlayerId,t: U16): Entry {
    e.(name:=name,remembered:=p,played:=0,turns:=e.turns[p:=t])
  }
  // Each directory is independently checked for target collisions. Recognition
  // is S01's family suffix rule; parent directory names themselves are retained.
  function CopiedLeaf(original: Bytes,newStem: Bytes,suffix: Bytes,fFamily: bool): Bytes {
    if fFamily then newStem+suffix else original
  }
  predicate FCopyTargets(keys: seq<Bytes>) {
    forall i,j | 0<=i<j<|keys| :: keys[i]!=keys[j]
  }
  datatype RenameResult = BeforeHoldFailure | RestoredFailure | RecoveryAtHold | PublishedClean | PublishedWithHold
  function RenameOutcome(fStage: bool,fHold: bool,fPublish: bool,fRestore: bool,fCleanup: bool): RenameResult {
    if !fStage || !fHold then BeforeHoldFailure
    else if !fPublish then (if fRestore then RestoredFailure else RecoveryAtHold)
    else if fCleanup then PublishedClean else PublishedWithHold
  }
  predicate FRenameSuccess(r: RenameResult) { r in {PublishedClean,PublishedWithHold} }
  datatype DeleteResult = NoDelete | PartialDelete | Deleted | DeletedRecentWarning
  function DeleteOutcome(fConfirmed: bool,fRemoved: bool,fRecent: bool): DeleteResult {
    if !fConfirmed then NoDelete else if !fRemoved then PartialDelete
    else if fRecent then Deleted else DeletedRecentWarning
  }
  function RecurseDelete(fDirectory: bool,fLink: bool): bool { fDirectory && !fLink }
  predicate FExportFamily(fBad: bool,fXY: bool,fPlayable: bool) { !fBad && fXY && fPlayable }
  // Picker dispatch can return true before copying. UI must distinguish it.
  datatype ExportResult = PickerStarted | ExportCancelled | ExportFailed | ExportComplete
  function ExportReturn(r: ExportResult): bool { r in {PickerStarted,ExportComplete} }
  lemma RenamePreservesIdentity(e: Entry,name: Bytes)
    ensures Renamed(e,name).lid==e.lid && Renamed(e,name).title==e.title
    ensures Renamed(e,name).exchange==e.exchange && Renamed(e,name).turns==e.turns
    ensures Renamed(e,name).remembered==e.remembered && Renamed(e,name).played==e.played
  {}
  lemma DuplicateNotPlayed(e: Entry,name: Bytes)
    ensures Duplicated(e,name).played==0 && Duplicated(e,name).remembered==-1
    ensures Duplicated(e,name).lid==e.lid && Duplicated(e,name).exchange==e.exchange
  {}
  lemma ContinueTieKeepsFirst(a: Recent,b: Recent)
    requires a.played==b.played
    ensures Latest([a,b],NoRecent)==SomeRecent(a)
  {
    assert Consider(NoRecent,a)==SomeRecent(a);
    assert Consider(SomeRecent(a),b)==SomeRecent(a);
    assert Latest([],SomeRecent(a))==SomeRecent(a);
    assert Latest([b],SomeRecent(a))==SomeRecent(a);
  }
  lemma NoLinkTraversal(fDirectory: bool)
    ensures !RecurseDelete(fDirectory,true)
  {}
  lemma {:isolate_assertions} LibraryVectors() {
    var a := Recent([65],0,10,false);
    var b := Recent([66],1,9,true);
    assert Consider(SomeRecent(a),b)==SomeRecent(a);
    assert Latest([],SomeRecent(a))==SomeRecent(a);
    assert Latest([b],SomeRecent(a))==SomeRecent(a);
    assert Latest([a,b],NoRecent)==SomeRecent(a);
    assert Continue([a,b],false)==M.ChoosePlayer;
    assert Continue([a,b],true)==M.Unavailable;
    assert FNewFamily(true,false,true,true,{},true);
    assert !FNewFamily(true,false,true,true,{},false);
    assert TurnAction(true,false,10,10)==M.Conflict;
    assert TurnAction(true,false,10,12)==M.Later;
    var keys: seq<Bytes> := [[97],[97]];
    assert keys[0]==keys[1];
    assert !FCopyTargets(keys);
    assert RenameOutcome(true,true,false,false,false)==RecoveryAtHold;
    assert FRenameSuccess(RenameOutcome(true,true,true,false,false));
    assert DeleteOutcome(true,false,true)==PartialDelete;
    assert ExportReturn(PickerStarted) && ExportReturn(ExportComplete);
    assert OrdersAction(true,true,false)==ConfirmOrders;
    assert FBackupDirectory([98,97,99,107,117,112,46,48,48,48]);
    var badBackup: Bytes := [98,97,99,107,117,112,46,49];
    assert badBackup[6..]==[46,49];
    assert 46 in badBackup[6..];
    assert !X.FDigits(badBackup[6..]);
    assert !FBackupDirectory(badBackup);
    assert ImportTarget(64)==ChooseMatch && ImportTarget(65)==TargetOverflow;
  }
}
