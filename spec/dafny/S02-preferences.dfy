include "S04-arithmetic.dfy"
module S02Preferences {
  import opened S04Arithmetic
  import C = S01Codecs
  datatype Root = Explicit | ProfileOverride | SettingsOverride | PlatformConfig | CurrentDirectory
  function Location(explicit: bool,profile: bool,settings: bool,platform: bool): Root {
    if explicit then Explicit else if profile then ProfileOverride else if settings then SettingsOverride else if platform then PlatformConfig else CurrentDirectory
  }
  // ReadIniSettings stores pane values in int16_t and layout in uint16_t,
  // then compares layout through int16_t. Narrow before clamping.
  function ProfileI16(n: int): int { C.Signed(n,16) }
  function PaneSize(n: int): int { Clamp(ProfileI16(n),10,2000) }
  function Layout(n: int): int { Clamp(ProfileI16(n),0,2) }
  function Backups(n: int): int { if 1<=n<=999 then n else 1 }
  function ConfiguredBackups(n: int): int { Backups(ProfileI16(n)) }
  function SortColumn(packed: nat): nat { packed%256 }
  function Ascending(packed: nat): bool { (packed/256)%2==1 }
  function Column(current: nat,fSeparator: bool): nat requires current<=1 { if fSeparator then 1 else current }
  function Compact(paths: seq<string>): seq<string> decreases |paths| {
    if |paths|==0 then [] else (if paths[0]=="" then [] else [paths[0]])+Compact(paths[1..])
  }
  lemma PaneBound(n: int) ensures 10<=PaneSize(n)<=2000 {}
  lemma LayoutBound(n: int) ensures 0<=Layout(n)<=2 {}
  lemma {:isolate_assertions} AcceptanceVectors()
    ensures Location(false,true,true,true)==ProfileOverride
    ensures PaneSize(0)==10 && PaneSize(5000)==2000 && Backups(0)==1
    ensures Layout(32768)==0 && Layout(65536)==0 && Layout(65636)==2
    ensures PaneSize(40000)==10 && PaneSize(65636)==100
    ensures ConfiguredBackups(65538)==2 && Backups(65538)==1
    ensures ConfiguredBackups(1000)==1
    ensures SortColumn(259)==3 && Ascending(259)
    ensures Compact(["a","","b"])==["a","b"]
  {
    assert C.Pow2(4)==16;
    assert C.Pow2(8)==256;
    assert C.Pow2(12)==4096;
    assert C.Pow2(15)==32768;
    assert C.Pow2(16)==65536;
    assert ProfileI16(32768)==-32768 && ProfileI16(65536)==0;
    assert ProfileI16(40000)==-25536 && ProfileI16(65636)==100;
    assert ProfileI16(65538)==2;
  }
}
