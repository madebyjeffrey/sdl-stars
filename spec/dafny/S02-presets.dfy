// ReadIniSettings / WriteIniSettings. Storage codecs only; application of
// transport actions and production queues belongs to M10 and M09.
module S02Presets {
  type Nibble = n: int | 0<=n<16
  type Word = n: int | 0<=n<65536
  function Letter(n: Nibble): char { (97+n) as char }
  predicate Alphabet(s: string) { forall i | 0<=i<|s| :: 'a'<=s[i]<='p' }
  function Value(c: char): Nibble requires 'a'<=c<='p' { (c as int)-97 }
  function Encode(w: Word): string
    ensures |Encode(w)|==4 && Alphabet(Encode(w))
  { [Letter(w%16),Letter((w/16)%16),Letter((w/256)%16),Letter(w/4096)] }
  function Decode(s: string): Word
    requires |s|==4 && Alphabet(s)
  { Value(s[0])+16*Value(s[1])+256*Value(s[2])+4096*Value(s[3]) }
  lemma WordRoundTrip(w: Word) ensures Decode(Encode(w))==w {
    assert w==w%16+16*(w/16);
    assert w/16==(w/16)%16+16*(w/256);
    assert w/256==(w/256)%16+16*(w/4096);
  }
  datatype Action = Action(action: Nibble,quantity: int)
  predicate ValidAction(a: Action) { 0<=a.quantity<4096 }
  function ActionText(a: Action): string
    requires ValidAction(a)
    ensures |ActionText(a)|==4 && Alphabet(ActionText(a))
  { [Letter(a.action),Letter(a.quantity%16),Letter((a.quantity/16)%16),Letter(a.quantity/256)] }
  function ReadAction(s: string): Action
    requires |s|==4 && Alphabet(s)
    ensures ValidAction(ReadAction(s))
  { Action(Value(s[0]),Value(s[1])+16*Value(s[2])+256*Value(s[3])) }
  lemma ActionRoundTrip(a: Action)
    requires ValidAction(a)
    ensures ReadAction(ActionText(a))==a
  {
    assert a.quantity==a.quantity%16+16*(a.quantity/16);
    assert a.quantity/16==(a.quantity/16)%16+16*(a.quantity/256);
  }
  function ReadActions(s: string): seq<Action>
    requires |s|%4==0 && Alphabet(s)
    ensures |ReadActions(s)|==|s|/4
    decreases |s|
  { if |s|==0 then [] else [ReadAction(s[..4])]+ReadActions(s[4..]) }
  function ActionTexts(rows: seq<Action>): string
    requires forall a <- rows :: ValidAction(a)
    ensures |ActionTexts(rows)|==4*|rows| && Alphabet(ActionTexts(rows))
    decreases |rows|
  { if |rows|==0 then "" else ActionText(rows[0])+ActionTexts(rows[1..]) }
  lemma ActionsRoundTrip(rows: seq<Action>)
    requires forall a <- rows :: ValidAction(a)
    ensures ReadActions(ActionTexts(rows))==rows
    decreases |rows|
  {
    if |rows|>0 {
      ActionRoundTrip(rows[0]); ActionsRoundTrip(rows[1..]);
      assert ActionTexts(rows)[..4]==ActionText(rows[0]);
      assert ActionTexts(rows)[4..]==ActionTexts(rows[1..]);
      assert rows==[rows[0]]+rows[1..];
    }
  }
  datatype Transport = Transport(valid: bool,name: string,actions: seq<Action>)
  function ReadTransport(s: string): Transport {
    if 20<=|s|<=32 && Alphabet(s[..20]) then Transport(true,s[20..],ReadActions(s[..20]))
    else Transport(false,"",seq(5,i=>Action(0,0)))
  }
  function WriteTransport(t: Transport): string
    requires |t.actions|==5 && (forall a <- t.actions :: ValidAction(a)) && |t.name|<=12
  { if t.valid then ActionTexts(t.actions)+t.name else "" }
  lemma TransportRoundTrip(t: Transport)
    requires t.valid && |t.actions|==5 && (forall a <- t.actions :: ValidAction(a)) && |t.name|<=12
    ensures ReadTransport(WriteTransport(t))==t
  {
    ActionsRoundTrip(t.actions);
    assert WriteTransport(t)[..20]==ActionTexts(t.actions);
    assert WriteTransport(t)[20..]==t.name;
  }
  // PRODQ1: low six bits mdIdle, high ten bits cQuan.
  function Normalize(w: Word): Word
    ensures Normalize(w)%64<7 && Normalize(w)/64<=1020
  { (if w%64>=7 then 0 else w%64)+64*(if w/64>1020 then 1 else w/64) }
  function ReadRows(s: string): seq<Word>
    requires |s|%4==0 && Alphabet(s)
    ensures |ReadRows(s)|==|s|/4
    decreases |s|
  { if |s|==0 then [] else [Normalize(Decode(s[..4]))]+ReadRows(s[4..]) }
  function RowTexts(rows: seq<Word>): string
    ensures |RowTexts(rows)|==4*|rows| && Alphabet(RowTexts(rows))
    decreases |rows|
  { if |rows|==0 then "" else Encode(rows[0])+RowTexts(rows[1..]) }
  function NormalizedRows(rows: seq<Word>): seq<Word>
    ensures |NormalizedRows(rows)|==|rows|
    decreases |rows|
  { if |rows|==0 then [] else [Normalize(rows[0])]+NormalizedRows(rows[1..]) }
  lemma RowsRoundTrip(rows: seq<Word>)
    ensures ReadRows(RowTexts(rows))==NormalizedRows(rows)
    decreases |rows|
  {
    if |rows|>0 {
      WordRoundTrip(rows[0]); RowsRoundTrip(rows[1..]);
      assert RowTexts(rows)[..4]==Encode(rows[0]);
      assert RowTexts(rows)[4..]==RowTexts(rows[1..]);
    }
  }
  datatype Production = Production(valid: bool,name: string,noResearch: bool,rows: seq<Word>)
  function ReadProduction(s: string): Production {
    if |s|<3 || |s|>64 then Production(false,"",false,[]) else
    var count := (s[1] as int)-97;
    var end := 2+4*count;
    if 0<=count<=12 && end<=|s| && Alphabet(s[..end]) && |s|-end<=12
    then Production(true,s[end..],s[0]!='a',ReadRows(s[2..end]))
    else Production(false,"",false,[])
  }
  function WriteProduction(p: Production): string
    requires |p.rows|<=12 && |p.name|<=12
  { if p.valid then [if p.noResearch then 'b' else 'a',Letter(|p.rows|)]+RowTexts(p.rows)+p.name else "" }
  function LoadProduction(s: string,first: bool,defaultName: string): Production {
    var p := ReadProduction(s);
    if first then p.(valid:=true,name:=defaultName) else p
  }
  function WriteAttempts(transportDirty: bool,productionDirty: bool): nat {
    (if transportDirty then 4 else 0)+(if productionDirty then 5 else 0)
  }
  lemma ProductionRoundTrip(p: Production)
    requires p.valid && |p.rows|<=12 && |p.name|<=12
    requires 2+4*|p.rows|+|p.name|>=3
    ensures ReadProduction(WriteProduction(p))==Production(true,p.name,p.noResearch,NormalizedRows(p.rows))
  {
    RowsRoundTrip(p.rows);
    var text := WriteProduction(p);
    var end := 2+4*|p.rows|;
    assert text[1]==Letter(|p.rows|);
    assert (text[1] as int)-97==|p.rows|;
    assert text[2..end]==RowTexts(p.rows);
    assert text[end..]==p.name;
    assert text[..end]==[if p.noResearch then 'b' else 'a',Letter(|p.rows|)]+RowTexts(p.rows);
    assert Alphabet(text[..end]);
  }
  lemma NormalizeIdempotent(w: Word) ensures Normalize(Normalize(w))==Normalize(w) {}
  lemma {:isolate_assertions} PresetVectors()
  {
    assert ReadAction("pbad")==Action(15,769);
    assert Decode("pppp")==65535 && Normalize(65535)==64;
    assert Normalize(65343)==65280; // item 63 -> mine; quantity 1020 preserved
    assert ReadProduction("aax")==Production(true,"x",false,[]);
    assert ReadProduction("aa")==Production(false,"",false,[]);
    assert ReadProduction("abppppx")==Production(true,"x",false,[64]);
    assert ReadProduction("pbbaaaN")==Production(true,"N",true,[1]);
    assert LoadProduction("!",true,"Default")==Production(true,"Default",false,[]);
    assert LoadProduction("pbbaaaN",true,"Default")==Production(true,"Default",true,[1]);
    assert WriteProduction(Production(true,"",false,[]))=="aa";
    assert WriteTransport(Transport(false,"",seq(5,i=>Action(0,0))))=="";
  }
}
