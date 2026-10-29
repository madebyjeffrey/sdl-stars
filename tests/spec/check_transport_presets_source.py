#!/usr/bin/env python3
"""Exercise extracted M10 preset and transport-control code with native layouts.

GUI/localization/write-back calls are doubles. The edit double supplies the
single-line synchronous EN_CHANGE behavior of the SDL wrapper; real lifetime,
focus and rendering remain separate obligations. Undefined slot scans are not run.
"""
from pathlib import Path
from check_s02_source import struct_start
import os
import shlex
import subprocess
import tempfile

from check_s02_source import ROOT, function, enum_source
from check_s02_state_source import between


def main():
    ui = (ROOT / 'src/ui/shipui.c').read_text()
    custom = function('src/ui/ship2ui.c', 'INT_PTR CALLBACK ZipOrderDlg(')
    types = (ROOT / 'src/core/structs.h').read_text()
    enums = enum_source()
    declarations = r'''
#include <stdint.h>
typedef uint16_t StringId;
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define TRUE 1
#define FALSE 0
#define LOWORD(x) ((uint16_t)(x))
#define MAKEINTRESOURCE(x) (x)
#define MakeProcInstance(x,y) (x)
#define FreeProcInstance(x) ((void)(x))
#define wsprintf sprintf
#define IDC_IMPORT 1
#define IDC_RENAME 2
#define IDC_DELETE 3
#define IDD_RENAME 4
#define IDD_ZIP_PROD 5
#define ZipOrderDlg 6
#define RenameZipDlg 7
#define idsCustomD 8
#define idsUnusedD 9
#define idsCustomize 10
#define idsWithinDLY 11
#define idsAnyEnemy 12
#define idsDYearC 13
#define idsIindefinitely 14
#define idsAmountCargoMaySpecifyHereMustBetween 15
#define idsAction 100
#define idsSetWaypoint 109
#define idsLoadDunnage 107
#define idsLoadOptimal 110
#define CB_GETCURSEL 20
#define CB_SETCURSEL 21
#define CB_RESETCONTENT 22
#define CB_ADDSTRING 23
#define idWriteBack -2
#define MB_ICONHAND 0
#define tileStarbaseOrWaypoint 1
#define PCTD "%d"
typedef int HWND;
typedef intptr_t LPARAM;
typedef struct { int16_t x,y; } POINT16;
'''
    start = enums.index('enum XferActionType : uint16_t {')
    declarations += enums[start:enums.index('\n};', start)+3] + '\n'
    for tag, alias in [('_itemaction','ITEMACTION'), ('_taskxport','TASKXPORT'), ('_tasklaymines','TASKLAYMINES'),
                       ('_taskpatrol','TASKPATROL'), ('_tasksell','TASKSELL'), ('_order','ORDER'), ('_ziporder','ZIPORDER')]:
        start = struct_start(types, tag)
        declarations += types[start:types.index('\n};',start)+3] + f'\ntypedef struct {tag} {alias};\n'
    declarations += r'''
static ZIPORDER vrgZip[4];
static struct { ORDER rgordStore[2]; ORDER *rgord() { return rgordStore; } } orders;
static struct { struct { void *unused; typeof(orders) *lpplord; } fl; int iwpAct; } sel;
static struct { int fChgZipOrd; } gd;
static struct { int cPlayer; } game;
static HWND rghwndOrderDD[3]={10,11,12},hwndOrderED=13,hwndFrame,hwndPlanet;
static int hInst,idPlayer,iResTechNow,popupResult,popupCount,renameResult,renameCalls,outerResult;
static int writes,draws,updates,alerts,hasCargo,notifications,depth,checks,enabledDelete,enabledRename;
static int comboCount[3],comboSelection[3];
static char comboLabels[3][20][80],szWork[256],editText[80],renameText[13],radioLabel[80];
static ORDER snapshots[16];
static char rgszMineralsStorage[5][80]={"Ironium","Boranium","Germanium","Colonists","Fuel"};
static char *rgszMinerals[5]={rgszMineralsStorage[0],rgszMineralsStorage[1],rgszMineralsStorage[2],rgszMineralsStorage[3],rgszMineralsStorage[4]};
static char rgszZipOrderStorage[4][80]={"Load","Unload","Wait","Clear"};
static char *rgszZipOrder[4]={rgszZipOrderStorage[0],rgszZipOrderStorage[1],rgszZipOrderStorage[2],rgszZipOrderStorage[3]};
static char actionLabelsStorage[11][80]={"(no action)","Load All Available","Unload All","Load Exactly...","Unload Exactly...","Fill Up to %...","Wait for %...","Load Dunnage","Set Amount to...","Set Waypoint to...","Load Optimal"};
static char *actionLabels[11]={actionLabelsStorage[0],actionLabelsStorage[1],actionLabelsStorage[2],actionLabelsStorage[3],actionLabelsStorage[4],actionLabelsStorage[5],actionLabelsStorage[6],actionLabelsStorage[7],actionLabelsStorage[8],actionLabelsStorage[9],actionLabelsStorage[10]};
static char szLiteral0[] = "Player";
static char szLiteral1[] = "Custom%d";
static char szLiteral2[] = "Unused%d";
static char szLiteral3[] = "%d LY";
static char szLiteral4[] = "%d Year%c";
static char szLiteral5[] = "Other";
static void amount(void);
void UpdateOrdersDDs(int16_t);
static HWND GetDlgItem(HWND hwnd,int id) { (void)hwnd;return id; }
static intptr_t SendMessage(HWND hwnd,int message,int value,LPARAM param) {
    int c=hwnd-10;if(c<0 || c>=3)abort();
    if(message==CB_GETCURSEL)return comboSelection[c];
    if(message==CB_RESETCONTENT){comboCount[c]=0;comboSelection[c]=-1;return 0;}
    if(message==CB_ADDSTRING){if(comboCount[c]>=20)abort();strcpy(comboLabels[c][comboCount[c]],(const char *)param);return comboCount[c]++;}
    if(message==CB_SETCURSEL){comboSelection[c]=(value>=0 && value<comboCount[c]) ? value:-1;return comboSelection[c];}
    abort();
}
static void SetWindowText(HWND hwnd,const char *s) {
    if(hwnd==hwndOrderED){strcpy(editText,s);if(notifications){if(++depth>3)abort();amount();depth--;}}
    else strcpy(radioLabel,s);
}
static void GetWindowText(HWND hwnd,char *out,int capacity) {
    size_t n=strlen(editText);(void)hwnd;if(n>=(size_t)capacity)n=capacity-1;memcpy(out,editText,n);out[n]=0;
}
static int FLookupFleet(int id,void *p) {
    (void)id;(void)p;if(writes<16)snapshots[writes]=orders.rgord()[sel.iwpAct];writes++;return 0;
}
static void DrawPlanShip(void *p,int tile) { (void)p;(void)tile;draws++; }
static int LGetFleetStat(void *p,int stat) { (void)p;(void)stat;return hasCargo; }
static char *PszPlayerName(int i,int a,int b,int c,int d,void *p) {(void)i;(void)a;(void)b;(void)c;(void)d;(void)p;return szLiteral0;}
static char *PszGetCompressedString(int id) {
    if(id>=idsAction && id<=idsLoadOptimal)return actionLabels[id-idsAction];
    if(id==idsCustomD)return szLiteral1;if(id==idsUnusedD)return szLiteral2;
    if(id==idsWithinDLY)return szLiteral3;if(id==idsDYearC)return szLiteral4;
    return szLiteral5;
}
static const char *PszFormatIds(int id,void *p) { (void)id;(void)p;return "amount"; }
static int CchGetString(int id,char *out) { strcpy(out,PszGetCompressedString(id));return (int)strlen(out); }
static void AlertSz(const char *s,int flags) { (void)s;(void)flags;alerts++; }
static void EnableWindow(HWND hwnd,int value) {if(hwnd==IDC_DELETE)enabledDelete=value;if(hwnd==IDC_RENAME)enabledRename=value;}
static void InvalidateRect(HWND hwnd,void *rc,int erase) {(void)hwnd;(void)rc;(void)erase;}
static void SetFocus(HWND hwnd) {(void)hwnd;}
static int DialogBox(int instance,int resource,HWND hwnd,int proc) {
    (void)instance;(void)hwnd;(void)proc;
    if(resource==IDD_RENAME){renameCalls++;if(renameResult)strcpy(szWork,renameText);return renameResult;}
    memset(vrgZip,0x5a,sizeof(vrgZip));gd.fChgZipOrd=1;return outerResult;
}
static int PopupMenu(HWND hwnd,int x,int y,int count,void *p,const char *const *labels,int selected,int flag) {
    (void)hwnd;(void)x;(void)y;(void)p;(void)labels;(void)selected;(void)flag;popupCount=count;return popupResult;
}
#define CHECK(x) do { checks++;if(!(x)){fprintf(stderr,"witness %d failed: %s\n",checks,#x);return 1;} } while(0)
'''.replace('typeof(orders)', 'struct OrderArray')
    declarations = declarations.replace('static struct { ORDER rgordStore[2]; ORDER *rgord() { return rgordStore; } } orders;', 'static struct OrderArray { ORDER rgordStore[2]; ORDER *rgord() { return rgordStore; } } orders;')
    bodies = function('src/ui/ship2ui.c', 'void EnableZipBtns(') + '\n'
    block = between(custom, '            case IDC_IMPORT:\n            case IDC_RENAME:', '            case IDC_HELP:')
    bodies += 'static void edit(int wParam) {int hwnd=1,lpProc;char *psz,*pszT;switch(wParam){' + block + '}}\n'
    click = function('src/ui/shipui.c', 'HCURSOR ClickInShipOrders(')
    block = between(click, '            sz255[0] = -1;', '        } else {\n            GlobalPD.grPopup')
    bodies += 'static void popup(void) {int16_t i,cMax,iSkip,fRet;int lpProc;POINT16 pt={0,0};char sz255[2],*rgszZip[11];ZIPORDER rgzo[4];TASKXPORT *lptxp;' + block + '}\n'
    start = ui.index('    } else if ((HWND)lParam == hwndOrderED && HIWORD(wParam) == 768) {')
    start = ui.index('        lSel = SendMessage(', start)
    block = ui[start:ui.index('\n    }\n    return;',start)]
    bodies += 'static void amount(void) {int32_t lSel,lMin;int16_t iInit,i,fPercent=FALSE;char rgb[8];' + block + '}\n'
    start = ui.index('    } else if ((HWND)lParam == rghwndOrderDD[2]) {')
    start = ui.index('            lSel = SendMessage(',start)
    end = ui.index('            DrawPlanShip(NULL, tileStarbaseOrWaypoint);',start)+len('            DrawPlanShip(NULL, tileStarbaseOrWaypoint);')
    bodies += 'static void action(void) {int32_t lSel,lMin;' + ui[start:end] + '}\n'
    bodies += '#define UpdateOrdersDDs UpdateOrdersBody\n' + function('src/ui/shipui.c', 'void UpdateOrdersDDs(') + '\n#undef UpdateOrdersDDs\nvoid UpdateOrdersDDs(int16_t level) {updates++;UpdateOrdersBody(level); }\n'
    block = between(ui,'            fActive = TRUE;\n            switch (l) {','            psz = vrgszUnits[i];')
    bodies += 'static int quantityControl(int l,int i,int *unit) {int fActive;' + block + '*unit=i;return fActive;}\n'
    block = between(custom,'                iAction = static_cast<XferActionType>(vrgZip[iResTechNow].txp.rgia[i].iAction());', '                TextOut(hdc, xCtr + 6,')
    bodies += 'static void paintLabel(int i) {int cch;XferActionType iAction;' + block + '}\n'
    bodies += r'''
static void reset(void) {
    memset(vrgZip,0,sizeof(vrgZip));memset(&orders,0,sizeof(orders));memset(&sel,0,sizeof(sel));
    sel.fl.lpplord=&orders;sel.iwpAct=1;orders.rgord()[1].grTask=1;
    comboCount[0]=comboCount[2]=10;comboCount[1]=5;
    comboSelection[0]=1;comboSelection[1]=comboSelection[2]=0;
    writes=draws=updates=alerts=renameCalls=gd.fChgZipOrd=depth=0;
    notifications=0;hasCargo=1;renameResult=1;renameText[0]=0;editText[0]=0;
}
'''
    witnesses = r'''
int main(void) {
    int mode,i,mask,k,rank,n,pass,expected;ZIPORDER before[4];TASKXPORT prior;unsigned char header[8];
    CHECK(sizeof(ITEMACTION)==2 && sizeof(TASKXPORT)==10 && sizeof(ORDER)==18 && sizeof(ZIPORDER)==24);
    for(mode=0;mode<4;mode++) {
        reset();orders.rgord()[1].grTask=2;comboSelection[0]=2; // direct handler does not change task
        for(i=0;i<5;i++){orders.rgord()[1].txp.rgia[i].SetIAction(15);orders.rgord()[1].txp.rgia[i].SetCQuan(1234+i);}
        orders.rgord()[1].pt=(POINT16){123,-45};orders.rgord()[1].id=231;orders.rgord()[1].iWarp=9;
        orders.rgord()[1].grobj=2;orders.rgord()[1].fValidTask=1;orders.rgord()[1].fNoAutoTrack=1;orders.rgord()[1].fUnused=3;
        memcpy(header,&orders.rgord()[1],8);popupResult=mode;popup();CHECK(memcmp(header,&orders.rgord()[1],8)==0);
        CHECK(writes==1 && updates==1 && draws==1 && orders.rgord()[1].grTask==2);
        for(i=0;i<5;i++){
            expected=mode==3 ? 0 : mode==2 && i<3 ? 100 : 1234+i;
            CHECK(orders.rgord()[1].txp.rgia[i].cQuan()==expected);
            expected=mode==3 ? 0 : i==4 ? 7 : mode==1 ? 2 : i==3 ? 0 : mode==0 ? 1:6;
            CHECK(orders.rgord()[1].txp.rgia[i].iAction()==expected);
        }
    }
    for(mask=0;mask<16;mask++) {
        reset();k=0;for(i=0;i<4;i++){vrgZip[i].fValid=(mask>>i)&1;k+=vrgZip[i].fValid;vrgZip[i].txp.rgia[0].SetCQuan(100+i);}
        memcpy(before,vrgZip,sizeof(before));rank=0;
        for(i=0;i<4;i++)if(vrgZip[i].fValid){popupResult=5+rank++;writes=0;popup();CHECK(orders.rgord()[1].txp.rgia[0].cQuan()==100+i && writes==1);}
        popupResult=(k ? 6+k:5);outerResult=0;gd.fChgZipOrd=0;writes=0;popup();
        CHECK(popupCount==(k ? 7+k:6) && popupCount<=11 && memcmp(before,vrgZip,96)==0 && gd.fChgZipOrd && writes==0);
    }
    reset();prior=orders.rgord()[1].txp;popupResult=-1;popup();CHECK(writes==0);
    popupResult=4;popup();CHECK(writes==1 && memcmp(&prior,&orders.rgord()[1].txp,10)==0);
    popupResult=-2;popup();CHECK(writes==2 && memcmp(&prior,&orders.rgord()[1].txp,10)==0);
    popupResult=5;outerResult=65536;memcpy(before,vrgZip,96);popup();CHECK(memcmp(before,vrgZip,96)==0);
    popupResult=5;outerResult=-1;popup();CHECK(vrgZip[0].fValid==0x5a);
    reset();iResTechNow=0;renameResult=0;memcpy(before,vrgZip,sizeof(before));edit(IDC_IMPORT);
    CHECK(renameCalls==1 && gd.fChgZipOrd && memcmp(before,vrgZip,96)==0);
    renameResult=-1;edit(IDC_RENAME);CHECK(strcmp(vrgZip[0].szName,"Custom0")==0 && !vrgZip[0].fValid);
    strcpy(renameText,"A&B");for(i=0;i<5;i++){orders.rgord()[1].txp.rgia[i].SetIAction(15);orders.rgord()[1].txp.rgia[i].SetCQuan(4095);}
    edit(IDC_IMPORT);CHECK(vrgZip[0].fValid && strcmp(radioLabel,"A&&B")==0 && memcmp(&vrgZip[0].txp,&orders.rgord()[1].txp,10)==0 && enabledDelete && enabledRename);
    memcpy(before,vrgZip,96);edit(IDC_DELETE);before[0].fValid=0;
    CHECK(memcmp(before,vrgZip,96)==0 && enabledDelete && enabledRename); // no button refresh after Delete
    EnableZipBtns(1,0);CHECK(!enabledDelete && !enabledRename);
    // Exact amount normalization over every int16 bit pattern and both limits.
    reset();notifications=1;comboSelection[1]=0;
    for(pass=0;pass<2;pass++)for(n=0;n<65536;n++) {
        int narrowed=n<32768 ? n:n-65536;int maximum=pass ? 100:4000;
        expected=narrowed<0 ? 0:narrowed>maximum ? maximum:narrowed;
        comboSelection[2]=pass ? 5:3;sprintf(editText,"%d",n);writes=alerts=0;amount();
        CHECK(orders.rgord()[1].txp.rgia[4].cQuan()==expected && alerts==(narrowed!=expected) && writes==(narrowed!=expected ? 2:1));
    }
    strcpy(editText,"65636");comboSelection[2]=5;alerts=0;amount();CHECK(orders.rgord()[1].txp.rgia[4].cQuan()==100 && alerts==0 && strcmp(editText,"65636")==0);
    strcpy(editText,"-65535");comboSelection[2]=3;alerts=0;amount();CHECK(orders.rgord()[1].txp.rgia[4].cQuan()==1 && alerts==0 && strcmp(editText,"-65535")==0);
    strcpy(editText,"40001234");amount();CHECK(orders.rgord()[1].txp.rgia[4].cQuan()==2427 && strcmp(editText,"40001234")==0);
    strcpy(editText,"junk");amount();CHECK(orders.rgord()[1].txp.rgia[4].cQuan()==0 && strcmp(editText,"junk")==0);
    reset();notifications=1;for(i=0;i<5;i++){orders.rgord()[1].txp.rgia[i].SetIAction(15);orders.rgord()[1].txp.rgia[i].SetCQuan(4095);}
    popupResult=2;popup();CHECK(snapshots[0].txp.rgia[4].cQuan()==4095 && orders.rgord()[1].txp.rgia[4].cQuan()==4000 && writes==3 && alerts==1);
    CHECK(orders.rgord()[1].txp.rgia[0].cQuan()==100 && orders.rgord()[1].txp.rgia[3].cQuan()==4095 && strcmp(comboLabels[2][7],"Load Optimal")==0);
    reset();notifications=1;orders.rgord()[1].txp.rgia[0].SetIAction(5);orders.rgord()[1].txp.rgia[0].SetCQuan(4095);
    UpdateOrdersDDs(1);CHECK(comboSelection[1]==1 && orders.rgord()[1].txp.rgia[0].cQuan()==100 && writes==2 && alerts==1);
    reset();notifications=1;hasCargo=0;orders.rgord()[1].txp.rgia[0].SetIAction(5);orders.rgord()[1].txp.rgia[0].SetCQuan(4095);
    UpdateOrdersDDs(1);CHECK(comboCount[1]==1 && comboSelection[1]==0 && orders.rgord()[1].txp.rgia[0].cQuan()==4095);
    reset();notifications=1;orders.rgord()[1].txp.rgia[4].SetIAction(15);orders.rgord()[1].txp.rgia[4].SetCQuan(4095);
    UpdateOrdersDDs(1);CHECK(comboSelection[2]==-1 && orders.rgord()[1].txp.rgia[4].cQuan()==4000);
    reset();notifications=1;comboSelection[1]=1;comboSelection[2]=5;orders.rgord()[1].txp.rgia[0].SetCQuan(4095);action();
    CHECK(snapshots[0].txp.rgia[0].iAction()==5 && snapshots[0].txp.rgia[0].cQuan()==4095 && orders.rgord()[1].txp.rgia[0].cQuan()==100 && writes==3);
    comboSelection[1]=0;comboSelection[2]=-1;action();CHECK(orders.rgord()[1].txp.rgia[4].iAction()==15);
    for(pass=0;pass<2;pass++)for(mask=0;mask<32;mask++) {
        int selected=4,priority[5]={4,0,1,2,3};reset();notifications=1;hasCargo=pass;
        for(i=0;i<5;i++){orders.rgord()[1].txp.rgia[i].SetIAction((mask>>i)&1);orders.rgord()[1].txp.rgia[i].SetCQuan(4095);}
        for(i=0;i<(pass ? 5:1);i++)if((mask>>priority[i])&1){selected=priority[i];break;}
        UpdateOrdersDDs(1);CHECK(comboSelection[1]==(selected==4 ? 0:selected+1));
        for(i=0;i<5;i++)CHECK(orders.rgord()[1].txp.rgia[i].cQuan()==(i==selected ? 4000:4095));
    }
    for(i=-1;i<16;i++){int unit;int enabled=quantityControl(i,4,&unit);CHECK(enabled==(i!=0 && i!=1 && i!=2 && i!=7) && unit==((i==5 || i==6) ? 5:4));}
    reset();iResTechNow=0;vrgZip[0].txp.rgia[0].SetIAction(5);vrgZip[0].txp.rgia[0].SetCQuan(120);paintLabel(0);
    CHECK(strcmp(szWork,"Fill Up to % 120kT")==0);
    vrgZip[0].txp.rgia[0].SetIAction(7);paintLabel(0);CHECK(strcmp(szWork,"Load Dunnage")==0);
    printf("M10 source-extracted transport witnesses: %d assertions passed\n",checks);return 0;
}
'''
    with tempfile.TemporaryDirectory(prefix='m10-presets-') as work:
        source = Path(work) / 'witness.cpp'
        binary = Path(work) / 'witness'
        source.write_text('#include "' + str(ROOT/'src/core/taskviews.h') + '"\n' + declarations + bodies + witnesses)
        subprocess.run(shlex.split(os.environ.get('CXX', 'c++')) +
                       ['-std=gnu++20','-include',str(ROOT/'src/platform/value.h'),'-x','c++','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-Wno-deprecated-declarations','-fsanitize=undefined','-fno-sanitize-recover=undefined',
                        str(source),'-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True)


if __name__ == '__main__':
    main()
