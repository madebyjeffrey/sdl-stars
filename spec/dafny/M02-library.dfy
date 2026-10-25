include "S01-library-names.dfy"
include "M04-race-model.dfy"

module M02Library {
  import opened S01
  import N = S01LibraryNames
  import R = M04Race
  // Persisted PLAYER prefix after clearing fLearned; names end at their NUL.
  // S01 owns extraction/layout; this is stronger than checksum equality.
  datatype Design = Design(prefix: Bytes,singular: Bytes,plural: Bytes)
  predicate FSame(a: Design,b: Design) { a==b }
  type Version = n: int | 1<=n<=999 witness 1
  datatype Candidate = Candidate(filename: Bytes,fileKey: Bytes,stemKey: Bytes,
    raceKey: Bytes,version: Version,fPortable: bool,fReadable285: bool,design: Design)
  datatype Found = None | Some(entry: Candidate)
  datatype Versions = Versions(maximum: nat,latest: Found,same: Found)
  function Greater(a: nat,b: nat): nat { if a>b then a else b }
  // Lexical order is supplied by native SDL_strcasecmp; -1 means earlier.
  function Scan(v: Versions,c: Candidate,stemKey: Bytes,raceKey: Bytes,d: Design,cmpLatest: int): Versions {
    if !c.fPortable then v else
    var hi := if c.stemKey==stemKey then Greater(v.maximum,c.version) else v.maximum;
    if !c.fReadable285 || c.raceKey!=raceKey then v.(maximum:=hi) else
    var latest := if v.latest.None? || c.version>v.latest.entry.version ||
      (c.version==v.latest.entry.version && cmpLatest<0) then Some(c) else v.latest;
    var same := if FSame(d,c.design) && (v.same.None? || c.version>v.same.entry.version) then Some(c) else v.same;
    Versions(Greater(hi,c.version),latest,same)
  }
  datatype Existing = NoExisting | UnreadableExisting | ReadableExisting(name: Bytes,fSame: bool,fProtected: bool)
  datatype Choice = Cancel | Update | NewVersion
  datatype SavePlan = Failed | Cancelled | Reuse(name: Bytes) | WriteExisting(name: Bytes) | WriteVersion(version: nat)
  function Plan(existing: Existing,same: Found,choice: Choice,maximum: nat,
    fNew: bool,fAuthenticated: bool,fNameValid: bool,fAvailable: bool): SavePlan {
    if existing.UnreadableExisting? then Failed
    else if existing.ReadableExisting? && existing.fSame then Reuse(existing.name)
    else if same.Some? then Reuse(same.entry.filename)
    else if existing.ReadableExisting? && choice==Cancel then Cancelled
    else if existing.NoExisting? || choice==NewVersion then
      (if maximum<999 && fNameValid && fAvailable then WriteVersion(maximum+1) else Failed)
    else if existing.fProtected && fNew && !fAuthenticated then Cancelled
    else WriteExisting(existing.name)
  }
  // Selected edit destination wins even after its stored name changes. New
  // uses the latest same-race candidate; an unreadable target fails before reuse.
  function Target(fNew: bool,selected: Found,latest: Found): Found {
    if !fNew && selected.Some? then selected else latest
  }
  datatype RaceState = RaceState(design: Design,password: Bytes,file: Bytes,path: Bytes,fNew: bool)
  function Restore(prior: RaceState,current: RaceState,fRace: bool,fPath: bool): RaceState {
    RaceState(if fRace then prior.design else current.design,
      if fRace then prior.password else current.password,
      if fRace then prior.file else current.file,
      if fPath then prior.path else current.path,
      if fPath then prior.fNew else current.fNew)
  }
  function WizardExit(prior: RaceState,current: RaceState,fSuccess: bool): RaceState {
    Restore(prior,current,!fSuccess,true)
  }
  predicate FListed(fPortable: bool,fNonempty: bool) { fPortable && fNonempty }
  predicate FVersionReadable(fRegular: bool,fNotLink: bool,fBof285Race: bool,fDecoded: bool) {
    fRegular && fNotLink && fBof285Race && fDecoded
  }
  // External copy/replacement result, not a power-loss/write-flush theorem.
  function PublishedBytes(prior: Bytes,candidate: Bytes,fReplace: bool): Bytes {
    if fReplace then candidate else prior
  }
  function ImportAndEdit(fImport: bool,fEdit: bool): bool { fImport }
  // FDuplicateLibraryRace first removes .rN; SuggestRaceFilename removes one
  // more final dotted suffix. This observed behavior is not corrected here.
  function DuplicateStem(filename: Bytes): Bytes { N.DropSuffix(N.DropSuffix(filename)) }
  function SuggestedVersion(occupied: set<nat>): nat { N.FirstFree(occupied,1,999) }
  function StageAttempts(): nat { 1000 }
  datatype LegacyPolicy = RootUnsupported | AcceptedAdrConversion
  datatype LegacyDecision = RejectLegacy | NeedReplacement | ConvertLegacy | CancelLegacy
  function LegacyProtected(policy: LegacyPolicy,fValidated: bool,fConfirmed: bool): LegacyDecision {
    if !fValidated || policy==RootUnsupported then RejectLegacy
    else if fConfirmed then ConvertLegacy else NeedReplacement
  }
  lemma OccupiedStemIsNotRace(v: Versions,c: Candidate,s: Bytes,r: Bytes,d: Design,cmp: int)
    requires c.fPortable && c.stemKey==s && c.raceKey!=r
    ensures Scan(v,c,s,r,d,cmp).maximum>=c.version
    ensures Scan(v,c,s,r,d,cmp).latest==v.latest && Scan(v,c,s,r,d,cmp).same==v.same
  {}
  lemma SaveCancellation(e: Existing,same: Found,n: nat,fNew: bool,a: bool,v: bool,free: bool)
    requires e.ReadableExisting? && !e.fSame && same.None?
    ensures Plan(e,same,Cancel,n,fNew,a,v,free)==Cancelled
  {}
  lemma VersionAboveOccupied(e: Existing,same: Found,n: nat,fNew: bool,a: bool,v: bool,free: bool)
    ensures Plan(e,same,NewVersion,n,fNew,a,v,free).WriteVersion? ==>
      Plan(e,same,NewVersion,n,fNew,a,v,free).version==n+1 && n<999
  {}
  lemma CancelRestores(prior: RaceState,current: RaceState)
    ensures WizardExit(prior,current,false)==prior
  {}
  lemma ImportSurvivesEditCancel() ensures ImportAndEdit(true,false) {}
  lemma LibraryVectors() {
    var d := Design([1],[65],[65,115]);
    var c := Candidate([97,46,114,55],[97,46,114,55],[97],[98],7,true,true,d);
    var v := Versions(0,None,None);
    assert Scan(v,c,[97],[97],d,0)==Versions(7,None,None);
    var e := ReadableExisting([97,46,114,49],false,false);
    assert Plan(e,None,NewVersion,7,false,true,true,true)==WriteVersion(8);
    assert Plan(e,None,NewVersion,999,false,true,true,true)==Failed;
    assert Plan(e,Some(c),Cancel,999,false,false,false,false)==Reuse(c.filename);
    assert !FSame(d,Design([2],[65],[65,115]));
    assert FListed(true,true) && !FVersionReadable(true,false,true,true);
    assert LegacyProtected(RootUnsupported,true,true)==RejectLegacy;
    assert LegacyProtected(AcceptedAdrConversion,true,true)==ConvertLegacy;
  }
}
