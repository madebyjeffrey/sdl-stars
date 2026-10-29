#!/usr/bin/env python3
"""Actual native mining/depletion and fleet capacity against an independent interpreter.

Uses common.h layouts, fresh generated IDs, actual race/mine/capacity/RNG bodies.
Operating-limit and part-pointer abilities are explicit providers. Undefined
arithmetic/slot/waypoint candidates are excluded before native execution.
"""
from pathlib import Path
import math
import os
import random
import shlex
import struct
import subprocess
import sys
import tempfile
from check_s02_source import ROOT, function
from check_native_colony_source import Boundary, checked, signed, div


def band(concentration):
    return 100 if concentration>100 else 10 if concentration<5 else 25 if concentration<25 else concentration


def deplete(concentration, level, demand):
    while demand>0 and concentration>1:
        normalized=level or 256; effective=band(concentration)
        left=normalized*12500//256//effective
        if left<=demand:
            demand-=left;concentration-=1;level=0
        else:
            remaining=max(1,(left-demand)*256//(12500//effective))
            if remaining>=normalized:
                remaining=normalized-1
            level=remaining
            if not level:
                concentration-=1
            break
    return concentration,level


def capacity(c):
    if c[23]<=0:
        return 0,0
    if c[24]>16:
        raise Boundary('hull slot extent')
    if c[25]!=128 or c[26]>6:
        return 0,0
    local=signed(c[24]*c[27]*c[28],32)
    subtotal=checked(local*c[23],'fleet mining multiplication')
    total=signed(subtotal*(16 if c[0]==2 else 1),32)
    return min(4000,total),c[24]*(16 if c[0]==2 else 1)


def reference(c):
    if c[0]:
        value,lookups=capacity(c)
        return [value,lookups]
    stock=c[12:15].copy();conc=c[15:18].copy();level=c[18:21].copy();out=[777]*3
    marks=[c[29+i*8+6] for i in range(4)];draws=0;mine_calls=0;part_calls=0;oper_calls=0;seed1=7;seed2=11
    def draw():
        nonlocal draws,seed1,seed2
        draws+=1
        if not c[10]:
            return c[11]
        k=div(seed1,53668)
        seed1=signed((seed1-k*53668)*40014-k*12211,32)
        if seed1<0:seed1+=2147483563
        k=div(seed2,52774)
        seed2=signed((seed2-k*52774)*40692-k*3791,32)
        if seed2<0:seed2+=2147483399
        z=seed1-seed2
        if z<1:z+=2147483562
        return z%100
    def axes(mines,efficiency,remote):
        values=[]
        for i in range(3):
            effective=conc[i]
            if effective<30 and c[7] and (not remote or (c[4]!=-1 and c[6]==8)):
                effective=30
            actual=checked(mines*effective,'mine/concentration product')
            scaled=actual if remote else div(checked(actual*efficiency,'mine efficiency product'),10)
            whole=div(scaled,100);fraction=scaled-whole*100
            if fraction and c[9]:
                if draw()<fraction:whole+=1
            values.append(whole)
            if c[8]:
                stock[i]=checked(stock[i]+whole,'mineral stock addition')
                conc[i],level[i]=deplete(conc[i],level[i],max(0,div(actual,100)))
        return values
    if c[1]==-1:
        if c[4]==-1 or c[5]==0:
            out=[-1]*3
        else:
            oper_calls=1
            if c[6]==8:
                if c[5]<0:raise Boundary('operating negative root')
                mines=signed(math.trunc(math.sqrt(float(c[5]))),16)
            else:
                mines=min(4095,c[2])
            out=axes(mines,10 if c[6]==8 else c[3],False)
            if c[6]==8:
                for i in range(c[21]):
                    if i==c[22]:break
                    f=c[29+8*i:37+8*i]
                    if f[0]!=0 or f[1]!=c[4] or f[2] or f[3]>1:continue
                    if f[4]<0:raise Boundary('first waypoint dereference')
                    if f[4]!=3:continue
                    mine_calls+=1
                    if f[7]:
                        quantity,calls=capacity(c);part_calls+=calls
                    else:quantity=f[5]
                    if quantity<=0:continue
                    child=axes(quantity,10,True)
                    out=[checked(out[k]+child[k],'remote quantity addition') for k in range(3)]
                    if c[8]:marks[i]=0
    else:
        out=axes(c[1],10,True)
    return out+stock+conc+level+marks+[draws,mine_calls,part_calls,oper_calls,seed1,seed2]


def base():
    # 29 scalar fields plus four eight-field native fleet projections.
    return [0,-1,100,10,0,10000,1,0,1,0,0,0,0,0,0,100,50,1,0,128,255,0,-1,1,1,128,0,1,5]+sum(
        ([0,0,0,1,3,100,1,0] for _ in range(4)),[])


def cases():
    # Every stored concentration/level pair, with both branches and demand scales.
    for concentration in range(256):
        for level in range(256):
            for mines in (1,100,4000):
                c=base();c[1]=mines;c[15:18]=[concentration]*3;c[18:21]=[level]*3;yield c
    for efficiency in range(-128,128):
        for mines in (-32768,-1,0,1,10,32767):
            for generating in (0,1):
                c=base();c[2]=mines;c[3]=efficiency;c[9]=generating;c[11]=40;yield c
    for ar in (1,8):
        for owner in (-1,0):
            for population in (0,1,10000,-1):
                for count in (-2,-1,0,1,4000):
                    for home in (0,1):
                        for apply in (0,1):
                            c=base();c[1]=count;c[4]=owner;c[5]=population;c[6]=ar;c[7]=home;c[8]=apply;c[9]=1;c[15:18]=[0,1,29];yield c
    # All short-circuit fleet fields and first-hole behavior, including signed cord.
    for apply in (0,1):
        for cord in (-32768,-1,0,1,2,32767):
            for task in (-1,0,3,15):
                for owner in (0,1):
                    for planet in (0,1):
                        for dead in (0,1):
                            for amount in (-1,0,1,4000):
                                for hole in (-1,0,1):
                                    c=base();c[6]=8;c[8]=apply;c[21]=2;c[22]=hole;c[29:37]=[planet,owner,dead,cord,task,amount,1,0];yield c
    for rng_mode in (0,1):
        for draw_value in range(100):
            c=base();c[6]=8;c[9]=1;c[10]=rng_mode;c[11]=draw_value;c[21]=4;c[15:18]=[100,99,50];c[18:21]=[1,1,1]
            for i in range(4):c[29+8*i+5]=1+i*99
            yield c
            c=c.copy();c[8]=0;yield c
    rng=random.Random(0xDE91E7E)
    for _ in range(12000):
        c=base();c[1]=rng.choice([-1,-2147483648,-2,0,1,4000,2147483647]);c[2]=rng.randrange(-32768,32768);c[3]=rng.randrange(-128,128)
        c[4]=rng.choice([-1,0]);c[5]=rng.choice([0,1,10000,2147483647]);c[6]=rng.choice([1,8]);c[7:11]=[rng.randrange(2) for _ in range(4)]
        c[11]=rng.randrange(100);c[12:15]=[rng.choice([-2147483648,-1,0,10000,2147483647]) for _ in range(3)]
        c[15:21]=[rng.randrange(256) for _ in range(6)];c[21]=rng.randrange(5);c[22]=rng.choice([-1,0,1,2,3]);yield c
    for mode in (1,2):
        for extent in (0,1,16,17,255):
            for ships in (-32768,-1,0,1,32767):
                for kind in (0,128,129):
                    for item in (0,6,7,255):
                        for count in (0,1,255):
                            for ability in (-32768,-1,0,1,5,32767):
                                c=base();c[0]=mode;c[23:29]=[ships,extent,kind,item,count,ability];yield c
    # Actual CMineFromLpfl contributes to recursive AR mining, not just standalone.
    for ships in (1,10,32767):
        c=base();c[6]=8;c[21]=2;c[23]=ships;c[24]=16;c[27]=255;c[28]=25
        for i in range(2):c[29+8*i+7]=1
        yield c
    for reverse in (False,True):
        c=base();c[6]=8;c[21]=2;c[15:18]=[100]*3;c[18:21]=[0]*3
        c[34],c[42]=(4000,100) if reverse else (100,4000)
        yield c
    c=base();c[2]=10;c[3]=-1;c[15:18]=[1]*3;c[9]=1;yield c


def main():
    declarations=r'''
#include "common.h"
#include <assert.h>
#include <stdlib.h>
#include <inttypes.h>
static int32_t cfg[61];static PLANET planet;PLAYER rgplr[16];GDATA gd;
static FLEET fleets[4];static FLEET *fleetPointers[4];FLEET **rglpfl=fleetPointers;int16_t cFleet;
static union {max_align_t align;uint8_t bytes[sizeof(PLORD)+sizeof(ORDER)];} orderStorage[4];static PLORD *orders[4];
static SHDEF designs[16];SHDEF *rglpshdef[16];static MINING mining[8];
int32_t lRandSeed1,lRandSeed2;static int draws,mineCalls,partCalls,operCalls;
static int16_t NativeRandom(int16_t bound);static int32_t NativeCMineFromLpfl(FLEET *fleet);
int16_t FLookupPart(PART *part){assert(part->hs.grhst==hstMining && part->hs.iItem<=iminingAlienMiner);partCalls++;part->pmining=&mining[part->hs.iItem];return TRUE;}
int32_t CMineFromLpfl(FLEET *fleet){mineCalls++;int i=(int)(fleet-fleets);assert(0<=i && i<4);return cfg[29+8*i+7] ? NativeCMineFromLpfl(fleet):cfg[29+8*i+5];}
int16_t CMaxOperableMines(PLANET *p,int16_t player,int16_t next){(void)p;assert(player==cfg[4] && !next);return (int16_t)cfg[2];}
int16_t Random(int16_t bound){assert(bound==100);draws++;return cfg[10] ? NativeRandom(bound):(int16_t)cfg[11];}
'''
    getter=next(line for line in (ROOT/'src/core/race.c').read_text().splitlines() if line.startswith('int16_t GetRaceStat('))
    bodies=getter+'\n'+function('src/core/planet.c','int16_t CMinesOperating(').replace('    iplr =', '    operCalls++;\n    iplr =',1)+'\n'
    bodies+=function('src/core/ship2.c','int32_t CMineFromLpfl(').replace('int32_t CMineFromLpfl(', 'static int32_t NativeCMineFromLpfl(',1)+'\n'
    bodies+=function('src/core/mine.c','void EstMineralsMined(')+'\n'
    bodies+=function('src/core/utilgen.c','int16_t STARS_TEST_IMPL(Random)(').replace('int16_t STARS_TEST_IMPL(Random)(', 'static int16_t NativeRandom(',1)+'\n'
    harness=r'''
static void setup(void){
    memset(&planet,0,sizeof(planet));memset(rgplr,0,sizeof(rgplr));memset(&gd,0,sizeof(gd));memset(designs,0,sizeof(designs));
    memset(fleets,0,sizeof(fleets));memset(orderStorage,0,sizeof(orderStorage));for(int i=0;i<4;i++)orders[i]=(PLORD *)orderStorage[i].bytes;draws=mineCalls=partCalls=operCalls=0;lRandSeed1=7;lRandSeed2=11;
    planet.iPlayer=(int16_t)cfg[4];planet.rgwtMin[3]=cfg[5];planet.cMines=4095;planet.fHomeworld=cfg[7];gd.fGeneratingTurn=cfg[9];cFleet=(int16_t)cfg[21];
    for(int i=0;i<16;i++){rgplr[i].rgAttr[rsMajorAdv]=(int8_t)cfg[6];rgplr[i].rgAttr[rsMineProd]=(int8_t)cfg[3];rglpshdef[i]=designs;
        designs[i].hul.chs=(uint8_t)cfg[24];for(int j=0;j<16;j++){designs[i].hul.rghs[j].grhst=(HullSlotType)cfg[25];designs[i].hul.rghs[j].iItem=cfg[26];designs[i].hul.rghs[j].cItem=cfg[27];}}
    for(int i=0;i<8;i++)mining[i].grAbility=(int16_t)cfg[28];
    for(int i=0;i<3;i++){planet.rgwtMin[i]=cfg[12+i];planet.rgMinConc[i]=(uint8_t)cfg[15+i];planet.rgpctMinLevel[i]=(uint8_t)cfg[18+i];}
    for(int i=0;i<4;i++){int at=29+8*i;fleets[i].idPlanet=(int16_t)cfg[at];fleets[i].iPlayer=(int16_t)cfg[at+1];fleets[i].fDead=cfg[at+2];
        fleets[i].cord=(int16_t)cfg[at+3];fleets[i].lpplord=cfg[at+4]<0 ? NULL:orders[i];orders[i]->rgord[0].grTask=cfg[at+4]<0 ? 0:cfg[at+4];fleets[i].fHereAllTurn=cfg[at+6];
        for(int j=0;j<16;j++)fleets[i].rgcsh[j]=(int16_t)((j==0 || cfg[0]==2) ? cfg[23]:0);rglpfl[i]=i==cfg[22] ? NULL:&fleets[i];}
}
int main(int argc,char **argv){assert(argc==2);FILE *f=fopen(argv[1],"rb");assert(f);unsigned long cases=0,checks=0;int64_t expected[22];
    while(fread(cfg,sizeof(cfg),1,f)==1){unsigned n=cfg[0] ? 2:22;assert(fread(expected,sizeof(*expected),n,f)==n);setup();int64_t actual[22];
        if(cfg[0]){actual[0]=NativeCMineFromLpfl(&fleets[0]);actual[1]=partCalls;}else{int32_t result[3]={777,777,777};EstMineralsMined(&planet,result,cfg[1],(int16_t)cfg[8]);
            for(int i=0;i<3;i++){actual[i]=result[i];actual[3+i]=planet.rgwtMin[i];actual[6+i]=planet.rgMinConc[i];actual[9+i]=planet.rgpctMinLevel[i];}
            for(int i=0;i<4;i++)actual[12+i]=fleets[i].fHereAllTurn;
            actual[16]=draws;actual[17]=mineCalls;actual[18]=partCalls;actual[19]=operCalls;actual[20]=lRandSeed1;actual[21]=lRandSeed2;
        }
        for(unsigned i=0;i<n;i++){checks++;if(actual[i]!=expected[i]){fprintf(stderr,"case %lu mode %d field %u actual %lld expected %lld (count %d conc %d level %d)\n",cases,cfg[0],i,(long long)actual[i],(long long)expected[i],cfg[1],cfg[15],cfg[18]);return 1;}}
        cases++;
    }
    assert(feof(f));fclose(f);printf("Native mining/depletion: %lu cases, %lu assertions passed\n",cases,checks);return 0;
}
'''
    with tempfile.TemporaryDirectory(prefix='mining-contract-') as work:
        path=Path(work);data=path/'vectors.bin';excluded={};count=0
        with data.open('wb') as stream:
            for c in cases():
                assert len(c)==61
                try:result=reference(c)
                except Boundary as why:
                    excluded[str(why)]=excluded.get(str(why),0)+1;continue
                stream.write(struct.pack('<61i',*c));stream.write(struct.pack('<'+'q'*len(result),*result));count+=1
        print(f'Prepared {count} defined mining vectors; excluded before C: {excluded}',flush=True)
        for generator in ['text/textgen.py','data/datagen.py']:
            subprocess.run([sys.executable,str(ROOT/generator),str(path)],check=True,stdout=subprocess.DEVNULL)
        source=path/'witness.cpp';source.write_text(declarations+bodies+harness);binary=path/'witness'
        subprocess.run(shlex.split(os.environ.get('CXX','c++'))+['-std=gnu++20','-x','c++','-Wall','-Wextra','-Werror',
            '-fsanitize=address,undefined','-fno-sanitize-recover=all','-ffp-contract=off','-fno-fast-math',
            '-I'+str(ROOT/'src/core'),'-I'+str(ROOT/'src/platform'),'-I'+str(path),str(source),'-lm','-o',str(binary)],check=True)
        subprocess.run([str(binary),str(data)],check=True)


if __name__=='__main__':main()
