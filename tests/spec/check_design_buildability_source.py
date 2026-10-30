#!/usr/bin/env python3
"""Actual FCanBuildShdef composed with actual catalog/trait/technology helpers.

Checks hull signed-word/byte mapping, early failure, occupied-slot order,
headless/player context, restoration and whole native input frames. Candidates
that reach invalid player/slot reads are excluded before C, not recovered by it.
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
from check_part_availability_source import native_parts,LIMITS,policy,tech


class Boundary(Exception):pass


def base():
    return [7,0,0,0,9,0,65535,0,0,0]+[26]*6+[value for _ in range(16) for value in (8,0,0)]


def cases():
    for hull in range(65536):
        c=base();c[2]=hull;yield c
    for saved in range(-32768,32768):
        c=base();c[0]=saved;yield c
    for selected in range(-32768,32768):
        c=base();c[1]=selected;yield c
    for count in range(256):
        for failure in (-2,-1,0,7,15):
            c=base();c[3]=count
            if failure==-2:c[2]=255 # unavailable hull exits before slot count matters
            elif failure>=0:c[16+3*failure:19+3*failure]=[3,255,1]
            yield c
    # Every category row in every slot: slot traversal reuses the actual lookup.
    for primary in range(10):
        for level in (0,26):
            for kind,n in enumerate(LIMITS):
                for item in range(n+1):
                    for at in range(16):
                        c=base();c[1]=primary;c[3]=at+1;c[4]=primary;c[5]=0xffffffff if at%2 else 0;c[6]=65535 if item%2 else 0;c[7]=at;c[10:16]=[level]*6
                        c[16+3*at:19+3*at]=[1<<kind,item,255];yield c
    # All occupancy masks distinguish skipped garbage from requested parts.
    for occupied in range(65536):
        c=base();c[3]=16;c[8]=occupied%2;c[9]=(occupied//2)%2
        for at in range(16):c[16+3*at:19+3*at]=[8 if occupied&(1<<at) else 65535,0 if occupied&(1<<at) else 255,1 if occupied&(1<<at) else 0]
        yield c
    for selected in (-1,0,1,15):
        for flags in (0,0xffffffff):
            for failure in range(-1,16):
                c=base();c[1]=selected;c[3]=16;c[5]=flags;c[8]=1;c[9]=1
                for at in range(16):c[16+3*at:19+3*at]=[8,0,1+(at*17)%255]
                if failure>=0:c[16+3*failure:19+3*failure]=[3,0,1]
                yield c
    rng=random.Random(110416)
    for _ in range(20000):
        c=base();c[:10]=[rng.randrange(-32768,32768),rng.choice([-1]+list(range(16))),rng.randrange(65536),rng.randrange(18),rng.randrange(-128,128),rng.randrange(1<<32),rng.randrange(65536),rng.randrange(16),rng.randrange(2),rng.randrange(2)]
        c[10:16]=[rng.randrange(-128,128) for _ in range(6)]
        for at in range(16):c[16+3*at:19+3*at]=[rng.choice([3,0]+[1<<k for k in range(16)]),rng.randrange(256),rng.choice([0,1,255])]
        yield c


def reference(c,table):
    saved,selected,hull,count,primary,flags,grants,current=c[:8]
    if selected!=-1 and not 0<=selected<16:raise Boundary('lookup player index')
    queries=[];requests=[];headless=selected==-1
    def lookup(mask,item,quantity):
        kind=mask.bit_length()-1 if mask and mask&(mask-1)==0 else 16
        start=len(queries);calls=[] if headless else [0]
        if (kind,item) not in table:status=0
        else:
            need,ability=table[kind,item];allowed,extra=policy(headless,primary,flags,grants,kind,item,ability);calls+=extra
            if not allowed:status=-1
            elif headless:status=1
            else:status=tech(c[10:16],need,current);calls+=[2]
        requests.append([selected,mask,item,quantity,status,start]);queries.extend(calls)
        return status==1
    signed=hull if hull<32768 else hull-65536
    result=lookup(1024 if signed>=32 else 16384,(hull-(32 if signed>=32 else 0))%256,-1)
    if result:
        for at in range(count):
            if at>=16:raise Boundary('design slot read')
            mask,item,quantity=c[16+3*at:19+3*at]
            if quantity and not lookup(mask,item,quantity):result=False;break
    assert len(requests)<=17 and len(queries)<=128
    return [int(result),saved,len(requests),len(queries)]+[v for row in requests+[[-1]*6]*(17-len(requests)) for v in row]+queries+[-1]*(128-len(queries))


def main():
    declarations,bodies=native_parts(128)
    bodies=bodies.replace('int16_t FLookupPart(','int16_t NativeLookupPart(',1)
    bodies+=r'''
static int cLookups,rgRequests[17][6];
int16_t FLookupPart(PART *ppart){
    CHECK(cLookups<17);int at=cLookups;uint16_t mask=ppart->hs.grhst;unsigned item=ppart->hs.iItem;
    rgRequests[at][0]=idPlayer;rgRequests[at][1]=mask;rgRequests[at][2]=item;
    rgRequests[at][3]=at==0?-1:ppart->hs.cItem;rgRequests[at][5]=cCalls;
    int16_t result=NativeLookupPart(ppart);rgRequests[at][4]=result;
    CHECK(ppart->hs.grhst==mask && ppart->hs.iItem==item);
    if(at!=0)CHECK(ppart->hs.cItem==rgRequests[at][3]);
    if(result!=mdPartAvailInvalid){unsigned kind=16;for(unsigned k=0;k<16;k++)if(mask==(UINT32_C(1)<<k))kind=k;CHECK(kind<16);CHECK(ppart->pcom()==LpcomRow(kind,item));}
    cLookups++;return result;
}
'''
    bodies+=function('src/core/util.c','int16_t FCanBuildShdef(')+'\n'
    harness=r'''
int main(int argc,char **argv){
    const unsigned rgcRows[]={iengineCount,iscannerCount,ishieldCount,iarmorCount,ibeamCount,itorpCount,ibombCount,iminingCount,iminesCount,ispecialSBCount,isbhullCount,ispecialECount,ispecialMCount,iterraCount,ihuldefOrbitalFort,iplanetaryCount};
    assert(argc==3 && CHAR_MIN==-128);FILE *file=fopen(argv[2],argv[1][0]=='e'?"wb":"rb");assert(file);
    if(argv[1][0]=='e'){
        for(unsigned k=0;k<16;k++){
            for(unsigned item=0;item<rgcRows[k];item++){
                int32_t rgData[9];NATIVEPARTCOMMONVIEW p=LpcomRow(k,item);rgData[0]=k;rgData[1]=item;
                for(int f=0;f<6;f++)rgData[2+f]=p.rgTech()[f];
                rgData[8]=k==15?rgplanetary[item].grAbility:0;
                assert(fwrite(rgData,sizeof(rgData),1,file)==1);
            }
        }
        fclose(file);return 0;
    }
    int32_t rgIn[298];SHDEF design,designBefore;PLAYER rgBefore[16];GAME gameBefore;
    while(fread(rgIn,sizeof(rgIn),1,file)==1){
        memset(rgplr,0x53,sizeof(rgplr));memset(&game,0x35,sizeof(game));memset(&design,0x5a,sizeof(design));
        int player=rgIn[1]==-1?0:rgIn[1];CHECK(player>=0 && player<16);
        rgplr[player].rgAttr[rsMajorAdv]=(int8_t)rgIn[4];rgplr[player].grbitAttr=static_cast<RaceTraitBits>((uint32_t)rgIn[5]);rgplr[player].grbitTrader=static_cast<GrbitTrader>((uint16_t)rgIn[6]);rgplr[player].SetITechNow(rgIn[7]);
        for(int f=0;f<6;f++)rgplr[player].rgTech[f]=(int8_t)rgIn[10+f];
        design.hul.ihuldef=static_cast<HulDef>((uint16_t)rgIn[2]);design.hul.chs=(uint8_t)rgIn[3];design.fFree=rgIn[8];design.fGift=rgIn[9];
        for(int at=0;at<16;at++){design.hul.rghs[at].grhst=static_cast<HullSlotType>((uint16_t)rgIn[16+3*at]);design.hul.rghs[at].iItem=rgIn[17+3*at];design.hul.rghs[at].cItem=rgIn[18+3*at];}
        idPlayer=(int16_t)rgIn[0];memcpy(rgBefore,rgplr,sizeof(rgBefore));memcpy(&gameBefore,&game,sizeof(game));memcpy(&designBefore,&design,sizeof(design));
        cCalls=cLookups=0;memset(rgCalls,-1,sizeof(rgCalls));memset(rgRequests,-1,sizeof(rgRequests));
        int result=FCanBuildShdef(&design,(int16_t)rgIn[1]);
        if(result!=rgIn[64]){fprintf(stderr,"case %lu hull %d count %d selected %d: result %d expected %d\n",cCases,rgIn[2],rgIn[3],rgIn[1],result,rgIn[64]);abort();}cChecks++;
        CHECK(idPlayer==rgIn[65]);CHECK(cLookups==rgIn[66]);CHECK(cCalls==rgIn[67]);
        CHECK(memcmp(&design,&designBefore,sizeof(design))==0);CHECK(memcmp(rgplr,rgBefore,sizeof(rgplr))==0);CHECK(memcmp(&game,&gameBefore,sizeof(game))==0);
        for(int at=0;at<17;at++)for(int f=0;f<6;f++)CHECK(rgRequests[at][f]==rgIn[68+6*at+f]);
        for(int at=0;at<128;at++)CHECK(rgCalls[at]==rgIn[170+at]);
        cCases++;
    }
    CHECK(feof(file));fclose(file);printf("Native design buildability: %lu cases, %lu assertions passed; actual lookup/race/technology/catalog composition, order, restoration and SHDEF/PLAYER/GAME frames checked\n",cCases,cChecks);return 0;
}
'''
    with tempfile.TemporaryDirectory(prefix='design-buildability-') as work:
        path=Path(work)
        for generator in ('text/textgen.py','data/datagen.py'):
            subprocess.run([sys.executable,str(ROOT/generator),str(path)],check=True,stdout=subprocess.DEVNULL)
        source=path/'witness.cpp';source.write_text(declarations+bodies+harness);binary=path/'witness'
        subprocess.run(shlex.split(os.environ.get('CXX','c++'))+['-std=gnu++20','-include',str(ROOT/'src/platform/value.h'),'-x','c++','-Wall','-Wextra','-Werror','-Wno-misleading-indentation',
            '-fsanitize=address,undefined','-fno-sanitize-recover=all','-iquote',str(ROOT/'src/core'),'-iquote',str(ROOT/'src/platform'),'-iquote',str(path),str(source),str(path/'data_tables.cpp'),'-lm','-o',str(binary)],check=True)
        export=path/'catalog.bin';subprocess.run([str(binary),'export',str(export)],check=True)
        table={row[:2]:(row[2:8],row[8]) for row in struct.iter_unpack('<9i',export.read_bytes())}
        assert len(table)==sum(LIMITS) and all((k,i) in table for k,n in enumerate(LIMITS) for i in range(n))
        data=path/'vectors.bin';count=0;excluded={}
        with data.open('wb') as stream:
            for c in cases():
                assert len(c)==64
                try:expected=reference(c,table)
                except Boundary as why:excluded[str(why)]=excluded.get(str(why),0)+1;continue
                assert len(expected)==234
                c[5]=c[5] if c[5]<0x80000000 else c[5]-0x100000000
                stream.write(struct.pack('<298i',*c,*expected));count+=1
        print(f'Prepared {count} defined cases; excluded before C: {excluded}',flush=True)
        subprocess.run([str(binary),'verify',str(data)],check=True)


if __name__=='__main__':main()
