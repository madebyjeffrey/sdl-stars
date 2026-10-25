include "S01-orders.dfy"
include "M03-scoring.dfy"
module M15Reports {
  // One bool per event message; inbound player messages follow event messages.
  predicate Visible(filtered: bool,showFiltered: bool,onlyFiltered: bool) {
    if showFiltered && !onlyFiltered then true else filtered==onlyFiltered
  }
  function Next(filters: seq<bool>,inbound: nat,at: int,show: bool,only: bool): int
    requires -1<=at<=|filters|+inbound
    ensures -1<=Next(filters,inbound,at,show,only)<|filters|+inbound
    ensures Next(filters,inbound,at,show,only)==-1 || at<Next(filters,inbound,at,show,only)
    decreases |filters|+inbound-at
  { if at+1>=|filters|+inbound then -1 else if at+1>=|filters| || Visible(filters[at+1],show,only) then at+1 else Next(filters,inbound,at+1,show,only) }
  function Previous(filters: seq<bool>,inbound: nat,at: int,show: bool,only: bool): int
    requires -1<=at<=|filters|+inbound
    ensures Previous(filters,inbound,at,show,only)==-1 || 0<=Previous(filters,inbound,at,show,only)<at
    decreases at
  {
    if at<=0 then -1
    else if at-1>=|filters| || Visible(filters[at-1],show,only) then at-1
    else Previous(filters,inbound,at-1,show,only)
  }
  predicate Eligible(filters: seq<bool>,inbound: nat,i: int,show: bool,only: bool) {
    0<=i<|filters|+inbound && (i>=|filters| || Visible(filters[i],show,only))
  }
  lemma NextNearest(fs: seq<bool>,n: nat,i: int,s: bool,o: bool)
    requires -1<=i<|fs|+n
    ensures Next(fs,n,i,s,o)==-1 ==> forall j | i<j<|fs|+n :: !Eligible(fs,n,j,s,o)
    ensures Next(fs,n,i,s,o)!=-1 ==> Eligible(fs,n,Next(fs,n,i,s,o),s,o)
    ensures Next(fs,n,i,s,o)!=-1 ==> forall j | i<j<Next(fs,n,i,s,o) :: !Eligible(fs,n,j,s,o)
    decreases |fs|+n-i
  {
    if i+1<|fs| && !Visible(fs[i+1],s,o) { NextNearest(fs,n,i+1,s,o); }
  }
  lemma PreviousNearest(fs: seq<bool>,n: nat,i: int,s: bool,o: bool)
    requires -1<=i<=|fs|+n
    ensures Previous(fs,n,i,s,o)==-1 || 0<=Previous(fs,n,i,s,o)<i
    ensures Previous(fs,n,i,s,o)==-1 ==> forall j | 0<=j<i :: !Eligible(fs,n,j,s,o)
    ensures Previous(fs,n,i,s,o)!=-1 ==> Eligible(fs,n,Previous(fs,n,i,s,o),s,o)
    ensures Previous(fs,n,i,s,o)!=-1 ==> forall j | Previous(fs,n,i,s,o)<j<i :: !Eligible(fs,n,j,s,o)
    decreases i
  {
    if 0<i<=|fs| && !Visible(fs[i-1],s,o) { PreviousNearest(fs,n,i-1,s,o); }
  }
  lemma PreviousUndoesNext(fs: seq<bool>,n: nat,i: int,s: bool,o: bool)
    requires Eligible(fs,n,i,s,o)
    requires Next(fs,n,i,s,o)!=-1
    ensures Previous(fs,n,Next(fs,n,i,s,o),s,o)==i
  {
    NextBound(fs,n,i,s,o); NextNearest(fs,n,i,s,o);
    var next := Next(fs,n,i,s,o);
    PreviousNearest(fs,n,next,s,o);
    var previous := Previous(fs,n,next,s,o);
    if previous<i { assert !Eligible(fs,n,i,s,o); }
    if previous>i { assert !Eligible(fs,n,previous,s,o); }
  }
  lemma NextUndoesPrevious(fs: seq<bool>,n: nat,i: int,s: bool,o: bool)
    requires Eligible(fs,n,i,s,o)
    requires Previous(fs,n,i,s,o)!=-1
    ensures Next(fs,n,Previous(fs,n,i,s,o),s,o)==i
  {
    PreviousNearest(fs,n,i,s,o);
    var previous := Previous(fs,n,i,s,o);
    NextBound(fs,n,previous,s,o); NextNearest(fs,n,previous,s,o);
    var next := Next(fs,n,previous,s,o);
    if next<i { assert !Eligible(fs,n,i,s,o) || !Eligible(fs,n,next,s,o); }
    if next>i { assert !Eligible(fs,n,i,s,o); }
  }
  predicate PlanetRow(fOwned: bool,fFull: bool) { fOwned && fFull }
  function EnemyRows(available: nat): nat { if available>1020 then 1020 else available }
  datatype DraftAction = Delete | KeepOrWrite
  function FinishDraft(bytes: nat,fDelete: bool): DraftAction { if fDelete || bytes==0 then Delete else KeepOrWrite }
  lemma NextBound(fs: seq<bool>,n: nat,i: int,s: bool,o: bool)
    requires -1<=i<|fs|+n
    ensures Next(fs,n,i,s,o)==-1 || i<Next(fs,n,i,s,o)<|fs|+n
    decreases |fs|+n-i
  { if i+1<|fs| && !Visible(fs[i+1],s,o) { NextBound(fs,n,i+1,s,o); } }
  lemma {:isolate_assertions} AcceptanceVectors()
    ensures Next([true,false],1,-1,false,false)==1
    ensures Next([true,false],1,1,false,true)==2
    ensures Next([false],0,0,true,false)==-1
    ensures EnemyRows(1500)==1020 && FinishDraft(0,false)==Delete
    ensures Previous([true,false],1,3,false,true)==2
    ensures Previous([true,false],1,2,false,true)==0
    ensures Previous([true,false],1,-1,false,false)==-1
    ensures Previous([],0,0,false,false)==-1
    ensures Next([false],0,1,false,false)==-1 && Next([],0,0,false,false)==-1
    ensures Next([true,true],1,-1,false,false)==2 && Next([true,true],1,0,false,true)==1
  {}
}
