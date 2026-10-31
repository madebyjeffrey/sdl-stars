#!/usr/bin/env python3
"""Native terraform catalog/bounds/ranking/remote/application integration.

Actual common.h structures, fresh catalogs and lookup/race/technology/habitat
bodies. Separate reference keeps native narrowing and partial-output semantics.
Scripted lookup/desirability modes isolate controllers; real modes compose them.
Reached native undefined domains are excluded before C under ASan/UBSan.
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
from check_s02_source import ROOT,function
from check_construction_buildability_source import native_buildable_construction
from check_native_colony_source import signed,div,checked,Boundary
from check_part_availability_source import tech


def base():
    return [1,0,(1<<20)-1,3,0,1,1,1]+[70,70,70]+[70,70,70]+[50,50,50]+[25,25,25]+[75,75,75]+[40,40,40]+[20,20,20]+[60,60,60]+[26]*12+[1,1,-1,0,0,0,0,0,0,0]


def bounds(current,original,ideal,immune,ability,helpful):
    if not ability or immune:return -1,-1
    low=signed(original-ability,16);high=signed(original+ability,16)
    low=max(1,low) if low<current else -1;high=min(99,high) if high>current else -1
    if helpful:
        if current==ideal:return -1,-1
        if current>ideal:return max(low,ideal) if low!=-1 else -1,-1
        return -1,min(high,ideal) if high!=-1 else -1
    distance=abs(current-ideal);dl=abs(low-ideal) if low!=-1 else 0;dh=abs(high-ideal) if high!=-1 else 0
    if distance>=max(dl,dh):return -1,-1
    return (low,-1) if dl>=dh else (-1,high)


def habitat(env,fields):
    ideal,low,high=fields[:3],fields[3:6],fields[6:9];positive=negative=0;modifier=10000
    for x,p,lo,hi in zip(env,ideal,low,high):
        if hi<0:positive=checked(positive+10000,'immune habitat sum')
        elif lo<=x<=hi:
            d=p-lo if x<p else hi-p;q=signed(div(signed(abs(x-p)*100,16),d),16)
            penalty=signed(abs(x-p)*2-d,16);q=signed(100-q,16);positive=signed(positive+q*q,32)
            if penalty>0:modifier=div(checked(modifier*signed(2*d-penalty,16),'habitat modifier product'),signed(2*d,16))
        else:negative=checked(negative+min(15,lo-x if x<lo else x-hi),'negative habitat sum')
    if negative:return signed(-(negative%65536),16)
    if positive<0:raise Boundary('habitat square root')
    converted=int(math.sqrt(float(positive)/3.0)+0.9)
    return signed(div(checked(converted*modifier,'habitat final product'),10000),16)


def reference(c,table):
    mode,script,mask,saved,owner,actor,helpful,built=c[:8];env=c[8:11].copy();original=c[11:14]
    if mode in (3,4):helpful=1  # Construction and optimum preview request helpful terraforming.
    low=[9876]*3;high=[9876]*3;costs=[9876]*3;calls=[];desires=[];messages=0;global_player=saved
    def player(index):
        if not 0<=index<16:raise Boundary('player index')
        # All other physical player entries have the owner's declared fields.
        return (c[23:32],c[38:44],c[45]) if index==1 else (c[14:23],c[32:38],c[44])
    injected=None
    def can():
        nonlocal low,high,costs,global_player
        low=[9876]*3;high=[9876]*3;costs=[9876]*3
        previous=global_player;selected=owner if previous==-1 else previous;global_player=selected
        def lookup(item):
            calls.append(item)
            if selected!=-1 and not 0<=selected<16:raise Boundary('lookup player index')
            if script:return bool(mask&(1<<item))
            if selected==-1:return True
            fields,levels,tt=injected if injected is not None and selected==owner else player(selected)
            return (item>=8 or tt) and tech(levels,table[item][:6],0)==1
        choices=[]
        for group in (range(7,-1,-1),range(11,7,-1),range(15,11,-1),range(19,15,-1)):
            choices.append(next((i for i in group if lookup(i)),None))
        def row(item):
            return (c[47],c[48]) if item==c[46] and script else table[item][6:8]
        moves=[0]*3;found=choices[0] is not None
        if found:
            ability,cost=row(choices[0]);moves=[ability]*3;costs=[signed(cost,16)]*3
        for axis,item in enumerate(choices[1:]):
            if item is not None and row(item)[0]>moves[axis]:found=True;moves[axis]=row(item)[0];costs[axis]=signed(row(item)[1],16)
        if not found:global_player=previous;return False
        for axis in range(3):
            if not moves[axis]:low[axis]=high[axis]=-1;continue
            fields,_,_=injected if injected is not None else player(selected)
            low[axis],high[axis]=bounds(env[axis],original[axis],fields[axis],fields[3+axis]==-1,moves[axis],helpful)
        global_player=previous;return any(x!=-1 for x in low+high)
    def best():
        nonlocal global_player,env
        if owner==-1:return 0
        previous=global_player;global_player=owner
        if not can():global_player=previous;return 0
        fields,_,_=injected if injected is not None else player(owner)
        start=len(desires)
        def desire():
            at=len(desires)-start;desires.append([owner]+env.copy())
            return c[50+at] if c[49] else habitat(env,fields)
        baseline=desire();scores=[0]*3
        for axis in range(3):
            endpoint=low[axis] if low[axis]!=-1 else high[axis]
            if endpoint==-1:continue
            before=env[axis];env[axis]=signed(endpoint,8);delta=signed(desire()-baseline,16)
            absolute=signed(-delta,16) if delta<0 else delta
            scores[axis]=signed(div(signed(100*absolute,16),abs(before-endpoint))+1,16);env[axis]=before
        winner=max(range(3),key=lambda i:scores[i]);global_player=previous
        return -(winner+1) if low[winner]!=-1 else winner+1
    if mode==0:answer=int(can())
    elif mode==1:answer=best()
    elif mode==2:
        own_fields,_,_=player(owner);_,levels,tt=player(actor);injected=(own_fields,levels,tt);answer=best();injected=None
    elif mode==4:
        available=can();fields,_,_=player(actor);before=env.copy()
        if available:
            for axis in range(3):
                current=env[axis];ideal=fields[axis];endpoint=-1
                if fields[3+axis]!=-1 and current!=ideal:
                    if current<ideal and high[axis]>current:endpoint=min(ideal,high[axis])
                    elif current>ideal and low[axis]!=-1 and low[axis]<current:endpoint=max(ideal,low[axis])
                if endpoint!=-1:env[axis]=signed(endpoint,8)
        evaluated=saved if available else actor;target,_,_=player(evaluated)
        desires.append([evaluated]+env.copy());answer=c[50] if c[49] else habitat(env,target);env=before
    else:
        answer=1
        for _ in range(built%65536):
            choice=best()
            if choice:
                at=abs(choice)-1;env[at]=max(1,min(99,env[at]+(-1 if choice<=0 else 1)));messages+=1
    assert len(calls)<=128 and len(desires)<=16
    out=[answer,global_player]+env+low+high+costs+[len(calls),len(desires),messages]
    return out+calls+[-1]*(128-len(calls))+sum(desires,[])+[-999]*(64-4*len(desires))


def cases():
    # Every availability subset isolates the exact four descending scans.
    for mask in range(1<<20):
        c=base();c[:3]=[0,1,mask];yield c
    for current in range(-128,128):
        for original in range(-128,128):
            for helpful in (0,1):
                c=base();c[:2]=[0,1];c[6]=helpful;c[8]=current;c[11]=original;yield c
    for ability in range(-32768,32768):
        c=base();c[:3]=[0,1,1<<7];c[46:49]=[7,ability,65535];yield c
    for word in range(-32768,32768):
        for mode in (0,1):
            c=base();c[0]=mode;c[3]=word;yield c
    for level in range(-128,128):
        for field in range(6):
            for tt in (0,1):
                for mode in (0,1,2,3,4):
                    c=base();c[0]=mode;c[32+field]=level;c[38+field]=level;c[44]=tt;c[45]=1-tt;yield c
    for mode in range(5):
        for helpful in (0,1):
            for mask in (0,1,1<<7,1<<8,1<<11,1<<19,(1<<20)-1):
                for owner in (-1,0,1,15,16):
                    c=base();c[:3]=[mode,1,mask];c[4]=owner;c[6]=helpful;yield c
    for value in range(-128,128):
        for field in range(9):
            for helpful in (0,1):
                c=base();c[6]=helpful;c[14+field]=value;yield c
    # Ranking provider cases exercise pre-division I16 narrowing independently.
    for delta in range(-32768,32768):
        c=base();c[49]=1;c[50:54]=[0,delta,0,0];yield c
    rng=random.Random(8082026)
    for _ in range(30000):
        c=base();c[:8]=[rng.randrange(5),rng.randrange(2),rng.randrange(1<<20),rng.choice([-1,0,1,3,15]),rng.choice([-1,0,1,15]),rng.randrange(16),rng.randrange(2),rng.randrange(3)]
        c[8:14]=[rng.randrange(-128,128) for _ in range(6)]
        c[14:32]=[rng.randrange(-128,128) for _ in range(18)]
        c[32:44]=[rng.randrange(-128,128) for _ in range(12)];c[44:46]=[rng.randrange(2),rng.randrange(2)]
        yield c


def native_terraform_construction():
    declarations,bodies,helpers=native_buildable_construction()
    declarations+='\nstatic_assert(hstTerra==8192 && envNone==-1 && envImmune==-1 && iterraCount==20 && ibitRaceTT==1,"terraform model identities");\n'
    declarations=declarations.replace('#include <setjmp.h>','#include <setjmp.h>\nstatic int32_t cfg[54];static int currentDesire,terraCalls,desireCalls,terraItems[128],desireInputs[16][4],observedLow[3],observedHigh[3],observedCost[3];static TERRA scripted[20];')
    stub='int16_t PctPlanetDesirability(PLANET *p,int16_t player){assert(p==&planet && !player);return 100;}'
    assert stub in declarations
    declarations=declarations.replace(stub,r'''
static int16_t NativeHabitat(PLANET *,int16_t);
int16_t PctPlanetDesirability(PLANET *p,int16_t player){
    assert(p==&planet && desireCalls<16);int at=desireCalls++;desireInputs[at][0]=player;
    for(int i=0;i<3;i++)desireInputs[at][i+1]=p->rgEnvVar[i];
    return cfg[49]?(int16_t)cfg[50+currentDesire++]:NativeHabitat(p,player);
}
''')
    stub='int16_t IBestTerraform(PLANET *p,int16_t help){assert(p==&planet && help);terraformCalls++;if(terraformChoice<-3 || terraformChoice>3)longjmp(boundary,1);return terraformChoice;}'
    assert stub in declarations
    declarations=declarations.replace(stub,'static int16_t NativeBest(PLANET *,int16_t);\nint16_t IBestTerraform(PLANET *p,int16_t help){terraformCalls++;currentDesire=0;return NativeBest(p,help);}')
    declarations=declarations.replace('static jmp_buf boundary;','')
    at=bodies.index('int16_t FLookupPart(PART *part){');end=bodies.index('\n}\n',at)+3
    gate=bodies[at:end].replace('int16_t FLookupPart(','static int16_t GateLookup(',1)
    bodies=bodies[:at]+gate+r'''
int16_t FLookupPart(PART *part){
    if(part->hs.grhst!=hstTerra)return GateLookup(part);
    assert(terraCalls<128 && part->hs.iItem<20);terraItems[terraCalls++]=part->hs.iItem;
    if(!cfg[1])return NativeLookupPart(part);
    part->SetPterra(scripted+part->hs.iItem);return (cfg[2]&(1<<part->hs.iItem))?1:0;
}
'''+bodies[end:]
    bodies+='\n'+function('src/core/planet.cpp','int16_t FCanTerraformLppl(').replace('int16_t FCanTerraformLppl(','static int16_t NativeCan(',1)
    bodies+=r'''
int16_t FCanTerraformLppl(PLANET *p,int16_t *low,int16_t *high,int16_t *cost,int16_t help){
    // Supply initialized local outputs for observation; Best never reads them
    // on FALSE and ignores cost, so defined caller outcomes are unchanged.
    for(int i=0;i<3;i++)low[i]=high[i]=cost[i]=9876;
    int16_t result=NativeCan(p,low,high,cost,help);
    for(int i=0;i<3;i++){observedLow[i]=low[i];observedHigh[i]=high[i];observedCost[i]=cost[i];}return result;
}
'''
    bodies+='\n'+function('src/core/planet.cpp','int16_t PctPlanetDesirability(').replace('int16_t PctPlanetDesirability(','static int16_t NativeHabitat(',1)
    bodies+='\n'+function('src/core/planet.cpp','int16_t IBestTerraform(').replace('int16_t IBestTerraform(','static int16_t NativeBest(',1)
    bodies+='\n'+function('src/core/planet.cpp','int16_t PctPlanetOptValue(')
    bodies+='\n'+function('src/core/turn2.cpp','int16_t IBestRemoteTerra(')
    return declarations,bodies,helpers


def main():
    declarations,bodies,helpers=native_terraform_construction()
    harness=r'''
int main(int argc,char **argv){
    assert(argc==3);FILE *file=argv[2][0]=='-'?stdin:fopen(argv[2],argv[1][0]=='e'?"wb":"rb");assert(file);
    if(argv[1][0]=='e'){
        for(int i=0;i<20;i++){int32_t r[8];for(int f=0;f<6;f++)r[f]=rgterra[i].rgTech[f];r[6]=rgterra[i].grAbility;r[7]=rgterra[i].resCost;assert(fwrite(r,sizeof(r),1,file)==1);}fclose(file);return 0;
    }
    int32_t data[263];unsigned long cases=0;
    while(fread(data,sizeof(data),1,file)==1){
        reset();memcpy(cfg,data,sizeof(cfg));currentDesire=terraCalls=desireCalls=0;memset(terraItems,-1,sizeof(terraItems));
        for(int i=0;i<16;i++)for(int f=0;f<4;f++)desireInputs[i][f]=-999;
        for(int i=0;i<3;i++)observedLow[i]=observedHigh[i]=observedCost[i]=9876;
        for(int i=0;i<20;i++)scripted[i]=rgterra[i];if(cfg[46]>=0){scripted[cfg[46]].grAbility=(int16_t)cfg[47];scripted[cfg[46]].resCost=(uint16_t)cfg[48];}
        idPlayer=(int16_t)cfg[3];planet.iPlayer=(int16_t)cfg[4];
        for(int i=0;i<3;i++){planet.rgEnvVar[i]=(int8_t)cfg[8+i];planet.rgEnvVarOrig[i]=(int8_t)cfg[11+i];}
        for(int p=0;p<16;p++){
            int fields=p==1?23:14,levels=p==1?38:32;rgplr[p].grbitAttr=static_cast<RaceTraitBits>((p==1?cfg[45]:cfg[44])?UINT32_C(1)<<ibitRaceTT:0);
            for(int i=0;i<3;i++){rgplr[p].rgEnvVar[i]=(int8_t)cfg[fields+i];rgplr[p].rgEnvVarMin[i]=(int8_t)cfg[fields+3+i];rgplr[p].rgEnvVarMax[i]=(int8_t)cfg[fields+6+i];}
            for(int f=0;f<6;f++)rgplr[p].rgTech[f]=(int8_t)cfg[levels+f];
        }
        PLAYER expectedPlayers[16];PLANET expectedPlanet=planet;GAME expectedGame=game;memcpy(expectedPlayers,rgplr,sizeof(rgplr));
        int result;int16_t low[3],high[3],cost[3];
        if(cfg[0]==0)result=FCanTerraformLppl(&planet,low,high,cost,(int16_t)cfg[6]);
        else if(cfg[0]==1)result=IBestTerraform(&planet,(int16_t)cfg[6]);
        else if(cfg[0]==2)result=IBestRemoteTerra(&planet,(int16_t)cfg[5],(int16_t)cfg[6]);
        else if(cfg[0]==4)result=PctPlanetOptValue(&planet,(int16_t)cfg[5]);
        else result=call(grobjPlanet,iobjMaxTerraform,cfg[7]);
        int actual[209];actual[0]=result;actual[1]=idPlayer;
        for(int i=0;i<3;i++){actual[2+i]=planet.rgEnvVar[i];actual[5+i]=observedLow[i];actual[8+i]=observedHigh[i];actual[11+i]=observedCost[i];expectedPlanet.rgEnvVar[i]=data[56+i];}
        actual[14]=terraCalls;actual[15]=desireCalls;actual[16]=messages;
        for(int i=0;i<128;i++)actual[17+i]=terraItems[i];for(int i=0;i<16;i++)for(int f=0;f<4;f++)actual[145+4*i+f]=desireInputs[i][f];
        for(int i=0;i<209;i++){if(actual[i]!=data[54+i]){fprintf(stderr,"terraform case %lu mode %d mask %x field %d got %d expected %d\n",cases,cfg[0],cfg[2],i,actual[i],data[54+i]);return 1;}checks++;}
        CHECK(memcmp(expectedPlayers,rgplr,sizeof(rgplr))==0);CHECK(memcmp(&expectedPlanet,&planet,sizeof(planet))==0);CHECK(memcmp(&expectedGame,&game,sizeof(game))==0);CHECK(!randomCalls);
        cases++;
    }
    CHECK(feof(file));fclose(file);reset();printf("Terraform selection: %lu cases, %d assertions passed\n",cases,checks);return 0;
}
'''
    # row() belongs to the shared helper factory but is unused in this fixture.
    helpers=helpers[helpers.index('static void reset(void)'):]
    with tempfile.TemporaryDirectory(prefix='terraform-source-') as work:
        path=Path(work)
        for generator in ('text/textgen.py','data/datagen.py'):
            subprocess.run([sys.executable,str(ROOT/generator),str(path)],check=True,stdout=subprocess.DEVNULL)
        source=path/'witness.cpp';source.write_text(declarations+bodies+helpers+harness);binary=path/'witness'
        subprocess.run(shlex.split(os.environ.get('CXX','c++'))+['-std=gnu++20','-DSTARS_TEST_STATE_DOUBLE','-include',str(ROOT/'src/platform/value.h'),'-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-Wno-unused-parameter',
            '-fsanitize=address,undefined','-fno-sanitize-recover=all','-iquote',str(ROOT/'src/core'),'-iquote',str(ROOT/'src/platform'),'-iquote',str(path),str(source),str(path/'data_tables.cpp'),'-lm','-o',str(binary)],check=True)
        export=path/'catalog.bin';subprocess.run([str(binary),'export',str(export)],check=True);table=list(struct.iter_unpack('<8i',export.read_bytes()));assert len(table)==20
        count=0;excluded={};process=subprocess.Popen([str(binary),'verify','-'],stdin=subprocess.PIPE)
        try:
            with process.stdin as stream:
                for c in cases():
                    assert len(c)==54,len(c)
                    try:out=reference(c,table)
                    except Boundary as why:excluded[str(why)]=excluded.get(str(why),0)+1;continue
                    assert len(out)==209
                    stream.write(struct.pack('<263i',*c,*out));count+=1
        except BrokenPipeError:
            process.wait();raise
        result=process.wait();print(f'Prepared {count} cases; excluded before C: {excluded}',flush=True)
        if result:raise subprocess.CalledProcessError(result,process.args)


if __name__=='__main__':main()
