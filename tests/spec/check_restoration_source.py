#!/usr/bin/env python3
"""Native-source witnesses for M07 restoration, M15 traversal and S06 lookup.

Actual C function bodies run over small projected native structs. UI callbacks
are logged doubles, including explicit post-callback mutation cases. This is
control/lookup evidence, not proof of callback purity or actual GUI rendering.
"""
from pathlib import Path
import os
import shlex
import subprocess
import tempfile

from check_s02_source import ROOT, function
from check_s02_state_source import between


def main():
    mdi = (ROOT / "src/ui/mdi.cpp").read_text()
    post = between(mdi, "        case IDM_FRAME_POST_OPEN:\n", "        case IDM_FLEET_DELETE_WAYPOINT:")
    post = post[post.index("\n") + 1:]
    opened = function("src/ui/mdi.cpp", "int16_t FOpenGame(")
    tail = opened[opened.index("    CreateChildWindows();") + len("    CreateChildWindows();"):]
    declarations = r'''
#include <assert.h>
#include <stdint.h>
typedef uint16_t GrobjClass;
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define TRUE 1
#define FALSE 0
#define imsgNone -1
#define iplrNone -1
#define grobjNone 0
#define WM_COMMAND 1
#define IDM_FRAME_POST_OPEN 2
typedef int16_t MessageId;
typedef struct { int16_t id,iPlayer; } PLANET;
typedef struct { int16_t id,iPlayer; } FLEET;
static PLANET planets[16],*lpPlanets=planets;
static FLEET fleets[16],*rglpfl[16];
static struct { int16_t idPlanetHome; uint16_t cFleet; } rgplr[16];
static struct { int32_t lid; uint16_t turn,cPlanMax; } game;
static struct { int16_t idPlayer,iObjSel,iMsg; uint16_t grobjSel:4; int32_t lid; uint16_t turn; } ini;
static struct { int16_t fGotoVCR,fTutorial,fNoScannerDraw; } gd;
static struct { int16_t fChange; } tutor;
static int16_t idPlayer,cPlanet,cFleet,cMsg,vcmsgplrIn,iMsgCur,fViewFilteredMsg,fFreeingTitle;
static unsigned char bitfMsgFiltered[49];
static int16_t ids[8];
static void *hwndMessage=(void *)1,*hwndScanner=(void *)2,*hwndFrame=(void *)3,*hwndTitle;
static char trace[1024];
static int mutateSelect,mutateTitle,mutateTutor,lookups,checks;
static void event(const char *text) { assert(strlen(trace)+strlen(text)<sizeof(trace));strcat(trace,text); }
static void selected(char kind,int16_t id) {
    char text[32];snprintf(text,sizeof(text),"%c%d;",kind,id);event(text);
    if(mutateSelect) { ini.grobjSel=4;ini.iObjSel=5;mutateSelect=0; }
}
static void SelectAdjPlanet(int16_t delta,int16_t id) { assert(delta==0);selected('P',id); }
static void SelectAdjFleet(int16_t delta,int16_t id) { assert(delta==0);selected('L',id); }
static int16_t IdmGetMessageN(int16_t i) { assert(i>=0 && i<cMsg);lookups++;return ids[i]; }
static void SetMsgTitle(void *hwnd) { assert(hwnd==hwndMessage);event("T;");if(mutateTitle)gd.fTutorial=0; }
static void InvalidateRect(void *hwnd,void *rect,int erase) {
    assert(!rect && erase);
    if(hwnd==hwndMessage)event("I;");else { assert(hwnd==hwndScanner && !gd.fNoScannerDraw);event("S;"); }
}
static void AdvanceTutor(void) {
    assert(tutor.fChange);event("A;");tutor.fChange=0;
    if(mutateTutor) { ini.grobjSel=1;ini.iObjSel=5;planets[1].iPlayer=idPlayer;iMsgCur=3;gd.fGotoVCR=1; }
}
static void RefitFrameChildren(void) { assert(gd.fNoScannerDraw);event("R;"); }
static void UpdateWindow(void *hwnd) { assert(hwnd==hwndScanner && !gd.fNoScannerDraw);event("U;"); }
static void DestroyWindow(void *hwnd) { (void)hwnd;event("D;"); }
static void BringUpHostDlg(void) { event("H;"); }
static void reset(void) {
    int i;memset(planets,0,sizeof(planets));memset(fleets,0,sizeof(fleets));memset(rglpfl,0,sizeof(rglpfl));
    memset(rgplr,0,sizeof(rgplr));memset(bitfMsgFiltered,0,sizeof(bitfMsgFiltered));
    idPlayer=0;game.lid=7;game.turn=2400;game.cPlanMax=10;rgplr[0].idPlanetHome=2;rgplr[0].cFleet=1;
    cPlanet=2;planets[0]=(PLANET){2,0};planets[1]=(PLANET){5,1};
    cFleet=1;fleets[0]=(FLEET){0,0};rglpfl[0]=&fleets[0];
    ini.idPlayer=0;ini.iObjSel=0;ini.grobjSel=2;ini.lid=7;ini.turn=2400;ini.iMsg=1;
    cMsg=3;vcmsgplrIn=1;iMsgCur=3;fViewFilteredMsg=0;
    for(i=0;i<8;i++)ids[i]=(int16_t)(i*8+1);bitfMsgFiltered[ids[1]>>3]|=1<<(ids[1]&7);
    gd.fGotoVCR=1;gd.fTutorial=1;gd.fNoScannerDraw=0;tutor.fChange=0;
    hwndScanner=(void *)2;hwndTitle=NULL;fFreeingTitle=0;
    mutateSelect=mutateTitle=mutateTutor=lookups=0;trace[0]=0;
}
#define CHECK(x) do { checks++;if(!(x)){fprintf(stderr,"witness %d failed: %s; trace=%s\n",checks,#x,trace);return 1;} } while(0)
'''
    bodies = "\n".join([
        function("src/core/msg.cpp", "int16_t IMsgNext("),
        function("src/core/msg.cpp", "int16_t IMsgPrev("),
        function("src/core/util.cpp", "PLANET *LpplFromId("),
        function("src/core/util.cpp", "FLEET *LpflFromId("),
        function("src/core/util.cpp", "int16_t FFindSomethingAndSelectIt(").replace(
            "int16_t FFindSomethingAndSelectIt()", "static int16_t findBody(void)"),
        'static int16_t FFindSomethingAndSelectIt(void) { event("F;");return findBody(); }',
        function("src/ui/mdi.cpp", "void RestoreSelection("),
        "static void postOpen(void) { do {\n" + post + "} while(0); }",
        "static void SendMessage(void *hwnd,int message,int command,int param) { assert(hwnd==hwndFrame && message==WM_COMMAND && command==IDM_FRAME_POST_OPEN && param==0);postOpen(); }",
        "static int finishOpen(void) { int16_t grobjIni;\n" + tail,
    ])
    witnesses = r'''
int main(void) {
    int n,inbound,mask,show,only,at,j,k,expected,next,previous,eligible[8],count;
    reset();RestoreSelection();CHECK(strcmp(trace,"L0;T;I;A;")==0 && ini.grobjSel==0 && iMsgCur==2 && !gd.fGotoVCR && !tutor.fChange);
    reset();ini.lid=8;RestoreSelection();CHECK(strcmp(trace,"P2;")==0 && ini.iObjSel==2 && ini.grobjSel==0 && iMsgCur==3 && gd.fGotoVCR);
    reset();ini.iObjSel=512;RestoreSelection();CHECK(strcmp(trace,"T;I;A;P2;")==0 && ini.iObjSel==2 && ini.grobjSel==0);
    reset();ini.grobjSel=1;ini.iObjSel=5;RestoreSelection();CHECK(strcmp(trace,"T;I;A;P2;")==0 && ini.iObjSel==2);
    reset();ini.grobjSel=4;ini.iObjSel=5;RestoreSelection();CHECK(strcmp(trace,"T;I;A;F;P2;")==0 && ini.iObjSel==5 && ini.grobjSel==0);
    reset();ini.grobjSel=4;ini.iObjSel=2;RestoreSelection();CHECK(strcmp(trace,"T;I;A;P2;")==0 && ini.grobjSel==0);
    reset();ini.grobjSel=0;ini.iMsg=0;RestoreSelection();CHECK(strcmp(trace,"")==0 && iMsgCur==3 && gd.fGotoVCR);
    reset();ini.turn=2399;RestoreSelection();CHECK(strcmp(trace,"L0;")==0 && iMsgCur==3 && gd.fGotoVCR);
    reset();ini.iMsg=99;RestoreSelection();CHECK(iMsgCur==0 && strcmp(trace,"L0;T;I;A;")==0);
    reset();ini.iMsg=4;RestoreSelection();CHECK(iMsgCur==-1);
    reset();ini.iObjSel=8192;RestoreSelection();CHECK(strcmp(trace,"L8192;T;I;A;")==0 && ini.grobjSel==0);
    reset();mutateSelect=1;RestoreSelection();CHECK(ini.grobjSel==0 && ini.iObjSel==5 && strcmp(trace,"L0;T;I;A;")==0);
    reset();mutateTitle=1;RestoreSelection();CHECK(strcmp(trace,"L0;T;I;")==0);
    reset();mutateTutor=1;RestoreSelection();CHECK(strcmp(trace,"L0;T;I;A;P5;")==0 && ini.grobjSel==0 && iMsgCur==3 && gd.fGotoVCR);
    reset();cPlanet=0;rgplr[0].idPlanetHome=-1;ini.lid=8;cFleet=2;rglpfl[0]=NULL;rglpfl[1]=&fleets[0];
    RestoreSelection();CHECK(strcmp(trace,"F;")==0 && ini.grobjSel==0);
    reset();planets[0].iPlayer=1;planets[1].iPlayer=0;CHECK(FFindSomethingAndSelectIt() && strcmp(trace,"F;P5;")==0);
    reset();cPlanet=0;CHECK(FFindSomethingAndSelectIt() && strcmp(trace,"F;L0;")==0);
    reset();cPlanet=0;cFleet=0;CHECK(!FFindSomethingAndSelectIt() && strcmp(trace,"F;")==0);
    reset();CHECK(LpplFromId(5)==&planets[1] && LpplFromId(1)==NULL && LpplFromId(-1)==NULL);
    game.cPlanMax=2;CHECK(LpplFromId(1)==&planets[1] && LpplFromId(1)->id==5 && LpplFromId(2)==NULL);
    reset();CHECK(LpflFromId(8192)==&fleets[0] && LpflFromId(512)==NULL);
    reset();cFleet=2;rgplr[1].cFleet=1;fleets[1]=(FLEET){512,1};rglpfl[1]=&fleets[1];
    CHECK(LpflFromId(8704)==&fleets[1]);
    reset();cMsg=2;vcmsgplrIn=1;ids[1]=ids[0];memset(bitfMsgFiltered,0,sizeof(bitfMsgFiltered));bitfMsgFiltered[ids[0]>>3]=1<<(ids[0]&7);iMsgCur=-1;
    CHECK(IMsgNext(FALSE)==2 && IMsgNext(TRUE)==0);iMsgCur=0;CHECK(IMsgNext(TRUE)==1);
    reset();postOpen();CHECK(strcmp(trace,"L0;T;I;A;R;S;U;")==0 && !gd.fNoScannerDraw);
    reset();hwndScanner=NULL;gd.fNoScannerDraw=1;postOpen();CHECK(strcmp(trace,"")==0 && ini.grobjSel==2 && gd.fNoScannerDraw);
    reset();CHECK(finishOpen()==1 && strcmp(trace,"L0;T;I;A;R;S;U;")==0);
    reset();ini.grobjSel=0;ini.lid=8;CHECK(finishOpen()==1 && strcmp(trace,"P2;R;S;U;F;P2;")==0);
    reset();hwndScanner=NULL;CHECK(finishOpen()==1 && strcmp(trace,"F;P2;")==0 && ini.grobjSel==0);
    reset();hwndScanner=NULL;cPlanet=0;CHECK(finishOpen()==1 && strcmp(trace,"")==0 && ini.grobjSel==0);
    reset();idPlayer=iplrNone;CHECK(finishOpen()==0 && strcmp(trace,"H;")==0);
    // Independent eligible-index oracle, across all small message views/modes.
    for(n=0;n<=4;n++)for(inbound=0;inbound<=2;inbound++)for(mask=0;mask<(1<<n);mask++)
    for(show=0;show<2;show++)for(only=0;only<2;only++) {
        reset();cMsg=n;vcmsgplrIn=inbound;fViewFilteredMsg=show;memset(bitfMsgFiltered,0,sizeof(bitfMsgFiltered));count=0;
        for(j=0;j<n+inbound;j++) {
            int filtered=j<n && (mask&(1<<j));
            if(filtered)bitfMsgFiltered[ids[j]>>3]|=1<<(ids[j]&7);
            if(j>=n || (show && !only) || !!filtered==only)eligible[count++]=j;
        }
        for(at=-1;at<=n+inbound;at++) {
            iMsgCur=(int16_t)at;lookups=0;expected=-1;
            for(k=count-1;k>=0;k--)if(eligible[k]<at){expected=eligible[k];break;}
            previous=IMsgPrev((int16_t)only);CHECK(previous==expected);
            if(show && !only)CHECK(lookups==0);
            if(at<=n+inbound) {
                expected=-1;lookups=0;
                for(k=0;k<count;k++)if(eligible[k]>at){expected=eligible[k];break;}
                next=IMsgNext((int16_t)only);CHECK(next==expected);
                if(show && !only)CHECK(lookups==0);
                if(next!=-1)for(k=0;k<count;k++)if(eligible[k]==at) {
                    iMsgCur=(int16_t)next;CHECK(IMsgPrev((int16_t)only)==at);
                }
            }
        }
    }
    printf("Restoration, lookup and message source witnesses: %d assertions passed\n",checks);
    return 0;
}
'''
    with tempfile.TemporaryDirectory(prefix="restore-source-") as work:
        source = Path(work) / "witness.cpp"
        binary = Path(work) / "witness"
        source.write_text(declarations + bodies + witnesses)
        subprocess.run(shlex.split(os.environ.get("CXX", "c++")) +
                       ["-std=gnu++20", "-DSTARS_TEST_STATE_DOUBLE", "-include", str(ROOT / "src/platform/value.h"), "-Wall", "-Wextra", "-Werror", "-Wno-misleading-indentation", "-fsanitize=undefined",
                        "-fno-sanitize-recover=undefined", str(source), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()
