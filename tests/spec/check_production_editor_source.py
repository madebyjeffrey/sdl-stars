#!/usr/bin/env python3
"""Source-extracted production initialization and Add/Remove/reorder witnesses.

Queue/catalog layouts and command bodies are native. Planet/design/helper and
GUI inputs are bounded projections/doubles. A memcpy guard stops at overlapping
sanitation ranges before undefined behavior; it does not repair the algorithm.
"""
from cpp_production_storage import declarations as storage_declarations, definitions as storage_definitions
from pathlib import Path
from check_s02_source import struct_start
import os
import shlex
import subprocess
import tempfile

from check_s02_source import ROOT, function, enum_source
from check_s02_state_source import between


def main():
    types = (ROOT / 'src/core/structs.h').read_text()
    enums = enum_source()
    ui = function('src/ui/produceui.cpp', 'void ProdCommandHandler(')
    declarations = r'''
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#define TRUE 1
#define FALSE 0
#define LOWORD(x) ((uint16_t)(x))
#define min(a,b) ((a)<(b)?(a):(b))
#define grobjPlanet 1
#define grobjFleet 2
#define raMacintosh 8
#define raTerra 3
#define rsMajorAdv 0
#define hstPlanetary 1
#define iplanetaryGenesisDevice 1
#define mdPartAvailAvailable 2
#define htOrd 1
#define VK_SHIFT 1
#define VK_CONTROL 2
#define IDC_PRODUCTION_ADD 3
#define IDC_PRODUCTION_REMOVE 4
#define IDC_PRODUCTION_ITEM_DOWN 5
#define IDC_PRODUCTION_ITEM_UP 6
#define IDC_PRODUCTION_AVAILABLE_ITEMS 7
#define IDC_PRODUCTION_QUEUE 8
#define LB_GETCURSEL 9
#define LB_SETCURSEL 10
#define MB_OK 0
typedef int HeapType;
typedef int HWND;
'''
    start = enums.index('enum ProdItemType : uint16_t {')
    declarations += enums[start:enums.index('\n};', start)+3] + '\n'
    for tag, alias in [('_prod','PROD'),('PLPROD','PLPROD'),('_pl','PL')]:
        start = struct_start(types, tag)
        declarations += types[start:types.index('\n};',start)+3]+f'\ntypedef struct {tag} {alias};\n'
    declarations += r'''
typedef struct { struct { int grhst,iItem; } hs; } PART;
typedef struct { struct { uint16_t wtCargoMax; } hul; } HULDEF;
typedef struct { int fFree,fGift;struct { uint16_t wtEmpty,ihuldef; } hul; } SHDEF;
static struct {
    struct {
        PLPROD *lpplprod;int fStarbase,isb;
        uint32_t cMines:12,cFactories:12;
        uint32_t cDefenses:12,iScanner:5,fNoResearch:1;
    } pl;
} sel;
static struct { int fNoResearchSav,fTutorial; } gd;
static int idPlayer,rgplr[1],primary,genesis,warp,maxFactories,maxMines,maxDefenses,terraform,tutorials;
static SHDEF rgshdef[16],baseDefs[10],*rglpshdefSB[1]={baseDefs};
static HULDEF hullDefs[10];
static PROD catalog[64],*pProdGlob=catalog;
static int16_t cProdGlob;
static PLPROD *lpplProdGlob;
static int shift,ctrl,queueFill,sourceFill,redraw,beeps,setSelection,allocations,reallocations,failReallocation,checks;
static int32_t sourceSelection,queueSelection;
static size_t lastBytes,overlapBytes;
static int overlapOffset;
static jmp_buf env;
static void *LpAlloc(uint16_t cb,HeapType ht) {(void)ht;allocations++;lastBytes=cb;void *p=calloc(1,cb);if(!p)abort();return p;}
static void *LpReAlloc(void *p,uint16_t cb,HeapType ht) {(void)ht;reallocations++;lastBytes=cb;if(failReallocation)longjmp(env,2);p=realloc(p,cb);if(!p)abort();return p;}
static void FreeLp(void *p,HeapType ht) {(void)ht;free(p);}
static HULDEF *LphuldefFromId(int id) {if(id<0 || id>=10)abort();return &hullDefs[id];}
static int FLookupPart(PART *p) {(void)p;return genesis ? mdPartAvailAvailable:0;}
static int16_t IWarpMAFromLppl(void *p,void *q) {(void)p;(void)q;return warp;}
static int16_t CMaxFactories(void *p,int player) {(void)p;(void)player;return maxFactories;}
static int16_t CMaxMines(void *p,int player) {(void)p;(void)player;return maxMines;}
static int16_t CMaxDefenses(void *p,int player) {(void)p;(void)player;return maxDefenses;}
static int16_t IpctCanTerraformLppl(void *p) {(void)p;return terraform;}
static int GetRaceStat(void *p,int stat) {(void)p;(void)stat;return primary;}
static void AdvanceTutor(void) {tutorials++;}
static void *guardedMemcpy(void *dst,const void *src,size_t n) {
    uintptr_t d=(uintptr_t)dst,s=(uintptr_t)src;
    if(n && d<s+n && s<d+n){overlapBytes=n;overlapOffset=((char *)dst-(char *)lpplProdGlob->rgprod())/4;longjmp(env,1);}
    return memcpy(dst,src,n);
}
static HWND GetDlgItem(HWND hwnd,int id) {(void)hwnd;return id;}
static intptr_t SendMessage(HWND hwnd,int message,int value,intptr_t param) {
    (void)param;if(message==LB_GETCURSEL)return hwnd==IDC_PRODUCTION_QUEUE ? queueSelection:sourceSelection;
    if(message==LB_SETCURSEL){setSelection=value;queueSelection=value;return value;}abort();
}
static int GetAsyncKeyState(int key) {return (key==VK_SHIFT ? shift:ctrl) ? 0x8000:0;}
static void FillPlanetProdLB(HWND hwnd,PLPROD *p,void *q) {(void)hwnd;(void)p;(void)q;queueFill++;}
static void FillProdSrcLB(HWND hwnd,int value) {(void)hwnd;(void)value;sourceFill++;}
static void MessageBeep(int value) {(void)value;beeps++;}
#define CHECK(x) do {checks++;if(!(x)){fprintf(stderr,"witness %d failed: %s\n",checks,#x);return 1;}}while(0)
'''
    bodies = function('src/core/memory.cpp','PL *LpplAlloc(')+'\n'
    bodies += function('src/core/memory.cpp','PL *LpplReAlloc(')+'\n'
    bodies += function('src/core/memory.cpp','void FreePl(')+'\n'
    bodies += '#define memcpy guardedMemcpy\n'+function('src/core/produce.cpp','void InitProduction(')+'\n#undef memcpy\n'
    block = between(ui,'    case IDC_PRODUCTION_ADD:', '    case IDC_PRODUCTION_CLEAR:')
    move = between(ui,'    case IDC_PRODUCTION_ITEM_DOWN:', '    case IDC_HELP:')
    bodies += '''static void command(int cmd) {
int hwnd=1;int32_t lSel;int16_t iSrc,iDst,iMac,c,fRefillSrc;PROD prod,prodLast;
if(cmd==100)goto AddItem;if(cmd==101)goto RemoveItem;
switch(cmd){'''+block+move+'''}return;RedrawText:redraw++;}
'''
    bodies += r'''
static PROD row(unsigned count,unsigned item,unsigned kind) {PROD p={};p.cItem=count;p.iItem=item;p.grobj=kind;return p;}
static void reset(unsigned n,int hasQueue) {
    int i;if(lpplProdGlob)FreePl(lpplProdGlob);lpplProdGlob=NULL;
    if(sel.pl.lpplprod)FreePl(sel.pl.lpplprod);memset(&sel,0,sizeof(sel));memset(&gd,0,sizeof(gd));
    if(hasQueue){sel.pl.lpplprod=LpplAlloc<PLPROD>(4,n,htOrd);sel.pl.lpplprod->iprodMac=n;}
    memset(catalog,0,sizeof(catalog));pProdGlob=catalog;cProdGlob=0;
    memset(rgshdef,0,sizeof(rgshdef));memset(baseDefs,0,sizeof(baseDefs));memset(hullDefs,0,sizeof(hullDefs));
    for(i=0;i<16;i++)rgshdef[i].fFree=1;
    for(i=0;i<10;i++){baseDefs[i].fFree=1;baseDefs[i].hul.ihuldef=i;}
    primary=genesis=warp=maxFactories=maxMines=maxDefenses=terraform=tutorials=0;
    sel.pl.iScanner=31;shift=ctrl=queueFill=sourceFill=redraw=beeps=allocations=reallocations=failReallocation=0;
    sourceSelection=0;queueSelection=-1;setSelection=-99;overlapBytes=0;overlapOffset=-1;
}
static void working(unsigned n) {reset(0,0);lpplProdGlob=LpplAlloc<PLPROD>(4,n,htOrd);lpplProdGlob->iprodMac=n;allocations=0;}
'''
    witnesses = r'''
int main(void) {
    int i,mode,available,request;PROD a=row(10,7,1),b=row(1,8,1),c=row(1,9,1);PROD bad=row(9,0,2); // Ship design 0 with no construction base is unavailable.
    CHECK(sizeof(PROD)==4 && sizeof(PLPROD)==4);
    reset(0,0);sel.pl.fNoResearch=1;gd.fTutorial=1;InitProduction(catalog);
    CHECK(gd.fNoResearchSav && lpplProdGlob->iprodMac==0 && lpplProdGlob->iprodMax==2 && cProdGlob==9 && tutorials==1);
    CHECK(catalog[0].iItem==11 && catalog[1].iItem==27 && catalog[2].iItem==0 && catalog[8].iItem==6);
    reset(0,1);sel.pl.isb=15;InitProduction(catalog);CHECK(lpplProdGlob->iprodMax==0 && lpplProdGlob->iprodMac==0 && lastBytes==4);
    reset(0,0);sel.pl.fStarbase=1;genesis=1;warp=9;maxFactories=maxMines=5000;maxDefenses=1024;terraform=1024;
    hullDefs[0].hul.wtCargoMax=100;
    for(i=0;i<16;i++){rgshdef[i].fFree=0;rgshdef[i].hul.wtEmpty=100;}
    for(i=0;i<10;i++)baseDefs[i].fFree=0;
    InitProduction(catalog);CHECK(cProdGlob==43);
    for(i=0;i<16;i++)CHECK(catalog[i].grobj==2 && catalog[i].iItem==(unsigned)i && catalog[i].cItem==1023);
    for(i=16;i<25;i++)CHECK(catalog[i].grobj==2 && catalog[i].iItem==(unsigned)(i+1) && catalog[i].cItem==1);
    CHECK(catalog[25].iItem==13 && catalog[26].iItem==14 && catalog[29].iItem==17);
    CHECK(catalog[30].iItem==7 && catalog[30].cItem==1020 && catalog[31].iItem==8 && catalog[31].cItem==1020);
    CHECK(catalog[32].iItem==9 && catalog[32].cItem==0 && catalog[35].iItem==12 && catalog[35].cItem==0);
    for(i=43;i<64;i++)CHECK(catalog[i].cItem==0 && catalog[i].grobj==0 && catalog[i].pct==0 && catalog[i].unused==0);
    reset(0,0);sel.pl.fStarbase=1;hullDefs[0].hul.wtCargoMax=100;
    for(i=0;i<4;i++){rgshdef[i].fFree=0;rgshdef[i].hul.wtEmpty=100;}
    rgshdef[0].fGift=1;rgshdef[1].hul.wtEmpty=101;rgshdef[2].fFree=1;
    InitProduction(catalog);CHECK(catalog[0].grobj==2 && catalog[0].iItem==3 && catalog[1].grobj==1);
    reset(0,0);primary=8;InitProduction(catalog);CHECK(cProdGlob==5 && catalog[0].iItem==11 && catalog[1].iItem==3 && catalog[4].iItem==6);
    reset(0,0);primary=3;InitProduction(catalog);CHECK(cProdGlob==7 && catalog[6].iItem==6);
    reset(0,0);maxFactories=-32768;sel.pl.cFactories=1;InitProduction(catalog);CHECK(catalog[0].iItem==7 && catalog[0].cItem==1020);
    // One following row is a defined non-overlapping copy, but pointer and index diverge.
    reset(2,1);maxFactories=1;sel.pl.lpplprod->rgprod()[0]=bad;sel.pl.lpplprod->rgprod()[1]=a;
    sel.pl.lpplprod->rgprod()[1].pct=66;sel.pl.lpplprod->rgprod()[1].unused=17;
    InitProduction(catalog);CHECK(lpplProdGlob->iprodMac==1 && lpplProdGlob->rgprod()[0].cItem==10 && lpplProdGlob->rgprod()[1].cItem==1);
    CHECK(lpplProdGlob->rgprod()[0].pct==66 && lpplProdGlob->rgprod()[0].unused==17 && catalog[0].cItem==0 && sel.pl.lpplprod->rgprod()[1].cItem==10);
    reset(3,1);maxFactories=2;sel.pl.lpplprod->rgprod()[0]=bad;sel.pl.lpplprod->rgprod()[1]=a;sel.pl.lpplprod->rgprod()[2]=a;
    if(setjmp(env)==0){InitProduction(catalog);CHECK(0);}
    CHECK(overlapBytes==8 && overlapOffset==0 && lpplProdGlob->iprodMac==3 && catalog[0].cItem==2);
    reset(4,1);maxFactories=2;sel.pl.lpplprod->rgprod()[0]=row(1,7,1);sel.pl.lpplprod->rgprod()[1]=bad;sel.pl.lpplprod->rgprod()[2]=a;sel.pl.lpplprod->rgprod()[3]=a;
    if(setjmp(env)==0){InitProduction(catalog);CHECK(0);}
    CHECK(overlapBytes==8 && overlapOffset==1 && catalog[0].cItem==1);
    reset(2,1);maxFactories=5;sel.pl.lpplprod->rgprod()[0]=row(3,7,1);sel.pl.lpplprod->rgprod()[1]=row(4,7,1);
    InitProduction(catalog);CHECK(lpplProdGlob->rgprod()[0].cItem==3 && lpplProdGlob->rgprod()[1].cItem==2 && catalog[0].cItem==0);
    // All finite/unlimited source amounts and modifier combinations for new rows.
    for(mode=0;mode<4;mode++)for(available=1;available<=1023;available++) {
        working(0);ctrl=(mode&2)!=0;shift=(mode&1)!=0;catalog[0]=row(available,7,1);cProdGlob=1;
        request=ctrl ? (shift ? 1020:100) : shift ? 10:1;if(request>available)request=available;
        command(IDC_PRODUCTION_ADD);
        CHECK(lpplProdGlob->iprodMac==1 && lpplProdGlob->rgprod()[0].cItem==(unsigned)request && lpplProdGlob->iprodMax==4);
        CHECK(catalog[0].cItem==(unsigned)(available==1023 ? 1023:available-request));
        CHECK(queueFill==1 && sourceFill==(catalog[0].cItem==0) && redraw==1 && setSelection==1 && reallocations==1);
    }
    working(2);lpplProdGlob->rgprod()[0]=a;lpplProdGlob->rgprod()[1]=b;catalog[0]=c;cProdGlob=1;queueSelection=1;command(IDC_PRODUCTION_ADD);
    CHECK(lpplProdGlob->iprodMac==3 && lpplProdGlob->rgprod()[0].iItem==7 && lpplProdGlob->rgprod()[1].iItem==9 && lpplProdGlob->rgprod()[2].iItem==8 && setSelection==2);
    working(2);lpplProdGlob->rgprod()[0]=a;lpplProdGlob->rgprod()[1]=b;catalog[0]=b;cProdGlob=1;queueSelection=1;command(100);
    CHECK(lpplProdGlob->iprodMac==2 && lpplProdGlob->rgprod()[1].cItem==2 && reallocations==0);
    working(2);lpplProdGlob->rgprod()[0]=a;lpplProdGlob->rgprod()[1]=b;catalog[0]=c;cProdGlob=1;queueSelection=0;command(IDC_PRODUCTION_ADD);
    CHECK(lpplProdGlob->rgprod()[0].iItem==9 && setSelection==1);
    working(2);lpplProdGlob->rgprod()[0]=a;lpplProdGlob->rgprod()[1]=b;catalog[0]=c;cProdGlob=1;command(IDC_PRODUCTION_ADD);
    CHECK(lpplProdGlob->rgprod()[2].iItem==9 && setSelection==3);
    working(40);for(i=0;i<40;i++)lpplProdGlob->rgprod()[i]=a;catalog[0]=c;cProdGlob=1;queueSelection=0;command(IDC_PRODUCTION_ADD);
    CHECK(lpplProdGlob->iprodMac==40 && catalog[0].cItem==0 && beeps==1 && queueFill==0 && sourceFill==0 && redraw==1);
    catalog[0]=a;command(IDC_PRODUCTION_ADD);CHECK(lpplProdGlob->iprodMac==40 && lpplProdGlob->rgprod()[0].cItem==11);
    working(0);catalog[0]=row(0,7,1);catalog[1]=b;cProdGlob=2;command(IDC_PRODUCTION_ADD);
    CHECK(lpplProdGlob->rgprod()[0].iItem==8 && catalog[0].cItem==0 && catalog[1].cItem==0 && sourceFill==1);
    working(0);sourceSelection=-1;command(IDC_PRODUCTION_ADD);CHECK(lpplProdGlob->iprodMac==0 && redraw==0 && queueFill==0);
    working(0);cProdGlob=0;command(IDC_PRODUCTION_ADD);CHECK(lpplProdGlob->rgprod()[0].cItem==1 && catalog[0].cItem==1023); // allocated inactive source slot
    working(0);catalog[0]=row(1023,3,1);cProdGlob=1;shift=ctrl=1;command(IDC_PRODUCTION_ADD);
    CHECK(lpplProdGlob->rgprod()[0].cItem==1020);command(IDC_PRODUCTION_ADD);CHECK(lpplProdGlob->rgprod()[0].cItem==1);
    working(0);catalog[0]=a;cProdGlob=1;failReallocation=1;
    if(setjmp(env)==0){command(IDC_PRODUCTION_ADD);CHECK(0);}
    CHECK(catalog[0].cItem==9 && lpplProdGlob->iprodMac==0 && queueFill==0 && redraw==0);
    working(1);lpplProdGlob->rgprod()[0]=row(1023,7,1);lpplProdGlob->rgprod()[0].pct=75;catalog[0]=a;cProdGlob=1;queueSelection=1;command(IDC_PRODUCTION_ADD);
    CHECK(lpplProdGlob->rgprod()[0].cItem==1020 && lpplProdGlob->rgprod()[0].pct==75 && catalog[0].cItem==9);
    // Remove uses native ten-bit return and preserves progress/unused on survivors.
    for(mode=0;mode<4;mode++)for(available=0;available<=1023;available++) {
        working(1);ctrl=(mode&2)!=0;shift=(mode&1)!=0;lpplProdGlob->rgprod()[0]=row(available,7,1);
        catalog[0]=row(1020,7,1);cProdGlob=1;queueSelection=1;
        request=ctrl ? (shift ? 1020:100) : shift ? 10:1;if(request>available)request=available;
        command(IDC_PRODUCTION_REMOVE);
        CHECK(lpplProdGlob->iprodMac==(available>request) && catalog[0].cItem==(unsigned)((1020+request)%1024));
        if(available>request)CHECK(lpplProdGlob->rgprod()[0].cItem==(unsigned)(available-request));
    }
    working(1);lpplProdGlob->rgprod()[0]=row(4,7,1);catalog[0]=row(1020,7,1);cProdGlob=1;ctrl=1;queueSelection=1;command(IDC_PRODUCTION_REMOVE);
    CHECK(lpplProdGlob->iprodMac==0 && catalog[0].cItem==0 && setSelection==-99 && queueFill==1 && sourceFill==0);
    working(1);lpplProdGlob->rgprod()[0]=row(1023,3,1);catalog[0]=row(1023,3,1);cProdGlob=1;queueSelection=1;command(101);
    CHECK(lpplProdGlob->iprodMac==1 && lpplProdGlob->rgprod()[0].cItem==3 && catalog[0].cItem==1023);
    working(2);lpplProdGlob->rgprod()[0]=b;lpplProdGlob->rgprod()[1]=c;catalog[0]=row(0,8,1);cProdGlob=1;queueSelection=1;command(IDC_PRODUCTION_REMOVE);
    CHECK(lpplProdGlob->iprodMac==1 && lpplProdGlob->rgprod()[0].iItem==9 && setSelection==1 && sourceFill==1);
    working(1);lpplProdGlob->rgprod()[0]=a;lpplProdGlob->rgprod()[0].pct=66;lpplProdGlob->rgprod()[0].unused=17;catalog[0]=a;cProdGlob=1;queueSelection=1;command(IDC_PRODUCTION_REMOVE);
    CHECK(lpplProdGlob->rgprod()[0].cItem==9 && lpplProdGlob->rgprod()[0].pct==66 && lpplProdGlob->rgprod()[0].unused==17);
    working(2);lpplProdGlob->rgprod()[0]=a;lpplProdGlob->rgprod()[1]=b;queueSelection=1;command(IDC_PRODUCTION_ITEM_DOWN);
    CHECK(lpplProdGlob->rgprod()[0].iItem==8 && lpplProdGlob->rgprod()[1].iItem==7 && setSelection==2);
    command(IDC_PRODUCTION_ITEM_UP);CHECK(lpplProdGlob->rgprod()[0].iItem==7 && setSelection==1);
    reset(0,0);printf("M09 source-extracted editor witnesses: %d assertions passed\n",checks);return 0;
}
'''
    with tempfile.TemporaryDirectory(prefix='m09-editor-') as work:
        source=Path(work)/'witness.cpp';binary=Path(work)/'witness'
        source.write_text(declarations + storage_declarations(allocate=True, resize=True, free=True) + bodies + storage_definitions('(void)lp;return lastBytes;', allocate=True, resize=True, free=True) + witnesses)
        subprocess.run(shlex.split(os.environ.get('CXX','c++'))+
                       ['-std=gnu++20','-DSTARS_TEST_STATE_DOUBLE','-include',str(ROOT/'src/platform/value.h'),'-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-fsanitize=address,undefined','-fno-sanitize-recover=all',
                        str(source),'-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True)


if __name__ == '__main__':
    main()
