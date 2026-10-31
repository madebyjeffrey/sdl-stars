#!/usr/bin/env python3
"""M09 preset witnesses using current source blocks and native record layouts.

GUI, localization and allocation outcomes are controlled doubles. This checks
queue/customization effects, not actual rendering, heap exhaustion or whole-GUI
refinement. No out-of-allocation catalog access is executed.
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
    ui = (ROOT / 'src/ui/produceui.cpp').read_text()
    types = (ROOT / 'src/core/structs.h').read_text()
    declarations = r'''
#include <stdint.h>
typedef uint16_t StringId;
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#define TRUE 1
#define FALSE 0
#define LOWORD(x) ((uint16_t)(x))
#define MAKEINTRESOURCE(x) (x)
#define MakeProcInstance(x,y) (x)
#define FreeProcInstance(x) ((void)(x))
#define wsprintf sprintf
#define grobjPlanet 1
#define mdIdleFactory 7
#define iobjDefense 2
#define iobjAlchemy 3
#define iobjMinTerraform 4
#define iobjMaxTerraform 5
#define raMacintosh 8
#define raTerra 3
#define rsMajorAdv 0
#define Energy 0
#define htOrd 1
#define IDC_IMPORT 0x816
#define IDC_PRODUCTION_CLEAR 2
#define IDC_PRODUCTION_QUEUE 3
#define IDC_PRODUCTION_AVAILABLE_ITEMS 4
#define IDC_PRODUCTION_RESEARCH_LEFTOVERS_ONLY 5
#define IDC_ZIP_PROD_QUEUE 6
#define IDC_RENAME 7
#define IDC_DELETE 8
#define IDD_RENAME 9
#define IDD_ZIP_PROD 10
#define BM_SETCHECK 11
#define LB_SETCURSEL 12
#define LB_RESETCONTENT 13
#define LB_ADDSTRING 14
#define idsCustomD 15
#define idsUnusedD 16
#define idsAutoBuildOrders 17
#define idsSD2 18
#define idsMines 100
#define idsThereIsntEnoughFreeMemoryModifyProduction 200
#define MB_ICONHAND 0
#define RenameZipDlg 1
#define ZipProdDlg 2
typedef int HeapType;
typedef int HWND;
typedef intptr_t LPARAM;
typedef struct { int left,top,right,bottom; } RECT;
'''
    for tag, alias in [('_prod', 'PROD'), ('PLPROD', 'PLPROD'), ('_pl', 'PL'),
                       ('_prodq1', 'PRODQ1'), ('_zipprodq1', 'ZIPPRODQ1'), ('_zipprodq', 'ZIPPRODQ')]:
        start = struct_start(types, tag)
        end = types.index('\n};', start) + 3
        declarations += types[start:end] + f'\ntypedef struct {tag} {alias};\n'
    declarations += r'''
static PLPROD *lpplProdGlob;
static PROD pProdGlob[64];
static int cProdGlob,primary,rgplr[1],idPlayer,hInst,hwndFrame,iResTechNow;
static int hwndProdDlg,fDlgUp,freed,ended,alerts,allocations,failAllocation;
static int renameCalls,renameResult,outerResult,checks,enabledDelete,enabledRename;
static int vyZPDStatic,dyArial8=8,labelCount;
static size_t lastBytes;
static ZIPPRODQ vrgZipProd[5];
static struct { struct { int fNoResearch; } pl; } sel;
static struct { int fChgZipProd; } gd;
static char szWork[256],renameText[13],label[80],labels[12][80];
static jmp_buf env;
static jmp_buf *penvMem;
static void *LpAlloc(uint16_t cb,HeapType ht) {
    (void)ht;allocations++;lastBytes=cb;
    if(failAllocation)longjmp(env,1);
    void *p=calloc(1,cb);if(!p)abort();return p;
}
static void FreeLp(void *p,HeapType ht) { (void)ht;freed++;free(p); }
static int GetRaceStat(void *p,int stat) { (void)p;(void)stat;return primary; }
static HWND GetDlgItem(HWND hwnd,int id) { (void)hwnd;return id; }
static void SendMessage(HWND hwnd,int message,int value,LPARAM param) {
    (void)hwnd;(void)value;
    if(message==LB_RESETCONTENT)labelCount=0;
    if(message==LB_ADDSTRING){if(labelCount>=12)abort();strcpy(labels[labelCount++],(const char *)param);}
}
static void FillPlanetProdLB(HWND hwnd,PLPROD *q,void *p) { (void)hwnd;(void)q;(void)p; }
static void FillProdSrcLB(HWND hwnd,int n) { (void)hwnd;(void)n; }
static void SetWindowText(HWND hwnd,const char *s) { (void)hwnd;strcpy(label,s); }
static void SetFocus(HWND hwnd) { (void)hwnd; }
static void EnableWindow(HWND hwnd,int enabled) {
    if(hwnd==IDC_DELETE)enabledDelete=enabled;
    if(hwnd==IDC_RENAME)enabledRename=enabled;
}
static void GetClientRect(HWND hwnd,RECT *rc) { (void)hwnd;memset(rc,0,sizeof(*rc)); }
static void InvalidateRect(HWND hwnd,RECT *rc,int erase) { (void)hwnd;(void)rc;(void)erase; }
static const char *PszGetCompressedString(int id) {
    if(id==idsCustomD)return "Custom%d";
    if(id==idsUnusedD)return "Unused%d";
    return "Automatic orders";
}
static void CchGetString(int id,char *out) {
    if(id==idsSD2)strcpy(out,"%s %d");else sprintf(out,"Item%d",id-idsMines);
}
static int DialogBox(int instance,int resource,HWND hwnd,int proc) {
    (void)instance;(void)hwnd;(void)proc;
    if(resource==IDD_RENAME){renameCalls++;if(renameResult)strcpy(szWork,renameText);return renameResult;}
    memset(vrgZipProd,0x5a,4*sizeof(ZIPPRODQ));vrgZipProd[4].cpq=9;gd.fChgZipProd=1;
    return outerResult;
}
static void EndDialog(HWND hwnd,int result) { (void)hwnd;(void)result;ended++; }
static const char *PszFormatIds(int id,void *p) { (void)id;(void)p;return "memory"; }
static void AlertSz(const char *s,int flags) { (void)s;(void)flags;alerts++; }
#define CHECK(x) do { checks++;if(!(x)){fprintf(stderr,"witness %d failed: %s\n",checks,#x);return 1;} } while(0)
'''
    bodies = function('src/core/memory.cpp', 'PL *LpplAlloc(') + '\n'
    bodies += function('src/core/memory.cpp', 'void FreePl(') + '\n'
    bodies += function('src/ui/produceui.cpp', 'void EnableZipProdBtns(') + '\n'
    bodies += function('src/ui/produceui.cpp', 'void FillZipProdLB(') + '\n'
    block = between(ui, '    case IDC_PRODUCTION_CLEAR:\n    case IDC_IMPORT:', '    case IDC_PRODUCTION_AVAILABLE_ITEMS:')
    bodies += 'static void apply(int wParam,int lParam) {int hwnd=1,ipl,iSrc,cMax,iDst;PROD *lpprod;PLPROD *lpplprodT;switch(wParam){' + block + '}RedrawText:; }\n'
    block = between(ui, '        for (cMax = 0; cMax < 4 && (!vrgZipProd[cMax].fValid || i-- != 0); cMax++) {', '        ProdCommandHandler(hwnd, 0x816, cMax);')
    bodies += 'static int popupSlot(int i) {int cMax;' + block + 'return cMax;}\n'
    custom = function('src/ui/produceui.cpp', 'INT_PTR CALLBACK ZipProdDlg(')
    block = between(custom, '            case IDC_IMPORT:\n            case IDC_RENAME:', '            case IDC_HELP:')
    bodies += 'static void edit(int wParam) {int hwnd=1,i,cpq,lpProc;char *psz,*pszT;switch(wParam){' + block + '}}\n'
    block = between(ui, '            memcpy(rgzp, vrgZipProd, 160);', '        if (i < 0)')
    # Includes the closing brace of the caller's Customize branch.
    bodies += 'static void customize(void) {ZIPPRODQ rgzp[4];int lpProc,hwnd=1;int16_t fRet;do {if(1){' + block + '}while(0);}\n'
    recovery = function('src/ui/produceui.cpp', 'int16_t ChangeProduction(')
    block = recovery.split('} catch (const StarsError &) {', 1)[1].rsplit('}', 2)[0]
    bodies += 'static int recover(void) {' + block + '}\n'
    bodies += r'''
static void reset(unsigned n) {
    if(lpplProdGlob)FreePl(lpplProdGlob);
    failAllocation=0;lpplProdGlob=LpplAlloc<PLPROD>(4,n ? n:1,htOrd);lpplProdGlob->iprodMac=n;
    memset(pProdGlob,0,sizeof(pProdGlob));memset(vrgZipProd,0,sizeof(vrgZipProd));
    cProdGlob=0;primary=0;sel.pl.fNoResearch=0;gd.fChgZipProd=0;
    freed=ended=alerts=allocations=renameCalls=0;renameResult=1;renameText[0]=0;label[0]=0;
}
'''
    witnesses = r'''
int main(void) {
    int i,n,k;PROD manual={.cItem=1,.iItem=7,.grobj=1,.pct=35,.unused=17};
    PROD automatic={.cItem=10,.iItem=0,.grobj=1,.pct=50,.unused=3};ZIPPRODQ snapshot[5];
    CHECK(sizeof(PROD)==4 && sizeof(PLPROD)==4 && sizeof(ZIPPRODQ)==40);
    reset(2);lpplProdGlob->rgprod()[0]=automatic;lpplProdGlob->rgprod()[1]=manual;
    pProdGlob[0]=PROD{.cItem=20,.iItem=0,.grobj=1,.pct=0,.unused=0};pProdGlob[1]=manual;cProdGlob=2;
    vrgZipProd[0].cpq=1;vrgZipProd[0].rgpq[0].w=64;vrgZipProd[0].fNoResearch=1;
    apply(IDC_IMPORT,0);
    CHECK(lpplProdGlob->iprodMac==2 && lpplProdGlob->iprodMax==3 && lastBytes==16);
    CHECK(memcmp(&lpplProdGlob->rgprod()[0],&manual,4)==0 && lpplProdGlob->rgprod()[1].cItem==1);
    CHECK(lpplProdGlob->rgprod()[1].pct==0 && lpplProdGlob->rgprod()[1].unused==0 && lpplProdGlob->rgprod()[2].cItem==0);
    CHECK(pProdGlob[0].cItem==30 && pProdGlob[1].cItem==2 && sel.pl.fNoResearch==1 && freed==1);
    // Invalid flag is not a command guard; traits filter each packed row.
    for(primary=0;primary<9;primary++) {
        int selected=primary;reset(0);primary=selected;vrgZipProd[0].cpq=7;
        for(i=0;i<7;i++)vrgZipProd[0].rgpq[i].w=64+i;
        apply(IDC_IMPORT,0);CHECK(lpplProdGlob->iprodMac==(primary==8 ? 4 : primary==3 ? 5 : 7));
        for(i=0;i<lpplProdGlob->iprodMac;i++)CHECK((primary!=8 || lpplProdGlob->rgprod()[i].iItem>2) && (primary!=3 || (lpplProdGlob->rgprod()[i].iItem!=4 && lpplProdGlob->rgprod()[i].iItem!=5)));
    }
    reset(2);lpplProdGlob->rgprod()[0]=manual;lpplProdGlob->rgprod()[1]=manual;
    lpplProdGlob->rgprod()[0].cItem=3;pProdGlob[0]=manual;pProdGlob[0].cItem=1020;cProdGlob=1;
    sel.pl.fNoResearch=1;apply(IDC_PRODUCTION_CLEAR,0);
    CHECK(pProdGlob[0].cItem==1023 && lpplProdGlob->iprodMac==0 && sel.pl.fNoResearch && allocations==0 && freed==0);
    reset(1);lpplProdGlob->rgprod()[0]=manual;lpplProdGlob->rgprod()[0].cItem=4;
    pProdGlob[0]=manual;pProdGlob[0].cItem=1020;cProdGlob=1;apply(IDC_PRODUCTION_CLEAR,0);CHECK(pProdGlob[0].cItem==0);
    reset(1);lpplProdGlob->rgprod()[0]=manual;apply(IDC_PRODUCTION_CLEAR,0);CHECK(pProdGlob[0].cItem==1); // inactive, allocated slot
    reset(40);for(i=0;i<40;i++)lpplProdGlob->rgprod()[i]=manual;
    pProdGlob[0]=manual;pProdGlob[0].cItem=1023;cProdGlob=1;vrgZipProd[0].cpq=12;
    apply(IDC_IMPORT,0);CHECK(lpplProdGlob->iprodMac==52 && lpplProdGlob->iprodMax==52);
    reset(255);for(i=0;i<255;i++)lpplProdGlob->rgprod()[i]=manual;
    pProdGlob[0]=manual;pProdGlob[0].cItem=1023;cProdGlob=1;vrgZipProd[0].cpq=1;vrgZipProd[0].rgpq[0].w=64;
    apply(IDC_IMPORT,0);CHECK(lastBytes==1028 && lpplProdGlob->iprodMac==0 && lpplProdGlob->iprodMax==0);
    CHECK(lpplProdGlob->rgprod()[254].unused==17 && lpplProdGlob->rgprod()[255].cItem==1);
    // Every accepted native count/preset-count pair, including 256..267 storage.
    for(n=0;n<=255;n++)for(k=0;k<=12;k++) {
        int capacity=n+k ? n+k:1;
        reset(n);for(i=0;i<n;i++)lpplProdGlob->rgprod()[i]=manual;
        pProdGlob[0]=manual;pProdGlob[0].cItem=1023;cProdGlob=1;vrgZipProd[0].cpq=k;
        for(i=0;i<k;i++)vrgZipProd[0].rgpq[i].w=67;
        apply(IDC_IMPORT,0);
        CHECK(lastBytes==(size_t)(4+4*capacity) && lpplProdGlob->iprodMax==(capacity&255));
        CHECK(lpplProdGlob->iprodMac==((n+k)&255));
        if(n)CHECK(memcmp(&lpplProdGlob->rgprod()[n-1],&manual,4)==0);
        if(k)CHECK(lpplProdGlob->rgprod()[n+k-1].iItem==3 && lpplProdGlob->rgprod()[n+k-1].pct==0);
    }
    reset(1);lpplProdGlob->rgprod()[0]=manual;pProdGlob[0]=manual;cProdGlob=1;
    sel.pl.fNoResearch=1;hwndProdDlg=1;fDlgUp=1;failAllocation=1;penvMem=&env;
    if(setjmp(env)==0){apply(IDC_IMPORT,0);CHECK(0);}
    CHECK(pProdGlob[0].cItem==2 && lpplProdGlob!=NULL && freed==0);
    CHECK(recover()==0 && lpplProdGlob==NULL && hwndProdDlg==0 && !fDlgUp);
    CHECK(freed==1 && ended==1 && alerts==1 && sel.pl.fNoResearch==1);
    reset(14);lpplProdGlob->rgprod()[0]=manual;for(i=1;i<14;i++)lpplProdGlob->rgprod()[i]=automatic;
    strcpy(vrgZipProd[0].szName,"Default");iResTechNow=0;sel.pl.fNoResearch=1;renameResult=0;edit(IDC_IMPORT);
    CHECK(renameCalls==0 && vrgZipProd[0].cpq==12 && vrgZipProd[0].fValid && vrgZipProd[0].fNoResearch && gd.fChgZipProd);
    CHECK(strcmp(vrgZipProd[0].szName,"Default")==0 && vrgZipProd[0].rgpq[11].w==640);
    iResTechNow=1;vrgZipProd[1].rgpq[11].w=511;memcpy(snapshot,vrgZipProd,sizeof(snapshot));gd.fChgZipProd=0;edit(IDC_IMPORT);
    CHECK(memcmp(snapshot,vrgZipProd,sizeof(snapshot))==0 && gd.fChgZipProd && renameCalls==1);
    renameResult=-1;renameText[0]=0;edit(IDC_RENAME);
    CHECK(strcmp(vrgZipProd[1].szName,"Custom1")==0 && !vrgZipProd[1].fValid);
    strcpy(renameText,"A&B");lpplProdGlob->iprodMac=2;edit(IDC_IMPORT);
    CHECK(strcmp(label,"A&&B")==0 && vrgZipProd[1].cpq==1 && vrgZipProd[1].rgpq[0].w==640 && vrgZipProd[1].rgpq[11].w==511);
    CHECK(enabledDelete && enabledRename);memcpy(snapshot,vrgZipProd,sizeof(snapshot));edit(IDC_DELETE);
    snapshot[1].fValid=0;CHECK(memcmp(snapshot,vrgZipProd,sizeof(snapshot))==0);
    EnableZipProdBtns(1,0);CHECK(!enabledDelete && !enabledRename);
    iResTechNow=0;edit(IDC_DELETE);CHECK(!vrgZipProd[0].fValid); // dispatch itself does not guard default
    memcpy(snapshot,vrgZipProd,sizeof(snapshot));outerResult=0;gd.fChgZipProd=0;customize();
    CHECK(memcmp(snapshot,vrgZipProd,160)==0 && vrgZipProd[4].cpq==9 && gd.fChgZipProd);
    outerResult=65536;customize();CHECK(memcmp(snapshot,vrgZipProd,160)==0);
    outerResult=-1;customize();CHECK(vrgZipProd[0].fValid==0x5a && vrgZipProd[4].cpq==9);
    reset(0);FillZipProdLB(1,&vrgZipProd[0]);CHECK(labelCount==1 && strcmp(labels[0],"Automatic orders")==0);
    vrgZipProd[0].fValid=1;FillZipProdLB(1,&vrgZipProd[0]);CHECK(labelCount==1 && strcmp(labels[0],"Automatic orders")==0);
    vrgZipProd[0].cpq=3;
    vrgZipProd[0].rgpq[0].w=3;vrgZipProd[0].rgpq[1].w=64;vrgZipProd[0].rgpq[2].w=1;
    FillZipProdLB(1,&vrgZipProd[0]);CHECK(labelCount==3 && strcmp(labels[0],"Item3")==0 && strcmp(labels[1],"Item0")==0 && strcmp(labels[2],"Item1 0")==0);
    memset(vrgZipProd,0,sizeof(vrgZipProd));vrgZipProd[1].fValid=vrgZipProd[3].fValid=1;
    CHECK(popupSlot(0)==1 && popupSlot(1)==3 && popupSlot(2)==4 && popupSlot(50)==4);
    FreePl(lpplProdGlob);printf("M09 source-extracted preset witnesses: %d assertions passed\n",checks);return 0;
}
'''
    with tempfile.TemporaryDirectory(prefix='m09-presets-') as work:
        source = Path(work) / 'witness.cpp'
        binary = Path(work) / 'witness'
        source.write_text(declarations + storage_declarations(allocate=True, resize=False, free=True) + bodies + storage_definitions('(void)lp;return lastBytes;', allocate=True, resize=False, free=True) + witnesses)
        subprocess.run(shlex.split(os.environ.get('CXX', 'c++')) +
                       ['-std=gnu++20','-DSTARS_TEST_STATE_DOUBLE','-include',str(ROOT/'src/platform/value.h'),'-Wall', '-Wextra', '-Werror','-Wno-misleading-indentation','-Wno-deprecated-declarations', '-fsanitize=undefined', '-fno-sanitize-recover=undefined',
                        str(source), '-o', str(binary)], check=True)
        subprocess.run([str(binary)], check=True)


if __name__ == '__main__':
    main()
