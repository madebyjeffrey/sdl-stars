include "S04-arithmetic.dfy"
module S05Desktop {
  import opened S04Arithmetic
  datatype Message = Message(window: int,code: nat,keys: nat,position: int,input: bool)
  function Motion(queue: seq<Message>,next: Message,fMain: bool): seq<Message>
    requires next.input
  {
    if !fMain then queue else if fMain && next.code==512 && |queue|>0 && queue[|queue|-1].input &&
      queue[|queue|-1].window==next.window && queue[|queue|-1].code==next.code && queue[|queue|-1].keys==next.keys
    then queue[..|queue|-1]+[next] else queue+[next]
  }
  predicate Matches(code: nat,fWindowMatches: bool,lo: nat,hi: nat) {
    code==18 || (fWindowMatches && ((lo==0 && hi==0) || lo<=code<=hi))
  }
  function TimerPeriod(ms: nat): int { Clamp(ms,10,2147483647) }
  predicate Due(now: nat,due: nat,queued: bool,filter: bool) { !queued && filter && now>=due }
  predicate CanPost(main: bool,hasWindow: bool,live: bool) { main && (!hasWindow || live) }
  lemma MotionSize(q: seq<Message>,n: Message,m: bool)
    requires n.input
    ensures |q|<=|Motion(q,n,m)|<=|q|+1
  {}
  lemma {:isolate_assertions} AcceptanceVectors()
    ensures Matches(18,false,100,200) && !Matches(512,false,0,0)
    ensures Motion([],Message(1,512,0,2,true),false)==[]
    ensures TimerPeriod(0)==10 && TimerPeriod(3000000000)==2147483647
    ensures !Due(100,10,true,true) && Due(100,10,false,true)
    ensures Motion([Message(1,512,0,1,true)],Message(1,512,0,2,true),true)==[Message(1,512,0,2,true)]
    ensures |Motion([Message(1,512,0,1,true)],Message(1,512,1,2,true),true)|==2
  {}
}
