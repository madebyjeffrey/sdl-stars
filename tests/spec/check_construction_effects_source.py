#!/usr/bin/env python3
"""Actual construction/application helpers with native common.h layouts.

Runs FBuildObject, CreateShip, both queue invalidators, LpthNew, CBuildProdItem,
Produce and RNG bodies. Catalog, economy, routing, message and low-level heap
services remain explicit providers; generated IDs come from current generators.
Undefined damage/axis inputs are guarded before native execution.
"""
from cpp_production_storage import declarations as storage_declarations, definitions as storage_definitions
from pathlib import Path
import os
import shlex
import subprocess
import sys
import tempfile
from check_s02_source import ROOT, function


def native_construction():
    declarations=r'''
#include "common.h"
#include "liststorage.h"
#include <assert.h>
#include <stdlib.h>
#include <setjmp.h>
GAME game;GDATA gd;SEL sel;PLAYER rgplr[16];POINT16 rgptPlan[999];
static SHDEF ships[16],bases[10];SHDEF *rglpshdef[16],*rglpshdefSB[16];
static FLEET fleetBodies[512],*fleetSlots[512];FLEET **rglpfl=fleetSlots;
static union {max_align_t align;uint8_t data[sizeof(PLORD)+3*sizeof(ORDER)];} orderStorage[512];
THING *lpThings;int16_t cThing,cThingAlloc,cFleet,idPlayer;
int32_t lRandSeed1,lRandSeed2;
static PLANET planet;PLANET *lpPlanets=&planet;int16_t cPlanet=1;
static uint16_t extra[999];uint16_t *vrgPlanResExtra=extra;
int32_t *vrgwtPopScore;int16_t *vrgiplrPopScore;
static HULDEF catalogHull;static struct {void *p;size_t bytes;int live;} heap[32];static int heaps;
static int checks,canBuild,catalogCalls,canCalls,lookups,fills,shipKills,packetKills,warpCalls;
static int16_t cap,operableCap,removed,gross,warpValues[2],twoValues[2],terraformChoice,partIndex;
static int terraformCalls,randomCalls,randomBounds[16],randomValues[16],messages,newFleets,routes,defaults;
static MessageId messageIds[64];static int16_t messageArgs[64][7],messageScanner[64];
static int warpBase[2],resources[4];static int32_t quote[4];static jmp_buf boundary;
static int slot(void *p){for(int i=heaps-1;i>=0;i--)if(heap[i].live && heap[i].p==p)return i;abort();}
void *LpAlloc(uint16_t size,HeapType type){(void)type;assert(heaps<32 && size);void *p=calloc(1,size);assert(p);heap[heaps].p=p;heap[heaps].bytes=size;heap[heaps++].live=1;return p;}
void *LpReAlloc(void *p,uint16_t size,HeapType type){(void)type;int at=slot(p);size_t old=heap[at].bytes;void *next=realloc(p,size);assert(next);heap[at].live=0;assert(heaps<32);
    if(size>old)memset((char *)next+old,0,size-old);heap[heaps].p=next;heap[heaps].bytes=size;heap[heaps++].live=1;return next;}
void FreeLp(void *p,HeapType type){(void)type;int at=slot(p);heap[at].live=0;free(p);}
int16_t FSendPlrMsg(int16_t player,MessageId id,MsgGoto target,int16_t a,int16_t b,int16_t c,int16_t d,int16_t e,int16_t f,int16_t g){
    (void)target;assert(player>=0 && player<16 && messages<64);messageIds[messages]=id;int16_t args[7]={a,b,c,d,e,f,g};memcpy(messageArgs[messages],args,sizeof(args));
    messageScanner[messages]=planet.iScanner;messages++;return TRUE;
}
int16_t FSendPlrMsg2(int16_t player,MessageId id,MsgGoto target,int16_t a,int16_t b){return FSendPlrMsg(player,id,target,a,b,0,0,0,0,0);}
int16_t FRemovePlayerMessage(int16_t player,MessageId id,MsgGoto target){assert(player==0 && target==planet.id);(void)id;return removed;}
int16_t CMaxFactories(PLANET *p,int16_t player){assert(p==&planet && !player);return cap;}
int16_t CMaxMines(PLANET *p,int16_t player){return CMaxFactories(p,player);}
int16_t CMaxDefenses(PLANET *p,int16_t player){return CMaxFactories(p,player);}
int16_t CMaxOperableFactories(PLANET *p,int16_t player,int16_t next){assert(next && p==&planet && !player);return operableCap;}
int16_t CMaxOperableMines(PLANET *p,int16_t player,int16_t next){return CMaxOperableFactories(p,player,next);}
int16_t CMaxOperableDefenses(PLANET *p,int16_t player,int16_t next){return CMaxOperableFactories(p,player,next);}
int16_t IpctCanTerraformLppl(PLANET *p){assert(p==&planet);return 10;}
int32_t ChgPopFromPlanet(PLANET *p,int16_t update){assert(p==&planet && !update);return -1;}
int16_t PctPlanetDesirability(PLANET *p,int16_t player){assert(p==&planet && !player);return 100;}
int16_t CResourcesAtPlanet(PLANET *p,int16_t player){assert(p==&planet && !player);return gross;}
void GetProductionCosts(PLANET *p,PROD *row,uint32_t *cost,int16_t player,int16_t one){assert(p==&planet && !player && one);(void)row;memcpy(cost,quote,sizeof(quote));}
void MineMinerals(void){}
void UpdatePopulations(void){}
void UpdateResearchStatus(int16_t generate){assert(generate);}
void RandomEvents(void){}
int16_t FCanBuildShdef(SHDEF *design,int16_t player){assert(player==0 && design);canCalls++;return (int16_t)canBuild;}
HULDEF *LphuldefFromId(int16_t hull){(void)hull;catalogCalls++;return &catalogHull;}
int16_t IWarpMAFromLppl(PLANET *p,int16_t *two){assert(p==&planet);int at=warpCalls<2 ? warpCalls:1;warpBase[at]=p->isb;warpCalls++;if(two)*two=twoValues[at];return warpValues[at];}
int16_t FLookupPlanet(int16_t id,PLANET *out){assert(id==planet.id);lookups++;*out=planet;return TRUE;}
void FillSelProdLB(void){fills++;}
FLEET *LpflNew(int16_t player,int16_t id){assert(!player && id==planet.id);newFleets++;FLEET *f=&fleetBodies[3];f->iPlayer=player;f->idPlanet=id;f->lpplord=LpStartPl<PLORD,ORDER>(orderStorage[3].data,sizeof(orderStorage[3].data));rgplr[player].cFleet++;return f;}
int32_t LGetFleetStat(FLEET *f,int16_t stat){assert(f && stat==1);return 12345;}
void AutoRouteFleet(FLEET *f,PLANET *p){assert(f && p==&planet);routes++;f->lpplord->rgord()[1].iWarp=1;}
void AutoFleetOrder(FLEET *f,PLANET *p){assert(f && p==&planet);defaults++;}
int16_t IBestTerraform(PLANET *p,int16_t help){assert(p==&planet && help);terraformCalls++;if(terraformChoice<-3 || terraformChoice>3)longjmp(boundary,1);return terraformChoice;}
void LookupBestPlanetaryScanner(PART *part){assert(idPlayer==planet.iPlayer);part->hs.iItem=(uint8_t)partIndex;}
static int16_t NativeRandom(int16_t bound);
int16_t Random(int16_t bound){assert(randomCalls<16);int16_t result=NativeRandom(bound);randomBounds[randomCalls]=bound;randomValues[randomCalls++]=result;return result;}
static void NativeKillQueuedShips(PLANET *p);
static void NativeKillQueuedMassPackets(PLANET *p);
void KillQueuedShips(PLANET *p){shipKills++;NativeKillQueuedShips(p);}
void KillQueuedMassPackets(PLANET *p){packetKills++;NativeKillQueuedMassPackets(p);}
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"construction witness %d: %s\n",checks,#x);return 1;}}while(0)
'''
    bodies='\n'.join(function('src/core/memory.c',s) for s in ['PL *LpplAlloc(','PL *LpplReAlloc(','void FreePl('])+'\n'
    bodies+=function('src/core/thing.c','THING *LpthNew(')+'\n'
    bodies+=function('src/core/utilgen.c','int16_t STARS_TEST_IMPL(Random)(').replace('int16_t STARS_TEST_IMPL(Random)(', 'static int16_t NativeRandom(',1)+'\n'
    bodies+=next(line for line in (ROOT/'src/core/race.c').read_text().splitlines() if line.startswith('int16_t GetRaceStat('))+'\n'
    bodies+=function('src/core/race.c','int16_t GetRaceGrbit(')+'\n'
    for name in ['KillQueuedShips','KillQueuedMassPackets']:
        bodies+=function('src/core/build.c','void '+name+'(').replace('void '+name+'(', 'static void Native'+name+'(',1)+'\n'
    bodies+='\n'.join(function('src/core/turn2.c',s) for s in ['int16_t FBuildObject(','void CreateShip(','int16_t CBuildProdItem(','void Produce('])
    witnesses=r'''
static PROD row(unsigned count,unsigned item,unsigned kind,unsigned pct){PROD r={};r.cItem=count;r.iItem=item;r.grobj=kind;r.pct=pct;r.unused=31;return r;}
static void reset(void){
    for(int i=0;i<heaps;i++)if(heap[i].live)free(heap[i].p);heaps=0;
    memset(&game,0,sizeof(game));memset(&gd,0,sizeof(gd));memset(&sel,0,sizeof(sel));memset(rgplr,0,sizeof(rgplr));
    memset(ships,0,sizeof(ships));memset(bases,0,sizeof(bases));memset(&planet,0,sizeof(planet));memset(fleetBodies,0,4*sizeof(fleetBodies[0]));memset(orderStorage,0,4*sizeof(orderStorage[0]));
    for(int i=0;i<4;i++){fleetSlots[i]=&fleetBodies[i];fleetBodies[i].lpplord=LpStartPl<PLORD,ORDER>(orderStorage[i].data,sizeof(orderStorage[i].data));}
    rglpshdef[0]=ships;rglpshdefSB[0]=bases;lpThings=NULL;cThing=cThingAlloc=cFleet=0;
    canBuild=1;catalogCalls=canCalls=lookups=fills=shipKills=packetKills=warpCalls=0;cap=operableCap=100;removed=0;gross=100;
    warpValues[0]=warpValues[1]=10;twoValues[0]=twoValues[1]=0;terraformChoice=1;terraformCalls=randomCalls=messages=newFleets=routes=defaults=0;
    partIndex=0;idPlayer=3;lRandSeed1=7;lRandSeed2=11;game.cPlayer=3;game.fNoRandom=1;gd.fGeneratingTurn=1;
    planet.id=5;planet.iPlayer=0;planet.iScanner=31;planet.idFling=8;planet.iWarpFling=6;
    planet.rgwtMin[0]=10;planet.rgwtMin[1]=20;planet.rgwtMin[2]=30;planet.rgwtMin[3]=1000;
    rgptPlan[5]=(POINT16){10,20};catalogHull.hul.wtCargoMax=123;quote[0]=quote[1]=quote[2]=0;quote[3]=10;extra[5]=0;
    for(int i=0;i<4;i++)resources[i]=10*(i+1);
}
static int16_t call(unsigned kind,int item,int built){return FBuildObject(&planet,(GrobjClass)kind,(int16_t)item,(int16_t)built,(int32_t *)resources);}
static int16_t s16(int n){return (int16_t)(uint16_t)n;}
static int damageExpected(int prior,int add,unsigned word,unsigned hp,unsigned *out){
    int share=word%128,damage=word/128;if(!prior || !damage){*out=0;return 1;}
    int affected=s16(share*prior/100);if(!affected)affected=1;int64_t product=(int64_t)(hp*(unsigned)damage/10)*affected;
    if(product<INT32_MIN || product>INT32_MAX)return 0;
    int original=(int)(product/50),total=s16(prior+add);if(!total)return 0;
    unsigned newShare=(unsigned)((affected*100)/total)&127;if(!newShare)newShare=1;
    int next=s16((int)newShare*total/100);if(!next)next=1;int64_t scaled=(int64_t)original*5;
    if(scaled<INT32_MIN || scaled>INT32_MAX || !hp)return 0;scaled=scaled/next*100;
    if(scaled<INT32_MIN || scaled>INT32_MAX)return 0;
    *out=newShare+128*((unsigned)(scaled/(int)hp)&511);return 1;
}
int main(void){
    static_assert(sizeof(PROD)==4 && sizeof(DV)==2 && sizeof(THPACK)==10 && sizeof(THING)==18,"native object layout");
    reset();CHECK(!call(0,0,1) && !call(1,-1,1) && !call(1,28,1));
    for(int raw=-32768;raw<=32767;raw++)for(unsigned which=0;which<6;which++){
        reset();unsigned items[]={0,8,1,7,2,9};unsigned item=items[which];cap=(int16_t)raw;removed=3;
        unsigned before=which<2 ? 2:which<4 ? 1:3;planet.cMines=2;planet.cFactories=1;planet.cDefenses=3;
        int allowed=s16(raw-(int)before),n=5>=allowed ? allowed:5;int accepted=n>0;
        CHECK(call(1,item,5)==accepted);
        unsigned after=which<2 ? planet.cMines:which<4 ? planet.cFactories:planet.cDefenses;
        CHECK(after==(accepted ? (before+(unsigned)n)%4096:before));
        CHECK(messages==accepted);
        if(accepted){MessageId id=which<2 ? idmHaveBuiltMine:which<4 ? idmHaveBuiltFactory:idmHaveBuiltDefenseOutpost;
            int sum=s16(n+3);CHECK(messageIds[0]==id+(sum>1));CHECK(sum<=1 || messageArgs[0][0]==sum);}
    }
    reset();planet.cMines=4095;cap=-32768;CHECK(call(1,0,1) && planet.cMines==0);
    reset();cap=32767;removed=32767;CHECK(call(1,0,32767) && planet.cMines==4095 && messageIds[0]==idmHaveBuiltMine);
    for(unsigned item=0;item<3;item++){reset();unsigned ids[]={3,10,11};PLANET before=planet;CHECK(call(1,ids[item],-1));CHECK(memcmp(&before,&planet,sizeof(planet))==0 && !messages);}
    for(int choice=-3;choice<=3;choice++)for(int start=-128;start<=127;start++){
        reset();terraformChoice=(int16_t)choice;for(int i=0;i<3;i++)planet.rgEnvVar[i]=(int8_t)start;CHECK(call(1,12,1));
        CHECK(terraformCalls==1 && messages==(choice!=0));
        for(int i=0;i<3;i++){int expected=start;if(choice && i==abs(choice)-1){expected+=choice<0 ? -1:1;if(expected<1)expected=1;if(expected>99)expected=99;}CHECK(planet.rgEnvVar[i]==expected);}
    }
    reset();terraformChoice=0;CHECK(call(1,4,-1) && terraformCalls==65535);
    reset();terraformChoice=4;int escaped=setjmp(boundary);if(!escaped){call(1,12,1);CHECK(0);}CHECK(escaped==1 && !messages);
    for(int ar=0;ar<2;ar++){
        reset();rgplr[0].rgAttr[rsMajorAdv]=ar ? raMacintosh:raCheapCol;planet.cMines=3;planet.cFactories=4;planet.cDefenses=5;planet.iScanner=6;
        CHECK(call(1,13,10) && randomCalls==12 && messages==3);
        CHECK(planet.cMines==(unsigned)(ar ? 3:0) && planet.iScanner==(unsigned)(ar ? 6:31));
        for(int i=0;i<3;i++){CHECK(planet.rgwtMin[i]==0);CHECK(planet.rgEnvVar[i]==randomValues[4*i]+randomValues[4*i+1]+1 && planet.rgEnvVarOrig[i]==planet.rgEnvVar[i]);
            CHECK(planet.rgMinConc[i]==randomValues[4*i+2]+randomValues[4*i+3]+25);CHECK(randomBounds[4*i]==50 && randomBounds[4*i+1]==50 && randomBounds[4*i+2]==40 && randomBounds[4*i+3]==40);}
        CHECK(resources[0]==10 && resources[3]==40 && (lRandSeed1!=7 || lRandSeed2!=11));
        CHECK(messageScanner[0]==6 && messageScanner[1]==6 && messageScanner[2]==6);
    }
    for(unsigned part=0;part<256;part++){
        reset();partIndex=(int16_t)part;planet.iScanner=7;CHECK(call(1,27,1));CHECK(planet.iScanner==part%32 && idPlayer==-1 && messageArgs[0][2]==(int)part && messageScanner[0]==7);
    }
    for(unsigned item=18;item<27;item++){reset();CHECK(call(1,item,1) && planet.iScanner==item-18 && idPlayer==3);}
    const int16_t abilities[]={-32768,-1,0,1,3,10,16,32767};
    const int16_t quantities[]={-1,1,1000};const int primaries[]={0,6,7,8};const int packets[]={6,14,15,16,17};
    for(unsigned t=0;t<4;t++)for(unsigned w=0;w<8;w++)for(int two=0;two<2;two++)for(int configured=0;configured<16;configured++)
    for(int destination=0;destination<2;destination++)for(unsigned b=0;b<3;b++)for(unsigned item=0;item<5;item++){
        reset();rgplr[0].rgAttr[rsMajorAdv]=(int8_t)primaries[t];warpValues[0]=abilities[w];twoValues[0]=(int16_t)two;planet.iWarpFling=configured;planet.idFling=destination ? 8:0;
        int accepted=abilities[w]!=0 && destination;CHECK(call(1,packets[item],quantities[b])==accepted);
        if(!accepted){CHECK(cThing==0);continue;}
        int mixed=packets[item]==6 || packets[item]==17;int size=mixed ? (primaries[t]==6 ? 25:40):(primaries[t]==6 ? 70:100);
        int quantity=size*quantities[b];if(quantity>32760)quantity=32760;quantity=s16(quantity);
        int asked=configured+4;if(asked<5 || asked>abilities[w]+3)asked=s16(abilities[w]+two);
        unsigned decay=(uint16_t)(asked<=abilities[w]+two ? 0:asked-abilities[w]-two);
        if(primaries[t]==7 && s16((int)decay)<3)decay=(uint16_t)(decay+1);
        CHECK(cThing==1 && lpThings[0].thp.iWarp==((unsigned)s16(asked-4)&15) && lpThings[0].thp.iDecayRate==(decay&3) && lpThings[0].thp.idPlanet==7);
        unsigned weight=0;for(int i=0;i<3;i++){int ore=mixed || i==packets[item]-14 ? quantity:0;CHECK(lpThings[0].thp.rgwtMin[i]==ore);weight=(weight+s16(ore+9)/10)&16383;}
        CHECK(lpThings[0].thp.wtMax==weight);
    }
    // Packet quantity saturation and weight's signed-16 narrowing.
    reset();CHECK(call(1,17,1000) && cThing==1);CHECK(lpThings[0].thp.rgwtMin[0]==32760 && lpThings[0].thp.wtMax==6556);
    reset();warpValues[0]=0;CHECK(!call(1,6,1) && messageIds[0]==idmMineralPacketFormedHasDisintegratedBecausePlanet);
    reset();planet.idFling=0;CHECK(!call(1,6,1) && messageIds[0]==idmMineralPacketFormedHasDisintegratedBecauseDidnt);
    reset();THING *p=LpthNew(0,ithMineralPacket);p->pt=rgptPlan[5];p->thp.iWarp=6;p->thp.idPlanet=7;p->thp.iDecayRate=0;p->thp.wtMax=1629;
    for(int i=0;i<3;i++)p->thp.rgwtMin[i]=32760;CHECK(call(1,17,1) && cThing==1);CHECK(p->thp.rgwtMin[0]==32760 && p->thp.wtMax==6556 && messageIds[0]==idmHasProducedMineralPacketWhichHasCombined);
    p->thp.wtMax=1630;CHECK(call(1,17,1) && cThing==2);
    reset();cThing=cThingAlloc=512;lpThings=static_cast<THING *>(LpAlloc((uint16_t)(512*sizeof(THING)),htThings));
    for(int i=0;i<512;i++){lpThings[i].ith=ithMineralPacket;lpThings[i].iplr=0;lpThings[i].id=i;lpThings[i].pt.x=99;}
    CHECK(call(1,17,1) && cThing==512 && messageIds[0]==idmHasOrdersBuildMineralPacketEitherDoesnt);
    // Fleet limit uses first-order point, first null/greater owner and strict
    // 32766-built > prior count, independently of fleet point or dead flag.
    reset();planet.fStarbase=1;rgplr[0].cFleet=512;CHECK(!call(2,0,1) && !newFleets && messageIds[0]==idmStarbaseBuiltNewShipSTypeLost);
    reset();planet.fStarbase=1;rgplr[0].cFleet=513;CHECK(call(2,0,1) && newFleets==1 && defaults==1 && fleetBodies[3].rgwtMin[4]==12345);
    reset();CHECK(!call(2,0,1) && !canCalls && !messages);
    for(int base=0;base<2;base++)for(int isFree=0;isFree<2;isFree++)for(int available=0;available<2;available++){
        reset();planet.fStarbase=1;canBuild=available;if(base)bases[0].fFree=isFree;else ships[0].fFree=isFree;
        CHECK(call(2,base ? 16:0,1)==(!isFree && available));CHECK(canCalls==!isFree);
        if(isFree || !available)CHECK(messages==!base);
    }
    const uint16_t cargos[]={0,1,65535};
    for(unsigned cargo=0;cargo<3;cargo++){reset();bases[0].hul.wtCargoMax=cargos[cargo];CHECK(call(2,16,1));
        CHECK(messageIds[0]==idmHasBuiltNew+(cargo==0 ? 0:cargo==2 ? 2:1) && messageArgs[0][2]==123 && catalogCalls==1);}
    reset();planet.fStarbase=1;rgplr[0].cFleet=512;cFleet=1;fleetSlots[0]=NULL;CHECK(!call(2,0,1));
    reset();planet.fStarbase=1;rgplr[0].cFleet=512;cFleet=2;fleetBodies[0].iPlayer=1;fleetBodies[1].lpplord->rgord()[0].pt=rgptPlan[5];CHECK(!call(2,0,1));
    reset();planet.fStarbase=1;rgplr[0].cFleet=512;cFleet=1;fleetBodies[0].lpplord->rgord()[0].pt=rgptPlan[5];fleetBodies[0].rgcsh[0]=32765;CHECK(!call(2,0,1));
    fleetBodies[0].rgcsh[0]=32764;fleetBodies[0].fDead=1;CHECK(call(2,0,1) && fleetBodies[0].rgcsh[0]==32765);
    for(unsigned word=0;word<65536;word++){
        reset();planet.fStarbase=1;rgplr[0].cFleet=512;cFleet=1;fleetBodies[0].lpplord->rgord()[0].pt=rgptPlan[5];fleetBodies[0].rgcsh[0]=10;fleetBodies[0].rgdv[0].dp=(uint16_t)word;ships[0].hul.dp=100;
        unsigned expected;CHECK(damageExpected(10,1,word,100,&expected));CHECK(call(2,0,1));CHECK(fleetBodies[0].rgdv[0].dp==expected && fleetBodies[0].rgcsh[0]==11 && ships[0].cBuilt==1 && ships[0].cExist==1);
    }
    reset();fleetBodies[0].rgcsh[0]=32767;ships[0].cBuilt=ships[0].cExist=UINT32_MAX;CreateShip(0,&fleetBodies[0],0,1);
    CHECK(fleetBodies[0].rgcsh[0]==-32768 && ships[0].cBuilt==0 && ships[0].cExist==0);
    // Actual base invalidators reset surviving base progress in both the
    // copied prefix and old physical source slot.
    reset();planet.fStarbase=1;planet.isb=0;bases[0].hul.ihuldef=static_cast<HulDef>(10);bases[1].hul.ihuldef=static_cast<HulDef>(5);bases[0].cExist=1;
    planet.lpplprod=LpplAlloc<PLPROD>(4,3,htOrd);planet.lpplprod->iprodMac=3;
    planet.lpplprod->rgprod()[0]=row(1,0,2,50);planet.lpplprod->rgprod()[1]=row(1,16,2,59);planet.lpplprod->rgprod()[2]=row(1,14,1,0);
    sel.grobj=grobjPlanet;sel.pl.id=planet.id;warpValues[0]=warpValues[1]=0;
    CHECK(call(2,17,100) && shipKills==1 && packetKills==1 && lookups==2 && fills==2);
    CHECK(planet.isb==1 && bases[0].cExist==0 && bases[1].cExist==1 && bases[1].cBuilt==1 && !planet.idFling && !planet.iWarpFling);
    CHECK(planet.lpplprod->iprodMac==1 && planet.lpplprod->rgprod()[0].iItem==16 && !planet.lpplprod->rgprod()[0].pct);
    CHECK(!planet.lpplprod->rgprod()[1].pct && warpBase[0]==0 && warpBase[1]==1);
    reset();planet.fStarbase=1;planet.isb=0;bases[0].cExist=0;bases[0].cBuilt=UINT32_MAX;CHECK(call(2,16,0));CHECK(bases[0].cExist==0 && bases[0].cBuilt==0);
    for(unsigned prior=0;prior<10;prior++)for(unsigned next=0;next<10;next++)for(int has=0;has<2;has++)for(int oldWarp=0;oldWarp<2;oldWarp++)for(int newWarp=0;newWarp<2;newWarp++){
        reset();planet.fStarbase=has;planet.isb=prior;planet.iWarpFling=9;
        for(unsigned i=0;i<10;i++){bases[i].cBuilt=i;bases[i].cExist=10+i;}
        warpValues[0]=oldWarp ? 10:0;warpValues[1]=newWarp ? 12:0;twoValues[1]=1;
        CHECK(call(2,16+(int)next,9));
        CHECK(warpCalls==(oldWarp ? 1:2) && planet.isb==next && planet.fStarbase);
        CHECK(bases[next].cBuilt==next+1 && bases[next].cExist==10+next+1-(unsigned)(has && prior==next));
        CHECK(planet.iWarpFling==(unsigned)(oldWarp ? 9:newWarp ? 9:0) && planet.idFling==(unsigned)(oldWarp || newWarp ? 8:0));
    }
    // Real object failures after actual construction charges, not a supplied
    // FBuildObject result. No compatible fleet at the exact 512 limit.
    reset();planet.id=0;rgptPlan[0]=(POINT16){10,20};game.cPlayer=1;game.cPlanMax=1;
    planet.fStarbase=1;bases[0].hul.ihuldef=ihuldefOrbitalFort;rgplr[0].cFleet=512;cFleet=512;ships[0].cBuilt=ships[0].cExist=512;
    for(int i=0;i<512;i++){memset(&fleetBodies[i],0,sizeof(FLEET));memset(orderStorage[i].data,0,sizeof(orderStorage[i].data));fleetSlots[i]=&fleetBodies[i];
        fleetBodies[i].ifl=i;fleetBodies[i].iPlayer=0;fleetBodies[i].iplr=0;fleetBodies[i].idPlanet=idPlanetDeepSpace;fleetBodies[i].rgcsh[0]=1;fleetBodies[i].pt=(POINT16){99,99};
        fleetBodies[i].lpplord=LpStartPl<PLORD,ORDER>(orderStorage[i].data,sizeof(orderStorage[i].data));fleetBodies[i].cord=1;fleetBodies[i].lpplord->iordMac=1;fleetBodies[i].lpplord->rgord()[0].pt=fleetBodies[i].pt;}
    planet.lpplprod=LpplAlloc<PLPROD>(4,1,htOrd);planet.lpplprod->iprodMac=1;planet.lpplprod->rgprod()[0]=row(1,0,2,0);
    Produce();CHECK(!planet.lpplprod && ships[0].cBuilt==512 && ships[0].cExist==512 && rgplr[0].lResLastYear==90 && planet.rgwtMin[0]==10 && !newFleets);
    reset();cap=2;operableCap=5;planet.lpplprod=LpplAlloc<PLPROD>(4,1,htOrd);planet.lpplprod->iprodMac=1;planet.lpplprod->rgprod()[0]=row(5,7,1,0);
    Produce();CHECK(planet.cFactories==2 && rgplr[0].lResLastYear==50 && !planet.lpplprod);
    // Genesis clears native planet stock directly; the caller's working stock
    // is later written back. Keep this distinction visible in the specification.
    reset();planet.lpplprod=LpplAlloc<PLPROD>(4,1,htOrd);planet.lpplprod->iprodMac=1;planet.lpplprod->rgprod()[0]=row(1,13,1,0);
    Produce();CHECK(randomCalls==12 && planet.rgwtMin[0]==10 && planet.rgwtMin[1]==20 && planet.rgwtMin[2]==30 && rgplr[0].lResLastYear==90);
    reset();printf("Construction effects actual-source fixtures: %d assertions passed\n",checks);return 0;
}
'''
    return declarations + storage_declarations(), bodies + storage_definitions('return heap[slot(const_cast<void *>(lp))].bytes;'), witnesses


def main():
    declarations,bodies,witnesses=native_construction()
    with tempfile.TemporaryDirectory(prefix='construction-effects-') as work:
        path=Path(work)
        for generator in ['text/textgen.py','data/datagen.py']:
            subprocess.run([sys.executable,str(ROOT/generator),str(path)],check=True,stdout=subprocess.DEVNULL)
        source=path/'witness.cpp';binary=path/'witness';source.write_text(declarations+bodies+witnesses)
        # Existing reconstructed bodies have an unused resource parameter and
        # corresponding signed/unsigned cost-pointer convention; match project
        # warning exclusions while making other warnings and sanitizers fatal.
        subprocess.run(shlex.split(os.environ.get('CXX','c++'))+['-std=gnu++20','-include',str(ROOT/'src/platform/value.h'),'-x','c++','-Wall','-Wextra','-Werror','-Wno-misleading-indentation',
            '-Wno-unused-parameter','-fsanitize=address,undefined','-fno-sanitize-recover=all',
            '-iquote',str(ROOT/'src/core'),'-iquote',str(ROOT/'src/platform'),'-iquote',str(path),str(source),'-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True)


if __name__=='__main__':
    main()
