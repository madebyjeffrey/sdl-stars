#!/usr/bin/env python3
"""Actual terraforming totals, first-slot gate range and scanner fallback.

Reuses real lookup/catalog/terraform/native structs, compares independent
numeric/selection references, and runs production/application handoffs. Native
undefined reads and message-provider domains are excluded before C separately.
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
from check_terraform_source import native_terraform_construction,base as terra_base,cases as terra_cases,reference as terra_reference
from check_part_availability_source import policy,tech
from check_native_colony_source import signed,Boundary


def base(op):
    result=terra_base()[:54]+[0,1,0,1,16,0,1,255]+[0,0,0]*16
    result[0]=op
    return result


def reference(c,table,special,planetary):
    op=c[0];current=c[3];owner=c[4];extra=[0,-1,-1,-1,-1]+[-1]*64
    if op==0:
        count_input=c[:54];count_input[6]=1  # Ipct always requests helpful bounds.
        state=terra_reference(count_input,table)
        state[0]=signed(sum((c[8+i]-state[5+i] if state[5+i]!=-1 else 0)+(state[8+i]-c[8+i] if state[8+i]!=-1 else 0) for i in range(3)),16) if state[0] else 0
        return state+extra
    state=[0,current]+c[8:11]+[9876]*9+[0,0,0]+[-1]*128+[-999]*64
    assert len(state)==209
    trace=[];chosen=-1
    def lookup(mask,item,quantity):
        if current!=-1 and not 0<=current<16:raise Boundary('capability lookup player')
        row=(special if mask==512 else planetary)[item]
        if c[1]:status=1 if c[2]&(1<<item) else -1
        else:
            headless=current==-1;at=38 if current==1 else 32;primary=c[51] if current==1 else c[50]
            flags=(2 if (c[45] if current==1 else c[44]) else 0)|((1<<10) if c[52] else 0)
            allowed,_=policy(headless,primary,flags,0,9 if mask==512 else 15,item,row[6] if mask==32768 else 0)
            status=-1 if not allowed else 1 if headless else tech(c[at:at+6],row[:6],c[53])
        trace.append([mask,item,status,quantity]);return status
    if op==1:
        direct,has,index,present,count=c[54:59];index=index if direct else index%16
        if not direct and (owner==-1 or not has):return state+extra
        if not 0<=owner<16:raise Boundary('gate roster owner')
        if not present:raise Boundary('gate roster pointer')
        if not 0<=index<10:raise Boundary('gate design index')
        for at in range(count):
            if at==16:raise Boundary('gate hull slot')
            mask,item,quantity=c[62+3*at:65+3*at]
            if mask==512 and quantity>0 and 0<=item<=6:
                lookup(mask,item,quantity);value=c[47] if c[1] and item==c[46] else special[item][6]
                state[0]=10000 if value==-1 else value;break
    else:
        if op==3:
            if not 0<=owner<16:raise Boundary('message provider owner')
            current=owner
        for item in range(8,-1,-1):
            status=lookup(32768,item,-1 if op==3 else c[61]);chosen=item
            if status==1:break
        state[0]=1 if op==3 else chosen;state[1]=-1 if op==3 else current;state[16]=1 if op==3 else 0
    extra=[len(trace),chosen,trace[-1][0] if trace else -1,trace[-1][3] if trace else -1,current if trace else -1]
    return state+extra+sum(trace,[])+[-1]*(64-4*len(trace))


def cases():
    for op in (3,2,1,0):yield base(op)
    c=base(0);c[8]=c[11]=-23;c[14]=-50;c[17]=-75;c[20]=-25;yield c
    for c in terra_cases():
        if c[0]==0:yield c+[0,1,0,1,16,0,1,255]+[0,0,0]*16
    for mask in range(65536):
        for item in (0,6,7):
            c=base(1);c[62:65]=[mask,item,255];yield c
    for word in range(-32768,32768):
        c=base(1);c[1]=1;c[46:48]=[0,word];c[62:68]=[512,0,2,512,6,1];yield c
        for op in (1,2,3):
            c=base(op);c[3]=word;yield c
        c=base(1);c[4]=word;c[55]=0;yield c
        c=base(1);c[54]=1;c[56]=word;yield c
    for count in range(256):
        for candidate in (-1,0,15):
            for detail in (0,7):
                c=base(1);c[58]=count;c[59]=detail
                if candidate>=0:c[62+3*candidate:65+3*candidate]=[512,1,255]
                yield c
    for owner in (-1,0,1,15,16):
        for direct in (0,1):
            for index in (-1,0,9,10,15):
                for roster in (0,1):
                    c=base(1);c[4]=owner;c[54]=direct;c[56]=index;c[57]=roster;c[62:65]=[512,0,1];yield c
    for mask in range(512):
        for op in (2,3):
            for qty in (0,1,255):
                c=base(op);c[1]=1;c[2]=mask;c[61]=qty;yield c
    for level in range(-128,128):
        for field in range(6):
            for primary in (0,7,8,9):
                for op in (1,2,3):
                    c=base(op);c[3]=0;c[32+field]=level;c[50]=primary;c[62:65]=[512,6,2];yield c
    for primary in range(-128,128):
        for current in (-1,0,1,15):
            for no_scanners in (0,1):
                for op in (1,2,3):
                    c=base(op);c[3]=current;c[50]=c[51]=primary;c[52]=no_scanners;c[62:65]=[512,4,1];yield c
    rng=random.Random(8082027)
    for _ in range(20000):
        op=rng.choice([1,2,3]);c=base(op);c[1]=rng.randrange(2);c[2]=rng.randrange(65536);c[3]=rng.randrange(-1,17);c[4]=rng.randrange(-1,17)
        c[32:44]=[rng.randrange(-128,128) for _ in range(12)];c[50:54]=[rng.randrange(-128,128),rng.randrange(-128,128),rng.randrange(2),rng.randrange(16)]
        c[54:62]=[rng.randrange(2),rng.randrange(2),rng.randrange(-1,17),rng.randrange(2),rng.randrange(18),rng.randrange(8),rng.randrange(2),rng.randrange(256)]
        for at in range(16):c[62+3*at:65+3*at]=[rng.choice([0,512,1536]),rng.randrange(256),rng.randrange(256)]
        yield c


def main():
    declarations,bodies,helpers=native_terraform_construction()
    declarations=declarations.replace('#include <setjmp.h>','#include <setjmp.h>\nstatic int32_t capcfg[110];static int extraCalls,extraPlayer,extraTrace[16][4],selectedScanner;static SPECIALSB fakeSpecial[16];')
    declarations+='\nstatic_assert(hstSpecialSB==512 && hstPlanetary==32768 && iplanetaryViewer50==0 && iplanetarySnooper620X==8 && ispecialSBStargate100250==0 && ispecialSBStargateAnyAny==6 && ibitRaceNoAdvScanner==10,"colony capability identities");\n'
    stub='int16_t IpctCanTerraformLppl(PLANET *p){assert(p==&planet);return 10;}'
    assert stub in declarations;declarations=declarations.replace(stub,function('src/core/planet.c','int16_t IpctCanTerraformLppl('))
    stub='void LookupBestPlanetaryScanner(PART *part){assert(idPlayer==planet.iPlayer);part->hs.iItem=(uint8_t)partIndex;}'
    assert stub in declarations
    declarations=declarations.replace(stub,'static void NativeScanner(PART *part);\nvoid LookupBestPlanetaryScanner(PART *part){NativeScanner(part);selectedScanner=part->hs.iItem;}')
    start=bodies.index('int16_t FLookupPart(PART *part){');end=bodies.index('\n}\n',start)+3
    previous=bodies[start:end].replace('int16_t FLookupPart(','static int16_t TerraformLookup(',1)
    bodies=bodies[:start]+previous+r'''
int16_t FLookupPart(PART *part){
    unsigned mask=part->hs.grhst,item=part->hs.iItem;
    if(mask==hstTerra || fInCheck)return TerraformLookup(part);
    assert(extraCalls<16 && ((mask==hstSpecialSB && item<16) || (mask==hstPlanetary && item<15)));
    int quantity=capcfg[0]==3?-1:part->hs.cItem,status;extraPlayer=idPlayer;
    if(!cfg[1])status=NativeLookupPart(part);
    else{if(mask==hstSpecialSB)part->pspecialsb=fakeSpecial+item;else part->pplanetary=rgplanetary+item;status=(cfg[2]&(1<<item))?1:-1;}
    assert(part->pcom==(COMPART *)(mask==hstSpecialSB?(void *)(cfg[1]?fakeSpecial+item:rgspecialSB+item):(void *)(rgplanetary+item)));
    extraTrace[extraCalls][0]=mask;extraTrace[extraCalls][1]=item;extraTrace[extraCalls][2]=status;extraTrace[extraCalls++][3]=quantity;return status;
}
'''+bodies[end:]
    bodies+='\n'+function('src/core/parts.c','void LookupBestPlanetaryScanner(').replace('void LookupBestPlanetaryScanner(','static void NativeScanner(',1)
    bodies+='\n'+function('src/core/planet.c','int16_t StargateRangeFromLppl(')
    harness=r'''
static void ready(void){
    reset();memcpy(cfg,capcfg,sizeof(cfg));terraCalls=desireCalls=currentDesire=extraCalls=0;extraPlayer=selectedScanner=-1;memset(terraItems,-1,sizeof(terraItems));memset(extraTrace,-1,sizeof(extraTrace));
    for(int i=0;i<16;i++)for(int f=0;f<4;f++)desireInputs[i][f]=-999;
    for(int i=0;i<3;i++)observedLow[i]=observedHigh[i]=observedCost[i]=9876;
    for(int i=0;i<20;i++)scripted[i]=rgterra[i];if(cfg[46]>=0){scripted[cfg[46]].grAbility=(int16_t)cfg[47];scripted[cfg[46]].resCost=(uint16_t)cfg[48];}
    for(int i=0;i<16;i++){fakeSpecial[i]=rgspecialSB[i];if(i==cfg[46])fakeSpecial[i].grAbility2=(int16_t)cfg[47];}
    idPlayer=(int16_t)cfg[3];planet.iPlayer=(int16_t)cfg[4];planet.fStarbase=capcfg[55];planet.isb=capcfg[56];
    for(int i=0;i<3;i++){planet.rgEnvVar[i]=(int8_t)cfg[8+i];planet.rgEnvVarOrig[i]=(int8_t)cfg[11+i];}
    for(int p=0;p<16;p++){
        int fields=p==1?23:14,levels=p==1?38:32;
        rgplr[p].grbitAttr=static_cast<RaceTraitBits>(((p==1?cfg[45]:cfg[44])?UINT32_C(1)<<ibitRaceTT:0)|((capcfg[0]!=0 && cfg[52])?UINT32_C(1)<<ibitRaceNoAdvScanner:0));
        rgplr[p].rgAttr[rsMajorAdv]=capcfg[0]==0?0:(int8_t)(p==1?cfg[51]:cfg[50]);rgplr[p].iTechNow=capcfg[0]==0?0:cfg[53];
        rgplr[p].wScore=100+p;rgplr[p].lSalt=10000+p;rgplr[p].idPlanetHome=p;
        for(int i=0;i<3;i++){rgplr[p].rgEnvVar[i]=(int8_t)cfg[fields+i];rgplr[p].rgEnvVarMin[i]=(int8_t)cfg[fields+3+i];rgplr[p].rgEnvVarMax[i]=(int8_t)cfg[fields+6+i];}
        for(int f=0;f<6;f++)rgplr[p].rgTech[f]=(int8_t)cfg[levels+f];rglpshdefSB[p]=NULL;
    }
    if(cfg[4]>=0 && cfg[4]<16)rglpshdefSB[cfg[4]]=capcfg[57]?bases:NULL;
    int index=capcfg[54]?capcfg[56]:planet.isb;
    if(index>=0 && index<10){SHDEF *d=bases+index;d->hul.chs=capcfg[58];d->det=capcfg[59];d->fFree=capcfg[60];
        for(int i=0;i<16;i++){d->hul.rghs[i].grhst=static_cast<HullSlotType>(capcfg[62+3*i]);d->hul.rghs[i].iItem=capcfg[63+3*i];d->hul.rghs[i].cItem=capcfg[64+3*i];}}
}
int main(int argc,char **argv){
    assert(argc==3);FILE *file=argv[2][0]=='-'?stdin:fopen(argv[2],argv[1][0]=='e'?"wb":"rb");assert(file);
    if(argv[1][0]=='e'){
        for(int i=0;i<20;i++){int32_t r[8];for(int f=0;f<6;f++)r[f]=rgterra[i].rgTech[f];r[6]=rgterra[i].grAbility;r[7]=rgterra[i].resCost;assert(fwrite(r,sizeof(r),1,file)==1);}
        for(int kind=0;kind<2;kind++)for(int i=0;i<(kind?15:16);i++){int32_t r[7];COMPART *p=kind?(COMPART *)(rgplanetary+i):(COMPART *)(rgspecialSB+i);for(int f=0;f<6;f++)r[f]=p->rgTech[f];r[6]=kind?rgplanetary[i].grAbility:rgspecialSB[i].grAbility2;assert(fwrite(r,sizeof(r),1,file)==1);}
        fclose(file);return 0;
    }
    int32_t data[388];unsigned long cases=0,modes[4]={};
    while(fread(data,sizeof(data),1,file)==1){
        memcpy(capcfg,data,sizeof(capcfg));ready();PLAYER expectedPlayers[16];PLANET expectedPlanet=planet;GAME expectedGame=game;SHDEF expectedBases[10];
        memcpy(expectedPlayers,rgplr,sizeof(rgplr));memcpy(expectedBases,bases,sizeof(bases));
        int result;PART part={};part.hs.cItem=capcfg[61];part.pplanetary=rgplanetary+14;
        if(cfg[0]==0)result=IpctCanTerraformLppl(&planet);
        else if(cfg[0]==1)result=StargateRangeFromLppl(capcfg[54]?NULL:&planet,(int16_t)cfg[4],(int16_t)capcfg[56]);
        else if(cfg[0]==2){LookupBestPlanetaryScanner(&part);result=part.hs.iItem;CHECK(part.hs.grhst==hstPlanetary && part.hs.cItem==capcfg[61] && part.pplanetary==rgplanetary+part.hs.iItem);}
        else result=call(grobjPlanet,iobjPlanetaryScanner,1);
        int actual[278];actual[0]=result;actual[1]=idPlayer;
        for(int i=0;i<3;i++){actual[2+i]=planet.rgEnvVar[i];actual[5+i]=observedLow[i];actual[8+i]=observedHigh[i];actual[11+i]=observedCost[i];}
        actual[14]=terraCalls;actual[15]=desireCalls;actual[16]=messages;
        for(int i=0;i<128;i++)actual[17+i]=terraItems[i];for(int i=0;i<16;i++)for(int f=0;f<4;f++)actual[145+4*i+f]=desireInputs[i][f];
        actual[209]=extraCalls;actual[210]=selectedScanner;actual[211]=extraCalls?extraTrace[extraCalls-1][0]:-1;actual[212]=extraCalls?extraTrace[extraCalls-1][3]:-1;actual[213]=extraPlayer;
        for(int i=0;i<16;i++)for(int f=0;f<4;f++)actual[214+4*i+f]=extraTrace[i][f];
        for(int i=0;i<278;i++){if(actual[i]!=data[110+i]){fprintf(stderr,"colony capability case %lu op %d field %d got %d expected %d\n",cases,cfg[0],i,actual[i],data[110+i]);return 1;}checks++;}
        if(cfg[0]==3){expectedPlanet.iScanner=selectedScanner;CHECK(messageArgs[0][2]==selectedScanner && messageScanner[0]==31);}
        CHECK(memcmp(expectedPlayers,rgplr,sizeof(rgplr))==0);CHECK(memcmp(&expectedPlanet,&planet,sizeof(planet))==0);CHECK(memcmp(&expectedGame,&game,sizeof(game))==0);CHECK(memcmp(expectedBases,bases,sizeof(bases))==0);CHECK(!randomCalls);cases++;modes[cfg[0]]++;
    }
    CHECK(feof(file));fclose(file);
    // Actual Produce validation -> builder -> real count/selection/application.
    for(int distance=0;distance<=2;distance++){
        memset(capcfg,0,sizeof(capcfg));capcfg[3]=0;capcfg[4]=0;capcfg[44]=capcfg[45]=1;capcfg[46]=-1;capcfg[57]=1;
        for(int i=0;i<3;i++){capcfg[8+i]=capcfg[11+i]=50+(i==0?distance:0);capcfg[14+i]=50;capcfg[17+i]=25;capcfg[20+i]=75;}
        for(int i=0;i<6;i++)capcfg[32+i]=26;ready();
        planet.lpplprod=(PLPROD *)LpplAlloc(4,1,htOrd);planet.lpplprod->iprodMac=1;planet.lpplprod->rgprod[0]=row(3,mdIdleTerraform,grobjPlanet,0);
        Produce();CHECK(!planet.lpplprod && planet.rgEnvVar[0]==50 && rgplr[0].lResLastYear==100-distance*10);CHECK(terraformCalls==distance);
    }
    for(int i=0;i<4;i++)CHECK(modes[i]>0);
    capcfg[8]=capcfg[11]=-23;capcfg[14]=-50;capcfg[17]=-75;capcfg[20]=-25;ready();CHECK(IpctCanTerraformLppl(&planet)==-24);
    planet.lpplprod=(PLPROD *)LpplAlloc(4,1,htOrd);planet.lpplprod->iprodMac=1;planet.lpplprod->rgprod[0]=row(3,mdIdleTerraform,grobjPlanet,0);
    Produce();CHECK(!planet.lpplprod && !terraformCalls && rgplr[0].lResLastYear==100);
    reset();printf("Colony capabilities: %lu cases (count %lu, gate %lu, scanner %lu, generic %lu) plus four Produce clipping/application cases; %d assertions passed\n",cases,modes[0],modes[1],modes[2],modes[3],checks);return 0;
}
'''
    with tempfile.TemporaryDirectory(prefix='colony-capabilities-') as work:
        path=Path(work)
        for generator in ('text/textgen.py','data/datagen.py'):subprocess.run([sys.executable,str(ROOT/generator),str(path)],check=True,stdout=subprocess.DEVNULL)
        source=path/'witness.cpp';source.write_text(declarations+bodies+helpers+harness);binary=path/'witness'
        subprocess.run(shlex.split(os.environ.get('CXX','c++'))+['-std=gnu++20','-include',str(ROOT/'src/platform/value.h'),'-x','c++','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-Wno-unused-parameter','-fsanitize=address,undefined','-fno-sanitize-recover=all','-iquote',str(ROOT/'src/core'),'-iquote',str(ROOT/'src/platform'),'-iquote',str(path),str(source),str(path/'data_tables.cpp'),'-lm','-o',str(binary)],check=True)
        export=path/'catalog.bin';subprocess.run([str(binary),'export',str(export)],check=True);raw=export.read_bytes();table=list(struct.iter_unpack('<8i',raw[:640]));special=list(struct.iter_unpack('<7i',raw[640:1088]));planetary=list(struct.iter_unpack('<7i',raw[1088:]));assert (len(table),len(special),len(planetary))==(20,16,15)
        count=0;excluded={};process=subprocess.Popen([str(binary),'verify','-'],stdin=subprocess.PIPE)
        try:
            with process.stdin as stream:
                for c in cases():
                    assert len(c)==110
                    try:out=reference(c,table,special,planetary)
                    except Boundary as why:excluded[str(why)]=excluded.get(str(why),0)+1;continue
                    assert len(out)==278
                    stream.write(struct.pack('<388i',*c,*out));count+=1
        except BrokenPipeError:process.wait();raise
        result=process.wait();print(f'Prepared {count} cases; pre-C exclusions: {excluded}',flush=True)
        if result:raise subprocess.CalledProcessError(result,process.args)


if __name__=='__main__':main()
