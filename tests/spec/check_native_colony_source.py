#!/usr/bin/env python3
"""Native capacity/resources/installation/growth differential fixtures.

Actual common.h structs, fresh IDs, and actual habitat/race/planet/growth bodies
execute against an independent width-aware numeric interpreter. A second habitat
mode supplies raw signed helper results; growth can also be a declared provider.
Undefined vectors are filtered before C, not counted as native error returns.
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


class Boundary(Exception):
    pass


def checked(n, why):
    if not -(2**31) <= n < 2**31:
        raise Boundary(why)
    return n


def signed(n, width):
    n %= 2**width
    return n if n < 2**(width-1) else n-2**width


def div(n, d):
    if not d or (n == -(2**31) and d == -1):
        raise Boundary('division')
    return (abs(n)//abs(d)) * (-1 if (n<0)!=(d<0) else 1)


def fields(c, player):
    if player not in (0,1):
        raise Boundary('player index')
    return c[8:16] if player==0 else c[29:37]


def habitat(c, stored=False):
    if c[17]:
        return c[16]
    x=c[19 if stored else 18]
    if x<0:
        return -3*min(15,-x)
    if x>100:
        return -3*min(15,x-100)
    distance=abs(x-50); q=100-distance*2; modifier=10000;penalty=distance*2-50
    if penalty>0:
        for _ in range(3):
            modifier=modifier*(100-penalty)//100
    return int(math.sqrt(float(3*q*q)/3.0)+0.9)*modifier//10000


def capacity(c, player):
    primary,_,_,_,_,_,_,obrm=fields(c,player)
    if primary==8:
        if c[27]!=player or not c[28]:
            return 0
        relative=signed(c[22],16)
        if c[21]>=10 or not 0<=relative<5:
            raise Boundary('base/population index')
        value=[2500,5000,10000,20000,30000][relative]
    else:
        hab=habitat(c,True);value=500 if hab<5 else hab*100
        if primary==0:
            value-=value//2
        elif primary==9:
            value+=value//5
    return value+(value//10 if obrm else 0)


def growth(c, update):
    pop,delta,owner=c[1],c[2],c[6]
    if owner==-1 or pop==0:
        return 0,pop,delta
    primary,_,_,_,_,ideal,_,_=fields(c,owner);hab=habitat(c)
    if hab<0:
        loss=max(1,div(checked(pop*signed(-hab,16),'hostile product'),10))
        whole=div(loss,100);fraction=loss-whole*100;outdelta=delta-fraction
        if outdelta<0:
            whole+=1;outdelta+=100
        change=-whole
    else:
        maximum=capacity(c,owner);rate=signed((ideal*2 if primary==0 else ideal)*hab,16)
        if pop>div(maximum,4):
            fullness=div(checked(pop*1000,'fullness product'),maximum)
            if pop>=maximum:
                if pop<checked(maximum+10,'dead band'):
                    return 0,pop,delta
                rate=max(-300,99-div(fullness,10))*4
            else:
                retard=checked(1000-fullness,'retardation');square=checked(retard*retard,'retardation square')
                product=checked((rate if rate<1000 else div(rate,10))*square,'crowding product')
                rate=checked(div(product,562500)*(1 if rate<1000 else 10),'crowding rescale')
        rough=checked(pop*div(rate,100),'rough growth product')
        amount=div(checked(pop*rate,'fine growth product'),100) if rough<10000000 else rough
        whole=div(amount,100);fraction=amount-whole*100
        if whole==0 and fraction==0:
            fraction=1
        outdelta=signed(delta+fraction,16)
        if outdelta>=100:
            whole+=1;outdelta-=100
        elif outdelta<0:
            whole-=1;outdelta+=100
        change=whole
    if update:
        return change,checked(pop+change,'population writeback'),outdelta%256
    return change,pop,delta


def absolute(c, player, factories):
    primary,_,_,factop,mineop,_,_,_=fields(c,player)
    count=max(10,div(checked(capacity(c,player)*(factop if factories else mineop),'absolute product'),100))
    return 0 if primary==8 else signed(count,16)


def operable(c, player, factories):
    maximum=absolute(c,player,factories);primary,_,_,factop,mineop,_,_,_=fields(c,player);pop=c[1]
    if c[23]:
        extra=c[26] if c[25] else growth(c,False)[0]
        pop=checked(pop+extra,'next population')
    current=div(checked(pop*(factop if factories else mineop),'operable product'),100)
    count=maximum if maximum<current else signed(current,16)
    return 0 if primary==8 else max(1,count)


def defense(c, player, next_year=False):
    primary=fields(c,player)[0];maximum=0 if primary==8 else min(100,max(10,habitat(c)*4))
    if not next_year:
        return maximum
    pop=c[1]
    if c[23]:
        pop=checked(pop+(c[26] if c[25] else growth(c,False)[0]),'next population')
    current=min(1000,div(checked(pop+24,'defense rounding'),25));word=signed(current,16)
    return 0 if primary==8 else min(maximum,word)


def effective(pop, maximum):
    if pop<=maximum:
        return pop
    half=checked(div(checked(pop-maximum,'effective subtraction'),2)+maximum,'effective addition')
    return min(half,checked(maximum*2,'effective cap'))


def floating(value):
    if not math.isfinite(value) or not -(2**31)<=math.trunc(value)<2**31:
        raise Boundary('float-to-int')
    return math.trunc(value),struct.unpack('<Q',struct.pack('<d',value))[0]


def reference(c):
    op,pop,delta,owner,player=c[0],c[1],c[2],c[6],c[7];bits=0
    if op==0:
        result=capacity(c,player)
    elif op in (1,2):
        result=absolute(c,player,op==2)
    elif op in (3,4):
        result=operable(c,player,op==4)
    elif op in (5,6):
        if owner==-1:
            result=0
        elif fields(c,owner)[0]==8:
            if op==6:
                result=0
            else:
                if pop<0:
                    raise Boundary('negative square root')
                value,bits=floating(math.sqrt(float(pop)));result=signed(value,16)
        else:
            now=c.copy();now[23]=0;result=min(c[3 if op==5 else 4],operable(now,owner,op==6))
    elif op in (7,8):
        result=defense(c,player,op==8)
    elif op==9:
        if pop==0:
            return 0,pop,delta,0
        primary,eff,prod,_,_,_,energy,_=fields(c,player);q=effective(pop,capacity(c,player))
        if primary==8:
            if not eff:
                raise Boundary('float division zero')
            radicand=float(q)*max(1,energy)/eff
            if radicand<0:
                raise Boundary('negative square root')
            value,bits=floating(math.sqrt(radicand)*max(25,habitat(c))/10+0.999)
            result=signed(value,16)
        else:
            colonists=signed(div(q,eff),16);now=c.copy();now[23]=0;facts=min(c[4],operable(now,player,True))
            contribution=div(signed(checked(facts*prod,'factory product')+9,32),10)
            result=signed(colonists+contribution,16)
        if not result:
            result=1
    elif op==10:
        maximum=capacity(c,player)
        if maximum<=0:
            result=0
        else:
            numerator=signed(checked(pop*100,'capacity display product')+div(maximum,2),32)
            percentage=div(numerator,maximum);result=999 if percentage>=1000 else signed(percentage,16)
    else:
        result,pop,delta=growth(c,bool(c[24]))
    return result,pop,delta,bits


def base():
    # op,pop,delta,M/F/D,owner/query, player0 eight fields, habitat/mode,
    # live/stored environment, base/slot/hull, next/update, growth mode/value,
    # lookup owner/base, player1 eight fields.
    return [0,10000,37,500,1000,50,0,0,1,10,10,10,10,10,10,0,100,0,50,50,0,0,0,0,0,0,0,0,0,
            9,7,15,12,13,15,20,1]


def cases():
    for op in range(12):
        c=base();c[0]=op;yield c
    for hab in range(-32768,32768):
        for op in (0,7,8):
            c=base();c[0]=op;c[16]=hab;c[17]=1;yield c
    for raw in range(-128,128):
        for pop in (-2147483648,-327690,-1,0,1,10000,655360,2147483647):
            for op in (1,2,3,4,9):
                c=base();c[0]=op;c[1]=pop;c[9]=raw;c[10]=raw;c[11]=raw;c[12]=raw;yield c
    for delta in range(256):
        for pop in (1,99,100,2500,10000,10005,10010,100000,3000000):
            for hab in (-45,-1,0,100):
                for update in (0,1):
                    c=base();c[0]=11;c[1]=pop;c[2]=delta;c[16]=hab;c[17]=1;c[24]=update;yield c
    for primary in (0,1,8,9):
        for pop in (0,1,2,4,100,1600,32768,65536,100000,2147483647):
            for eff in (-128,-1,0,1,10,127):
                for energy in (-128,0,1,127):
                    for op in (5,6,9):
                        c=base();c[0]=op;c[1]=pop;c[8]=primary;c[9]=eff;c[14]=energy;c[20]=c[28]=1;yield c
    for primary in (0,1,8,9):
        for owner in (-1,0,1):
            for query in (0,1):
                for slot in (0,9,10,15):
                    for hull in (-1,0,1,4,5,32767,-32768):
                        for op in (0,1,2,9):
                            c=base();c[0]=op;c[6]=c[27]=owner;c[7]=query;c[8]=c[29]=primary;c[20]=c[28]=1;c[21]=slot;c[22]=hull;yield c
    rng=random.Random(0xC0107)
    for _ in range(12000):
        c=base();c[0]=rng.randrange(12);c[1]=rng.choice([0,1,100,10000,21474836,1000000,-1000024]);c[2]=rng.randrange(256)
        c[3:6]=[rng.randrange(4096) for _ in range(3)];c[8]=rng.choice([0,1,8,9]);c[9:15]=[rng.randrange(-128,128) for _ in range(6)]
        c[18]=rng.randrange(-128,128);c[19]=rng.randrange(-128,128);c[20]=c[28]=rng.randrange(2);c[22]=rng.randrange(5);c[23]=rng.randrange(2);c[24]=rng.randrange(2);yield c
    c=base();c[0]=9;c[1]=0;c[7]=-1;yield c # source early return before player access
    c=base();c[0]=9;c[1]=1600;c[8]=8;c[9]=c[14]=1;c[16]=16384;c[17]=1;c[20]=c[28]=1;yield c
    c=base();c[0]=9;c[9]=1;c[10]=c[11]=127;c[4]=4095;yield c
    c=base();c[0]=9;c[1]=65536;c[9]=1;c[10]=0;c[4]=0;c[16]=1000;c[17]=1;yield c
    c=base();c[0]=10;c[1]=21474836;yield c
    for hab in (-32768,-32767,-100,-1,0,100,32767):
        for update in (0,1):
            c=base();c[0]=11;c[1]=1;c[16]=hab;c[17]=1;c[24]=update;yield c
    for growth_value in (-2147483648,-1,0,1,100000,2147483647):
        for op in (3,4,8):
            c=base();c[0]=op;c[23]=c[25]=1;c[26]=growth_value;yield c
    # Query-player installation rates and owner-player growth are distinct.
    for owner in (0,1):
        for player in (0,1):
            for next_year in (0,1):
                for op in (3,4,5,6,8,9,11):
                    c=base();c[0]=op;c[1]=2500;c[6]=c[27]=owner;c[7]=player;c[23]=next_year;c[24]=1;yield c


def main():
    declarations=r'''
#include "common.h"
#include <assert.h>
#include <stdlib.h>
#include <inttypes.h>
PLAYER rgplr[16];static PLANET planet,stored;static SHDEF bases[2][10];SHDEF *rglpshdefSB[16];int16_t idPlayer;
/* SOURCE_POPULATION_TABLE */
static int32_t cfg[37];static int stats,lookups,growths,grbits;static uint64_t floatBits;
static int16_t NativeHabitat(PLANET *p,int16_t player);
static int32_t NativeGrowth(PLANET *p,int16_t update);
static double RecordFloat(double x){memcpy(&floatBits,&x,8);return x;}
int16_t FLookupPlanet(int16_t id,PLANET *out){assert(id==0);lookups++;*out=stored;return TRUE;}
int16_t PctPlanetDesirability(PLANET *p,int16_t player){assert(player==0 || player==1);return cfg[17] ? (int16_t)cfg[16]:NativeHabitat(p,player);}
int32_t ChgPopFromPlanet(PLANET *p,int16_t update){growths++;if(cfg[25]){assert(!update);return cfg[26];}return NativeGrowth(p,update);}
'''
    population_table=next(line for line in (ROOT/'src/core/globals.c').read_text().splitlines()
                          if line.startswith('int32_t') and 'rglPopMac[5]' in line)
    declarations=declarations.replace('/* SOURCE_POPULATION_TABLE */',population_table)
    getter=next(line for line in (ROOT/'src/core/race.c').read_text().splitlines() if line.startswith('int16_t GetRaceStat('))
    getter=getter.replace('return pplr->rgAttr[iStat];','stats++; return pplr->rgAttr[iStat];')
    grbit=function('src/core/race.c','int16_t GetRaceGrbit(').replace('    if (UINT32_C(1)', '    grbits++;\n    if (UINT32_C(1)',1)
    bodies=getter+'\n'+grbit+'\n'+function('src/core/race.c','int16_t PctTrueMaxGrowth(')+'\n'
    bodies+=function('src/core/planet.c','int16_t PctPlanetDesirability(').replace('int16_t PctPlanetDesirability(', 'static int16_t NativeHabitat(',1)+'\n'
    for signature in ['int32_t CalcPlanetMaxPop(','int16_t CMaxMines(','int16_t CMaxFactories(',
                      'int16_t CMaxOperableMines(','int16_t CMaxOperableFactories(','int16_t CMaxDefenses(',
                      'int16_t CMaxOperableDefenses(','int16_t CFactoriesOperating(','int16_t CMinesOperating(',
                      'int16_t CResourcesAtPlanet(','int16_t STARS_TEST_IMPL(PctPlanetCapacity)(']:
        body=function('src/core/planet.c',signature)
        if signature.startswith('int16_t CMinesOperating'):
            body=body.replace('(int32_t)sqrt((double)lppl->rgwtMin[3])','(int32_t)RecordFloat(sqrt((double)lppl->rgwtMin[3]))')
        if signature.startswith('int16_t CResourcesAtPlanet'):
            expression='(double)sqrt((double)((double)lPop * iEnergy / iEff)) * pctVal / 10 + 0.999'
            assert body.count(expression)==1
            body=body.replace('(int32_t)('+expression+')','(int32_t)RecordFloat('+expression+')')
        bodies+=body+'\n'
    bodies+=function('src/core/util.c','int32_t ChgPopFromPlanet(').replace('int32_t ChgPopFromPlanet(', 'static int32_t NativeGrowth(',1)
    harness=r'''
static void setup(void){
    memset(&planet,0,sizeof(planet));memset(rgplr,0,sizeof(rgplr));memset(bases,0,sizeof(bases));
    for(int player=0;player<2;player++){
        int at=player ? 29:8;rgplr[player].rgAttr[rsMajorAdv]=(int8_t)cfg[at];rgplr[player].rgAttr[rsResGen]=(int8_t)cfg[at+1];
        rgplr[player].rgAttr[rsFactProd]=(int8_t)cfg[at+2];rgplr[player].rgAttr[rsFactOperate]=(int8_t)cfg[at+3];rgplr[player].rgAttr[rsMineOperate]=(int8_t)cfg[at+4];
        rgplr[player].pctIdealGrowth=(int8_t)cfg[at+5];rgplr[player].rgTech[0]=(int8_t)cfg[at+6];rgplr[player].grbitAttr=static_cast<RaceTraitBits>(cfg[at+7] ? static_cast<uint32_t>(grbitRaceOBRM):0);
        for(int axis=0;axis<3;axis++){rgplr[player].rgEnvVarMin[axis]=0;rgplr[player].rgEnvVar[axis]=50;rgplr[player].rgEnvVarMax[axis]=100;}
        rglpshdefSB[player]=bases[player];for(int i=0;i<10;i++)bases[player][i].hul.ihuldef=(HulDef)(ihuldefOrbitalFort+cfg[22]);
    }
    planet.rgwtMin[3]=cfg[1];planet.iDeltaPop=cfg[2];planet.cMines=cfg[3];planet.cFactories=cfg[4];planet.cDefenses=cfg[5];planet.iPlayer=(int16_t)cfg[6];
    planet.fStarbase=cfg[20];planet.isb=cfg[21];for(int i=0;i<3;i++)planet.rgEnvVar[i]=(int8_t)cfg[18];
    stored=planet;stored.iPlayer=(int16_t)cfg[27];stored.fStarbase=cfg[28];for(int i=0;i<3;i++)stored.rgEnvVar[i]=(int8_t)cfg[19];
    stats=lookups=growths=grbits=0;floatBits=0;idPlayer=(int16_t)cfg[7];
}
int main(int argc,char **argv){
    assert(argc==2 && sizeof(double)==8);FILE *f=fopen(argv[1],"rb");assert(f);int64_t expected[3];uint64_t bits;unsigned long cases=0,checks=0;
    while(fread(cfg,sizeof(cfg),1,f)==1){assert(fread(expected,sizeof(expected),1,f)==1 && fread(&bits,sizeof(bits),1,f)==1);setup();int64_t result;
        switch(cfg[0]){
        case 0:result=CalcPlanetMaxPop(0,(int16_t)cfg[7]);break;
        case 1:result=CMaxMines(&planet,(int16_t)cfg[7]);break;
        case 2:result=CMaxFactories(&planet,(int16_t)cfg[7]);break;
        case 3:result=CMaxOperableMines(&planet,(int16_t)cfg[7],(int16_t)cfg[23]);break;
        case 4:result=CMaxOperableFactories(&planet,(int16_t)cfg[7],(int16_t)cfg[23]);break;
        case 5:result=CMinesOperating(&planet);break;
        case 6:result=CFactoriesOperating(&planet);break;
        case 7:result=CMaxDefenses(&planet,(int16_t)cfg[7]);break;
        case 8:result=CMaxOperableDefenses(&planet,(int16_t)cfg[7],(int16_t)cfg[23]);break;
        case 9:result=CResourcesAtPlanet(&planet,(int16_t)cfg[7]);break;
        case 10:result=PctPlanetCapacity(&planet);break;
        default:result=ChgPopFromPlanet(&planet,(int16_t)cfg[24]);break;
        }
        int64_t actual[]={result,planet.rgwtMin[3],planet.iDeltaPop};
        for(int i=0;i<3;i++){checks++;if(actual[i]!=expected[i]){fprintf(stderr,"case %lu op %d field %d actual %lld expected %lld (pop %d, rate %d)\n",cases,cfg[0],i,(long long)actual[i],(long long)expected[i],cfg[1],cfg[12]);return 1;}}
        checks++;if(floatBits!=bits){fprintf(stderr,"case %lu op %d float bits %" PRIu64 " expected %" PRIu64 "\n",cases,cfg[0],floatBits,bits);return 1;}
        assert(growths==(cfg[0]==11 || ((cfg[0]==3 || cfg[0]==4 || cfg[0]==8) && cfg[23])));checks++;
        if(cfg[0]==9 && cfg[1]==0){assert(!stats && !lookups && !growths);checks++;}
        if(cfg[0]==0){int primary=rgplr[cfg[7]].rgAttr[rsMajorAdv];
            assert(stats==(primary==8 ? 1:primary==0 ? 2:3) && lookups==1);
            assert(grbits==(primary!=8 || (stored.iPlayer==cfg[7] && stored.fStarbase)));checks+=2;}
        cases++;
    }
    assert(feof(f));fclose(f);printf("Native colony source differential: %lu cases, %lu assertions passed\n",cases,checks);return 0;
}
'''
    with tempfile.TemporaryDirectory(prefix='native-colony-') as work:
        path=Path(work);source=path/'witness.cpp';binary=path/'witness';data=path/'vectors.bin';excluded={};count=0
        with data.open('wb') as stream:
            for c in cases():
                assert len(c)==37
                try:
                    result=reference(c)
                except Boundary as why:
                    excluded[str(why)]=excluded.get(str(why),0)+1
                    continue
                stream.write(struct.pack('<37i3qQ',*c,*result));count+=1
        print(f'Prepared {count} defined native colony vectors; excluded before C: {excluded}',flush=True)
        for generator in ['text/textgen.py','data/datagen.py']:
            subprocess.run([sys.executable,str(ROOT/generator),str(path)],check=True,stdout=subprocess.DEVNULL)
        source.write_text(declarations+bodies+harness)
        subprocess.run(shlex.split(os.environ.get('CXX','c++'))+['-std=gnu++20','-DSTARS_TEST_STATE_DOUBLE','-include',str(ROOT/'src/platform/value.h'),'-x','c++','-Wall','-Wextra','-Werror','-Wno-misleading-indentation',
            '-Wno-unused-variable','-Wno-unused-but-set-variable','-fsanitize=address,undefined','-fno-sanitize-recover=all',
            '-ffp-contract=off','-fno-fast-math','-iquote',str(ROOT/'src/core'),'-iquote',str(ROOT/'src/platform'),'-iquote',str(path),str(source),'-lm','-o',str(binary)],check=True)
        subprocess.run([str(binary),str(data)],check=True)


if __name__=='__main__':
    main()
