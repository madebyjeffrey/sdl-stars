#!/usr/bin/env python3
"""Actual research quote/preview controllers with native records and width-aware reference.

Cost getters and scalar cost body are actual source; preview resource/estimator
callbacks are explicit post-state providers, including controlled escapes.
Undefined arithmetic/index candidates are excluded before C execution.
"""
from pathlib import Path
import os
import random
import shlex
import struct
import subprocess
import sys
import tempfile
from check_s02_source import ROOT, function
from check_native_colony_source import Boundary, checked, signed, div

TECH=[0,50,80,130,210,340,550,890,1440,2330,3770,6100,9870,13850,18040,22440,27050,31870,36900,42140,47590,53250,59120,65200,71490,77990,84700]


def mix(hash_value,*values):
    for value in values:hash_value=((hash_value*16777619)^(value&0xffffffff))&0xffffffff
    return hash_value


def reference(c):
    result=[0]*22;levels=c[4:10].copy();choices=c[10:16];hash_value=0;calls=0
    def cost(field,level):
        nonlocal hash_value,calls
        if not 0<=field<6 or not 0<=level<=26:raise Boundary('cost field/table index')
        base=signed(sum(levels)*10,16)+TECH[level]
        choice=choices[field];value=2*base-base//4 if choice<1 else base if choice==1 else div(base,2)
        value=checked(value*(2 if c[1] else 1),'cost arithmetic')
        calls+=1;hash_value=mix(hash_value,field,level,sum(levels),value)
        return value
    if c[0]==0:
        result[0]=cost(c[2],c[3])
    elif c[0]==1:
        targets=c[16:22]
        if any(level>26 for level in targets):result[0]=-1
        else:
            original=levels.copy();total=0
            for field in range(6):
                if targets[field]>levels[field]:
                    stored=signed(c[22+field],32)
                    scaled=checked(stored*(2 if c[1] else 1),'slow stored-spending multiplication')
                    current=checked(-scaled,'development spent negation')
                    while targets[field]>levels[field]:
                        current=checked(current+cost(field,levels[field]+1),'development field addition')
                        levels[field]+=1
                    total=checked(total+max(0,current),'development total addition')
            levels=original;result[0]=total
    else:
        player=c[29];alloc=[signed(c[30],8),signed(c[31],8)]+[0]*14
        if not 0<=player<16:raise Boundary('preview entry player index')
        saved=alloc[player];alloc[player]=signed(c[28],8);count=c[32];global_count=count
        if not 0<=count<=32767:raise Boundary('planet extent')
        planets=[c[38+5*(i%4):43+5*(i%4)].copy() for i in range(count)]
        gross_calls=forecast_calls=0;complete=True;total=0;mutated=False
        def callback(kind,index,argument):
            nonlocal player,calls,hash_value,mutated,global_count,gross_calls,forecast_calls,complete
            calls+=1;hash_value=mix(hash_value,kind,index,argument,alloc[0],alloc[1])
            if kind==1:gross_calls+=1
            else:forecast_calls+=1
            if (kind==1 or c[34]&128) and not mutated:
                if c[34]&1:player=c[35]
                if c[34]&2:
                    if not 0<=player<16:raise Boundary('callback player index')
                    alloc[player]=signed(c[36],8)
                if c[34]&4 and planets[index][4]!=-2:planets[index][1]=planets[index][4]
                if c[34]&8:global_count=0
                if c[34]&16 and index+1<count:planets[index+1][0]=player
                mutated=True
            if calls-1==c[33]:complete=False;return None
            return planets[index][2 if kind==1 else 3]
        for index in range(count):
            if planets[index][0]!=player:continue
            value=callback(1,index,player)
            if not complete:break
            if planets[index][1]>0:
                value=callback(2,index,player)
                if not complete:break
            total=checked(total+value,'preview total addition')
        if complete:
            if not 0<=player<16:raise Boundary('preview restore player index')
            alloc[player]=saved
        result[:10]=[total,alloc[0],alloc[1],player,calls,gross_calls,forecast_calls,hash_value,int(complete),global_count]
        for i in range(min(4,count)):
            result[10+i]=planets[i][0];result[14+i]=planets[i][1]
        result[18:22]=[alloc[i] for i in (2,3,14,15)]
        return result
    result[1:7]=levels;result[7]=calls;result[8]=hash_value
    result[9:15]=[value%4294967296 for value in c[22:28]];result[15:21]=choices
    return result


def base(mode):
    c=[0]*64;c[0]=mode;c[3]=1;c[10:16]=[1]*6;c[29]=0;c[30]=10;c[31]=20;c[32]=4;c[33]=-1;c[35]=1;c[36]=-1
    for i in range(4):c[38+5*i:43+5*i]=[i%2,(-1,0,1,1)[i],100+i,10+i,-2]
    return c


def cases():
    for choice in range(-128,128):
        for value in (-128,-1,0,26,127):
            for level in range(27):
                for slow in (0,1):
                    c=base(0);c[1]=slow;c[3]=level;c[4:10]=[value]*6;c[10]=choice;yield c
    for field in range(6):
        for initial in (-2,-1,0,1,25,26,127):
            for target in (-128,-1,0,1,6,26,27,127):
                for spent in (0,1,50,2147483647,-2147483648,-1):
                    for choice in (-128,0,1,2,127):
                        for slow in (0,1):
                            c=base(1);c[1]=slow;c[4+field]=initial;c[16+field]=target;c[22+field]=spent;c[10+field]=choice;yield c
    rng=random.Random(0xD3E3109)
    for _ in range(6000):
        c=base(rng.randrange(2));c[1]=rng.randrange(2);c[2]=rng.randrange(6);c[3]=rng.randrange(27)
        c[4:10]=[rng.randrange(27) for _ in range(6)];c[10:16]=[rng.randrange(3) for _ in range(6)]
        c[16:22]=[rng.randrange(27) for _ in range(6)];c[22:28]=[rng.randrange(300000) for _ in range(6)];yield c
    for pct in (-2147483648,-129,-128,-1,0,100,127,128,255,256,2147483647):
        for player in (0,1):
            for count in (0,1,4):
                for flags in (0,1,2,4,8,16,3,7,128|3):
                    for queue in (-1,0,1):
                        for after in (-2,-1,0,1):
                            for fail in (-1,0,1,2):
                                c=base(2);c[28]=pct;c[29]=player;c[32]=count;c[34]=flags;c[33]=fail
                                for i in range(4):c[39+5*i]=queue;c[42+5*i]=after
                                yield c
    for value in (-32768,-1,0,1,32767):
        for queue in (-1,0,1):
            c=base(2);c[32]=32767
            for i in range(4):c[38+5*i:43+5*i]=[0,queue,value,value,-2]
            yield c
    for player in (-1,16):
        c=base(2);c[29]=player;yield c
    for flags in (1,3):
        c=base(2);c[34]=flags;c[35]=-1;yield c
    for level in (-1,27):
        c=base(0);c[3]=level;yield c
    c=base(1);c[16:22]=[1,1,0,0,0,0];yield c
    for fields in (2,3):
        c=base(1);c[16:22]=[1 if i<fields else 0 for i in range(6)]
        c[22:28]=[-1073741000 if i<fields else 0 for i in range(6)];yield c
    for player in (2,15):
        c=base(2);c[29]=player;yield c
    c=base(2);c[34]=1;c[35]=15;yield c


def main():
    source=(ROOT/'src/core/research.c').read_text();start=source.index('int32_t  rglTechCost[27]')
    table=source[start:source.index(';',start)+1]
    declarations=r'''
#include "common.h"
#include <assert.h>
#include <stdlib.h>
#include <inttypes.h>
static int32_t cfg[64];PLAYER rgplr[16];GAME game;int16_t idPlayer,cPlanet;PLANET *lpPlanets;
static PLANET *storage;static int capacity;static PLPROD queues[4];
static int calls,grossCalls,forecastCalls,mutated;static uint32_t traceHash;static jmp_buf escaped;
static int32_t NativeTechCost(TechFieldType field,int16_t level,int16_t player);
static void hashValue(int32_t value){traceHash=traceHash*UINT32_C(16777619)^(uint32_t)value;}
int32_t GetTechLevelCost(TechFieldType field,int16_t level,int16_t player){int total=0;for(int i=0;i<6;i++)total+=rgplr[player].rgTech[i];
    int32_t result=NativeTechCost(field,level,player);calls++;hashValue(field);hashValue(level);hashValue(total);hashValue(result);return result;}
static void setQueue(int index,int value){if(value<0)storage[index].lpplprod=NULL;else{queues[index%4].iprodMac=(uint8_t)value;storage[index].lpplprod=&queues[index%4];}}
static void callback(int kind,int index,int argument){calls++;hashValue(kind);hashValue(index);hashValue(argument);hashValue(rgplr[0].pctResearch);hashValue(rgplr[1].pctResearch);
    if(kind==1)grossCalls++;else forecastCalls++;
    if((kind==1 || (cfg[34]&128)) && !mutated){
        if(cfg[34]&1)idPlayer=(int16_t)cfg[35];if(cfg[34]&2){assert(0<=idPlayer && idPlayer<16);rgplr[idPlayer].pctResearch=(int8_t)cfg[36];}
        if((cfg[34]&4) && cfg[42+5*(index%4)]!=-2)setQueue(index,cfg[42+5*(index%4)]);
        if(cfg[34]&8)cPlanet=0;if((cfg[34]&16) && index+1<cfg[32])storage[index+1].iPlayer=idPlayer;mutated=1;
    }
    if(calls-1==cfg[33])longjmp(escaped,1);
}
int16_t CResourcesAtPlanet(PLANET *planet,int16_t player){int index=(int)(planet-storage);assert(index>=0 && index<cfg[32]);callback(1,index,player);return (int16_t)cfg[40+5*(index%4)];}
void EstimateItemProdSched(PLANET *planet,PLPROD *row,ProdItemType item,int16_t *first,int16_t *last){int index=(int)(planet-storage);assert(!row && item==iprodEstimateResearchResources);callback(2,index,idPlayer);*first=(int16_t)cfg[41+5*(index%4)];*last=123;}
'''
    getter=next(line for line in (ROOT/'src/core/race.c').read_text().splitlines() if line.startswith('int16_t GetRaceStat('))
    bodies=table+'\n'+getter+'\n'+function('src/core/research.c','int32_t GetTechLevelCost(').replace('int32_t GetTechLevelCost(', 'static int32_t NativeTechCost(',1)+'\n'
    bodies+=function('src/core/research.c','int32_t CostOfDevelopingItem(')+'\n'+function('src/core/research.c','int32_t ProjectedResearchSpending(')+'\n'
    harness=r'''
static void setup(void){
    memset(rgplr,0,sizeof(rgplr));memset(&game,0,sizeof(game));memset(queues,0,sizeof(queues));calls=grossCalls=forecastCalls=mutated=0;traceHash=0;idPlayer=0;
    game.fSlowTech=cfg[1];for(int i=0;i<6;i++){rgplr[0].rgTech[i]=(int8_t)cfg[4+i];rgplr[0].rgAttr[8+i]=(int8_t)cfg[10+i];rgplr[0].rgResSpent[i]=(uint32_t)cfg[22+i];}
    if(cfg[0]==2){idPlayer=(int16_t)cfg[29];rgplr[0].pctResearch=(int8_t)cfg[30];rgplr[1].pctResearch=(int8_t)cfg[31];cPlanet=(int16_t)cfg[32];
        int count=cfg[32]>4 ? cfg[32]:4;if(count>capacity){free(storage);storage=calloc((size_t)count,sizeof(*storage));assert(storage);capacity=count;}lpPlanets=storage;
        memset(storage,0,(size_t)count*sizeof(*storage));for(int i=0;i<cfg[32];i++){storage[i].iPlayer=(int16_t)cfg[38+5*(i%4)];setQueue(i,cfg[39+5*(i%4)]);}
    }
}
int main(int argc,char **argv){assert(argc==2);FILE *file=fopen(argv[1],"rb");assert(file);unsigned long cases=0,checks=0;int64_t expected[22];
    while(fread(cfg,sizeof(cfg),1,file)==1){assert(fread(expected,sizeof(expected),1,file)==1);setup();int64_t actual[22]={0};
        if(cfg[0]==0)actual[0]=GetTechLevelCost((TechFieldType)cfg[2],(int16_t)cfg[3],0);
        else if(cfg[0]==1){char target[6];for(int i=0;i<6;i++)target[i]=(char)cfg[16+i];actual[0]=CostOfDevelopingItem(target);}
        else {int complete=0;int32_t value=0;if(!setjmp(escaped)){value=ProjectedResearchSpending(cfg[28]);complete=1;}
            actual[0]=complete ? value:0;actual[1]=rgplr[0].pctResearch;actual[2]=rgplr[1].pctResearch;actual[3]=idPlayer;
            actual[4]=calls;actual[5]=grossCalls;actual[6]=forecastCalls;actual[7]=traceHash;actual[8]=complete;actual[9]=cPlanet;
            for(int i=0;i<4 && i<cfg[32];i++){actual[10+i]=storage[i].iPlayer;actual[14+i]=storage[i].lpplprod ? storage[i].lpplprod->iprodMac:-1;}
            actual[18]=rgplr[2].pctResearch;actual[19]=rgplr[3].pctResearch;actual[20]=rgplr[14].pctResearch;actual[21]=rgplr[15].pctResearch;
        }
        if(cfg[0]!=2){for(int i=0;i<6;i++){actual[1+i]=rgplr[0].rgTech[i];actual[9+i]=rgplr[0].rgResSpent[i];actual[15+i]=rgplr[0].rgAttr[8+i];}actual[7]=calls;actual[8]=traceHash;actual[21]=idPlayer;}
        for(int i=0;i<22;i++){if(cfg[0]==2 && i==0 && !actual[8])continue;checks++;if(actual[i]!=expected[i]){fprintf(stderr,"case %lu mode %d field %d actual %lld expected %lld\n",cases,cfg[0],i,(long long)actual[i],(long long)expected[i]);return 1;}}cases++;
    }
    assert(feof(file));fclose(file);free(storage);printf("Native research quotes/preview: %lu cases, %lu assertions passed\n",cases,checks);return 0;
}
'''
    with tempfile.TemporaryDirectory(prefix='research-contract-') as work:
        path=Path(work);data=path/'vectors.bin';excluded={};count=0
        with data.open('wb') as stream:
            for c in cases():
                assert len(c)==64
                try:result=reference(c)
                except Boundary as why:excluded[str(why)]=excluded.get(str(why),0)+1;continue
                stream.write(struct.pack('<64i22q',*c,*result));count+=1
        print(f'Prepared {count} defined research vectors; excluded before C: {excluded}',flush=True)
        for generator in ['text/textgen.py','data/datagen.py']:
            subprocess.run([sys.executable,str(ROOT/generator),str(path)],check=True,stdout=subprocess.DEVNULL)
        c_file=path/'witness.c';c_file.write_text(declarations+bodies+harness);binary=path/'witness'
        subprocess.run(shlex.split(os.environ.get('CC','cc'))+['-std=gnu11','-Wall','-Wextra','-Werror','-Wno-unused-variable','-Wno-unused-but-set-variable','-Wno-pointer-sign',
            '-fsanitize=address,undefined','-fno-sanitize-recover=all','-I'+str(ROOT/'src/core'),'-I'+str(ROOT/'src/platform'),'-I'+str(path),str(c_file),'-lm','-o',str(binary)],check=True)
        subprocess.run([str(binary),str(data)],check=True)


if __name__=='__main__':main()
