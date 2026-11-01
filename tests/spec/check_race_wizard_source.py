#!/usr/bin/env python3
"""Actual RaceCreationWizard and password hash with scripted modal services."""
import json
from pathlib import Path
import subprocess
import tempfile
from check_s02_source import ROOT,function
from check_race_workflows_source import build

PRELUDE=r'''
#include <stdarg.h>
#include <limits.h>
#include "resource.h"
typedef intptr_t INT_PTR;typedef void (*FARPROC)(void);typedef int32_t BOOL;
#define MAKEINTRESOURCE(n) ((void *)(uintptr_t)(n))
static void *hInst;
int16_t fRCWReadOnly;RaceWizardPage iPanelActive;HWND hwndRaceParent;RECT *vrgrcRCW;
int32_t lSaltCur,lSaltLast;char szWork[360];
static int profile,targetPage,points,initialRO,noWrite,sequence,mode,passKind,pricePanel,stepCount,afterTarget,priced,checked,saved,ended,writes,first;
static int32_t NativeHash(char *);
static char *NativeSuggestion(char *);
static jmp_buf jump;
static void RaceWizardDlg1(void){}static void RaceWizardDlg2(void){}static void RaceWizardDlg3(void){}
static void RaceWizardDlg4(void){}static void RaceWizardDlg5(void){}static void RaceWizardDlg6(void){}
static void hex(const void *data,size_t n){const unsigned char *p=data;putchar('"');for(size_t i=0;i<n;i++)printf("%02x",p[i]);putchar('"');}
static void event(char kind,int64_t value,const char *a,const char *b){if(!first)putchar(',');first=0;printf("[\"%c\",%lld,",kind,(long long)value);hex(a,strlen(a));putchar(',');hex(b,strlen(b));putchar(']');}
static int procId(FARPROC p){return p==RaceWizardDlg1?1:p==RaceWizardDlg2?2:p==RaceWizardDlg3?3:p==RaceWizardDlg4?4:p==RaceWizardDlg5?5:6;}
static int resourceId(int n){return n==1?IDD_RACE_WIZARD_1:n==2?IDD_RACE_WIZARD_2:n==3?IDD_RACE_WIZARD_3:n==4?IDD_RACE_WIZARD_4:n==5?IDD_RACE_WIZARD_5:IDD_RACE_WIZARD_6;}
static int StarsCompatBeginDialogSequence(HWND parent){assert(parent==(HWND)7);event('b',sequence,"","");return sequence;}
static void StarsCompatEndDialogSequence(void){ended++;event('e',0,"","");assert(vrgrcRCW);vrgrcRCW[0]=(RECT){0,0,1,1};((uint8_t *)&vplr)[17]++;if(profile==11)longjmp(jump,1);}
static FARPROC MakeProcInstance(FARPROC proc,void *instance){assert(instance==hInst);event('m',procId(proc),"","");return proc;}
static void FreeProcInstance(FARPROC proc){event('f',procId(proc),"","");}
static INT_PTR DialogBox(void *instance,void *id,HWND parent,FARPROC proc){
    static const int order[]={1,4,5,2,3,6};assert(instance==hInst && parent==(HWND)7 && iPanelActive>=1 && iPanelActive<=6);
    assert(procId(proc)==order[iPanelActive-1] && (uintptr_t)id==(unsigned)resourceId(procId(proc)) && vrgrcRCW);
    vrgrcRCW[16]=(RECT){1,2,3,4};stepCount++;assert(stepCount<30);((uint8_t *)&vplr)[0]++;
    int64_t result=wizNext;
    if(profile==1)result=wizCancel;
    else if(profile==2)result=-1;
    else if(profile==7){event('d',iPanelActive,"escape","");longjmp(jump,1);}
    else if(afterTarget)result=wizCancel;
    else if(iPanelActive==targetPage){afterTarget=1;result=profile==12?(int64_t)points:wizFinish;}
    event('d',iPanelActive,"","");return (INT_PTR)result;
}
int16_t CAdvantagePoints(PLAYER *p){assert(p==&vplr);priced++;((uint8_t *)&vplr)[4]++;
    int16_t result=(int16_t)(profile==12?0:points);iPanelActive=(RaceWizardPage)pricePanel;event('p',result,"","");if(profile==8)longjmp(jump,1);return result;}
int32_t LSaltFromSz(char *p){int32_t n=NativeHash(p);event('h',n,p,"");return n;}
int16_t FCheckPassword(void){assert(lSaltCur==NativeHash(szRacePass) && lSaltLast==-5);checked++;((uint8_t *)&vplr)[8]++;event('k',profile!=5,"","");lSaltLast=77;if(profile==9)longjmp(jump,1);return profile!=5;}
int16_t FHasCurrentLibraryRace(void){int yes=fNewLibraryRace || szRaceLibraryPath[0]!=0;event('c',yes,"","");return yes;}
static int save(void){saved++;writes++;((uint8_t *)&vplr)[12]++;if(profile==10)longjmp(jump,1);return profile!=6;}
int16_t FSaveCurrentLibraryRace(HWND parent,PLAYER *p){assert(parent==(HWND)7 && p==&vplr);event('l',0,"","");return save();}
int16_t FSaveRace(char *name,PLAYER *p){assert(p==&vplr);event('s',name==szRaceFile,name,NativeSuggestion(name));return save();}
char *PszGetCompressedString(StringId id){assert(id==idsAdvantagePointsCurrentlyHoleDPointsCannot);return "points %d";}
static int wsprintf(char *out,const char *fmt,...){assert(out==szWork);va_list a;va_start(a,fmt);int n=vsnprintf(out,sizeof(szWork),fmt,a);va_end(a);return n;}
int16_t AlertSz(char *text,int16_t style){assert(style==MB_ICONHAND);event('a',atoi(text+7),"","");return 0;}
'''
HARNESS=r'''
int main(void){
    _Static_assert(CHAR_MIN<0,"native password bytes are signed");
    while(scanf("%d %d %d %d %d %d %d %d %d",&profile,&targetPage,&points,&initialRO,&noWrite,&sequence,&mode,&passKind,&pricePanel)==9){
        memset(&vplr,0,192);memset(szRacePass,'W',16);strcpy(szRacePass,passKind==0?"":passKind==1?"abc":passKind==2?"\xc3\xa9":"abcdefghijklmno");
        memset(szRaceFile,'C',16);strcpy(szRaceFile,mode==0?"":"old.r7");memset(szRaceFileSuggest,'S',256);strcpy(szRaceFileSuggest,"old.r7.full.r9");
        memset(szRaceLibraryPath,'P',512);strcpy(szRaceLibraryPath,mode==3?"R/Current.r5":"");fNewLibraryRace=mode==2?-7:0;
        lSaltCur=12345;lSaltLast=54321;stepCount=afterTarget=priced=checked=saved=ended=writes=0;first=1;
        printf("{\"events\":[");int result=-2;if(!setjmp(jump))result=RaceCreationWizard((HWND)7,(int16_t)initialRO,(int16_t)noWrite);
        printf("],\"result\":%d,\"panel\":%d,\"readonly\":%d,\"salt\":%d,\"last\":%d,\"writes\":%d,\"player\":",result,iPanelActive,fRCWReadOnly,lSaltCur,lSaltLast,writes);hex(&vplr,192);printf("}\n");
        /* No read or dereference of vrgrcRCW after its stack object's lifetime. */
    }
    (void)FSameLibraryRace;(void)SaveRaceState;(void)RestoreRaceState;return 0;
}
'''


def oracle(v):
    profile,target,points,readonly,no_write,sequence,mode,pw_kind,price_panel=v
    events=[];player=bytearray(192);panel=1;cur,last=12345,54321;writes=0;after=False
    password=(b'',b'abc','é'.encode(),b'abcdefghijklmno')[pw_kind]
    def event(k,n=0,a=b'',b=b''):events.append([k,n,a.hex(),b.hex()])
    def hash_bytes(p):
        n=0
        for i in range(0,len(p),2):
            a=p[i] if p[i]<128 else p[i]-256;n=(n+a)%2**32
            if i+1<len(p):b=p[i+1] if p[i+1]<128 else p[i+1]-256;n=(n*b)%2**32
        if p and n==0:n=1
        return n if n<2**31 else n-2**32
    class Escape(Exception):pass
    def close(result):
        if sequence:
            event('e');player[17]+=1
            if profile==11:raise Escape()
        return result
    result=-2;event('b',sequence)
    try:
        while True:
            dialog=(1,4,5,2,3,6)[panel-1];event('m',dialog);player[0]+=1
            if profile==7:event('d',panel,b'escape');raise Escape()
            code=2
            if profile==1:code=0
            elif profile==2:code=-1
            elif after:code=0
            elif panel==target:after=True;code=points if profile==12 else 3
            event('d',panel);event('f',dialog);code%=65536
            if code in (0,65535):result=close(0);break
            if panel>1 and code==1:panel-=1;continue
            if code!=3 and panel!=6:panel+=1;continue
            if readonly:result=close(0);break
            player[4]+=1;panel=price_panel;value=0 if profile==12 else points;event('p',value)
            if profile==8:raise Escape()
            if value<0:event('a',-value);panel=panel if 1<=panel<=6 else 1;continue
            cur=hash_bytes(password);event('h',cur,password);last=-5;player[8]+=1;event('k',int(profile!=5));last=77
            if profile==9:raise Escape()
            if profile==5:panel=1;continue
            if no_write:result=close(1);break
            has=mode in (2,3);event('c',int(has))
            if has:event('l')
            else:event('s',int(mode!=0),b'stars.r1' if mode==0 else b'old.r7',b'stars.r1' if mode==0 else b'old.r7.full.r9')
            writes+=1;player[12]+=1
            if profile==10:raise Escape()
            if profile==6:panel=1;continue
            result=close(1);break
    except Escape:pass
    return dict(events=events,result=result,panel=panel,readonly=readonly,salt=cur,last=last,writes=writes,player=bytes(player).hex())


def vectors():
    for profile in range(12):
        for page in range(1,7):
            for ro in (0,1,-7):
                for no_write in (0,1):
                    for sequence in (0,1):yield profile,page,-1 if profile==4 else 0,ro,no_write,sequence,3,1,page
    for page in range(1,7):
        for code in (-65537,-65536,-2,-1,0,1,2,3,4,5,65535,65536,65537,65538,65539):yield 12,page,code,0,0,1,0,2,page
    for page in range(1,7):
        for panel in (-1,0,7):yield 4,page,-32768,0,0,1,1,0,panel
    for mode in range(4):
        for password in range(4):yield 0,1,0,0,0,1,mode,password,1


def main():
    extra=PRELUDE.replace('#include "resource.h"','#include "'+str(ROOT/'res/resource.h')+'"')
    extra+='\n'+function('src/core/utilgen.c','int32_t LSaltFromSz(').replace('int32_t LSaltFromSz(','static int32_t NativeHash(',1)
    extra+='\n'+function('src/ui/raceui.c','int16_t RaceCreationWizard(')
    inputs=list(vectors())
    with tempfile.TemporaryDirectory(prefix='race-wizard-') as tmp:
        binary=build(Path(tmp),extra=extra,harness=HARNESS)
        run=subprocess.run([str(binary)],input=('\n'.join(' '.join(map(str,v)) for v in inputs)+'\n').encode(),stdout=subprocess.PIPE,check=True)
        lines=run.stdout.splitlines();assert len(lines)==len(inputs)
        for n,(v,line) in enumerate(zip(inputs,lines)):
            actual=json.loads(line);expected=oracle(v)
            for key,value in expected.items():assert actual[key]==value,(n,v,key,actual[key],value)
    print(f'{len(inputs)} actual wizard/hash cases match page dispatch, normalized return codes, retry/save/sequence calls and mutable state')
    print('Scripted page/pricing/password/save/sequence callbacks; no full page, native UI or post-return pointer-read qualification claim.')


if __name__=='__main__':main()
