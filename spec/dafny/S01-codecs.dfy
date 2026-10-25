include "../S01-game-file-contracts.dfy"

// Source: utilgen.c, race.c, save.c. Pure byte-level S01 definitions.
module S01Codecs {
  import opened S01
  type Nibble = n: int | 0 <= n < 16
  datatype MaybeBytes = BadBytes | BytesValue(rgb: Bytes)
  datatype TextRead = BadText | TextValue(rgbText: Bytes, cbUsed: nat)
  datatype XorState = XorState(s1: int, s2: int)
  datatype XorResult = XorResult(rgb: Bytes, state: XorState)

  function Pow2(c: nat): nat
    ensures Pow2(c) > 0
    decreases c
  { if c == 0 then 1 else 2 * Pow2(c - 1) }
  function Bits(n: int, ibit: nat, cbit: nat): nat { (n / Pow2(ibit)) % Pow2(cbit) }
  function Signed(n: int, cbit: nat): int
    requires cbit > 0
  { var u := n % Pow2(cbit); if u < Pow2(cbit - 1) then u else u - Pow2(cbit) }
  function DivC(n: int, d: int): int
    requires d > 0
  { if n < 0 then -((-n) / d) else n / d }
  function Byte(n: int): U8 { n % 256 }
  function LE(n: int, cb: nat): Bytes
    ensures |LE(n, cb)| == cb
    decreases cb
  { if cb == 0 then [] else [Byte(n)] + LE(n / 256, cb - 1) }
  function Unsigned(rgb: Bytes): nat
    decreases |rgb|
  { if |rgb| == 0 then 0 else (rgb[0] as int) + 256 * Unsigned(rgb[1..]) }
  function SliceWord(rgb: Bytes, ib: nat, cb: nat): nat
    requires ib + cb <= |rgb|
  { Unsigned(rgb[ib..ib + cb]) }
  function Zeros(cb: nat): Bytes
    ensures |Zeros(cb)| == cb
    decreases cb
  { if cb == 0 then [] else [0] + Zeros(cb - 1) }
  function Xor8(a: U8, b: U8): U8 { ((a as bv8) ^ (b as bv8)) as int }
  function Xor16(a: U16, b: U16): U16 { ((a as bv16) ^ (b as bv16)) as int }
  function Xor32(a: U32, b: U32): U32 { ((a as bv32) ^ (b as bv32)) as int }
  function PopCount(n: nat): nat
    decreases n
  { if n == 0 then 0 else n % 2 + PopCount(n / 2) }
  function Width(code: nat): nat
    requires code < 4
  { if code == 3 then 4 else code }
  function WidthCode(n: nat): nat { if n == 0 then 0 else if n <= 255 then 1 else if n <= 65535 then 2 else 3 }
  function PackedSize(mask: nat, count: nat): nat
    decreases count
  { if count == 0 then 0 else Width(mask % 4) + PackedSize(mask / 4, count - 1) }

  function Primes(): seq<int> {
    [3,5,7,11,13,17,19,23,29,31,37,41,43,47,53,59,
     61,67,71,73,79,83,89,97,101,103,107,109,113,127,131,137,
     139,149,151,157,163,167,173,179,181,191,193,197,199,211,223,227,
     229,233,239,241,251,257,263,279,271,277,281,283,293,307,311,313]
  }
  function NextSeed(s: int, d: int, a: int, r: int, m: int): int
    requires d > 0
  {
    var k := DivC(s, d);
    var t := Signed(a * (s - k * d) - k * r, 32);
    if t < 0 then Signed(t + m, 32) else t
  }
  function Next(state: XorState): XorState {
    XorState(NextSeed(state.s1, 53668, 40014, 12211, 2147483563),
             NextSeed(state.s2, 52774, 40692, 3791, 2147483399))
  }
  function Output(state: XorState): U32 { (state.s1 - state.s2) % 4294967296 }
  function Advance(state: XorState, count: nat): XorState
    decreases count
  { if count == 0 then state else Advance(Next(state), count - 1) }
  function InitXor(lid: U32, salt: nat, turn: U16, iPlayer: int, fCrippled: bool): XorState
    requires salt < 2048
  {
    var a := salt % 32 + (if salt / 1024 == 1 then 32 else 0);
    var b := (salt / 32) % 32 + (if salt / 1024 == 0 then 32 else 0);
    Advance(XorState(Primes()[a], Primes()[b]),
      ((lid as int) % 4 + 1) * ((turn as int) % 4 + 1) * (iPlayer % 4 + 1) +
      (if fCrippled then 1 else 0))
  }
  function XorPrefix(rgb: Bytes, w: U32): Bytes
    ensures |XorPrefix(rgb, w)| == |rgb|
    decreases |rgb|
  {
    if |rgb| == 0 then []
    else [Xor8(rgb[0], Byte(w))] + XorPrefix(rgb[1..], w / 256)
  }
  opaque function Crypt(rgb: Bytes, state: XorState): XorResult
    ensures |Crypt(rgb, state).rgb| == |rgb|
    decreases |rgb|
  {
    if |rgb| == 0 then XorResult([], state)
    else
      var cb := if |rgb| < 4 then |rgb| else 4;
      var next := Next(state);
      var tail := Crypt(rgb[cb..], next);
      XorResult(XorPrefix(rgb[..cb], Output(next)) + tail.rgb, tail.state)
  }
  function Transform(rt: RecordTag, rgb: Bytes, state: XorState): XorResult
    ensures |Transform(rt,rgb,state).rgb|==|rgb|
  {
    if rt in {0, 8} then XorResult(rgb, state) else Crypt(rgb, state)
  }
  // BOF initialization and raw .xy star packs are handled by file sequencing.
  lemma XorInvolution(a: bv8, b: bv8)
    ensures (a ^ b) ^ b == a
  {}
  lemma CryptConsumption(rgb: Bytes, state: XorState)
    ensures Crypt(rgb, state).state == Advance(state, (|rgb| + 3) / 4)
    decreases |rgb|
  {
    reveal Crypt();
    if |rgb| != 0 {
      var cb := if |rgb| < 4 then |rgb| else 4;
      CryptConsumption(rgb[cb..], Next(state));
      assert (|rgb|+3)/4 == 1+(|rgb|-cb+3)/4;
      assert Advance(state,(|rgb|+3)/4) == Advance(Next(state),(|rgb|-cb+3)/4);
    }
  }

  function Nibbles(rgb: Bytes): seq<Nibble>
    decreases |rgb|
  { if |rgb| == 0 then [] else [rgb[0] / 16, rgb[0] % 16] + Nibbles(rgb[1..]) }
  function Letter(n: nat, v: nat): U8
    requires n <= 14 && v < 16
  {
    if n <= 10 then Byte(" aehilnorst"[n] as int)
    else if n == 11 then Byte(65 + v)
    else if n == 12 then Byte(if v < 10 then 81 + v else 48 + v - 10)
    else if n == 13 then Byte(if v < 4 then 54 + v else "bcdfgjkmpquv"[v - 4] as int)
    else Byte("wxyz+-,!.?:;'*%$"[v] as int)
  }
  function DecodeNibbles(rgn: seq<Nibble>): MaybeBytes
    decreases |rgn|
  {
    if |rgn| == 0 || (|rgn| == 1 && rgn[0] == 15) then BytesValue([])
    else
      var cb := if rgn[0] <= 10 then 1 else if rgn[0] == 15 then 3 else 2;
      if |rgn| < cb then BadBytes
      else
        var b := if rgn[0] == 15 then Byte((rgn[1] as int) + 16 * (rgn[2] as int))
                 else Letter(rgn[0], if cb == 1 then 0 else rgn[1]);
        var tail := DecodeNibbles(rgn[cb..]);
        if tail.BadBytes? then BadBytes else BytesValue([b] + tail.rgb)
  }
  function Decompress(rgb: Bytes): MaybeBytes { DecodeNibbles(Nibbles(rgb)) }
  function FirstNul(rgb: Bytes): nat
    ensures FirstNul(rgb) <= |rgb|
    decreases |rgb|
  { if |rgb| == 0 || rgb[0] == 0 then 0 else 1 + FirstNul(rgb[1..]) }
  function ReadText(rgb: Bytes, cbMax: nat, fUtf8: bool): TextRead {
    if |rgb| == 0 then BadText
    else if rgb[0] == 0 then
      var cb := FirstNul(rgb[1..]);
      if cb == |rgb| - 1 || cb > cbMax || (fUtf8 && !FUtf8(rgb[1..1 + cb])) then BadText
      else TextValue(rgb[1..1 + cb], 2 + cb)
    else if 1 + (rgb[0] as int) > |rgb| then BadText
    else
      var text := Decompress(rgb[1..1 + (rgb[0] as int)]);
      if text.BadBytes? then BadText
      else if |text.rgb| > cbMax || 0 in text.rgb || (fUtf8 && !FUtf8(text.rgb)) then BadText
      else TextValue(text.rgb, 1 + (rgb[0] as int))
  }
  predicate FExactName(rgb: Bytes) {
    ReadText(rgb, 31, true).TextValue? && ReadText(rgb, 31, true).cbUsed == |rgb|
  }
  function RawText(rgb: Bytes): Bytes { [0] + rgb + [0] }

  function CleanName(rgb: Bytes): Bytes
    ensures |CleanName(rgb)| == |rgb|
  { rgb[..FirstNul(rgb)] + Zeros(|rgb| - FirstNul(rgb)) }
  function XorWords(rgb: Bytes): U16
    requires |rgb| % 2 == 0
    decreases |rgb|
  { if |rgb| == 0 then 0 else Xor16(Word(rgb, 0), XorWords(rgb[2..])) }
  function RaceChecksum(rgbPlayer: Bytes): U16
    requires |rgbPlayer| == 192
  { XorWords(rgbPlayer[..128] + CleanName(rgbPlayer[128..160]) + CleanName(rgbPlayer[160..192])) }
  function PasswordAcc(rgb: Bytes, acc: U32): U32
    decreases |rgb|
  {
    if |rgb| == 0 then acc
    else
      var sum := ((acc as int) + Signed(rgb[0], 8)) % 4294967296;
      if |rgb| == 1 then sum
      else PasswordAcc(rgb[2..], (sum * Signed(rgb[1], 8)) % 4294967296)
  }
  function Password(rgb: Bytes): U32
    requires 0 !in rgb
  {
    var n := PasswordAcc(rgb, 0);
    if |rgb| != 0 && n == 0 then 1 else n
  }
  function Cp1252(b: U8): nat {
    if b < 128 || b >= 160 then b
    else [8364,129,8218,402,8222,8230,8224,8225,710,8240,352,8249,338,141,381,143,
          144,8216,8217,8220,8221,8226,8211,8212,732,8482,353,8250,339,157,382,376][b - 128]
  }
  function Utf8Scalar(n: nat): Bytes
    requires n <= 1114111 && !(55296 <= n <= 57343)
  {
    if n < 128 then [Byte(n)]
    else if n < 2048 then [Byte(192 + n / 64), Byte(128 + n % 64)]
    else if n < 65536 then [Byte(224 + n / 4096), Byte(128 + (n / 64) % 64), Byte(128 + n % 64)]
    else [Byte(240 + n / 262144), Byte(128 + (n / 4096) % 64), Byte(128 + (n / 64) % 64), Byte(128 + n % 64)]
  }
  function LegacyText(rgb: Bytes): Bytes
    decreases |rgb|
  { if |rgb| == 0 then [] else Utf8Scalar(Cp1252(rgb[0])) + LegacyText(rgb[1..]) }
  // Writer selection: preferred table encodings, otherwise low/high literal escape.
  function ByteNibbles(b: U8): seq<Nibble> {
    if b==32 then [0]
    else if b==97 then [1] else if b==101 then [2] else if b==104 then [3]
    else if b==105 then [4] else if b==108 then [5] else if b==110 then [6]
    else if b==111 then [7] else if b==114 then [8] else if b==115 then [9] else if b==116 then [10]
    else if 65<=b<=80 then [11,(b as int)-65]
    else if 81<=b<=90 then [12,(b as int)-81]
    else if 48<=b<=53 then [12,(b as int)-48+10]
    else if 54<=b<=57 then [13,(b as int)-54]
    else if b==98 then [13,4] else if b==99 then [13,5] else if b==100 then [13,6]
    else if b==102 then [13,7] else if b==103 then [13,8] else if b==106 then [13,9]
    else if b==107 then [13,10] else if b==109 then [13,11] else if b==112 then [13,12]
    else if b==113 then [13,13] else if b==117 then [13,14] else if b==118 then [13,15]
    else if 119<=b<=122 then [14,(b as int)-119]
    else if b==43 then [14,4] else if b==45 then [14,5] else if b==44 then [14,6]
    else if b==33 then [14,7] else if b==46 then [14,8] else if b==63 then [14,9]
    else if b==58 then [14,10] else if b==59 then [14,11] else if b==39 then [14,12]
    else if b==42 then [14,13] else if b==37 then [14,14] else if b==36 then [14,15]
    else [15,b%16,b/16]
  }
  function EncodeNibbles(rgb: Bytes): seq<Nibble>
    decreases |rgb|
  { if |rgb|==0 then [] else ByteNibbles(rgb[0])+EncodeNibbles(rgb[1..]) }
  function PackNibbles(n: seq<Nibble>): Bytes
    decreases |n|
  {
    if |n|==0 then [] else if |n|==1 then [Byte(16*(n[0] as int)+15)]
    else [Byte(16*(n[0] as int)+(n[1] as int))]+PackNibbles(n[2..])
  }
  function WriteText(rgb: Bytes): Bytes
    requires FName(rgb)
  {
    var compressed:=PackNibbles(EncodeNibbles(rgb));
    if |compressed|==0 || |compressed|>31 then RawText(rgb)
    else [Byte(|compressed|)]+compressed
  }
  datatype LegacyPolicy = RootPolicy | ObservedReplacement
  predicate FLegacyConversion(rawName: Bytes,rawNames: Bytes,fSingle: bool,
    name: Bytes,names: Bytes,fNameConsent: bool,fProtected: bool,policy: LegacyPolicy,
    password: Bytes,fPasswordConsent: bool) {
    var fullName:=LegacyText(rawName);
    var fullNames:=LegacyText(if fSingle then rawName+[115] else rawNames);
    FTextPrefix(fullName,name,31) && FTextPrefix(fullNames,names,31) &&
    ((|fullName|>31 || |fullNames|>31) ==> fNameConsent) &&
    (fProtected ==> policy==ObservedReplacement && fPasswordConsent && |password|<=15 && FUtf8(password))
  }
  lemma ReadCompressed(b: Bytes, text: Bytes)
    requires 0 < |b| < 256 && Decompress(b)==BytesValue(text)
    requires |text|<=31 && FUtf8(text) && 0 !in text
    ensures ReadText([Byte(|b|)]+b,31,true)==TextValue(text,1+|b|)
  {
    assert ([Byte(|b|)]+b)[1..1+|b|]==b;
  }
  lemma {:isolate_assertions} CodecVectors()
    ensures ReadText([1,176],31,true) == TextValue([65],2)
    ensures ReadText([1,31],31,true) == TextValue([97],2)
    ensures Decompress([243,207,154]) == BytesValue([195,169])
    ensures Password([97,98,99]) == 9605
    ensures LegacyText([128]) == [226,130,172]
    ensures WidthCode(255) == 1 && WidthCode(256) == 2 && WidthCode(65536) == 3
  {
    assert Nibbles([176]) == [11,0];
    assert Nibbles([31]) == [1,15];
    assert Nibbles([154]) == [9,10];
    assert Nibbles([207,154]) == [12,15,9,10];
    assert Nibbles([243,207,154]) == [15,3,12,15,9,10];
    assert DecodeNibbles([11,0]) == BytesValue([65]);
    assert DecodeNibbles([15]) == BytesValue([]);
    assert DecodeNibbles([1,15]) == BytesValue([97]);
    assert DecodeNibbles([15,9,10]) == BytesValue([169]);
    assert DecodeNibbles([15,3,12,15,9,10]) == BytesValue([195,169]);
    assert Decompress([176]) == BytesValue([65]);
    assert Decompress([31]) == BytesValue([97]);
    assert FUtf8([65]) && FUtf8([97]);
    ReadCompressed([176],[65]);
    ReadCompressed([31],[97]);
    var first: Bytes := [176];
    var second: Bytes := [31];
    ReadCompressed(first,[65]);
    ReadCompressed(second,[97]);
    assert [Byte(1)]+first == [1,176];
    assert [Byte(1)]+second == [1,31];
    calc {
      ReadText([1,176],31,true);
      ReadText([Byte(1)]+first,31,true);
      TextValue([65],2);
    }
    calc {
      ReadText([1,31],31,true);
      ReadText([Byte(1)]+second,31,true);
      TextValue([97],2);
    }
  }
}
