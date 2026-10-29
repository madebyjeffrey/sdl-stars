#!/usr/bin/env python3
"""FBuildObject/Produce with actual build-check, availability and catalog bodies.

Economy, messages, warp, allocation and registration retain declared fixture
providers. Undefined owner/roster/design/check/metadata reads are filtered before
C. Gate success remains distinct from successful object application.
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
from check_construction_effects_source import native_construction
from check_part_availability_source import catalog_rows_source,LIMITS,IDENTITIES
from check_design_buildability_source import base as design_base,reference as design_reference,Boundary


def base():
    # item,base,owner,ships,bases,free,hull,count,mask,item,quantity,position,
    # primary,flags,grants,level,current,cargo,limit,saved,built,old slot,hull
    return [0,1,0,1,1,0,0,0,8,0,0,0,9,0,65535,26,0,0,0,3,1,9,32]


def cases():
    for item in range(-32768,32768):
        for has_base in (0,1):
            c=base();c[0]=item;c[1]=has_base;c[6]=32 if item>=16 else 0;yield c
    for owner in range(-32768,32768):
        c=base();c[1]=0;c[2]=owner;yield c
    for owner in range(-1,17):
        for item in (-1,0,15,16,25,26):
            for ships in (0,1):
                for bases in (0,1):
                    for free in (0,1):
                        c=base();c[0]=item;c[2]=owner;c[3]=ships;c[4]=bases;c[5]=free;c[6]=32 if item>=16 else 0;yield c
    for hull in range(65536):
        c=base();c[0]=16;c[6]=hull;yield c
    for owner in range(16):
        for item in range(26):
            for free in (0,1):
                for level in (0,26):
                    for variant in range(6):
                        c=base();c[0]=item;c[2]=owner;c[5]=free;c[6]=32 if item>=16 else 0;c[7]=16;c[11]=-1;c[12]=owner%10;c[13]=0xffffffff if variant%2 else 0;c[15]=level;c[17]=(0,1,65535)[variant%3];c[18]=variant//3;c[19]=(-1,0,1,15)[variant%4];c[20]=1+variant%2
                        if variant%3==1:c[8]=3;c[10]=1
                        elif variant%3==2:c[8]=8;c[9]=11;c[10]=255
                        yield c
    for count in range(256):
        for item in (0,16):
            for failure in (0,1,2):
                c=base();c[0]=item;c[6]=32 if item==16 else 0;c[7]=count
                if failure==1:c[6]=255
                if failure==2:c[8]=3;c[10]=1
                yield c
    rng=random.Random(190911)
    for _ in range(10000):
        c=base();c[:8]=[rng.randrange(26),rng.randrange(2),rng.randrange(16),rng.randrange(2),rng.randrange(2),rng.randrange(2),rng.randrange(37),rng.randrange(18)]
        c[8:21]=[rng.choice([0,3,65535]+[1<<k for k in range(16)]),rng.randrange(256),rng.choice([0,1,255]),rng.randrange(-1,16),rng.randrange(-128,128),rng.randrange(1<<32),rng.randrange(65536),rng.randrange(-128,128),rng.randrange(16),rng.choice([0,1,65535]),rng.randrange(2),rng.randrange(-32768,32768),rng.choice([1,2])]
        yield c


def reference(c,table):
    item,has_base,owner,ships,bases,free,hull,count=c[:8]
    empty=[-1]*230
    if item<16 and not has_base:return [0,0,0,0,0,0,0,c[19],0]+empty
    is_base=item>=16;index=item-16 if is_base else item
    if not 0<=owner<16:raise Boundary('roster owner')
    if not (bases if is_base else ships):raise Boundary('roster pointer')
    if index<0 or index>=(10 if is_base else 16):raise Boundary('design slot')
    if free:return [0,0,0,0,0,int(not is_base),0,c[19],0]+empty
    d=design_base();d[:10]=[c[19],owner,hull,count,c[12],c[13],c[14],c[16],free,0];d[10:16]=[c[15]]*6
    for at in range(16):
        if c[11]==-1 or at==c[11]:d[16+3*at:19+3*at]=c[8:11]
    check=design_reference(d,table);can=check[0]
    if can and is_base:
        signed=hull if hull<32768 else hull-65536
        if signed<0 or signed>=37:raise Boundary('base notification catalog')
    stage=0 if not can else 2 if is_base else 1
    applied=bool(can and (is_base or not c[18]))
    messages=1 if (not is_base or applied) else 0
    return [stage,int(applied),1,check[2],check[3],messages,int(is_base and can),c[19],int(applied and not is_base)]+check[4:]


def native_buildable_construction():
    declarations,bodies,witnesses=native_construction()
    stub='int16_t FCanBuildShdef(SHDEF *design,int16_t player){assert(player==0 && design);canCalls++;return (int16_t)canBuild;}'
    assert stub in declarations
    declarations=declarations.replace(stub,r'''
static int16_t NativeCanBuild(SHDEF *design,int16_t player);
static int fInCheck,cQueries,cGateLookups,mdCheckResult,rgQueries[128],rgGateRequests[17][6];
int16_t FCanBuildShdef(SHDEF *design,int16_t player){
    int16_t saved=idPlayer;assert(design && player==planet.iPlayer);canCalls++;fInCheck=1;
    int16_t value=NativeCanBuild(design,player);fInCheck=0;assert(idPlayer==saved);mdCheckResult=value;return value;
}
''')
    stub='HULDEF *LphuldefFromId(int16_t hull){(void)hull;catalogCalls++;return &catalogHull;}'
    assert stub in declarations
    declarations=declarations.replace(stub,'static HULDEF *NativeHullFromId(int16_t hull);\nHULDEF *LphuldefFromId(int16_t hull){catalogCalls++;return NativeHullFromId(hull);}')
    declarations=declarations.replace('assert(!player && id==planet.id);newFleets++;','assert(player==planet.iPlayer && id==planet.id);newFleets++;')
    declarations+='\n'.join(f'static_assert({name}=={value},"model identity {name}");' for name,value in IDENTITIES.items())+'\n'
    declarations+='static_assert(iplrNone==-1 && ihuldefDeathStart==ihuldefOrbitalFort+(int)isbhullCount-1,"native owner/catalog bounds");\n'
    bodies=bodies.replace('int16_t GetRaceStat(','static int16_t NativeGetRaceStat(',1).replace('int16_t GetRaceGrbit(','static int16_t NativeGetRaceGrbit(',1)
    extra=r'''
static void Query(int tag){if(fInCheck){assert(cQueries<128);rgQueries[cQueries++]=tag;}}
int16_t GetRaceStat(PLAYER *p,RaceStat stat){if(fInCheck)assert(p==&rgplr[idPlayer] && stat==rsMajorAdv);Query(0);return NativeGetRaceStat(p,stat);}
int16_t GetRaceGrbit(PLAYER *p,RaceGrbit bit){if(fInCheck)assert(p==&rgplr[idPlayer]);Query(100+bit);return NativeGetRaceGrbit(p,bit);}
'''
    extra+=function('src/core/parts.c','int16_t TechStatus(').replace('TechStatus(','NativeTechStatus(',1)+'\n'
    extra+=function('src/core/research.c','int16_t FShouldPartBeHidden(').replace('FShouldPartBeHidden(','NativeHidden(',1)+'\n'
    extra+='int16_t TechStatus(char *need){Query(2);return NativeTechStatus(need);}\nint16_t FShouldPartBeHidden(PART *part){Query(1);return NativeHidden(part);}\n'
    extra+=function('src/core/parts.c','int16_t FLookupPart(').replace('FLookupPart(','NativeLookupPart(',1)+'\n'+catalog_rows_source()
    extra+=r'''
int16_t FLookupPart(PART *part){
    assert(fInCheck && cGateLookups<17);int at=cGateLookups;unsigned mask=part->hs.grhst,item=part->hs.iItem;
    rgGateRequests[at][0]=idPlayer;rgGateRequests[at][1]=mask;rgGateRequests[at][2]=item;rgGateRequests[at][3]=at==0?-1:part->hs.cItem;rgGateRequests[at][5]=cQueries;
    int16_t status=NativeLookupPart(part);rgGateRequests[at][4]=status;
    if(status!=0){unsigned kind=16;for(unsigned k=0;k<16;k++)if(mask==(UINT32_C(1)<<k))kind=k;assert(kind<16 && part->pcom==LpcomRow(kind,item));}
    cGateLookups++;return status;
}
'''
    extra+=function('src/core/util.c','int16_t FCanBuildShdef(').replace('int16_t FCanBuildShdef(','static int16_t NativeCanBuild(',1)+'\n'
    # One-line source function extractor needs the complete declaration line.
    extra+=next(line for line in (ROOT/'src/core/parts.c').read_text().splitlines() if line.startswith('HULDEF *LphuldefSBFromId('))+'\n'
    extra+=function('src/core/parts.c','HULDEF *LphuldefFromId(').replace('HULDEF *LphuldefFromId(','static HULDEF *NativeHullFromId(',1)+'\n'
    bodies+=extra
    helpers=witnesses[:witnesses.index('static int16_t s16(')]
    return declarations,bodies,helpers


def main():
    declarations,bodies,helpers=native_buildable_construction()
    harness=r'''
int main(int argc,char **argv){
    const unsigned rgcRows[]={iengineCount,iscannerCount,ishieldCount,iarmorCount,ibeamCount,itorpCount,ibombCount,iminingCount,iminesCount,ispecialSBCount,isbhullCount,ispecialECount,ispecialMCount,iterraCount,ihuldefOrbitalFort,iplanetaryCount};
    assert(argc==3);FILE *file=fopen(argv[2],argv[1][0]=='e'?"wb":"rb");assert(file);
    if(argv[1][0]=='e'){
        for(unsigned k=0;k<16;k++){
            for(unsigned item=0;item<rgcRows[k];item++){
                int32_t data[9];COMPART *p=LpcomRow(k,item);data[0]=k;data[1]=item;
                for(int f=0;f<6;f++)data[2+f]=p->rgTech[f];data[8]=k==15?rgplanetary[item].grAbility:0;
                assert(fwrite(data,sizeof(data),1,file)==1);
            }
        }
        fclose(file);return 0;
    }
    int32_t data[262];unsigned long cases=0;
    while(fread(data,sizeof(data),1,file)==1){
        reset();for(int p=0;p<16;p++){rglpshdef[p]=NULL;rglpshdefSB[p]=NULL;}
        int item=data[0],base=item>=16,index=base?item-16:item,owner=data[2];
        planet.fStarbase=data[1];planet.iPlayer=(int16_t)owner;planet.isb=data[21];
        if(owner>=0 && owner<16){
            rglpshdef[owner]=data[3]?ships:NULL;rglpshdefSB[owner]=data[4]?bases:NULL;
            rgplr[owner].rgAttr[rsMajorAdv]=(int8_t)data[12];rgplr[owner].grbitAttr=static_cast<RaceTraitBits>((uint32_t)data[13]);rgplr[owner].grbitTrader=static_cast<GrbitTrader>((uint16_t)data[14]);rgplr[owner].iTechNow=data[16];
            for(int f=0;f<6;f++)rgplr[owner].rgTech[f]=(int8_t)data[15];rgplr[owner].cFleet=data[18]?512:0;
        }
        for(int i=0;i<16;i++){ships[i].cBuilt=100+i;ships[i].cExist=50+i;}
        for(int i=0;i<10;i++){bases[i].cBuilt=200+i;bases[i].cExist=70+i;}
        bases[data[21]].hul.ihuldef=static_cast<HulDef>((uint16_t)data[22]);
        if(index>=0 && index<(base?10:16)){
            SHDEF *d=(base?bases:ships)+index;d->fFree=data[5];d->hul.ihuldef=static_cast<HulDef>((uint16_t)data[6]);d->hul.chs=(uint8_t)data[7];d->hul.wtCargoMax=(uint16_t)data[17];
            for(int at=0;at<16;at++)if(data[11]==-1 || data[11]==at){d->hul.rghs[at].grhst=static_cast<HullSlotType>((uint16_t)data[8]);d->hul.rghs[at].iItem=data[9];d->hul.rghs[at].cItem=data[10];}
        }
        idPlayer=(int16_t)data[19];cQueries=cGateLookups=mdCheckResult=fInCheck=0;memset(rgQueries,-1,sizeof(rgQueries));memset(rgGateRequests,-1,sizeof(rgGateRequests));
        SHDEF shipExpected[16],baseExpected[10];PLAYER playerExpected[16];GAME gameExpected;PLANET planetExpected;
        memcpy(shipExpected,ships,sizeof(ships));memcpy(baseExpected,bases,sizeof(bases));memcpy(playerExpected,rgplr,sizeof(rgplr));memcpy(&gameExpected,&game,sizeof(game));memcpy(&planetExpected,&planet,sizeof(planet));
        int result=call(2,item,data[20]);int stage=canCalls && mdCheckResult?(base?2:1):0;
        int actual[]={stage,result,canCalls,cGateLookups,cQueries,messages,catalogCalls,idPlayer,newFleets};
        for(int i=0;i<9;i++){if(actual[i]!=data[23+i]){fprintf(stderr,"case %lu item %d owner %d hull %d: field %d got %d expected %d\n",cases,item,owner,data[6],i,actual[i],data[23+i]);return 1;}checks++;}
        for(int at=0;at<17;at++)for(int f=0;f<6;f++)CHECK(rgGateRequests[at][f]==data[32+6*at+f]);
        for(int at=0;at<128;at++)CHECK(rgQueries[at]==data[134+at]);
        if(result && base){
            if(data[1])baseExpected[data[21]].cExist--;
            baseExpected[index].cBuilt++;baseExpected[index].cExist++;planetExpected.fStarbase=1;planetExpected.isb=index;
            int raw=(int16_t)(uint16_t)data[6];unsigned cargo=raw>=32?rghuldefSB[raw-32].hul.wtCargoMax:rghuldef[raw].hul.wtCargoMax;
            CHECK(messageIds[0]==idmHasBuiltNew+(data[17]==0?0:data[17]==65535?2:1));CHECK(messageArgs[0][2]==(int16_t)cargo);
            CHECK(warpCalls==1 && !packetKills);CHECK(shipKills==(data[1] && (int16_t)baseExpected[data[21]].hul.ihuldef>(int16_t)(uint16_t)data[6]));
        }else if(result){
            shipExpected[index].cBuilt+=(uint32_t)data[20];shipExpected[index].cExist+=(uint32_t)data[20];playerExpected[owner].cFleet++;
            CHECK(fleetBodies[3].rgcsh[index]==data[20] && fleetBodies[3].iPlayer==owner && fleetBodies[3].rgwtMin[4]==12345);
            CHECK(defaults==1 && !routes);
        }else if(messages){CHECK(messageIds[0]==(stage==1?idmStarbaseBuiltNewShipSTypeLost:idmStarbaseFailedBuildNewShipTypeBecause));}
        CHECK(memcmp(shipExpected,ships,sizeof(ships))==0);CHECK(memcmp(baseExpected,bases,sizeof(bases))==0);CHECK(memcmp(playerExpected,rgplr,sizeof(rgplr))==0);CHECK(memcmp(&gameExpected,&game,sizeof(game))==0);CHECK(memcmp(&planetExpected,&planet,sizeof(planet))==0);
        cases++;
    }
    CHECK(feof(file));fclose(file);
    // Full Produce -> charged construction -> actual eligibility -> exact fleet limit failure.
    reset();planet.id=0;rgptPlan[0]=(POINT16){10,20};game.cPlayer=1;game.cPlanMax=1;planet.fStarbase=1;bases[0].hul.ihuldef=static_cast<HulDef>(32);
    rgplr[0].rgAttr[rsMajorAdv]=raNone;rgplr[0].grbitTrader=static_cast<GrbitTrader>(UINT16_MAX);for(int f=0;f<6;f++)rgplr[0].rgTech[f]=26;
    rgplr[0].cFleet=512;cFleet=512;ships[0].cBuilt=ships[0].cExist=512;
    for(int i=0;i<512;i++){
        memset(&fleetBodies[i],0,sizeof(FLEET));memset(orderStorage[i].data,0,sizeof(orderStorage[i].data));fleetSlots[i]=&fleetBodies[i];
        fleetBodies[i].ifl=i;fleetBodies[i].iPlayer=0;fleetBodies[i].iplr=0;fleetBodies[i].idPlanet=idPlanetDeepSpace;fleetBodies[i].rgcsh[0]=1;fleetBodies[i].pt=(POINT16){99,99};
        fleetBodies[i].lpplord=(PLORD *)orderStorage[i].data;fleetBodies[i].cord=1;fleetBodies[i].lpplord->iordMac=1;fleetBodies[i].lpplord->rgord[0].pt=fleetBodies[i].pt;
    }
    cQueries=cGateLookups=mdCheckResult=fInCheck=0;memset(rgQueries,-1,sizeof(rgQueries));memset(rgGateRequests,-1,sizeof(rgGateRequests));
    planet.lpplprod=LpplAlloc<PLPROD>(4,1,htOrd);planet.lpplprod->iprodMac=1;planet.lpplprod->rgprod()[0]=row(1,0,2,0);
    Produce();CHECK(canCalls==1 && mdCheckResult==1 && !newFleets && !planet.lpplprod && ships[0].cBuilt==512 && ships[0].cExist==512 && rgplr[0].lResLastYear==90 && planet.rgwtMin[0]==10);
    reset();printf("Construction actual eligibility integration: %lu cases plus post-charge Produce/fleet-limit case; %d assertions passed\n",cases,checks);return 0;
}
'''
    with tempfile.TemporaryDirectory(prefix='construction-buildability-') as work:
        path=Path(work)
        for generator in ('text/textgen.py','data/datagen.py'):
            subprocess.run([sys.executable,str(ROOT/generator),str(path)],check=True,stdout=subprocess.DEVNULL)
        source=path/'witness.cpp';source.write_text(declarations+bodies+helpers+harness);binary=path/'witness'
        subprocess.run(shlex.split(os.environ.get('CXX','c++'))+['-std=gnu++20','-include',str(ROOT/'src/platform/value.h'),'-x','c++','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-Wno-unused-parameter',
            '-fsanitize=address,undefined','-fno-sanitize-recover=all','-iquote',str(ROOT/'src/core'),'-iquote',str(ROOT/'src/platform'),'-iquote',str(path),str(source),str(path/'data_tables.cpp'),'-lm','-o',str(binary)],check=True)
        export=path/'catalog.bin';subprocess.run([str(binary),'export',str(export)],check=True)
        table={r[:2]:(r[2:8],r[8]) for r in struct.iter_unpack('<9i',export.read_bytes())};assert len(table)==sum(LIMITS)
        vectors=path/'vectors.bin';count=0;excluded={}
        with vectors.open('wb') as stream:
            for c in cases():
                assert len(c)==23
                try:out=reference(c,table)
                except Boundary as why:excluded[str(why)]=excluded.get(str(why),0)+1;continue
                assert len(out)==239
                c[13]=c[13] if c[13]<0x80000000 else c[13]-0x100000000
                stream.write(struct.pack('<262i',*c,*out));count+=1
        print(f'Prepared {count} defined cases; excluded before C: {excluded}',flush=True)
        subprocess.run([str(binary),'verify',str(vectors)],check=True)


if __name__=='__main__':main()
