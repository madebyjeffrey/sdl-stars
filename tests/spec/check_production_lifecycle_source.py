#!/usr/bin/env python3
"""Source-extracted M09 lifecycle / M07 starbase / initial-selection witnesses.

Actual FinishProduction, ChangeProduction, close/navigation command blocks,
InitializeProductionDlg and IdFindAdjStarbase run with native queue layouts.
Allocator, lookup, initializer, selection, modal and GUI callbacks are explicit
bounded doubles. This checks call ordering and partial state, not their full
implementation, persistent publication or rendering.
"""
from cpp_production_storage import declarations as storage_declarations, definitions as storage_definitions
from pathlib import Path
from check_s02_source import struct_start
import os
import shlex
import subprocess
import tempfile
from check_s02_source import ROOT, function
from check_s02_state_source import between


def main():
    types = (ROOT / 'src/core/structs.h').read_text()
    command = function('src/ui/produceui.c', 'void ProdCommandHandler(')
    declarations = r'''
#include <assert.h>
#include <stdint.h>
typedef uint16_t HeapType;
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "errors.h"
#define TRUE 1
#define FALSE 0
#define LOWORD(x) ((uint16_t)(x))
#define grobjPlanet 1
#define idWriteBack -1
#define idplNone -1
#define tileProductionOrOrbit 1
#define tilePlanetStats 2
#define IDOK 1
#define IDCANCEL 2
#define IDC_BACK 3
#define IDC_NEXT 4
#define VK_SHIFT 5
#define IDC_PRODUCTION_AVAILABLE_ITEMS 6
#define IDC_PRODUCTION_QUEUE 7
#define IDC_PRODUCTION_RESEARCH_LEFTOVERS_ONLY 8
#define LB_SETCURSEL 9
#define BM_SETCHECK 10
#define mdIdleFactory 7
#define idsProductionQueueS 1
#define idsThereIsntEnoughFreeMemoryModifyProduction 2
#define MB_ICONHAND 3
#define IDD_PRODUCTION 4
#define MAKEINTRESOURCE(x) (x)
#define wsprintf(dst,fmt,name) snprintf(dst,sizeof(dst),fmt,name)
typedef int HWND;
typedef int FARPROC;
typedef struct { int32_t left,top,right,bottom; } RECT;
'''
    for tag, alias in [('_prod','PROD'),('PLPROD','PLPROD'),('_pl','PL')]:
        start = struct_start(types, tag)
        declarations += types[start:types.index('\n};',start)+3]+f'\ntypedef struct {tag} {alias};\n'
    declarations += r'''
typedef struct { int16_t id,iPlayer;PLPROD *lpplprod;uint32_t fNoResearch:1,fStarbase:1,isb:4; } PLANET;
typedef struct { struct {uint16_t wtCargoMax;} hul; } HULDEF;
typedef struct { struct {uint16_t ihuldef;} hul; } SHDEF;
static struct { PLANET pl;int grobj; } sel;
static struct { int fNoResearchSav,fTutorial; } gd;
static struct { int fProgress; } tutor;
static PLANET planets[8],*lpPlanets=planets;
static SHDEF designs[8],*rglpshdefSB[1]={designs};
static HULDEF hulls[8];
static int16_t cPlanet,idPlayer,fAi;
static PLPROD *lpplProdGlob;
static HWND hwndProdDlg,hwndFrame=99;
static int fDlgUp,hInst,ProductionDlg,ptStickyProduceDlg,yTopFutureTech=20,dyArial8=10;

static char trace[8192];
static int checks,allocations,alive[128],frees,failAt,mutateWrite,mutateReload,mutateTutorial;
static int modalNext,shiftKey,queueSelection,sourceSelection,researchCheck,initCalls,writeCount,initialSelections;
static intptr_t modalResult;
static int writeResult,readResult;
static unsigned storedCounts[8],storedResearch[8];
static void *allocs[128];
static void event(const char *s) { assert(strlen(trace)+strlen(s)<sizeof(trace));strcat(trace,s); }
static PLPROD *queue(unsigned count) {
    assert(allocations<128 && count<=255);PLPROD *p=(PLPROD *)calloc(1,4+count*4);assert(p);p=LpStartPl<PLPROD,PROD>(p,4+count*4);
    p->cbItem=4;p->iprodMac=count;p->iprodMax=count;
    allocs[allocations]=p;alive[allocations++]=1;return p;
}
static int allocation(void *p) { for(int i=0;i<allocations;i++)if(allocs[i]==p)return i;abort(); }
static void FreeLp(void *p,int ht) {(void)ht;int i=allocation(p);assert(alive[i]);alive[i]=0;frees++;event("F;");}
static HULDEF *LphuldefFromId(int i) { assert(i>=0 && i<8);return &hulls[i]; }
static void fail(int stage) { if(failAt==stage)StarsThrowError(starsErrorMemory,0); }
static void InitProduction(PROD *rows) {
    event(rows ? "I;":"i;");initCalls++;gd.fNoResearchSav=sel.pl.fNoResearch;
    fail(rows ? 1:5);
    unsigned n=sel.pl.lpplprod ? sel.pl.lpplprod->iprodMac:0;
    lpplProdGlob=queue(n);
    if(n)memcpy(lpplProdGlob->rgprod(),sel.pl.lpplprod->rgprod(),n*4);
}
static int FLookupPlanet(int id,PLANET *p) {
    if(id==idWriteBack) {
        event("W;");fail(3);if(!writeResult)return 0;writeCount++;
        assert(p->id>=0 && p->id<8);storedCounts[p->id]=p->lpplprod ? p->lpplprod->iprodMac:0;
        storedResearch[p->id]=p->fNoResearch;
        if(mutateWrite)p->id=6;
    } else { char text[20];snprintf(text,sizeof(text),"L%d;",id);event(text);fail(4);if(mutateReload)fAi=1; }
    return id==idWriteBack ? writeResult:readResult;
}
static void FillSelProdLB(void) {event("Fill;");}
static void RedrawPlanShip(int tile) {assert(tile==tileProductionOrOrbit);event("Redraw;");}
static void AdvanceTutor(void) {assert(tutor.fProgress);event("Tutor;");if(mutateTutorial){sel.grobj=2;lpplProdGlob=queue(1);}}
static void DrawPlanShip(void *hdc,int tile) {assert(!hdc && tile==tilePlanetStats);event("Stats;");}
static FARPROC MakeProcInstance(int proc,int instance) {(void)proc;(void)instance;event("Make;");return 1;}
static void FreeProcInstance(FARPROC proc) {assert(proc==1);event("Unmake;");}
static void EndDialog(HWND hwnd,int result) {assert(hwnd==7);event(result ? "End1;":"End0;");}
static void StickyDlgPos(HWND hwnd,int *point,int restore) {assert(hwnd==7 && point==&ptStickyProduceDlg && !restore);event("Sticky;");}
static const char *PszFormatIds(int id,void *p) {(void)p;assert(id==idsThereIsntEnoughFreeMemoryModifyProduction);return "memory";}
static void AlertSz(const char *s,int flags) {assert(strcmp(s,"memory")==0 && flags==MB_ICONHAND);event("Alert;");}
static HWND GetDlgItem(HWND hwnd,int id) {assert(hwnd==7);return id;}
static const char *PszGetCompressedString(int id) {assert(id==idsProductionQueueS);return "Production %s";}
static const char *PszGetPlanetName(int id) {(void)id;return "Test";}
static void SetWindowText(HWND hwnd,const char *s) {assert(hwnd==7 && strcmp(s,"Production Test")==0);event("Title;");}
static void FillProdSrcLB(HWND hwnd,int row) {assert(hwnd==IDC_PRODUCTION_AVAILABLE_ITEMS && row==-1);event("Source;");}
static void FillPlanetProdLB(HWND hwnd,PLPROD *p,void *unused) {assert(hwnd==IDC_PRODUCTION_QUEUE && p==lpplProdGlob && !unused);event("Queue;");}
static intptr_t SendMessage(HWND hwnd,int msg,intptr_t value,intptr_t param) {
    assert(!param);
    if(msg==LB_SETCURSEL && hwnd==IDC_PRODUCTION_AVAILABLE_ITEMS){sourceSelection=value;event("Src0;");}
    else if(msg==LB_SETCURSEL && hwnd==IDC_PRODUCTION_QUEUE){queueSelection=value;initialSelections++;event("QSel;");}
    else {assert(msg==BM_SETCHECK && hwnd==IDC_PRODUCTION_RESEARCH_LEFTOVERS_ONLY);researchCheck=value;event("Check;");}
    return 0;
}
static int GetKeyState(int key) {assert(key==VK_SHIFT);return shiftKey ? -1:0;}
static void SelectAdjPlanet(int16_t delta,int16_t id) {
    char text[30];snprintf(text,sizeof(text),"Select%d:%d;",delta,id);event(text);
    if(id==idplNone)return;
    sel.pl.id=delta ? (sel.pl.id+delta+8)%8:id;
    sel.pl.fNoResearch=storedResearch[sel.pl.id];
    sel.pl.lpplprod=queue(storedCounts[sel.pl.id]);
}
static void GetClientRect(HWND hwnd,RECT *r) {assert(hwnd==7);event("Rect;");*r=(RECT){0,0,800,600};}
static void InvalidateRect(HWND hwnd,RECT *r,int erase) {assert(hwnd==7 && erase && r->top==20 && r->bottom==110);event("Paint;");}
static intptr_t DialogBox(int instance,int resource,HWND parent,FARPROC proc);
#define CHECK(x) do {checks++;if(!(x)){fprintf(stderr,"witness %d failed: %s; trace=%s\n",checks,#x,trace);return 1;}}while(0)
'''
    bodies = '\n'.join([
        function('src/core/memory.c','void FreePl('),
        function('src/core/produce.c','void FinishProduction('),
        function('src/core/planet.c','int16_t IdFindAdjStarbase('),
        function('src/ui/produceui.c','void InitializeProductionDlg('),
    ])
    cases = between(command, '    case IDOK:', '    case IDC_PRODUCTION_ITEM_DOWN:')
    bodies += '\nstatic void Command(HWND hwnd,int wParam) { int16_t c;RECT rc;switch(wParam) {\n'+cases+'\n}}\n'
    bodies += function('src/ui/produceui.c','int16_t ChangeProduction(')
    witnesses = r'''
static intptr_t DialogBox(int instance,int resource,HWND parent,FARPROC proc) {
    assert(instance==hInst && resource==IDD_PRODUCTION && parent==hwndFrame && proc==1 && fDlgUp);
    event("Modal;");hwndProdDlg=7;sel.pl.fNoResearch=1;fail(2);
    if(modalNext){Command(7,modalNext>0 ? IDC_NEXT:IDC_BACK);sel.pl.fNoResearch=!gd.fNoResearchSav;}
    Command(7,modalResult==0 ? IDCANCEL:IDOK);
    return modalResult;
}
static void reset(void) {
    for(int i=0;i<allocations;i++)free(allocs[i]);
    allocations=frees=failAt=mutateWrite=mutateReload=mutateTutorial=modalNext=shiftKey=0;
    writeCount=initCalls=initialSelections=0;writeResult=readResult=1;modalResult=0;trace[0]=0;
    memset(&sel,0,sizeof(sel));memset(&gd,0,sizeof(gd));memset(&tutor,0,sizeof(tutor));
    memset(storedCounts,0,sizeof(storedCounts));memset(storedResearch,0,sizeof(storedResearch));
    memset(planets,0,sizeof(planets));memset(hulls,0,sizeof(hulls));
    lpplProdGlob=NULL;sel.pl.id=2;sel.grobj=1;sel.pl.lpplprod=queue(2);
    hwndProdDlg=0;fDlgUp=0;idPlayer=fAi=cPlanet=0;
    queueSelection=sourceSelection=researchCheck=-99;
}
static int referenceTarget(int current,int next) {
    int eligible[8],n=0;
    for(int i=0;i<cPlanet;i++)if(planets[i].iPlayer==idPlayer && planets[i].fStarbase && hulls[designs[planets[i].isb].hul.ihuldef].hul.wtCargoMax)eligible[n++]=planets[i].id;
    if(!n)return -1;
    if(next){for(int i=0;i<n;i++)if(eligible[i]>current)return eligible[i];return eligible[0];}
    for(int i=n-1;i>=0;i--)if(eligible[i]<current)return eligible[i];return eligible[n-1];
}
int main(void) {
    static_assert(sizeof(PROD)==4,"PROD width");static_assert(sizeof(PLPROD)==4,"PLPROD header");
    for(int accept=0;accept<2;accept++)for(int empty=0;empty<3;empty++)for(int ai=0;ai<2;ai++)for(int tutorial=0;tutorial<2;tutorial++)for(int player=0;player<2;player++) {
        reset();PLPROD *old=sel.pl.lpplprod,*work=empty==2 ? NULL:queue(empty==0 ? 2:0);
        lpplProdGlob=work;sel.pl.fNoResearch=1;gd.fNoResearchSav=0;gd.fTutorial=tutorial;idPlayer=player;fAi=ai;
        FinishProduction(accept);
        CHECK(!lpplProdGlob && sel.pl.fNoResearch==(unsigned)accept);
        CHECK(alive[allocation(old)]==!accept);
        if(accept) {
            CHECK(sel.pl.lpplprod==(empty==0 ? work:NULL));CHECK(writeCount==1);
            CHECK(strstr(trace,"W;L2;")!=NULL);CHECK((strstr(trace,"Fill;Redraw;")!=NULL)==!ai);
        } else {CHECK(sel.pl.lpplprod==old && !writeCount);}
        if(work)CHECK(alive[allocation(work)]==(accept && empty==0));
        CHECK((strstr(trace,"Tutor;")!=NULL)==(tutorial && player==0));
    }
    for(int writeOk=0;writeOk<2;writeOk++)for(int readOk=0;readOk<2;readOk++) {
        reset();lpplProdGlob=queue(1);writeResult=writeOk;readResult=readOk;FinishProduction(1);
        CHECK(strstr(trace,"W;L2;Fill;Redraw;")!=NULL);CHECK(writeCount==writeOk && !lpplProdGlob);
    }
    reset();lpplProdGlob=queue(1);mutateWrite=mutateReload=1;FinishProduction(1);
    CHECK(strstr(trace,"W;L6;") && !strstr(trace,"Fill;"));
    reset();gd.fTutorial=mutateTutorial=1;lpplProdGlob=queue(1);FinishProduction(0);
    CHECK(lpplProdGlob && sel.grobj==2); // tutorial can repopulate after final null
    const intptr_t results[]={0,1,-1,65536,65537,-65536};
    for(unsigned i=0;i<sizeof(results)/sizeof(results[0]);i++) {
        reset();modalResult=results[i];PLPROD *old=sel.pl.lpplprod;
        CHECK(ChangeProduction(0)==1 && !FStarsRecoveryActive() && !hwndProdDlg && !fDlgUp && !lpplProdGlob);
        int accept=(int16_t)results[i]!=0;
        CHECK(writeCount==accept && sel.pl.fNoResearch==(unsigned)accept);
        CHECK(alive[allocation(old)]==!accept);CHECK((strstr(trace,"Stats;")!=NULL)==accept);
        CHECK(strstr(trace,"Unmake;")!=NULL && initCalls==1);
    }
    reset();sel.pl.fNoResearch=1;CHECK(ChangeProduction(1)==1);
    CHECK(!sel.pl.lpplprod && sel.pl.fNoResearch && !initCalls && !strstr(trace,"Modal;") && writeCount==1);
    reset();lpplProdGlob=queue(1);PLPROD *preexisting=lpplProdGlob;CHECK(ChangeProduction(1)==1);
    CHECK(sel.pl.lpplprod==preexisting); // direct Clear assumes global null; does not enforce it
    for(int stage=1;stage<=4;stage++) {
        reset();failAt=stage;modalResult=1;PLPROD *old=sel.pl.lpplprod;
        CHECK(ChangeProduction(0)==0 && !FStarsRecoveryActive() && !lpplProdGlob && !hwndProdDlg && !fDlgUp);
        CHECK(sel.pl.fNoResearch==(unsigned)(stage!=1));CHECK(strstr(trace,"Alert;")!=NULL);
        CHECK((strstr(trace,"End0;")!=NULL)==(stage==2));
        CHECK(alive[allocation(old)]==(stage<=2));
        if(stage>=3)CHECK(sel.pl.lpplprod && alive[allocation(sel.pl.lpplprod)]);
    }
    reset();modalNext=1;modalResult=0;storedCounts[3]=1;
    CHECK(ChangeProduction(0)==1);CHECK(storedCounts[2]==2 && storedResearch[2]==1);
    CHECK(sel.pl.id==3 && sel.pl.fNoResearch==0 && writeCount==1 && initCalls==2);
    CHECK(strstr(trace,"W;L2;Fill;Redraw;Select1:0;i;Title;Source;Src0;Queue;QSel;Check;Rect;Paint;")!=NULL);
    reset();modalNext=-1;failAt=5;modalResult=0;
    CHECK(ChangeProduction(0)==0 && writeCount==1 && storedResearch[2]==1 && sel.pl.id==1 && !lpplProdGlob);
    CHECK(strstr(trace,"Select-1:0;i;End0;Alert;")!=NULL);
    reset();shiftKey=1;modalNext=1;modalResult=0;
    CHECK(ChangeProduction(0)==1 && sel.pl.id==2 && initCalls==2 && writeCount==1);
    CHECK(strstr(trace,"Select0:-1;i;")!=NULL); // no base still commits and reinitializes
    reset();gd.fTutorial=mutateTutorial=1;modalResult=1;
    CHECK(ChangeProduction(0)==1 && sel.grobj==2 && !strstr(trace,"Stats;") && lpplProdGlob);
    for(int count=0;count<=8;count++)for(unsigned mask=0;mask<(1u<<count);mask++) {
        reset();lpplProdGlob=queue(count);int expected=0;
        for(int i=0;i<count;i++){lpplProdGlob->rgprod()[i].grobj=1;lpplProdGlob->rgprod()[i].iItem=(mask&(1u<<i)) ? 7:0;if(mask&(1u<<i))expected=i+1;}
        sel.pl.fNoResearch=1;InitializeProductionDlg(7);
        CHECK(queueSelection==expected && sourceSelection==0 && researchCheck==1 && initialSelections==1);
    }
    reset();lpplProdGlob=queue(2);lpplProdGlob->rgprod()[0].grobj=2;lpplProdGlob->rgprod()[1].grobj=1;
    InitializeProductionDlg(7);CHECK(queueSelection==1);
    const int orders[3][4]={{1,3,5,7},{7,1,5,3},{5,3,7,1}};
    for(int order=0;order<3;order++)for(unsigned mask=0;mask<256;mask++)for(int current=-1;current<=8;current++)for(int next=0;next<2;next++) {
        reset();cPlanet=4;
        for(int i=0;i<4;i++){
            unsigned mode=(mask>>(2*i))&3;planets[i].id=orders[order][i];planets[i].iPlayer=mode==1 ? 1:0;
            planets[i].fStarbase=mode!=2;planets[i].isb=i;designs[i].hul.ihuldef=i;hulls[i].hul.wtCargoMax=mode==3 ? 0:100;
        }
        CHECK(IdFindAdjStarbase(current,next)==referenceTarget(current,next));
    }
    reset();for(int i=0;i<allocations;i++)free(allocs[i]);
    printf("M09 lifecycle/M07 starbase source-extracted witnesses: %d passed\n",checks);
    return 0;
}
'''
    with tempfile.TemporaryDirectory(prefix='m09-lifecycle-') as work:
        path=Path(work);source=path/'witness.cpp';binary=path/'witness'
        source.write_text('#include "' + str(ROOT/'src/core/liststorage.h') + '"\n' + declarations + storage_declarations(allocate=False, resize=False, free=True) + bodies + storage_definitions('', allocate=False, resize=False, free=True) + witnesses)
        subprocess.run(shlex.split(os.environ.get('CXX','c++'))+['-std=gnu++20','-DSTARS_TEST_STATE_DOUBLE','-include',str(ROOT/'src/platform/value.h'),'-iquote',str(ROOT/'src/platform'),str(ROOT/'src/platform/errors.cpp'),'-Wall','-Wextra','-Werror','-Wno-misleading-indentation',
            '-fsanitize=address,undefined','-fno-sanitize-recover=all',str(source),'-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True)


if __name__=='__main__':
    main()
