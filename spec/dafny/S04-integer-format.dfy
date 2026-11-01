include "S04-utf8-prefix.dfy"

// Common, untruncated %d/%% subset of CchSprintf and SDL wsprintfA.
// Unsupported is a model boundary, not either C function's return value.
module S04IntegerFormat {
  import S = S01
  import U = S04Utf8Prefix
  function Digits(n: nat): S.Bytes
    ensures |Digits(n)|>=1
    decreases n
  { if n<10 then [(48+n) as S.U8] else Digits(n/10)+[(48+n%10) as S.U8] }
  function Decimal(n: int): S.Bytes {
    if n<0 then [45]+Digits(-n) else Digits(n)
  }
  datatype Result = Formatted(text: S.Bytes,used: nat) | MissingArgument | Unsupported
  function Prepend(prefix: S.Bytes,r: Result): Result {
    if r.Formatted? then Formatted(prefix+r.text,r.used) else r
  }
  function Scan(format: S.Bytes,n: int,available: bool): Result
    requires 0 !in format
    ensures Scan(format,n,available).Formatted? ==>
      Scan(format,n,available).used<=(if available then 1 else 0)
    decreases |format|
  {
    if |format|==0 then Formatted([],0) else
    if format[0]!=37 then Prepend([format[0]],Scan(format[1..],n,available)) else
    if |format|<2 then Unsupported else
    if format[1]==37 then Prepend([37],Scan(format[2..],n,available)) else
    if format[1]!=100 then Unsupported else
    if !available then MissingArgument else
    var tail:=Scan(format[2..],n,false);
    if tail.Formatted? then Formatted(Decimal(n)+tail.text,1) else tail
  }
  function Format(format: S.Bytes,n: int): Result
    requires 0 !in format && -2147483648<=n<=2147483647
  { Scan(format,n,true) }
  predicate Fits(r: Result,capacity: nat) {
    r.Formatted? && |r.text|<capacity && |r.text|<1024
  }
  predicate CallDomain(format: S.Bytes,n: int,capacity: nat)
    requires 0 !in format && -2147483648<=n<=2147483647
  { |format|<256 && Fits(Format(format,n),capacity) }
  lemma DecimalNotEmpty(n: int)
    ensures |Decimal(n)|>0
  {}
  lemma {:fuel Digits,6} DigitsFive(n: nat)
    requires n<100000
    ensures |Digits(n)|<=5
  {}
  lemma DecimalI16(n: S.I16)
    ensures |Decimal(n)|<=6
  { DigitsFive(if n<0 then -(n as int) else (n as int)); }
  lemma WideDecimalVectors()
    ensures Decimal(32768)==[51,50,55,54,56]
    ensures Decimal(-32768)==[45,51,50,55,54,56]
    ensures Decimal(-32767)==[45,51,50,55,54,55]
  {
    assert Digits(3)==[51];
    assert Digits(32)==[51,50];
    assert Digits(327)==[51,50,55];
    assert Digits(3276)==[51,50,55,54];
    assert Digits(32768)==[51,50,55,54,56];
    assert Digits(32767)==[51,50,55,54,55];
  }
  lemma NoPlaceholderConsumesNothing(format: S.Bytes,n: int)
    requires 0 !in format && 37 !in format
    ensures Scan(format,n,true)==Formatted(format,0)
    decreases |format|
  { if |format|>0 { NoPlaceholderConsumesNothing(format[1..],n); } }
  lemma Plain(format: S.Bytes,n: int,available: bool)
    requires 0 !in format && 37 !in format
    ensures Scan(format,n,available)==Formatted(format,0)
    decreases |format|
  { if |format|>0 { Plain(format[1..],n,available); } }
  lemma {:isolate_assertions} PlainPrefix(prefix: S.Bytes,tail: S.Bytes,n: int,available: bool)
    requires 0 !in prefix && 37 !in prefix && 0 !in tail
    ensures Scan(prefix+tail,n,available)==Prepend(prefix,Scan(tail,n,available))
    decreases |prefix|
  {
    if |prefix|>0 {
      PlainPrefix(prefix[1..],tail,n,available);
      assert (prefix+tail)[0]==prefix[0] && prefix[0]!=37;
      assert (prefix+tail)[1..]==prefix[1..]+tail;
      assert Scan(prefix+tail,n,available)==Prepend([prefix[0]],Scan(prefix[1..]+tail,n,available));
      assert prefix==[prefix[0]]+prefix[1..];
      PrependAssociative([prefix[0]],prefix[1..],Scan(tail,n,available));
      assert Prepend([prefix[0]],Prepend(prefix[1..],Scan(tail,n,available)))==Prepend(prefix,Scan(tail,n,available));
    } else {
      assert prefix==[] && prefix+tail==tail;
      assert Prepend([],Scan(tail,n,available))==Scan(tail,n,available);
    }
  }
  lemma PrependAssociative(a: S.Bytes,b: S.Bytes,r: Result)
    ensures Prepend(a,Prepend(b,r))==Prepend(a+b,r)
  {}
  lemma DecimalTemplate(prefix: S.Bytes,suffix: S.Bytes,n: int)
    requires 0 !in prefix && 37 !in prefix && 0 !in suffix && 37 !in suffix
    requires -2147483648<=n<=2147483647
    ensures Format(prefix+[37,100]+suffix,n)==Formatted(prefix+Decimal(n)+suffix,1)
  {
    Plain(suffix,n,false);
    assert Scan([37,100]+suffix,n,true)==Formatted(Decimal(n)+suffix,1);
    PlainPrefix(prefix,[37,100]+suffix,n,true);
    assert prefix+[37,100]+suffix==prefix+([37,100]+suffix);
  }
  lemma {:isolate_assertions} FormatVectors() {
    assert Decimal(-32768)==[45,51,50,55,54,56];
    assert Decimal(32768)==[51,50,55,54,56];
    assert Decimal(0)==[48];
    assert Format([37,100,37,37],-1)==Formatted([45,49,37],1);
    assert Format([37,100,37,100],1)==MissingArgument;
    assert Format([37,115],1)==Unsupported;
    assert Fits(Formatted([65],0),2) && !Fits(Formatted([65],0),1);
  }
}
