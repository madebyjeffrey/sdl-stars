include "S01-external.dfy"

// Concrete accepted/rejected values exercise definitions independently of C tests.
module S01Witnesses {
  import opened S01
  import opened S01Codecs
  import opened S01Records
  import opened S01Orders
  import opened S01Files
  import opened S01External

  lemma PlayerVector() {
    var emptyName: Bytes := [0,0];
    assert ReadText(emptyName,31,true)==TextValue([],2);
    var names: Bytes := [0,0,0,0];
    assert ReadText(names,31,true)==TextValue([],2);
    assert FNames(names,false,true);
    var player: Bytes := [0,0,0,0,0,0,0,0,0,0,0,0];
    assert FPlayer(player,false,false);
    assert !FPlayer(player[..7],false,false);
  }
  lemma PlanetVector() {
    var planet: Bytes := [0,0,0,0];
    assert FPlanet(planet,14,false);
    assert !FPlanet(planet,14,true);
  }
  lemma DesignPlanVector() {
    assert ReadText([0,0],31,true)==TextValue([],2);
    assert FExactName([0,0]);
    var design: Bytes := [0,0,0,0,0,0,0,0];
    assert FDesign(design);
    assert !FDesign(design[..6]);
    assert FPlan([0,0,0,0,0,0],false);
    assert FWaypoint(design,20);
    assert !FWaypoint(design,19);
  }
  lemma AiVector() {
    assert FAiHistory([2,0],EmptyHistory,0);
    assert FAiHistory([4,0,0,0],BaseHistory,0);
    assert FAiHistory([4,0,0,0],CyberHistory,1);
    assert !FAiHistory([4,0,0,0],CyberHistory,2);
  }
  lemma MessageVector() {
    var msg: Bytes := [0,0,0,0,0,0,0,0,0,0,0,0];
    var zeros4: Bytes := [0,0,0,0];
    assert Unsigned([0,0])==0;
    assert Unsigned([0,0,0])==0;
    assert Unsigned(zeros4)==0;
    assert msg[..4]==zeros4;
    assert SliceWord(msg,0,4)==0;
    assert D(msg,0)==0;
    assert W(msg,10)==0;
    assert Pow2(8)==256;
    assert Pow2(12)==4096;
    assert Pow2(15)==32768;
    assert Pow2(16)==65536;
    assert Signed(0,16)==0;
    assert Decompress([])==BytesValue([]);
    assert FUtf8([]);
    assert FPlayerMessage(msg,true);
  }
  lemma BattleVector() {
    var battle: Bytes := [0,0,0,0,0,0,14,0,0,0,0,0,0,0];
    assert Word(battle,6)==14;
    assert W(battle,6)==14;
    assert battle[3]==0;
    assert battle[14..]==[];
    assert EventsEnd([])==0;
    assert FBattle(battle);
    assert !FBattle(battle[..13]);
  }
  lemma QueueEntitlementOnce() {
    var p:=Prod(1,2,2,50);
    assert FSameItem(p,p);
    assert ProgressSource([p],p)==0;
    assert ProgressSource([p.(pct:=0)],p)==1;
    assert QueueProgress([p],[p,p])==[p,p.(pct:=0)];
  }
  lemma CrcByte0()
    ensures CrcBits(4294967295 ^ (49 as bv32),8)==2082672712
  {
    reveal CrcBits();
    assert CrcBits(4294967246,8)==CrcBits(2147483623,7);
    assert CrcBits(2147483623,7)==CrcBits(3527900371,6);
    assert CrcBits(3527900371,6)==CrcBits(2224766281,5);
    assert CrcBits(2224766281,5)==CrcBits(2952076676,4);
    assert CrcBits(2952076676,4)==CrcBits(1476038338,3);
    assert CrcBits(1476038338,3)==CrcBits(738019169,2);
    assert CrcBits(738019169,2)==CrcBits(4165345424,1);
    assert CrcBits(4165345424,1)==CrcBits(2082672712,0);
    assert CrcBits(2082672712,0)==2082672712;
  }
  lemma CrcByte1()
    ensures CrcBits(2082672712 ^ (50 as bv32),8)==2964110130
  {
    reveal CrcBits();
    assert CrcBits(2082672762,8)==CrcBits(1041336381,7);
    assert CrcBits(1041336381,7)==CrcBits(4071638846,6);
    assert CrcBits(4071638846,6)==CrcBits(2035819423,5);
    assert CrcBits(2035819423,5)==CrcBits(3507786479,4);
    assert CrcBits(3507786479,4)==CrcBits(2234698327,3);
    assert CrcBits(2234698327,3)==CrcBits(2938234379,2);
    assert CrcBits(2938234379,2)==CrcBits(3123212325,1);
    assert CrcBits(3123212325,1)==CrcBits(2964110130,0);
    assert CrcBits(2964110130,0)==2964110130;
  }
  lemma CrcByte2()
    ensures CrcBits(2964110130 ^ (51 as bv32),8)==2008521773
  {
    reveal CrcBits();
    assert CrcBits(2964110081,8)==CrcBits(3052330656,7);
    assert CrcBits(3052330656,7)==CrcBits(1526165328,6);
    assert CrcBits(1526165328,6)==CrcBits(763082664,5);
    assert CrcBits(763082664,5)==CrcBits(381541332,4);
    assert CrcBits(381541332,4)==CrcBits(190770666,3);
    assert CrcBits(190770666,3)==CrcBits(95385333,2);
    assert CrcBits(95385333,2)==CrcBits(4017043546,1);
    assert CrcBits(4017043546,1)==CrcBits(2008521773,0);
    assert CrcBits(2008521773,0)==2008521773;
  }
  lemma CrcByte3()
    ensures CrcBits(2008521773 ^ (52 as bv32),8)==1679564636
  {
    reveal CrcBits();
    assert CrcBits(2008521753,8)==CrcBits(3596832044,7);
    assert CrcBits(3596832044,7)==CrcBits(1798416022,6);
    assert CrcBits(1798416022,6)==CrcBits(899208011,5);
    assert CrcBits(899208011,5)==CrcBits(4151634565,4);
    assert CrcBits(4151634565,4)==CrcBits(2516776546,3);
    assert CrcBits(2516776546,3)==CrcBits(1258388273,2);
    assert CrcBits(1258388273,2)==CrcBits(3359129272,1);
    assert CrcBits(3359129272,1)==CrcBits(1679564636,0);
    assert CrcBits(1679564636,0)==1679564636;
  }
  lemma CrcByte4()
    ensures CrcBits(1679564636 ^ (53 as bv32),8)==873121251
  {
    reveal CrcBits();
    assert CrcBits(1679564649,8)==CrcBits(3753282708,7);
    assert CrcBits(3753282708,7)==CrcBits(1876641354,6);
    assert CrcBits(1876641354,6)==CrcBits(938320677,5);
    assert CrcBits(938320677,5)==CrcBits(4132328114,4);
    assert CrcBits(4132328114,4)==CrcBits(2066164057,3);
    assert CrcBits(2066164057,3)==CrcBits(3492485004,2);
    assert CrcBits(3492485004,2)==CrcBits(1746242502,1);
    assert CrcBits(1746242502,1)==CrcBits(873121251,0);
    assert CrcBits(873121251,0)==873121251;
  }
  lemma CrcByte5()
    ensures CrcBits(873121251 ^ (54 as bv32),8)==4136447134
  {
    reveal CrcBits();
    assert CrcBits(873121237,8)==CrcBits(4156416458,7);
    assert CrcBits(4156416458,7)==CrcBits(2078208229,6);
    assert CrcBits(2078208229,6)==CrcBits(3495426898,5);
    assert CrcBits(3495426898,5)==CrcBits(1747713449,4);
    assert CrcBits(1747713449,4)==CrcBits(3652025844,3);
    assert CrcBits(3652025844,3)==CrcBits(1826012922,2);
    assert CrcBits(1826012922,2)==CrcBits(913006461,1);
    assert CrcBits(913006461,1)==CrcBits(4136447134,0);
    assert CrcBits(4136447134,0)==4136447134;
  }
  lemma CrcByte6()
    ensures CrcBits(4136447134 ^ (55 as bv32),8)==2952566368
  {
    reveal CrcBits();
    assert CrcBits(4136447145,8)==CrcBits(2533234036,7);
    assert CrcBits(2533234036,7)==CrcBits(1266617018,6);
    assert CrcBits(1266617018,6)==CrcBits(633308509,5);
    assert CrcBits(633308509,5)==CrcBits(4284957070,4);
    assert CrcBits(4284957070,4)==CrcBits(2142478535,3);
    assert CrcBits(2142478535,3)==CrcBits(3529593667,2);
    assert CrcBits(3529593667,2)==CrcBits(2223516289,1);
    assert CrcBits(2223516289,1)==CrcBits(2952566368,0);
    assert CrcBits(2952566368,0)==2952566368;
  }
  lemma CrcByte7()
    ensures CrcBits(2952566368 ^ (56 as bv32),8)==1696539984
  {
    reveal CrcBits();
    assert CrcBits(2952566360,8)==CrcBits(1476283180,7);
    assert CrcBits(1476283180,7)==CrcBits(738141590,6);
    assert CrcBits(738141590,6)==CrcBits(369070795,5);
    assert CrcBits(369070795,5)==CrcBits(3880208965,4);
    assert CrcBits(3880208965,4)==CrcBits(2652579330,3);
    assert CrcBits(2652579330,3)==CrcBits(1326289665,2);
    assert CrcBits(1326289665,2)==CrcBits(3393079968,1);
    assert CrcBits(3393079968,1)==CrcBits(1696539984,0);
    assert CrcBits(1696539984,0)==1696539984;
  }
  lemma CrcByte8()
    ensures CrcBits(1696539984 ^ (57 as bv32),8)==873187033
  {
    reveal CrcBits();
    assert CrcBits(1696540009,8)==CrcBits(3744928148,7);
    assert CrcBits(3744928148,7)==CrcBits(1872464074,6);
    assert CrcBits(1872464074,6)==CrcBits(936232037,5);
    assert CrcBits(936232037,5)==CrcBits(4133380370,4);
    assert CrcBits(4133380370,4)==CrcBits(2066690185,3);
    assert CrcBits(2066690185,3)==CrcBits(3492748132,2);
    assert CrcBits(3492748132,2)==CrcBits(1746374066,1);
    assert CrcBits(1746374066,1)==CrcBits(873187033,0);
    assert CrcBits(873187033,0)==873187033;
  }
  lemma CrcEmpty(crc: bv32)
    ensures CrcBytes([],crc)==crc
  { reveal CrcBytes(); }
  lemma CrcCons(rgb: Bytes,crc: bv32,next: bv32,result: bv32)
    requires |rgb|>0
    requires CrcBits(crc ^ (rgb[0] as bv32),8)==next
    requires CrcBytes(rgb[1..],next)==result
    ensures CrcBytes(rgb,crc)==result
  { reveal CrcBytes(); }
  lemma CrcVector()
    ensures Crc32([49,50,51,52,53,54,55,56,57])==3421780262
  {
    var rgb: Bytes := [49,50,51,52,53,54,55,56,57];
    CrcEmpty(873187033);
    assert rgb[9..]==[];
    assert CrcBytes(rgb[9..],873187033)==873187033;
    CrcByte0();
    CrcByte1();
    CrcByte2();
    CrcByte3();
    CrcByte4();
    CrcByte5();
    CrcByte6();
    CrcByte7();
    CrcByte8();
    assert rgb[8]==57;
    assert rgb[8..][1..]==rgb[9..];
    CrcCons(rgb[8..],1696539984,873187033,873187033);
    assert rgb[7]==56;
    assert rgb[7..][1..]==rgb[8..];
    CrcCons(rgb[7..],2952566368,1696539984,873187033);
    assert rgb[6]==55;
    assert rgb[6..][1..]==rgb[7..];
    CrcCons(rgb[6..],4136447134,2952566368,873187033);
    assert rgb[5]==54;
    assert rgb[5..][1..]==rgb[6..];
    CrcCons(rgb[5..],873121251,4136447134,873187033);
    assert rgb[4]==53;
    assert rgb[4..][1..]==rgb[5..];
    CrcCons(rgb[4..],1679564636,873121251,873187033);
    assert rgb[3]==52;
    assert rgb[3..][1..]==rgb[4..];
    CrcCons(rgb[3..],2008521773,1679564636,873187033);
    assert rgb[2]==51;
    assert rgb[2..][1..]==rgb[3..];
    CrcCons(rgb[2..],2964110130,2008521773,873187033);
    assert rgb[1]==50;
    assert rgb[1..][1..]==rgb[2..];
    CrcCons(rgb[1..],2082672712,2964110130,873187033);
    assert rgb[0]==49;
    assert rgb[0..][1..]==rgb[1..];
    CrcCons(rgb[0..],4294967295,2082672712,873187033);
    assert ((873187033 as bv32) ^ 0xffffffff)==3421780262;
  }
  lemma StoredZipVector() {
    reveal CrcBytes();
    var e:=ZipEntry([97],[[97]],false,2048,0,0,false,false,0,0,0,0,0,
      67324752,2048,0,[97],[],[],0,0,true);
    assert Crc32([])==0;
    assert FSafeComponent([97]);
    assert FZipName([97],[[97]],false,true);
    assert FZipEntry(e,31);
    assert !FZipEntry(e.(localName:=[98]),31);
    assert !FZipEntry(e.(fSymlink:=true),31);
    assert !FZipEntry(e.(crc:=1),31);
    assert !FZipEntry(e.(cbPlain:=67108865),31);
  }
  lemma RecoveryVector() {
    var original:=Publication(Idle,[1],[2],[1],false,true);
    var staged:=original.(phase:=Staged);
    var held:=staged.(phase:=PriorHeld,fPriorAvailable:=true);
    var published:=held.(phase:=Published,current:=[2]);
    var failed:=published.(phase:=RecoveryRequired);
    var restored:=failed.(phase:=Restored,current:=[1]);
    assert FTransition(original,Stage,staged);
    assert FTransition(staged,HoldPrior,held);
    assert FTransition(held,Publish,published);
    assert FTransition(published,Fail,failed);
    assert FTransition(failed,Restore,restored);
    assert restored.current==original.current && restored.fOrdersPresent;
  }
}
