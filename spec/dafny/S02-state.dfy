include "S02-preferences.dfy"
include "S02-host-profile.dfy"

// ReadIniSettings, AddMRUFile and FDeleteLibraryGameRecentPaths.
// Strings are already bounded, NUL-free byte strings. Path comparison is
// ASCII C-locale folding; path-under-library and I/O results are adapter inputs.
module S02State {
  import P = S02Profile
  import H = S02HostProfile
  import S = S02Preferences
  import C = S01Codecs
  datatype Selection = Selection(kind: int,player: int,objectId: int)
  datatype SelectionRead = OutsideAtoiDomain | Loaded(selection: Selection)
  function Kind(c: char): int {
    if c=='P' then 1 else if c=='S' then 2 else if c=='E' then 4 else 0
  }
  function ReadSelection(text: string,prior: Selection): SelectionRead {
    if |text|<3 then Loaded(prior.(kind:=0)) else
    var playerOK := 'B'<=text[1]<='Q';
    var player := if playerOK then (text[1] as int)-66 else prior.player;
    var kind := if playerOK then Kind(text[0]) else 0;
    if kind==0 then Loaded(Selection(0,player,prior.objectId)) else
    var objectId := H.Decimal(text[2..]);
    if -2147483648<=objectId<=2147483647
    then Loaded(Selection(kind,player,S.ProfileI16(objectId))) else OutsideAtoiDomain
  }
  datatype IdentityRead = OutsideSignedDomain | Identity(value: nat)
  function HexDigit(c: char): nat {
    if '0'<=c<='9' then (c as int)-48
    else if 'a'<=c<='f' then (c as int)-87 else 0
  }
  function IdentityLoop(text: string,acc: nat): IdentityRead
    requires acc<=2147483647
    ensures IdentityLoop(text,acc).Identity? ==> IdentityLoop(text,acc).value<=2147483647
    decreases |text|
  {
    if |text|==0 then Identity(acc) else
    var shifted := acc*16;
    if shifted>2147483647 then OutsideSignedDomain else
    var next := shifted+HexDigit(text[0]);
    if next>2147483647 then OutsideSignedDomain else IdentityLoop(text[1..],next)
  }
  function ReadIdentity(text: string): IdentityRead { IdentityLoop(text,0) }

  function Eligible(paths: seq<string>): seq<string>
    ensures |Eligible(paths)|==|paths|
    decreases |paths|
  { if |paths|==0 then [] else [if |paths[0]|>=4 then paths[0] else ""]+Eligible(paths[1..]) }
  lemma CompactLength(paths: seq<string>)
    ensures |S.Compact(paths)|<=|paths|
    decreases |paths|
  { if |paths|>0 { CompactLength(paths[1..]); } }
  function Packed(paths: seq<string>): seq<string>
    ensures |Packed(paths)|==|paths|
  {
    CompactLength(paths);
    var kept := S.Compact(paths);
    kept+seq(|paths|-|kept|,i=>"")
  }
  datatype RecentLoad = RecentLoad(startup: bool,base: string,paths: seq<string>)
  function LoadRecent(paths: seq<string>,startupText: string,oldBase: string): RecentLoad
    requires |paths|==9
    ensures |LoadRecent(paths,startupText,oldBase).paths|==9
    ensures LoadRecent(paths,startupText,oldBase).startup==(|startupText|>=4)
  {
    RecentLoad(|startupText|>=4,if |startupText|>=4 then startupText else oldBase,Packed(Eligible(paths)))
  }
  function Same(a: string,b: string): bool { P.Fold(a)==P.Fold(b) }
  function Insertion(paths: seq<string>,name: string,i: nat): nat
    requires |paths|==9 && 1<=i<=8
    ensures i<=Insertion(paths,name,i)<=8
    decreases 8-i
  { if i==8 || Same(paths[i],name) then i else Insertion(paths,name,i+1) }
  function AddRows(paths: seq<string>,name: string): seq<string>
    requires |paths|==9
    ensures |AddRows(paths,name)|==9
  {
    if Same(paths[0],name) then paths else
    var i := Insertion(paths,name,1);
    [name]+paths[..i]+paths[i+1..]
  }
  function PublishRows(wanted: seq<string>,disk: seq<string>,ok: seq<bool>): seq<string>
    requires |wanted|==|disk|==|ok|
    ensures |PublishRows(wanted,disk,ok)|==|disk|
  { seq(|disk|,i requires 0<=i<|disk| =>if ok[i] then wanted[i] else disk[i]) }
  datatype RecentResult = RecentResult(memory: seq<string>,disk: seq<string>,attempts: nat,success: bool)
  // AddMRUFile is void: success below means it reached its return, not durable I/O.
  function Add(paths: seq<string>,disk: seq<string>,name: string,hasStorage: bool,ok: seq<bool>): RecentResult
    requires |paths|==|disk|==|ok|==9
  {
    if !hasStorage || Same(paths[0],name) then RecentResult(paths,disk,0,true) else
    var wanted := AddRows(paths,name);
    RecentResult(wanted,PublishRows(wanted,disk,ok),9,true)
  }
  function Removed(paths: seq<string>,under: seq<bool>): seq<string>
    requires |paths|==|under|==9
    ensures |Removed(paths,under)|==9
  {
    var kept := seq(9,i requires 0<=i<9 =>if under[i] then "" else paths[i]);
    if under[0] then kept else Packed(kept)
  }
  function DeleteWrites(memory: seq<string>,disk: seq<string>,wanted: seq<string>,ok: seq<bool>,i: nat,clearFirst: bool): RecentResult
    requires |memory|==|disk|==|wanted|==9 && |ok|==10 && i<=9
    ensures |DeleteWrites(memory,disk,wanted,ok,i,clearFirst).memory|==9
    ensures |DeleteWrites(memory,disk,wanted,ok,i,clearFirst).disk|==9
    ensures DeleteWrites(memory,disk,wanted,ok,i,clearFirst).memory[..i]==memory[..i]
    ensures i<=DeleteWrites(memory,disk,wanted,ok,i,clearFirst).attempts<=10
    decreases 9-i
  {
    if i==9 then
      if !clearFirst then RecentResult(memory,disk,9,true)
      else RecentResult(memory,if ok[9] then disk[0:=""] else disk,10,ok[9])
    else
      var updated := memory[i:=wanted[i]];
      if !ok[i] then RecentResult(updated,disk,i+1,false)
      else DeleteWrites(updated,disk[i:=wanted[i]],wanted,ok,i+1,clearFirst)
  }
  function Delete(paths: seq<string>,disk: seq<string>,under: seq<bool>,hasStorage: bool,formatOK: bool,ok: seq<bool>): RecentResult
    requires |paths|==|disk|==|under|==9 && |ok|==10
  {
    if !hasStorage then RecentResult(paths,disk,0,true)
    else if !formatOK then RecentResult(paths,disk,0,false)
    else DeleteWrites(paths,disk,Removed(paths,under),ok,0,under[0])
  }
  lemma DeleteSuccessInstalls(memory: seq<string>,disk: seq<string>,wanted: seq<string>,ok: seq<bool>,i: nat,clearFirst: bool)
    requires |memory|==|disk|==|wanted|==9 && |ok|==10 && i<=9
    ensures DeleteWrites(memory,disk,wanted,ok,i,clearFirst).success ==>
      DeleteWrites(memory,disk,wanted,ok,i,clearFirst).memory[i..]==wanted[i..]
    decreases 9-i
  {
    if i<9 && ok[i] {
      DeleteSuccessInstalls(memory[i:=wanted[i]],disk[i:=wanted[i]],wanted,ok,i+1,clearFirst);
      var result := DeleteWrites(memory[i:=wanted[i]],disk[i:=wanted[i]],wanted,ok,i+1,clearFirst);
      if result.success {
        assert result.memory[i]==wanted[i];
        assert result.memory[i..]==[result.memory[i]]+result.memory[i+1..];
        assert wanted[i..]==[wanted[i]]+wanted[i+1..];
      }
    }
  }
  lemma ShortSelectionRetainsIdentity(text: string,prior: Selection)
    requires |text|<3
    ensures ReadSelection(text,prior)==Loaded(Selection(0,prior.player,prior.objectId))
  {}
  lemma {:isolate_assertions} {:fuel IdentityLoop,12} {:fuel Insertion,10} {:fuel S.Compact,12} {:fuel P.Fold,6} StateVectors()
  {
    assert C.Pow2(4)==16;
    assert C.Pow2(8)==256;
    assert C.Pow2(12)==4096;
    assert C.Pow2(15)==32768;
    assert C.Pow2(16)==65536;
    var prior := Selection(2,7,90);
    assert ReadSelection("N",prior)==Loaded(Selection(0,7,90));
    assert ReadSelection("NQx",prior)==Loaded(Selection(0,15,90));
    assert ReadSelection("PA3",prior)==Loaded(Selection(0,7,90));
    assert ReadSelection("SB-1x",prior)==Loaded(Selection(2,0,-1));
    assert ReadSelection("PBx",prior)==Loaded(Selection(1,0,0));
    assert ReadIdentity("1A")==Identity(16);
    assert ReadIdentity("1a")==Identity(26);
    assert ReadIdentity("7fffffff")==Identity(2147483647);
    assert ReadIdentity("80000000")==OutsideSignedDomain;
    var paths := ["aaaa","bbbb","cccc","dddd","eeee","ffff","gggg","hhhh","iiii"];
    assert AddRows(paths,"AAAA")==paths;
    assert P.Fold(paths[0])[0]=='a';
    assert P.Fold(paths[1])[0]=='b';
    assert P.Fold("cccc")[0]=='c';
    assert !Same(paths[0],"cccc") && !Same(paths[1],"cccc") && Same(paths[2],"cccc");
    assert Insertion(paths,"cccc",1)==2;
    assert paths[..2]==["aaaa","bbbb"] && paths[3..]==["dddd","eeee","ffff","gggg","hhhh","iiii"];
    assert AddRows(paths,"cccc")==["cccc","aaaa","bbbb","dddd","eeee","ffff","gggg","hhhh","iiii"];
    assert AddRows(paths,"jjjj")==["jjjj","aaaa","bbbb","cccc","dddd","eeee","ffff","gggg","hhhh"];
    var fail := [false,true,true,true,true,true,true,true,true,true];
    var under := [true,false,false,false,false,false,false,false,false];
    var r := Delete(paths,paths,under,true,true,fail);
    assert !r.success && r.attempts==1 && r.memory[0]=="" && r.memory[1]=="bbbb" && r.disk==paths;
    assert Removed(paths,under)[1]=="bbbb";
    assert LoadRecent([".","bbbb","","","","","","",""],".","prior").startup==false;
    assert LoadRecent([".","bbbb","","","","","","",""],".","prior").base=="prior";
    assert LoadRecent([".","bbbb","","","","","","",""],".","prior").paths==["bbbb","","","","","","","",""];
    assert LoadRecent([".","bbbb","","","","","","",""],"aaaa","prior").startup;
    assert LoadRecent([".","bbbb","","","","","","",""],"aaaa","prior").base=="aaaa";
  }
}
