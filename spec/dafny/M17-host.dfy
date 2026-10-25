include "S01-files.dfy"
include "S02-preferences.dfy"
module M17Host {
  import Preferences = S02Preferences
  datatype LogCheck = Valid | Missing | Partial | WrongGame | WrongTurn | Invalid
  // The error branches precede the dead-player branch in CFindTurnsOutstanding.
  function Status(ai: bool,dead: bool,check: LogCheck): int {
    if ai || check==Valid then 0 else if check==WrongGame then 5 else if check==WrongTurn then 4
    else if check==Invalid then 3 else if dead then -1 else if check==Partial then 2 else 1
  }
  function Outstanding(statuses: seq<int>): nat decreases |statuses| {
    if |statuses|==0 then 0 else (if statuses[0]>0 then 1 else 0)+Outstanding(statuses[1..])
  }
  predicate Generate(fWait: bool,fTry: bool,outstanding: nat) { (!fWait && !fTry) || outstanding==0 }
  function Backups(request: int): int { Preferences.ConfiguredBackups(request) }
  predicate EnsureAI(ai: bool,alreadyDone: bool,logOpened: bool) { ai && !alreadyDone && !logOpened }
  lemma InvalidDeadStillOutstanding() ensures Status(false,true,Invalid)==3 {}
  lemma {:isolate_assertions} AcceptanceVectors()
    ensures Status(true,false,Missing)==0 && Status(false,true,Missing)==-1
    ensures Status(false,true,WrongGame)==5 && Outstanding([0,-1,2,1])==2
    ensures Outstanding([0,-1,1,2,5])==3
    ensures Generate(false,false,3) && !Generate(false,true,3) && Generate(true,false,0)
    ensures Backups(65538)==2
    ensures Backups(1000)==1 && !EnsureAI(true,false,true)
  { Preferences.AcceptanceVectors(); }
}
