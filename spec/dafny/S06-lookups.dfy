// util.c LpplFromId / LpflFromId. Entries project native id and iPlayer.
// Located is an array index, not a new object identity. NullAccess marks a
// native dereference outside the defined domain; C does not return that value.
module S06Lookups {
  datatype Entry = Entry(id: int,owner: int)
  datatype Slot = Hole | Present(value: Entry)
  datatype Result = Absent | Located(index: nat) | NullAccess
  function Binary(rows: seq<Slot>,id: int,lo: int,hi: nat): Result
    requires -1<=lo && hi<=|rows|
    ensures Binary(rows,id,lo,hi).Located? ==>
      Binary(rows,id,lo,hi).index<|rows| && rows[Binary(rows,id,lo,hi).index].Present? &&
      rows[Binary(rows,id,lo,hi).index].value.id==id
    decreases hi-lo
  {
    if lo+1>=hi then Absent else
    var mid := (lo+hi)/2;
    if rows[mid].Hole? then NullAccess
    else if rows[mid].value.id<id then Binary(rows,id,mid,hi)
    else if rows[mid].value.id==id then Located(mid) else Binary(rows,id,lo,mid)
  }
  function Planet(rows: seq<Entry>,maximum: nat,id: int): Result
    ensures Planet(rows,maximum,id).Located? ==> Planet(rows,maximum,id).index<|rows|
    ensures !Planet(rows,maximum,id).NullAccess?
  {
    if id<0 || id>=maximum then Absent
    else if |rows|==maximum then Located(id)
    else
      var slots := seq(|rows|,i requires 0<=i<|rows| => Present(rows[i]));
      BinaryWithoutHoles(slots,id,-1,|rows|);
      Binary(slots,id,-1,|rows|)
  }
  function CountBefore(counts: seq<nat>,player: nat): nat
    requires player<=|counts|
    decreases player
  { if player==0 then 0 else CountBefore(counts,player-1)+counts[player-1] }
  function Fleet(rows: seq<Slot>,counts: seq<nat>,word: int): Result
    requires |counts|==16 && CountBefore(counts,16)<=32767
    ensures Fleet(rows,counts,word).Located? ==> Fleet(rows,counts,word).index<|rows|
  {
    var packed := word%8192;
    var player := packed/512;
    Binary(rows,packed,CountBefore(counts,player)-1,|rows|)
  }
  lemma BinaryWithoutHoles(rows: seq<Slot>,id: int,lo: int,hi: nat)
    requires -1<=lo && hi<=|rows|
    requires forall i | 0<=i<|rows| :: rows[i].Present?
    ensures !Binary(rows,id,lo,hi).NullAccess?
    decreases hi-lo
  {
    if lo+1<hi {
      var mid := (lo+hi)/2;
      if rows[mid].value.id<id { BinaryWithoutHoles(rows,id,mid,hi); }
      else if rows[mid].value.id>id { BinaryWithoutHoles(rows,id,lo,mid); }
    }
  }
  lemma {:isolate_assertions} {:fuel CountBefore,18} LookupVectors() {
    var planets := [Entry(2,1),Entry(5,0)];
    assert Planet(planets,9,5)==Located(1);
    assert Planet(planets,9,1)==Absent;
    assert Planet(planets,2,1)==Located(1);
    assert Planet(planets,2,2)==Absent;
    assert Binary([Hole],0,-1,1)==NullAccess;
    var counts := [1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0];
    assert CountBefore(counts,1)==1;
    assert CountBefore(counts,16)==2;
    assert Binary([Present(Entry(0,0)),Present(Entry(512,1))],512,0,2)==Located(1);
    assert Fleet([Present(Entry(0,0)),Present(Entry(512,1))],counts,8704)==Located(1);
  }
}
