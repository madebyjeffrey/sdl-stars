#!/usr/bin/env python3
"""Compare actual lookup/technology/hidden bodies, fresh catalogs and native frames.

The independent reference preserves source short-circuit query order. Catalog
requirements/planetary ability come from an independently indexed native export;
all versioned model identities and limits bind to fresh generated enums.
No game source, catalog or baseline is modified.
"""
from pathlib import Path
import os
import random
import re
import shlex
import struct
import subprocess
import sys
import tempfile
from check_s02_source import ROOT,function

LIMITS=[16,16,10,12,24,12,15,8,10,16,5,17,11,20,32,15]
IDENTITY_SOURCE=(ROOT/'spec/dafny/S03-lookup-identities.dfy').read_text()
IDENTITIES={name:int(value) for name,value in re.findall(r'const (\w+): nat := (\d+)',IDENTITY_SOURCE)}
assert set(re.findall(r'const (\w+)',IDENTITY_SOURCE))==set(IDENTITIES), 'unsupported model identity declaration'
I=type('Identities',(),IDENTITIES)
TRADER={(0,I.iengineEnigmaPulsar):512,(2,I.ishieldLangstonShell):4,(3,I.iarmorMegaPolyShell):8,
        (4,I.ibeamMultiContainedMunition):128,(5,I.itorpAntiMatterTorpedo):64,(6,I.ibombHushABoom):32,
        (7,I.iminingAlienMiner):16,(11,I.ispecialEMultiFunctionPod):2,(12,I.ispecialMMultiCargoPod):1,
        (12,I.ispecialMJumpGate):2048,(14,I.ihuldefMiniMorph):256,(15,I.iplanetaryGenesisDevice):1024}


def tech(have,need,current):
    missing=[i for i in range(6) if have[i]<need[i]]
    if not missing:return 1
    if missing==[current]:return 2 if need[current]==have[current]+1 else need[current]-have[current]+1
    return 99


def hidden(headless,grants,kind,item):
    flag=TRADER.get((kind,item),0)
    return not headless and flag!=0 and grants&flag==0


def policy(headless,primary,flags,grants,kind,item,ability):
    calls=[]
    def bit(index):calls.append(100+index);return bool(flags&(1<<index))
    def visible():calls.append(1);return not hidden(headless,grants,kind,item)
    ra=9 if headless else primary
    if headless and kind not in (2,3,4):return True,calls
    allowed=True
    if kind==0:
        if item==I.iengineSettlersDelight and ra!=0:allowed=False
        elif (I.iengineSubGalacticFuelScoop<=item<=I.iengineGalaxyScoop or item==I.iengineRadiatingHydroRamScoop) and bit(7):allowed=False
        elif item in (I.iengineGalaxyScoop,I.iengineFuelMizer) and not bit(0):allowed=False
        elif item==I.iengineInterspace10 and not bit(7):allowed=False
        else:allowed=visible()
    elif kind==1:
        if item in (I.iscannerFerretScanner,I.iscannerDolphinScanner,I.iscannerElephantScanner) and bit(10):allowed=False
        elif item in (I.iscannerChameleonScanner,I.iscannerPickPocketScanner,I.iscannerRobberBaronScanner) and ra!=1:allowed=False
    elif kind==2:
        allowed=not ((item==I.ishieldShadowShield and ra!=1) or (item==I.ishieldCrobySharmor and ra!=4)) and visible()
    elif kind==3:
        allowed=not ((item==I.iarmorDepletedNeutronium and ra!=1) or (item==I.iarmorFieldedKelarium and ra!=4)) and visible()
    elif kind==4:
        allowed=not ((item==I.ibeamMiniGun and ra!=4) or (item in (I.ibeamBlunderbuss,I.ibeamGatlingNeutrinoCannon) and ra!=2)) and visible()
    elif kind==5:allowed=visible()
    elif kind==6:
        allowed=not ((I.ibombSmartBomb<=item<=I.ibombAnnihilatorBomb and ra==4) or (item==I.ibombRetroBomb and ra!=3)) and visible()
    elif kind==7:
        if item in (I.iminingRoboMiner,I.iminingRoboMaxiMiner,I.iminingRoboSuperMiner,I.iminingRoboMidgetMiner,I.iminingRoboUltraMiner) and bit(9):allowed=False
        elif item in (I.iminingRoboMidgetMiner,I.iminingRoboUltraMiner) and not bit(2):allowed=False
        elif item==I.iminingOrbitalAdjuster and ra!=3:allowed=False
        else:allowed=visible()
    elif kind==8:
        if item in (I.iminesMineDispenser40,I.iminesMineDispenser80,I.iminesMineDispenser130,I.iminesHeavyDispenser50,I.iminesHeavyDispenser110,I.iminesHeavyDispenser200,I.iminesSpeedTrap30,I.iminesSpeedTrap50) and ra!=5:allowed=False
        elif item==I.iminesSpeedTrap20 and ra not in (4,5):allowed=False
        elif item==I.iminesMineDispenser50 and ra==2:allowed=False
    elif kind==9:
        if I.ispecialSBMassDriver5<=item<=I.ispecialSBUltraDriver13:allowed=item in (I.ispecialSBMassDriver7,I.ispecialSBUltraDriver10) or ra==6
        elif I.ispecialSBStargate100250<=item<=I.ispecialSBStargateAnyAny:
            allowed=not (ra!=7 and (item==I.ispecialSBStargateAny300 or item>=I.ispecialSBStargate100Any)) and ra!=0
    elif kind==10:
        if item in (I.isbhullSpaceDock,I.isbhullUltraStation) and not bit(3):allowed=False
        elif item==I.isbhullDeathStar and ra!=8:allowed=False
    elif kind==11:
        allowed=visible()
        if allowed:
            allowed=not ((item in (I.ispecialETransportCloaking,I.ispecialEUltraStealthCloak) and ra!=1) or
                         (item==I.ispecialEEnergyDampener and ra!=5) or (item==I.ispecialEAntiMatterGenerator and ra!=7) or
                         (item==I.ispecialEFluxCapacitor and ra!=0) or
                         (item in (I.ispecialEJammer10,I.ispecialEJammer50,I.ispecialETachyonDetector) and ra!=4))
    elif kind==12:
        allowed=visible()
        if allowed:allowed=not (item==I.ispecialMColonizationModule and ra==8) and (item!=I.ispecialMOrbitalConstructionModule or ra==8)
    elif kind==13:
        if I.iterraTotalTerraform3<=item<=I.iterraTotalTerraform30:allowed=bit(1)
    elif kind==14:
        if (item in (I.ihuldefMiniColonyShip,I.ihuldefMetaMorph) and ra!=0) or (item in (I.ihuldefFuelTransport,I.ihuldefSuperFreighter) and ra!=4):allowed=False
        elif item in (I.ihuldefMiner,I.ihuldefMaxiMiner,I.ihuldefMidgetMiner,I.ihuldefUltraMiner) and bit(9):allowed=False
        elif item in (I.ihuldefMidgetMiner,I.ihuldefMiner,I.ihuldefUltraMiner) and not bit(2):allowed=False
        elif ((item in (I.ihuldefDreadnought,I.ihuldefBattleCruiser) and ra!=2) or
              (item in (I.ihuldefRogue,I.ihuldefStealthBomber) and ra!=1) or
              (item in (I.ihuldefMiniMineLayer,I.ihuldefSuperMineLayer) and ra!=5)):allowed=False
        else:allowed=visible()
    elif kind==15:
        scanner=I.iplanetaryViewer50<=item<=I.iplanetarySnooper620X
        if scanner and ability<0 and bit(10):allowed=False
        elif (scanner and ra==8) or (I.iplanetarySDI<=item<=I.iplanetaryNeutronShield and ra==8) or (I.iplanetaryLaserBattery<=item<=I.iplanetaryNeutronShield and ra==2):allowed=False
        else:allowed=visible()
    return allowed,calls


def cases():
    relevant=(0,1,2,3,7,9,10)
    for primary in range(10):
        for combination in range(128):
            flags=sum(1<<bit for i,bit in enumerate(relevant) if combination&(1<<i))
            levels=[(primary+combination)%27]*6
            for grants in (0,65535):
                for kind,count in enumerate(LIMITS):
                    for item in range(count+1):yield [0,0,primary,flags,grants,(combination+primary)%16,1<<kind,item,combination%256,combination%16]+levels+[0]*6
    for primary in range(-128,128):
        for flags in (0,0xffffffff):
            for kind,count in enumerate(LIMITS):
                for item in range(count+1):yield [0,0,primary,flags,0,15,1<<kind,item,255,15]+[26]*6+[0]*6
    for mask in range(65536):
        for headless in (0,1):
            for mode in (0,1):yield [mode,headless,9,0xffffffff,0,0,mask,0,0,0]+[0]*12
    for kind in range(16):
        for item in range(256):
            for headless in (0,1):yield [0,headless,4,0,0,0,1<<kind,item,255,0]+[0]*12
    # Every trader-grant word is checked directly for every special trader part.
    for grants in range(65536):
        for (kind,item) in TRADER:yield [1,0,0,0,grants,0,1<<kind,item,0,0]+[0]*12
    for have in range(-128,128):
        for need in range(-128,128):
            for current in (0,1,15):yield [2,0,0,0,0,current,1,0,0,0]+[have,0,0,0,0,0]+[need,0,0,0,0,0]
    for missing in range(64):
        for current in range(16):yield [2,0,0,0,0,current,1,0,0,0]+[0]*6+[int(bool(missing&(1<<i))) for i in range(6)]
    rng=random.Random(110312)
    for _ in range(10000):
        kind=rng.randrange(16);yield [0,rng.randrange(2),rng.randrange(-128,128),rng.randrange(1<<32),rng.randrange(1<<16),rng.randrange(16),1<<kind,rng.randrange(256),rng.randrange(256),rng.randrange(16)]+[rng.randrange(-128,128) for _ in range(12)]


def catalog_rows_source():
    tables=['rgengine','rgscanner','rgshield','rgarmor','rgbeam','rgtorp','rgbomb','rgmining','rgmines','rgspecialSB','rghuldefSB','rgspecialE','rgspecialM','rgterra','rghuldef','rgplanetary']
    return 'static COMPART *LpcomRow(unsigned k,unsigned item){switch(k){\n'+''.join(f'case {k}:return (COMPART *)&{table}[item]'+('.hul' if k in (10,14) else '')+';\n' for k,table in enumerate(tables))+'default:abort();}}\n'


def native_parts(call_capacity=8):
    declarations=r'''
#include "common.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
PLAYER rgplr[16];int16_t idPlayer;GAME game;
static unsigned long cCases,cChecks;
static int rgCalls[8],cCalls;
#define CHECK(f) do { cChecks++;if(!(f)){fprintf(stderr,"case %lu check failed at line %d: %s\n",cCases,__LINE__,#f);abort();}} while(0)
static void Call(int tag){CHECK(cCalls<8);rgCalls[cCalls++]=tag;}
'''
    declarations+='\n'.join(f'static_assert({name}=={value},"model identity {name}");' for name,value in IDENTITIES.items())+'\n'
    bindings={'raCheapCol':0,'raStealth':1,'raAttack':2,'raTerra':3,'raDefend':4,'raMines':5,'raMassAccel':6,'raStargate':7,'raMacintosh':8,'raNone':9,
              'ibitRaceIFE':0,'ibitRaceTT':1,'ibitRaceARM':2,'ibitRaceISB':3,'ibitRaceNoRamscoops':7,'ibitRaceOBRM':9,'ibitRaceNoAdvScanner':10,
              'grbitTraderCargo':1,'grbitTraderSpecial':2,'grbitTraderShield':4,'grbitTraderArmor':8,'grbitTraderMiner':16,'grbitTraderBomb':32,
              'grbitTraderTorp':64,'grbitTraderBeam':128,'grbitTraderHull':256,'grbitTraderEngine':512,'grbitTraderGenesis':1024,'grbitTraderJumpgate':2048}
    mask_names=('hstEngine','hstScanner','hstShield','hstArmor','hstBeam','hstTorp','hstBomb','hstMining','hstMines','hstSpecialSB','hstSBHull','hstSpecialE','hstSpecialM','hstTerra','hstHull','hstPlanetary')
    bindings.update({name:1<<k for k,name in enumerate(mask_names)})
    declarations+='\n'.join(f'static_assert({name}=={value},"native protocol {name}");' for name,value in bindings.items())+'\n'
    bodies=next(x for x in (ROOT/'src/core/race.c').read_text().splitlines() if x.startswith('int16_t GetRaceStat(')).replace('GetRaceStat(','NativeGetRaceStat(',1)+'\n'
    bodies+=function('src/core/race.c','int16_t GetRaceGrbit(').replace('GetRaceGrbit(','NativeGetRaceGrbit(',1)+'\n'
    bodies+=function('src/core/parts.c','int16_t TechStatus(').replace('TechStatus(','NativeTechStatus(',1)+'\n'
    bodies+=function('src/core/research.c','int16_t FShouldPartBeHidden(').replace('FShouldPartBeHidden(','NativeHidden(',1)+'\n'
    bodies+=r'''
int16_t GetRaceStat(PLAYER *p,RaceStat stat){CHECK(idPlayer>=0 && idPlayer<16 && p==&rgplr[idPlayer] && stat==rsMajorAdv);Call(0);return NativeGetRaceStat(p,stat);}
int16_t GetRaceGrbit(PLAYER *p,RaceGrbit bit){CHECK(idPlayer>=0 && idPlayer<16 && p==&rgplr[idPlayer]);Call(100+bit);return NativeGetRaceGrbit(p,bit);}
int16_t TechStatus(char *need){Call(2);return NativeTechStatus(need);}
int16_t FShouldPartBeHidden(PART *part){Call(1);return NativeHidden(part);}
'''
    bodies+=function('src/core/parts.c','int16_t FLookupPart(')+'\n'
    bodies+=catalog_rows_source()
    declarations=declarations.replace("rgCalls[8]","rgCalls["+str(call_capacity)+"]").replace("cCalls<8","cCalls<"+str(call_capacity))
    return declarations,bodies


def main():
    declarations,bodies=native_parts()
    harness=r'''
int main(int argc,char **argv){
    const unsigned rgcRows[]={iengineCount,iscannerCount,ishieldCount,iarmorCount,ibeamCount,itorpCount,ibombCount,iminingCount,iminesCount,ispecialSBCount,isbhullCount,ispecialECount,ispecialMCount,iterraCount,ihuldefOrbitalFort,iplanetaryCount};
    assert(argc==3 && CHAR_MIN==-128 && sizeof(rgplr[0].rgTech)==6);
    FILE *file=fopen(argv[2],argv[1][0]=='e'?"wb":"rb");assert(file);
    if(argv[1][0]=='e'){
        for(unsigned k=0;k<16;k++){
          for(unsigned item=0;item<rgcRows[k];item++){
            int32_t rgData[9];COMPART *p=LpcomRow(k,item);rgData[0]=k;rgData[1]=item;
            for(int f=0;f<6;f++)rgData[2+f]=p->rgTech[f];
            rgData[8]=k==15?rgplanetary[item].grAbility:0;
            assert(fwrite(rgData,sizeof(rgData),1,file)==1);
          }
        }
        fclose(file);return 0;
    }
    int32_t rgIn[33];static COMPART comPrior;
    while(fread(rgIn,sizeof(rgIn),1,file)==1){
        unsigned mask=(unsigned)rgIn[6],item=(unsigned)rgIn[7],kind=16;
        for(unsigned k=0;k<16;k++)if(mask==(UINT32_C(1)<<k))kind=k;
        memset(rgplr,0x53,sizeof(rgplr));memset(&game,0x35,sizeof(game));
        idPlayer=(int16_t)(rgIn[1]?iplrNone:rgIn[9]);int target=rgIn[9];
        rgplr[target].rgAttr[rsMajorAdv]=(int8_t)rgIn[2];rgplr[target].grbitAttr=static_cast<RaceTraitBits>((uint32_t)rgIn[3]);rgplr[target].grbitTrader=static_cast<GrbitTrader>((uint16_t)rgIn[4]);rgplr[target].iTechNow=rgIn[5];
        for(int f=0;f<6;f++)rgplr[target].rgTech[f]=(int8_t)rgIn[10+f];
        PLAYER rgBefore[16];GAME gameBefore;memcpy(rgBefore,rgplr,sizeof(rgBefore));memcpy(&gameBefore,&game,sizeof(game));int16_t idBefore=idPlayer;
        PART part,expected;memset(&part,0xa5,sizeof(part));part.hs.grhst=static_cast<HullSlotType>((uint16_t)mask);part.hs.iItem=item;part.hs.cItem=rgIn[8];part.pcom=&comPrior;memcpy(&expected,&part,sizeof(part));
        memset(rgCalls,-1,sizeof(rgCalls));cCalls=0;int result;
        if(rgIn[0]==0){result=FLookupPart(&part);if(kind<16 && item<rgcRows[kind])expected.pcom=LpcomRow(kind,item);}
        else if(rgIn[0]==1)result=FShouldPartBeHidden(&part);
        else {char rgNeed[6],rgNeedBefore[6];for(int f=0;f<6;f++)rgNeed[f]=(char)rgIn[16+f];memcpy(rgNeedBefore,rgNeed,sizeof(rgNeed));result=TechStatus(rgNeed);CHECK(memcmp(rgNeed,rgNeedBefore,sizeof(rgNeed))==0);}
        if(result!=rgIn[22]){fprintf(stderr,"mode %d mask %u item %u primary %d flags %u grants %u current %d: got %d expected %d\n",rgIn[0],mask,item,rgIn[2],(uint32_t)rgIn[3],(uint32_t)rgIn[4],rgIn[5],result,rgIn[22]);abort();}cChecks++;
        CHECK(memcmp(&part,&expected,sizeof(part))==0);CHECK(memcmp(rgplr,rgBefore,sizeof(rgBefore))==0);CHECK(memcmp(&game,&gameBefore,sizeof(game))==0);CHECK(idPlayer==idBefore);
        CHECK(cCalls==rgIn[24]);for(int i=0;i<8;i++)CHECK(rgCalls[i]==rgIn[25+i]);
        cCases++;
    }CHECK(feof(file));fclose(file);printf("Native part availability: %lu cases, %lu assertions passed; pointers, query order and PLAYER/GAME/global frames checked\n",cCases,cChecks);return 0;
}
'''
    with tempfile.TemporaryDirectory(prefix='part-availability-') as work:
        path=Path(work)
        for generator in ('text/textgen.py','data/datagen.py'):
            subprocess.run([sys.executable,str(ROOT/generator),str(path)],check=True,stdout=subprocess.DEVNULL)
        source=path/'witness.cpp';source.write_text(declarations+bodies+harness);binary=path/'witness'
        subprocess.run(shlex.split(os.environ.get('CXX','c++'))+['-std=gnu++20','-include',str(ROOT/'src/platform/value.h'),'-x','c++','-Wall','-Wextra','-Werror',
            '-fsanitize=address,undefined','-fno-sanitize-recover=all','-I'+str(ROOT/'src/core'),'-I'+str(ROOT/'src/platform'),'-I'+str(path),str(source),str(path/'data_tables.cpp'),'-lm','-o',str(binary)],check=True)
        export=path/'catalog.bin';subprocess.run([str(binary),'export',str(export)],check=True)
        table={}
        for row in struct.iter_unpack('<9i',export.read_bytes()):table[row[0],row[1]]=(row[2:8],row[8])
        assert len(table)==sum(LIMITS) and all((k,i) in table for k,count in enumerate(LIMITS) for i in range(count))
        model_masks=re.search(r'function Masks\(\).*?\{\s*\[([0-9,\s]+)\]\s*\}',(ROOT/'spec/dafny/S03-research-catalog.dfy').read_text(),re.S)
        assert model_masks and [int(v) for v in model_masks[1].split(',')]==[1<<k for k in range(16)]
        model_limits=re.search(r'function Limits\(\).*?\{\s*\[([0-9,\s]+)\]\s*\}',(ROOT/'spec/dafny/S03-research-catalog.dfy').read_text(),re.S)
        assert model_limits and [int(v) for v in model_limits[1].split(',')]==LIMITS
        data=path/'vectors.bin';count=0
        with data.open('wb') as stream:
            for c in cases():
                mode,headless,primary,flags,grants,current,mask,item=c[:8];kind=(mask.bit_length()-1) if mask and mask&(mask-1)==0 else 16
                if mode==2:status=tech(c[10:16],c[16:22],current);calls=[2]
                elif mode==1:status=int(hidden(headless,grants,kind,item));calls=[1]
                else:
                    calls=[] if headless else [0]
                    if (kind,item) not in table:status=0
                    else:
                        need,ability=table[kind,item];allowed,queries=policy(headless,primary,flags,grants,kind,item,ability);calls+=queries
                        if not allowed:status=-1
                        elif headless:status=1
                        else:status=tech(c[10:16],need,current);calls+=[2]
                assert len(c)==22 and len(calls)<=8
                c[3]=flags if flags<0x80000000 else flags-0x100000000
                stream.write(struct.pack('<33i',*c,status,0,len(calls),*(calls+[-1]*(8-len(calls)))));count+=1
        print(f'Prepared {count} cases using {len(table)} actual catalog rows; {len(IDENTITIES)} model identities and native enum/bit protocols bound',flush=True)
        subprocess.run([str(binary),'verify',str(data)],check=True)


if __name__=='__main__':main()
