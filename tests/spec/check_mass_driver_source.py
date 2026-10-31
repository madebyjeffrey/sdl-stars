#!/usr/bin/env python3
"""Actual IWarpMAFromLppl plus construction/packet integration.

Native common.h structs and generated enums; a separate word/slot interpreter
supplies expectations. Reached invalid native reads are excluded before C.
Output is null or separate live storage; aliasing/lifetime is not inferred.
"""
from pathlib import Path
import os
import random
import shlex
import struct
import subprocess
import sys
import tempfile
from check_s02_source import ROOT,function
from check_construction_buildability_source import native_buildable_construction


class Boundary(Exception):
    pass


def base():
    return [0,1,0,0,1,7,16,1]+[0,0,0]*16


def reference(c):
    owner,has,index,current,roster,detail,count,output=c[:8]
    two=0 if output else 1234
    if owner==-1 or not has:return [0,two]
    if not 0<=owner<16:raise Boundary('roster owner')
    if not roster:raise Boundary('roster pointer')
    if index>=10:raise Boundary('base design read')
    if current not in (-1,owner) and detail!=7:return [0,two]
    if count>16:raise Boundary('hull slot read')
    values=[c[9+3*i]-2 for i in range(count) if c[8+3*i]==512 and c[10+3*i]>0 and 7<=c[9+3*i]<=15]
    warp=max(values,default=0)
    return [warp,int(values.count(warp)>1) if output else 1234]


def cases():
    for mask in range(65536):
        for item in (7,15,16):
            c=base();c[8:11]=[mask,item,255];yield c
    for word in range(-32768,32768):
        for has in (0,1):
            c=base();c[0]=word;c[1]=has;yield c
        for detail in (0,7):
            c=base();c[3]=word;c[5]=detail;c[8:14]=[512,7,2,512,7,1];yield c
    for item in range(256):
        for quantity in range(256):
            c=base();c[8:11]=[512,item,quantity];yield c
    for position in range(16):
        for count in range(17):
            for item in range(18):
                for output in (0,1):
                    c=base();c[6]=count;c[7]=output;c[8+3*position:11+3*position]=[512,item,2];yield c
    for count in range(256):
        for detail in (0,7):
            for current in (-1,0,1):
                for output in (0,1):
                    c=base();c[3]=current;c[5]=detail;c[6]=count;c[7]=output;yield c
    for owner in range(-2,18):
        for index in range(16):
            for roster in (0,1):
                for current in (-1,owner,17):
                    for output in (0,1):
                        c=base();c[:8]=[owner,1,index,current,roster,0,255,output];yield c
    for a in range(18):
        for b in range(18):
            for d in range(18):
                c=base();c[8:17]=[512,a,2,512,b,1,512,d,1];yield c
    rng=random.Random(80819)
    for _ in range(20000):
        c=base();c[:8]=[rng.randrange(-1,17),rng.randrange(2),rng.randrange(16),rng.randrange(-2,18),rng.randrange(2),rng.randrange(8),rng.randrange(18),rng.randrange(2)]
        for i in range(16):c[8+3*i:11+3*i]=[rng.choice([512,1536,65535,0]),rng.randrange(256),rng.randrange(256)]
        yield c


def main():
    declarations,bodies,helpers=native_buildable_construction()
    stub='int16_t IWarpMAFromLppl(PLANET *p,int16_t *two){assert(p==&planet);int at=warpCalls<2 ? warpCalls:1;warpBase[at]=p->isb;warpCalls++;if(two)*two=twoValues[at];return warpValues[at];}'
    assert stub in declarations
    declarations=declarations.replace(stub,r'''
static int16_t NativeWarp(PLANET *p,int16_t *two);
int16_t IWarpMAFromLppl(PLANET *p,int16_t *two){
    assert(p==&planet);int at=warpCalls<2?warpCalls:1;warpBase[at]=p->isb;warpCalls++;
    return NativeWarp(p,two);
}
static_assert(iplrNone==-1 && detAll==7 && hstSpecialSB==512 && ispecialSBMassDriver5==7 && ispecialSBUltraDriver13==15,"mass driver identities");
''')
    bodies+='\n'+function('src/core/planet.c','int16_t IWarpMAFromLppl(').replace('int16_t IWarpMAFromLppl(','static int16_t NativeWarp(',1)
    harness=r'''
static void eligible(void){
    fInCheck=cQueries=cGateLookups=mdCheckResult=0;rgplr[0].rgAttr[rsMajorAdv]=raMassAccel;rgplr[0].grbitTrader=static_cast<GrbitTrader>(UINT16_MAX);
    for(int i=0;i<6;i++)rgplr[0].rgTech[i]=26;
}
static void drivers(SHDEF *d,int pattern){
    memset(&d->hul,0,sizeof(d->hul));d->det=detAll;d->hul.ihuldef=static_cast<HulDef>(32);d->hul.chs=3;
    // none; one occupied slot containing two; two equal slots; lower ties
    // superseded by a higher slot; highest tied with a lower slot between.
    const int items[6][3]={{0,0,0},{7,0,0},{7,7,0},{7,7,15},{15,7,15},{15,0,0}};
    for(int i=0;i<3;i++){d->hul.rghs[i].grhst=hstSpecialSB;d->hul.rghs[i].iItem=items[pattern][i];d->hul.rghs[i].cItem=items[pattern][i]?2:0;}
}
int main(int argc,char **argv){
    assert(argc==2);FILE *file=fopen(argv[1],"rb");assert(file);int32_t data[58];unsigned long cases=0;
    while(fread(data,sizeof(data),1,file)==1){
        reset();for(int i=0;i<16;i++)rglpshdefSB[i]=NULL;
        planet.iPlayer=(int16_t)data[0];planet.fStarbase=data[1];planet.isb=data[2];idPlayer=(int16_t)data[3];
        if(data[0]>=0 && data[0]<16)rglpshdefSB[data[0]]=data[4]?bases:NULL;
        if(data[2]<10){SHDEF *d=bases+data[2];d->det=data[5];d->hul.chs=data[6];
            for(int i=0;i<16;i++){d->hul.rghs[i].grhst=static_cast<HullSlotType>(data[8+3*i]);d->hul.rghs[i].iItem=data[9+3*i];d->hul.rghs[i].cItem=data[10+3*i];}}
        PLANET expectedPlanet=planet;SHDEF expectedBases[10];PLAYER expectedPlayers[16];GAME expectedGame=game;
        memcpy(expectedBases,bases,sizeof(bases));memcpy(expectedPlayers,rgplr,sizeof(rgplr));
        int16_t two=1234;int warp=NativeWarp(&planet,data[7]?&two:NULL);
        if(warp!=data[56] || two!=data[57]){fprintf(stderr,"mass driver case %lu got %d/%d expected %d/%d\n",cases,warp,two,data[56],data[57]);return 1;}checks+=2;
        CHECK(memcmp(&expectedPlanet,&planet,sizeof(planet))==0);CHECK(memcmp(expectedBases,bases,sizeof(bases))==0);
        CHECK(memcmp(expectedPlayers,rgplr,sizeof(rgplr))==0);CHECK(memcmp(&expectedGame,&game,sizeof(game))==0);CHECK(idPlayer==data[3]);CHECK(!randomCalls);
        cases++;
    }
    CHECK(feof(file));fclose(file);
    const int warps[6]={0,5,5,13,13,13},twos[6]={0,0,1,0,1,0};unsigned integrations=0;
    for(int old=0;old<6;old++)for(int next=0;next<6;next++)for(int had=0;had<2;had++)for(int same=0;same<2;same++)
    for(int current=-1;current<2;current++)for(int detail=0;detail<8;detail++)for(int configured=0;configured<16;configured++){
        reset();eligible();idPlayer=current;planet.fStarbase=had;planet.isb=0;planet.iWarpFling=configured;planet.idFling=8;
        drivers(bases,old);drivers(bases+1,next);bases[0].det=bases[1].det=detail;
        int selected=same?0:1,visible=current==-1 || current==0 || detail==detAll;
        int before=had && visible?warps[old]:0,after=visible?warps[same?old:next]:0,bonus=twos[same?old:next];
        int expected=before?configured:after?after+bonus-4:0;
        CHECK(call(grobjFleet,16+selected,1));CHECK(warpCalls==(before?1:2));CHECK(warpBase[0]==0);
        if(!before)CHECK(warpBase[1]==selected);
        CHECK(planet.isb==selected && planet.fStarbase);CHECK(planet.iWarpFling==expected);
        CHECK(planet.idFling==(before || after?8:0));CHECK(packetKills==(!before && !after));CHECK(idPlayer==current);integrations++;
    }
    // Actual packet construction consumes the real slot maximum/duplicate bit.
    for(int pattern=0;pattern<6;pattern++)for(int configured=0;configured<16;configured++)for(int destination=0;destination<2;destination++){
        reset();eligible();idPlayer=0;planet.fStarbase=1;drivers(bases,pattern);planet.iWarpFling=configured;planet.idFling=destination?8:0;
        int accepted=warps[pattern]>0 && destination;CHECK(call(grobjPlanet,14,1)==accepted);
        if(accepted){
            int asked=configured+4;if(asked<5 || asked>warps[pattern]+3)asked=warps[pattern]+twos[pattern];
            int decay=asked<=warps[pattern]+twos[pattern]?0:asked-warps[pattern]-twos[pattern];
            CHECK(cThing==1 && lpThings[0].thp().iWarp()==asked-4 && lpThings[0].thp().iDecayRate()==decay);
        }else CHECK(cThing==0);
        integrations++;
    }
    for(int pattern=0;pattern<6;pattern++){
        reset();eligible();drivers(bases,pattern);planet.lpplprod=LpplAlloc<PLPROD>(4,1,htOrd);
        planet.lpplprod->iprodMac=1;planet.lpplprod->rgprod()[0]=row(1,16,grobjFleet,0);
        Produce();CHECK(planet.fStarbase && planet.isb==0 && !planet.lpplprod);
        CHECK(planet.iWarpFling==(warps[pattern]?warps[pattern]+twos[pattern]-4:0));
        CHECK(packetKills==!warps[pattern] && rgplr[0].lResLastYear==90);integrations++;
    }
    reset();printf("Mass driver source: %lu defined standalone cases; %u construction/packet cases; %d assertions passed\n",cases,integrations,checks);return 0;
}
'''
    with tempfile.TemporaryDirectory(prefix='mass-driver-') as work:
        path=Path(work)
        for generator in ('text/textgen.py','data/datagen.py'):
            subprocess.run([sys.executable,str(ROOT/generator),str(path)],check=True,stdout=subprocess.DEVNULL)
        source=path/'witness.cpp';source.write_text(declarations+bodies+helpers+harness);binary=path/'witness'
        subprocess.run(shlex.split(os.environ.get('CXX','c++'))+['-std=gnu++20','-DSTARS_TEST_STATE_DOUBLE','-include',str(ROOT/'src/platform/value.h'),'-x','c++','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-Wno-unused-parameter',
            '-fsanitize=address,undefined','-fno-sanitize-recover=all','-iquote',str(ROOT/'src/core'),'-iquote',str(ROOT/'src/platform'),'-iquote',str(path),str(source),str(path/'data_tables.cpp'),'-o',str(binary)],check=True)
        vectors=path/'vectors.bin';count=0;excluded={}
        with vectors.open('wb') as stream:
            for c in cases():
                try:out=reference(c)
                except Boundary as why:excluded[str(why)]=excluded.get(str(why),0)+1;continue
                stream.write(struct.pack('<58i',*c,*out));count+=1
        print(f'Prepared {count} cases; excluded before C: {excluded}',flush=True)
        subprocess.run([str(binary),str(vectors)],check=True)


if __name__=='__main__':main()
