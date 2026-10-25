include "S02-preferences.dfy"

module S02Layout {
  import P = S02Preferences
  datatype Tile = Tile(id: nat,column: nat,popped: bool,other: int)
  function FindTile(tiles: seq<Tile>,start: nat,id: nat): nat
    requires start<=|tiles|
    ensures start<=FindTile(tiles,start,id)<=|tiles|
    decreases |tiles|-start
  {
    if start==|tiles| || tiles[start].id==id then start else FindTile(tiles,start+1,id)
  }
  function Install(tiles: seq<Tile>,prefix: nat,index: nat,column: nat,popped: bool): seq<Tile>
    requires prefix<=index<|tiles|
    ensures |Install(tiles,prefix,index,column,popped)|==|tiles|
  {
    var chosen := tiles[index].(column:=column,popped:=popped);
    if index==prefix then tiles[prefix:=chosen]
    else tiles[index:=tiles[prefix]][prefix:=chosen]
  }
  function RaiseTail(tiles: seq<Tile>,prefix: nat,column: nat): seq<Tile>
    requires prefix<=|tiles|
    ensures |RaiseTail(tiles,prefix,column)|==|tiles|
  { seq(|tiles|,i requires 0<=i<|tiles| =>
      if i<prefix || tiles[i].column>=column then tiles[i] else tiles[i].(column:=column)) }
  function ReadTiles(format: string,tiles: seq<Tile>,prefix: nat,column: nat): seq<Tile>
    requires prefix<=|tiles| && column<=1
    ensures |ReadTiles(format,tiles,prefix,column)|==|tiles|
    decreases |format|
  {
    if |format|==0 || format[0]=='\0' then RaiseTail(tiles,prefix,column)
    else if format[0]=='*' then ReadTiles(format[1..],tiles,prefix,1)
    else if !('a'<=format[0]<='z' || 'A'<=format[0]<='Z') then ReadTiles(format[1..],tiles,prefix,column)
    else
      var popped := 'A'<=format[0]<='Z';
      var id := (format[0] as int)-(if popped then 65 else 97);
      var index := FindTile(tiles,prefix,id);
      if index==|tiles| then ReadTiles(format[1..],tiles,prefix,column)
      else ReadTiles(format[1..],Install(tiles,prefix,index,column,popped),prefix+1,column)
  }
  function Stars(n: nat): string decreases n { if n==0 then "" else "*"+Stars(n-1) }
  function WriteTiles(tiles: seq<Tile>,column: nat): string
    requires forall t <- tiles :: t.id<16
    decreases |tiles|
  {
    if |tiles|==0 then "" else
      var next := if tiles[0].column>column then tiles[0].column else column;
      Stars(next-column)+[(tiles[0].id+(if tiles[0].popped then 65 else 97)) as char]+WriteTiles(tiles[1..],next)
  }
  predicate CoordChar(c: char) { c=='-' || '0'<=c<='9' }
  function Accumulate(s: string,n: nat): nat
    requires forall c <- s :: CoordChar(c)
    decreases |s| {
    if |s|==0 then n else
      var next: int := if s[0]=='-' then n else 10*n+(s[0] as int)-48;
      assert CoordChar(s[0]);
      Accumulate(s[1..],next)
  }
  function Coord(s: string): int
    requires forall c <- s :: CoordChar(c)
  { (if '-' in s then -1 else 1)*Accumulate(s,0) }
  datatype Placement = Placement(a: int,b: int,c: int,d: int,maximized: bool,minimized: bool,initialized: bool)
  function ReadPlacement(s: string,main: bool): Placement {
    if |s|!=17 || !(s[0] in "MRI") || !(forall c <- s[1..] :: CoordChar(c)) then
      Placement(-32768,0,-32768,0,main,false,false)
    else Placement(Coord(s[1..5]),Coord(s[5..9]),Coord(s[9..13]),Coord(s[13..17]),s[0]=='M',s[0]=='I',true)
  }
  // WriteIniSettings changes only byte zero of the prior MineralScale string.
  function WaitText(mineralText: string,activeTimer: bool): string
    requires |mineralText|>0
  { [if activeTimer then '1' else '0']+mineralText[1..] }
  function U16(n: int): nat { n%65536 }
  function BitFlag(n: int): bool { n%2==1 }
  function Nonzero16(n: int): bool { U16(n)!=0 }
  function Zoom(n: int,prior: int): int { if 1<=U16(n)<=9 then U16(n)-5 else prior }
  function Radar(n: int): nat { if U16(n)<=100 then U16(n) else 100 }
  function MineralScale(n: int): int { var x := P.ProfileI16(n); if 100<=x<=30000 then x else 5000 }
  function Graph(n: int): nat { if n%16<=7 then n%16 else 7 }
  function InvalidScan(n: int): bool { var w := U16(n); w%16+(w/16384)*16384>=6 }
  function Scan(n: int): nat { if InvalidScan(n) then 0 else U16(n) }
  function ShipFilter(mode: int,filter: int): nat { if InvalidScan(mode) then 0 else U16(filter) }
  function Installed(current: nat,stored: int): nat { if current<U16(stored) then current else U16(stored) }
  function TrialOver(current: nat,stored: int): bool { current>=(Installed(current,stored)+21)%65536 }
  lemma PrefixPreserved(format: string,tiles: seq<Tile>,prefix: nat,column: nat)
    requires prefix<=|tiles| && column<=1
    ensures ReadTiles(format,tiles,prefix,column)[..prefix]==tiles[..prefix]
    decreases |format|
  {
    if |format|>0 && format[0]!='\0' {
      if format[0]=='*' { PrefixPreserved(format[1..],tiles,prefix,1); }
      else if !('a'<=format[0]<='z' || 'A'<=format[0]<='Z') { PrefixPreserved(format[1..],tiles,prefix,column); }
      else {
        var popped := 'A'<=format[0]<='Z';
        var id := (format[0] as int)-(if popped then 65 else 97);
        var index := FindTile(tiles,prefix,id);
        if index==|tiles| { PrefixPreserved(format[1..],tiles,prefix,column); }
        else { PrefixPreserved(format[1..],Install(tiles,prefix,index,column,popped),prefix+1,column); }
      }
    }
  }
  lemma {:isolate_assertions} {:fuel ReadTiles,8} {:fuel FindTile,8} {:fuel WriteTiles,8} {:fuel Accumulate,8} LayoutVectors() {
    var a := Tile(0,0,false,10); var b := Tile(1,0,false,20); var c := Tile(2,0,false,30);
    assert ReadTiles("C",[a,b,c],0,0)==[c.(popped:=true),b,a];
    assert ReadTiles("Cc",[a,b,c],0,0)==ReadTiles("C",[a,b,c],0,0);
    assert ReadTiles("*?*b",[a,b,c],0,0)==[b.(column:=1),a.(column:=1),c.(column:=1)];
    assert WriteTiles([a,c.(column:=1,popped:=true)],0)=="a*C";
    assert ReadPlacement("X",true)==Placement(-32768,0,-32768,0,true,false,false);
    assert ReadPlacement("bad",false).initialized==false;
    assert Coord("1-2-")==-12;
    assert ReadPlacement("R1-2-000000010002",false)==Placement(-12,0,1,2,false,false,true);
    assert WaitText("5000",true)=="1000" && WaitText("5000",false)=="0000";
    assert !BitFlag(2) && Nonzero16(2);
    assert Graph(16)==0 && Graph(15)==7 && Radar(-1)==100;
    assert Scan(224)==224 && Scan(6)==0 && ShipFilter(16384,7)==0;
  }
}
