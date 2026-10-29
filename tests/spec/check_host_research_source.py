#!/usr/bin/env python3
"""Native UpdateResearchStatus with actual cost/race bodies and declared part/message services.

Independent interpreter preserves signed arithmetic guards, unsigned progress,
redo/recursive passes, all equipment lookup/message ordering and packed policy.
No game source or baseline is changed. Invalid numeric/pointer candidates stop
before C. Missing-return fixtures use an explicit lookup longjmp provider.
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
from check_native_colony_source import Boundary,checked,signed,div
from check_research_source import TECH,mix


class Escape(Exception):pass


def reference(c):
    n,use_pool,slow,mode,message_return,escape,initial=c[:7]
    players=[]
    for i in range(16):
        p=c[8+26*i:34+26*i]
        players.append(dict(primary=p[0],general=p[1],tt=p[2],dead=p[3],crippled=p[4],now=p[5],next=p[6],pool=p[7],
                            levels=p[8:14].copy(),choices=p[14:20],spent=[x%4294967296 for x in p[20:26]]))
    if n>16:raise Boundary('player array extent')
    global_player=initial;hash_value=0;cost_calls=lookup_calls=msg_calls=general_calls=tt_calls=primary_calls=entry_calls=0
    last_field=last_level=0;type_calls=[0]*16
    def event(*values):
        nonlocal hash_value
        hash_value=mix(hash_value,*values)
    def trait(player,kind):
        nonlocal general_calls,tt_calls,primary_calls
        event(30,player,kind)
        if kind==0:general_calls+=1;return players[player]['general']
        if kind==1:tt_calls+=1;return players[player]['tt']
        primary_calls+=1;return players[player]['primary']
    def message(player,tag,target,a,b,d):
        nonlocal msg_calls,last_field,last_level,type_calls
        a,b,d=signed(a,16),signed(b,16),signed(d,16)
        event(10,player,tag,signed(target,16),a,b,d);msg_calls+=1
        if tag in (0,1):last_field=b;last_level=a;type_calls=[0]*16
    def cost(player,field):
        nonlocal cost_calls
        p=players[player];level=p['levels'][field]+1
        if not 0<=level<=26:raise Boundary('technology cost table')
        total=sum(p['levels']);base=signed(total*10,16)+TECH[level];choice=p['choices'][field]
        value=(2*base-base//4 if choice<1 else base if choice==1 else div(base,2))*(2 if slow else 1)
        cost_calls+=1;event(20,player,field,level,total,value)
        return checked(value,'technology cost')
    def lowest(levels):
        best=0
        for i in range(1,6):
            if levels[i]<levels[best]%256:best=i
        return best
    def stored(value):
        return checked(value+1,'slow spending round addition')//2 if slow else value
    def part(player,kind,item):
        nonlocal lookup_calls
        slot=item%256;event(50,player,kind,slot);lookup_calls+=1
        if lookup_calls-1==escape:raise Escape()
        at=type_calls[kind];type_calls[kind]+=1
        limit=0 if mode==0 else 18 if mode in (1,2,6) else (kind%3+1) if mode==3 else 32769 if mode==5 else 258
        if at>=limit:return 0,0
        if mode==6:raise Boundary('part requirement pointer')
        if mode==1:
            availability=(1,-1,2,99,1)[slot%5];requirement=last_level if slot%5==0 else signed(last_level-1,8)
        else:availability=1;requirement=last_level
        return availability,requirement
    def announce(player,field,level):
        for kind in range(16):
            item=0
            while True:
                availability,requirement=part(player,kind,item)
                if availability==0:break
                if availability==1 and level==requirement%256:
                    if kind==10:tag,target=2,-3
                    elif kind==14:tag,target=3,-3
                    else:
                        if kind==13 and trait(player,1) and item in (8,12,16):item=signed(item+1,16);continue
                        tag=4 if kind==15 and 9<=item<=13 else 5 if kind==15 and 0<=item<=8 else 6
                        target=signed((kind<<8)|0xc000|item,16)
                    message(player,tag,target,field,signed(1<<kind,16),item)
                item=signed(item+1,16)
    def update(pool_flag):
        nonlocal entry_calls,global_player
        entry_calls+=1;event(40,int(pool_flag),global_player)
        alive=0;totals=[0]*6
        for player in range(n):
            p=players[player];using_pool=pool_flag;general=trait(player,0);current=p['now'];next_field=p['next'];global_player=player
            if not p['dead']:alive+=1
            while True:
                redo=False
                for field in range(6):
                    spent=signed(p['spent'][field],32);change=False
                    if slow:spent=checked(spent*2,'slow stored research multiplication')
                    if field==current and using_pool and general<2:
                        if general:
                            redo=True;general=2;amount=div(checked(p['pool']+1,'general primary round addition'),2)
                            spent=checked(spent+amount,'primary research addition');totals[field]=checked(totals[field]+amount,'shared primary field total')
                            for other in range(6):
                                if other!=field:
                                    amount=div(signed(checked(p['pool']*3,'general research pool multiplication')+19,32),20)
                                    p['spent'][other]=(p['spent'][other]+(div(amount,2) if slow else amount))%4294967296
                                    totals[other]=checked(totals[other]+amount,'shared secondary field total')
                        else:
                            spent=checked(spent+p['pool'],'primary research addition');totals[field]=checked(totals[field]+p['pool'],'shared primary field total')
                    while p['levels'][field]<26 and (not p['crippled'] or p['levels'][field]<10):
                        price=cost(player,field)
                        if price>spent:p['spent'][field]=stored(spent)%4294967296;break
                        target=current;spent=checked(spent-price,'research cost subtraction');p['levels'][field]+=1;level=p['levels'][field]
                        if level==26 and next_field==6:next_field=7
                        if current==field and next_field!=6:
                            target=next_field if next_field!=7 else lowest(p['levels']);change=True
                        message(player,0 if not general else 1,-2,level,field,target)
                        announce(player,field,level)
                        if (using_pool or change or next_field==7) and next_field!=6 and field==current:
                            if next_field==7:
                                target=lowest(p['levels']);p['now']=target%16;p['spent'][field]=0;current=target;next_field=7
                            else:
                                p['next']=6;p['now']=next_field%16;p['spent'][field]=0;current=next_field;next_field=6
                            remaining=stored(spent)
                            if not 0<=current<6:raise Boundary('redirected research field index')
                            p['spent'][current]=(p['spent'][current]+remaining)%4294967296;using_pool=False;redo=True
                            break
                if not redo:break
        global_player=-1;redo=False
        if pool_flag and alive>1:
            for player in range(n):
                if trait(player,2)==1:
                    for field in range(6):
                        if totals[field]>0:
                            amount=div(div(totals[field],alive),2)
                            if amount>1:
                                redo=True;message(player,7,-2,field,signed(amount,16),0)
                                players[player]['spent'][field]=(players[player]['spent'][field]+stored(amount))%4294967296
            if redo:update(False)
    complete=True
    try:update(bool(use_pool))
    except Escape:complete=False
    result=[]
    for p in players:result+=p['levels']+p['spent']+[(p['now']%16)+16*(p['next']%16),p['pool']]
    return result+[global_player,hash_value,cost_calls,lookup_calls,msg_calls,general_calls,tt_calls,primary_calls,entry_calls,int(complete)]


def base(n=1):
    c=[n,1,0,0,1,-1,5,0]
    for _ in range(16):c += [0,0,0,0,0,0,6,0]+[0]*6+[1]*6+[0]*6
    return c


def cases():
    for policy in range(256):
        for pool in (0,49,50,1000):
            for general in (0,1):
                for slow in (0,1):
                    c=base();c[2]=slow;c[8+1]=general;c[8+5]=policy%16;c[8+6]=signed(policy//16,4);c[8+7]=pool;yield c
    for level in (-128,-2,-1,0,9,10,25,26,127):
        for raw_spent in (0,1,49,50,100000,2147483647,-2147483648,-1):
            for pool in (-2147483648,-1,0,101,715827882,2147483647):
                for general in (0,1):
                    for slow in (0,1):
                        for use_pool in (0,1):
                            c=base();c[1]=use_pool;c[2]=slow;c[9]=general;c[15]=pool;c[16]=level;c[28]=raw_spent;yield c
    for n in (0,1,2,4,16):
        for alive in (0,1,2,16):
            for slow in (0,1):
                for general in (0,1):
                    c=base(n);c[2]=slow
                    for i in range(16):at=8+26*i;c[at]=1 if i%2==0 else 0;c[at+1]=general;c[at+3]=int(i>=alive);c[at+7]=101
                    yield c
    for mode in (0,1,2,3,4,6):
        for tt in (0,1):
            for message_return in (0,1):
                for escape in (-1,0,3,18):
                    c=base();c[3]=mode;c[4]=message_return;c[5]=escape;c[10]=tt;c[15]=50;yield c
    rng=random.Random(0xA12C0DE)
    for _ in range(5000):
        n=rng.randrange(1,5);c=base(n);c[1]=rng.randrange(2);c[2]=rng.randrange(2);c[3]=rng.choice([0,0,0,3]);c[4]=rng.randrange(2)
        for i in range(n):
            at=8+26*i;c[at:at+8]=[rng.choice([0,1,9]),rng.randrange(2),rng.randrange(2),rng.randrange(2),rng.randrange(2),rng.randrange(6),rng.choice([0,1,2,3,4,5,6,7]),rng.randrange(1500)]
            c[at+8:at+14]=[rng.randrange(27) for _ in range(6)];c[at+14:at+20]=[rng.randrange(3) for _ in range(6)];c[at+20:at+26]=[rng.randrange(200000) for _ in range(6)]
        yield c
    c=base();c[8+4]=1;c[8+8]=9;c[8+7]=100000;yield c
    c=base();c[8+8]=26;c[8+7]=101;yield c
    for tt in (0,1):
        c=base();c[3]=5;c[10]=tt;c[15]=50;yield c
    c=base(5)
    for i in range(5):c[8+26*i+7]=600000000
    yield c
    c=base();c[2]=1;c[14]=1;c[15]=1;c[16:22]=[-1,1,0,0,0,0];c[28]=1073741823;yield c
    for n in (-32768,-1):
        c=base(n);yield c
    for flag in (-32768,2):
        c=base();c[1]=flag;c[15]=50;yield c


def main():
    research=(ROOT/'src/core/research.c').read_text();start=research.index('int32_t  rglTechCost[27]');table=research[start:research.index(';',start)+1]
    declarations=r'''
#include "common.h"
#include <setjmp.h>
#include <assert.h>
#include <stdlib.h>
#include <inttypes.h>
static int32_t cfg[424];PLAYER rgplr[16];GAME game;int16_t idPlayer;
static unsigned costs,lookups,messages,generals,tts,primaries,entries;static uint32_t hash;
static int lastField,lastLevel,typeCalls[16];static COMPART supplied;static jmp_buf escaped;
static int16_t NativeGetRaceStat(PLAYER *,RaceStat);static int16_t NativeGetRaceGrbit(PLAYER *,RaceGrbit);
static int32_t NativeTechCost(TechFieldType,int16_t,int16_t);static void NativeUpdateResearchStatus(int16_t);
static void hv(int32_t value){hash=hash*UINT32_C(16777619)^(uint32_t)value;}
int16_t GetRaceStat(PLAYER *p,RaceStat stat){if(stat==rsMajorAdv){primaries++;hv(30);hv((int32_t)(p-rgplr));hv(2);}return NativeGetRaceStat(p,stat);}
int16_t GetRaceGrbit(PLAYER *p,RaceGrbit bit){assert(bit==ibitRaceGeneralizedResearch || bit==ibitRaceTT);hv(30);hv((int32_t)(p-rgplr));hv(bit==ibitRaceTT ? 1:0);if(bit==ibitRaceTT)tts++;else generals++;return NativeGetRaceGrbit(p,bit);}
int32_t GetTechLevelCost(TechFieldType field,int16_t level,int16_t player){int total=0;for(int i=0;i<6;i++)total+=rgplr[player].rgTech[i];int32_t value=NativeTechCost(field,level,player);costs++;hv(20);hv(player);hv(field);hv(level);hv(total);hv(value);return value;}
void UpdateResearchStatus(int16_t pool){entries++;hv(40);hv(!!pool);hv(idPlayer);NativeUpdateResearchStatus(pool);}
static int kind(uint16_t mask){for(int i=0;i<16;i++)if(mask==(UINT32_C(1)<<i))return i;abort();}
int16_t FLookupPart(PART *part){int k=kind(part->hs.grhst),item=part->hs.iItem,at=typeCalls[k]++;hv(50);hv(idPlayer);hv(k);hv(item);lookups++;
    if((int)lookups-1==cfg[5])longjmp(escaped,1);
    int limit=cfg[3]==0 ? 0:(cfg[3]==1 || cfg[3]==2 || cfg[3]==6) ? 18:cfg[3]==3 ? k%3+1:cfg[3]==5 ? 32769:258;
    if(at>=limit){part->pcom=NULL;return mdPartAvailInvalid;}assert(cfg[3]!=6);
    int avail=mdPartAvailAvailable,requirement=lastLevel;
    if(cfg[3]==1){static const int states[]={1,-1,2,99,1};avail=states[item%5];if(item%5)requirement=(int8_t)(lastLevel-1);}
    memset(&supplied,0,sizeof(supplied));for(int i=0;i<6;i++)supplied.rgTech[i]=(int8_t)requirement;part->pcom=&supplied;return (int16_t)avail;
}
int16_t FSendPlrMsg(int16_t player,MessageId id,MsgGoto target,int16_t a,int16_t b,int16_t d,int16_t p4,int16_t p5,int16_t p6,int16_t p7){
    assert(!p4 && !p5 && !p6 && !p7);int tag;
    switch(id){case idmScientistsHaveCompletedResearchTechLevelWill:tag=0;break;case idmScientistsHaveCompletedResearchTechLevelPrimary:tag=1;break;
    case idmRecentBreakthroughHasAlsoGivenHullDesign:tag=2;break;case idmRecentBreakthroughHasAlsoGivenHullType:tag=3;break;
    case idmRecentBreakthroughHasAlsoTaughtHowBuild:tag=4;break;case idmRecentBreakthroughHasAlsoTaughtHowBuild2:tag=5;break;
    case idmRecentBreakthroughHasAlsoGivenBenefit:tag=6;break;case idmIntelligenceGatheringActivitiesCombinedSynergist:tag=7;break;default:abort();}
    messages++;hv(10);hv(player);hv(tag);hv((int16_t)target);hv(a);hv(b);hv(d);if(tag<2){lastField=b;lastLevel=a;memset(typeCalls,0,sizeof(typeCalls));}return (int16_t)cfg[4];
}
'''
    getter=next(x for x in (ROOT/'src/core/race.c').read_text().splitlines() if x.startswith('int16_t GetRaceStat(')).replace('int16_t GetRaceStat(', 'static int16_t NativeGetRaceStat(',1)
    bodies=table+'\n'+getter+'\n'+function('src/core/race.c','int16_t GetRaceGrbit(').replace('int16_t GetRaceGrbit(', 'static int16_t NativeGetRaceGrbit(',1)+'\n'
    bodies+=function('src/core/research.c','int32_t GetTechLevelCost(').replace('int32_t GetTechLevelCost(', 'static int32_t NativeTechCost(',1)+'\n'
    bodies+=next(x for x in (ROOT/'src/core/msg.c').read_text().splitlines() if x.startswith('int16_t FSendPlrMsg2('))+'\n'
    bodies+=function('src/core/turn2.c','void UpdateResearchStatus(').replace('void UpdateResearchStatus(', 'static void NativeUpdateResearchStatus(',1)+'\n'
    harness=r'''
static void setup(void){memset(rgplr,0,sizeof(rgplr));memset(&game,0,sizeof(game));hash=costs=lookups=messages=generals=tts=primaries=entries=0;lastField=lastLevel=0;memset(typeCalls,0,sizeof(typeCalls));game.cPlayer=(int16_t)cfg[0];game.fSlowTech=cfg[2];idPlayer=(int16_t)cfg[6];
    for(int i=0;i<16;i++){int at=8+26*i;PLAYER *p=&rgplr[i];p->rgAttr[rsMajorAdv]=(int8_t)cfg[at];p->grbitAttr=static_cast<RaceTraitBits>((cfg[at+1] ? UINT32_C(1)<<ibitRaceGeneralizedResearch:0)|(cfg[at+2] ? UINT32_C(1)<<ibitRaceTT:0));
        p->fDead=cfg[at+3];p->fCrippled=cfg[at+4];p->iTechNow=cfg[at+5];p->iTechNext=cfg[at+6];p->lResLastYear=cfg[at+7];
        for(int f=0;f<6;f++){p->rgTech[f]=(int8_t)cfg[at+8+f];p->rgAttr[8+f]=(int8_t)cfg[at+14+f];p->rgResSpent[f]=(uint32_t)cfg[at+20+f];}}
}
int main(int argc,char **argv){assert(argc==2);FILE *file=fopen(argv[1],"rb");assert(file);unsigned long cases=0,checks=0;int64_t expected[234];
    while(fread(cfg,sizeof(cfg),1,file)==1){assert(fread(expected,sizeof(expected),1,file)==1);setup();PLAYER before[16];memcpy(before,rgplr,sizeof(before));GAME gameBefore=game;int complete=0;
        if(!setjmp(escaped)){UpdateResearchStatus((int16_t)cfg[1]);complete=1;}
        int64_t actual[234];for(int i=0;i<16;i++){for(int f=0;f<6;f++){actual[i*14+f]=rgplr[i].rgTech[f];actual[i*14+6+f]=rgplr[i].rgResSpent[f];}actual[i*14+12]=(uint8_t)rgplr[i].iTechCur;actual[i*14+13]=rgplr[i].lResLastYear;
            PLAYER unchanged=rgplr[i];memcpy(unchanged.rgTech,before[i].rgTech,sizeof(unchanged.rgTech));memcpy(unchanged.rgResSpent,before[i].rgResSpent,sizeof(unchanged.rgResSpent));unchanged.iTechCur=before[i].iTechCur;assert(!memcmp(&unchanged,&before[i],sizeof(unchanged)));checks++;}
        assert(!memcmp(&game,&gameBefore,sizeof(game)));checks++;
        int64_t extra[]={idPlayer,hash,costs,lookups,messages,generals,tts,primaries,entries,complete};memcpy(actual+224,extra,sizeof(extra));
        for(int i=0;i<234;i++){checks++;if(actual[i]!=expected[i]){fprintf(stderr,"case %lu field %d actual %lld expected %lld (pool %d slow %d mode %d)\n",cases,i,(long long)actual[i],(long long)expected[i],cfg[1],cfg[2],cfg[3]);return 1;}}cases++;
    }
    assert(feof(file));fclose(file);printf("Native host research: %lu cases, %lu assertions passed\n",cases,checks);return 0;
}
'''
    with tempfile.TemporaryDirectory(prefix='host-research-') as work:
        path=Path(work);data=path/'vectors.bin';excluded={};count=0
        with data.open('wb') as stream:
            for c in cases():
                assert len(c)==424
                try:result=reference(c)
                except Boundary as why:excluded[str(why)]=excluded.get(str(why),0)+1;continue
                stream.write(struct.pack('<424i234q',*c,*result));count+=1
        print(f'Prepared {count} host research vectors; excluded before C: {excluded}',flush=True)
        for generator in ['text/textgen.py','data/datagen.py']:
            subprocess.run([sys.executable,str(ROOT/generator),str(path)],check=True,stdout=subprocess.DEVNULL)
        source=path/'witness.cpp';source.write_text(declarations+bodies+harness);binary=path/'witness'
        subprocess.run(shlex.split(os.environ.get('CXX','c++'))+['-std=gnu++20','-x','c++','-Wall','-Wextra','-Werror','-Wno-unused-variable','-Wno-unused-but-set-variable',
            '-fsanitize=address,undefined','-fno-sanitize-recover=all','-I'+str(ROOT/'src/core'),'-I'+str(ROOT/'src/platform'),'-I'+str(path),str(source),'-lm','-o',str(binary)],check=True)
        subprocess.run([str(binary),str(data)],check=True)

        # Bind actual byte catalog terminators without replacing availability logic.
        # Headless reads exercise every byte ID; player contexts exercise every
        # out-of-range ID for each signed primary trait before later availability.
        catalog_declarations=r'''
#include "common.h"
#include <setjmp.h>
#include <assert.h>
PLAYER rgplr[16];int16_t idPlayer;
'''
        catalog_bodies=next(x for x in (ROOT/'src/core/race.c').read_text().splitlines() if x.startswith('int16_t GetRaceStat('))+'\n'
        catalog_bodies+=function('src/core/race.c','int16_t GetRaceGrbit(')+'\n'
        catalog_bodies+=function('src/core/parts.c','int16_t FLookupPart(')+'\n'
        catalog_bodies+=function('src/core/parts.c','int16_t TechStatus(')+'\n'
        catalog_bodies+=function('src/core/research.c','int16_t FShouldPartBeHidden(')+'\n'
        model=(ROOT/'spec/dafny/S03-research-catalog.dfy').read_text()
        match=re.search(r'function Limits\(\).*?\{\s*\[([0-9,\s]+)\]\s*\}',model,re.S)
        if match is None:raise AssertionError('expected literal S03 research catalog limits')
        limits=[int(value) for value in match[1].split(',')]
        assert len(limits)==16 and all(0<=value<256 for value in limits)
        catalog_declarations+='const unsigned rgcModelLimits[]={'+','.join(map(str,limits))+'};\n'
        catalog_harness=r'''
int main(void){
    const unsigned counts[]={iengineCount,iscannerCount,ishieldCount,iarmorCount,ibeamCount,itorpCount,ibombCount,iminingCount,iminesCount,ispecialSBCount,isbhullCount,ispecialECount,ispecialMCount,iterraCount,ihuldefOrbitalFort,iplanetaryCount};
    unsigned long checks=0;PART part;memset(rgplr,0,sizeof(rgplr));
    for(unsigned k=0;k<16;k++){assert(counts[k]<256);checks++;assert(counts[k]==rgcModelLimits[k]);checks++;idPlayer=iplrNone;
        for(unsigned item=0;item<256;item++){memset(&part,0,sizeof(part));part.hs.grhst=(HullSlotType)(UINT32_C(1)<<k);part.hs.iItem=item;
            int available=FLookupPart(&part);if((available==mdPartAvailInvalid)!=(item>=counts[k])){fprintf(stderr,"catalog kind %u item %u count %u status %d\n",k,item,counts[k],available);return 1;}checks++;
            if(item<counts[k]){assert(part.pcom);checks++;}}
        idPlayer=0;for(int trait=-128;trait<128;trait++){rgplr[0].rgAttr[rsMajorAdv]=(int8_t)trait;
            for(unsigned item=counts[k];item<256;item++){memset(&part,0,sizeof(part));part.hs.grhst=(HullSlotType)(UINT32_C(1)<<k);part.hs.iItem=item;
                assert(FLookupPart(&part)==mdPartAvailInvalid);checks++;}}}
    printf("Native actual catalog terminators: %lu assertions passed, all 16 counts below byte wrap\n",checks);return 0;
}
'''
        catalog=path/'catalog.cpp';catalog.write_text(catalog_declarations+catalog_bodies+catalog_harness);catalog_binary=path/'catalog'
        subprocess.run(shlex.split(os.environ.get('CXX','c++'))+['-std=gnu++20','-x','c++','-Wall','-Wextra','-Werror','-Wno-unused-variable','-Wno-unused-but-set-variable',
            '-fsanitize=address,undefined','-fno-sanitize-recover=all','-I'+str(ROOT/'src/core'),'-I'+str(ROOT/'src/platform'),'-I'+str(path),str(catalog),str(path/'data_tables.cpp'),'-lm','-o',str(catalog_binary)],check=True)
        subprocess.run([str(catalog_binary)],check=True)


if __name__=='__main__':main()
