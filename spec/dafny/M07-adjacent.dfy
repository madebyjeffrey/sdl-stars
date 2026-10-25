// Construction-capable starbase navigation. Native catalog/pointer extraction
// is an explicit adapter; iteration order is the actual lpPlanets order.
module M07Adjacent {
  datatype Base = Base(id: int,owned: bool,present: bool,capacity: nat)
  predicate Eligible(b: Base) { b.owned && b.present && b.capacity!=0 }
  datatype Seen = Seen(first: int,last: int,after: int,before: int)
  function Scan(rows: seq<Base>,current: int,s: Seen): Seen
    decreases |rows|
  {
    if |rows|==0 then s else
    var b:=rows[0];
    var next:=if !Eligible(b) then s else Seen(
      if s.first==-1 then b.id else s.first,b.id,
      if b.id>current && s.after==-1 then b.id else s.after,
      if b.id<current then b.id else s.before);
    Scan(rows[1..],current,next)
  }
  function Target(rows: seq<Base>,current: int,next: bool): int {
    var s:=Scan(rows,current,Seen(-1,-1,-1,-1));
    if next then (if s.after==-1 then s.first else s.after)
    else (if s.before==-1 then s.last else s.before)
  }
  lemma NoEligible(rows: seq<Base>,current: int,s: Seen)
    requires forall r <- rows :: !Eligible(r)
    ensures Scan(rows,current,s)==s
    decreases |rows|
  { if |rows|>0 { NoEligible(rows[1..],current,s); } }
  lemma {:isolate_assertions} Vectors() {
    var rows: seq<Base> := [Base(9,true,true,1),Base(2,true,true,1),Base(7,true,true,1)];
    assert Target(rows,5,true)==9; // first qualifying greater ID, not minimum
    assert Target(rows,8,false)==7; // last qualifying smaller ID, not maximum
    assert Target(rows,10,true)==9 && Target(rows,0,false)==7;
    assert Target([Base(2,true,true,0),Base(3,false,true,10)],1,true)==-1;
    assert Target([Base(4,true,true,1)],4,true)==4;
    assert Target([Base(4,true,true,1)],4,false)==4;
  }
}
