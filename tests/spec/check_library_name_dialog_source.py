#!/usr/bin/env python3
"""Actual shared LibraryNameDlg and name helpers, with scripted window services."""
import json
from pathlib import Path
import subprocess
import tempfile
from check_s02_source import ROOT,function
from check_library_publication_source import extract
from check_race_workflows_source import build,prefix

PRELUDE=r'''
#include <stdarg.h>
#include <strings.h>
#include <ctype.h>
#include "resource.h"
#define SDL_strcasecmp strcasecmp
#define SDL_strncasecmp strncasecmp
#define SDL_tolower tolower
#define CALLBACK
#define WM_INITDIALOG 0x110
#define WM_COMMAND 0x111
#define SPI_GETWORKAREA 48
#define SM_CXSCREEN 0
#define SM_CYSCREEN 1
#define SWP_NOSIZE 1
#define SWP_NOZORDER 4
#define SWP_NOACTIVATE 16
#define DWLP_USER 8
#define EM_LIMITTEXT 0xc5
typedef uint32_t UINT;typedef uintptr_t WPARAM;typedef intptr_t LPARAM;typedef intptr_t INT_PTR;
char szBase[256];static char szLibraryRoot[256],szRaceLibraryRoot[256];static int16_t fRaceLibraryNameDialog;
static unsigned char output[256];static char edit[1024],scratch[256];static intptr_t user;
static int first,queries,freeAt,workOkay,visible,ownerOkay,dialogOkay,profile,openDialog,endValue;
static int movedX,movedY;
static void hex(const void *data,size_t n){const unsigned char *p=data;putchar('"');for(size_t i=0;i<n;i++)printf("%02x",p[i]);putchar('"');}
static void event(char k,const char *text,int value){if(!first)putchar(',');first=0;printf("[\"%c\",",k);hex(text,strlen(text));printf(",%d]",value);}
int CchSnprintf(char *out,size_t cap,const char *fmt,...){va_list a;va_start(a,fmt);int n=vsnprintf(out,cap,fmt,a);va_end(a);return n;}
static int16_t FLibraryNameAvailable(const char *root,const char *name){assert(!strcmp(root,szLibraryRoot));int yes=++queries==freeAt;event('g',name,yes);return yes;}
static int16_t FRaceLibraryNameAvailable(const char *name){int yes=++queries==freeAt;event('r',name,yes);return yes;}
char *PszGetCompressedString(StringId id){const char *p=id==idsLibraryInvalidName?"INVALID":id==idsLibraryNameTaken?"TAKEN":id==idsLibraryGameRevisionSuffix?"Revision %d":"GAMES";strcpy(scratch,p);event('s',p,0);return scratch;}
static int MessageBox(HWND hwnd,const char *body,const char *title,unsigned style){assert(hwnd==(HWND)1 && (!strcmp(body,"INVALID") || !strcmp(body,"TAKEN")) && !strcmp(title,"GAMES") && style==(MB_OK|MB_ICONEXCLAMATION));event('m',body,0);return 1;}
static int SystemParametersInfo(unsigned action,unsigned zero,RECT *out,unsigned flags){assert(action==SPI_GETWORKAREA && !zero && !flags);event('w',"",0);if(workOkay){*out=(RECT){-100,-50,540,430};}return workOkay;}
static int GetSystemMetrics(int id){event(id==0?'x':'y',"",0);return id==0?640:480;}
static int SetRect(RECT *r,int l,int t,int right,int bottom){*r=(RECT){l,t,right,bottom};return 1;}
static HWND GetParent(HWND hwnd){assert(hwnd==(HWND)1);event('p',"",0);return (HWND)2;}
static int IsWindowVisible(HWND hwnd){assert(hwnd==(HWND)2);event('i',"",0);return visible;}
static int GetWindowRect(HWND hwnd,RECT *out){
    if(hwnd==(HWND)2){event('o',"",0);*out=(RECT){50,60,450,360};return ownerOkay;}
    assert(hwnd==(HWND)1);event('d',"",0);*out=profile==1?(RECT){0,0,1000,700}:profile==2?(RECT){10,10,413,313}:profile==3?(RECT){0,0,401,301}:(RECT){0,0,320,200};return dialogOkay;
}
static int SetWindowPos(HWND hwnd,HWND after,int x,int y,int cx,int cy,unsigned flags){assert(hwnd==(HWND)1 && !after && !cx && !cy && flags==(SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE));movedX=x;movedY=y;event('v',"",0);return 1;}
static intptr_t SetWindowLongPtr(HWND hwnd,int id,intptr_t value){assert(hwnd==(HWND)1 && id==DWLP_USER);user=value;event('b',"",0);return 0;}
static intptr_t GetWindowLongPtr(HWND hwnd,int id){assert(hwnd==(HWND)1 && id==DWLP_USER);event('u',"",0);return user;}
static intptr_t SendDlgItemMessage(HWND hwnd,int id,unsigned msg,uintptr_t n,intptr_t zero){assert(hwnd==(HWND)1 && id==IDC_EDIT1 && msg==EM_LIMITTEXT && n==255 && !zero);event('l',"",255);return 0;}
static int SetDlgItemText(HWND hwnd,int id,const char *name){assert(hwnd==(HWND)1 && id==IDC_EDIT1);strcpy(edit,name);event('t',name,0);return 1;}
static unsigned GetDlgItemText(HWND hwnd,int id,char *out,int cap){assert(hwnd==(HWND)1 && id==IDC_EDIT1 && cap==256);snprintf(out,cap,"%s",edit);event('e',out,0);return (unsigned)strlen(out);}
static int EndDialog(HWND hwnd,intptr_t value){assert(hwnd==(HWND)1);openDialog=0;endValue=(int)value;event('q',"",(int)value);return 1;}
'''

HARNESS=r'''
static const char *text(int kind){static char longName[256];if(kind>=9){int n=kind==9?113:112;memset(longName,'A',n);strcpy(longName+n,kind==9?"\xc3\xa9Z":"\xf0\x9f\x8c\x9f");return longName;}
    return kind==0?"New.r5":kind==1?"bad/name":kind==2?"CON":kind==3?"":kind==4?"Game":kind==5?"A":kind==6?"A.B.r1":kind==7?"\xc3\xa9.r1":"Leading.r999";}
int main(void){int command,notify,mode,kind,root,second;
    while(scanf("%d %d %d %d %d %d %d %d %d %d %d %d",&command,&notify,&mode,&kind,&root,&freeAt,&profile,&workOkay,&visible,&ownerOkay,&dialogOkay,&second)==12){
        assert(root>=0 && root<256);memset(szLibraryRoot,'R',root);szLibraryRoot[root]=0;strcpy(szRaceLibraryRoot,szLibraryRoot);
        memset(output,90,256);strcpy((char *)output,"Original.r7");edit[0]=0;first=1;queries=0;openDialog=1;endValue=-9;fRaceLibraryNameDialog=(int16_t)mode;
        printf("{\"events\":[");assert(LibraryNameDlg((HWND)1,0x999,0,0)==0);assert(LibraryNameDlg((HWND)1,WM_INITDIALOG,0,(LPARAM)output)==1);
        strcpy(edit,text(kind));int a=(int)LibraryNameDlg((HWND)1,WM_COMMAND,(WPARAM)((unsigned)command+((unsigned)notify<<16)),0),b=-9;
        if(second && openDialog)b=(int)LibraryNameDlg((HWND)1,WM_COMMAND,IDOK,0);
        printf("],\"returns\":[%d,%d],\"point\":[%d,%d],\"open\":%d,\"end\":%d,\"output\":",a,b,movedX,movedY,openDialog,endValue);hex(output,256);printf(",\"edit\":");hex(edit,strlen(edit));printf("}\n");
    }
    (void)FSameLibraryRace;(void)SaveRaceState;(void)RestoreRaceState;(void)NativeSuggestion;return 0;
}
'''


def portable(name,race,root):
    if not name or len(name)>=256 or name[:1] in (b' ',b'.') or name[-1:] in (b' ',b'.'):return False
    if any(c<32 or c==127 or c in b'<>:"/\\|?*' for c in name):return False
    stem=name.split(b'.',1)[0].lower()
    if stem in (b'con',b'prn',b'aux',b'nul',b'clock$') or len(stem)==4 and stem[:3] in (b'com',b'lpt') and stem[3:4] in b'123456789':return False
    if not race:return root+2*len(name)+18<256
    base,dot,suffix=name.rpartition(b'.');digits=suffix[1:]
    return bool(base and dot and suffix[:1].lower()==b'r' and digits and len(digits)<=3 and digits.isdigit() and digits[:1]!=b'0' and len(name)<240 and root+len(name)+2<256)


def oracle(v):
    command,notify,mode,kind,root,free_at,profile,work_ok,visible,owner_ok,dialog_ok,second=v
    events=[];out=b'Original.r7\0'+b'Z'*(256-12);edit=b'Original.r7';opened=1;ended=-9;queries=0
    def event(k,s=b'',n=0):events.append([k,s.hex(),n])
    event('w')
    if work_ok:work=(-100,-50,540,430)
    else:work=(0,0,640,480);event('x');event('y')
    event('p');event('i')
    if visible:event('p');event('o')
    owner=(50,60,450,360) if visible and owner_ok else work
    event('d');rect=(0,0,1000,700) if profile==1 else (10,10,413,313) if profile==2 else (0,0,401,301) if profile==3 else (0,0,320,200)
    width,height=rect[2]-rect[0],rect[3]-rect[1]
    div=lambda n: n//2 if n>=0 else -((-n)//2)
    x=max(work[0],min(owner[0]+div(owner[2]-owner[0]-width),work[2]-width));y=max(work[1],min(owner[1]+div(owner[3]-owner[1]-height),work[3]-height))
    event('v');event('b');event('l',n=255);event('t',edit)
    edit=(b'New.r5',b'bad/name',b'CON',b'',b'Game',b'A',b'A.B.r1','é.r1'.encode(),b'Leading.r999',b'A'*113+'éZ'.encode(),b'A'*112+'🌟'.encode())[kind]
    def available(name):
        nonlocal queries
        queries+=1;yes=queries==free_at;event('r' if mode else 'g',name,int(yes));return yes
    def cmd(command):
        nonlocal out,edit,opened,ended
        if command==2:opened=0;ended=0;event('q',n=0);return 1
        if command!=1:return 0
        event('u');name=edit[:255];event('e',name)
        if not portable(name,mode!=0,root):error=b'INVALID'
        elif available(name):out=name+b'\0'+out[len(name)+1:];opened=0;ended=1;event('q',n=1);return 1
        else:
            if mode:
                stem=prefix(name.rsplit(b'.',1)[0] if b'.' in name else name,239)
                for n in range(1,1000):
                    candidate=stem+b'.r'+str(n).encode()
                    if available(candidate):break
                else:candidate=b''
                edit=candidate
            else:
                original=name;budget=div(256-root-19);candidate=name
                for n in range(2,10000):
                    if available(candidate):break
                    suffix=b' ('+str(n).encode()+b')';cut=min(len(original),max(0,budget-len(suffix)))
                    while cut>0 and cut<len(original) and original[cut]&0xc0==0x80:cut-=1
                    candidate=(original[:cut]+suffix)[:255]
                edit=candidate
            event('t',edit);error=b'TAKEN'
        event('s',error);event('s',b'GAMES');event('m',error);return 1
    a=cmd(command);b=cmd(1) if second and opened else -9
    return dict(events=events,returns=[a,b],point=[x,y],open=opened,end=ended,output=out.hex(),edit=edit.hex())


def vectors():
    for mode in (0,1,-7):
        for kind in range(11):
            for free in (1,2,4,20000):
                for second in (0,1):yield 1,65535,mode,kind,1,free,0,1,1,1,1,second
        for command in (2,9):yield command,1,mode,1,1,20000,0,1,1,1,1,1
    for profile in range(4):
        for work in (0,1):
            for visible in (0,1):
                for owner in (0,1):
                    for dialog in (0,1):yield 9,0,1,0,1,1,profile,work,visible,owner,dialog,0
    for root in (0,14,235,255):
        for mode in (0,1):yield 1,0,mode,5,root,20000,0,1,1,1,1,1


def main():
    extra=PRELUDE+'\n'+'\n'.join([extract('FPortableRaceFilename'),extract('FPortableLibraryName'),function('src/ui/libraryui.c','static void SuggestLibraryName('),
        function('src/ui/libraryui.c','static void SuggestRaceFilename('),function('src/ui/libraryui.c','INT_PTR CALLBACK LibraryNameDlg(')])
    # The shared factory includes core/native headers; only Win32 UI signatures
    # and services are stubbed. This is not a native window ABI test.
    extra=extra.replace('#include "resource.h"','#include "'+str(ROOT/'res/resource.h')+'"')
    inputs=list(vectors())
    with tempfile.TemporaryDirectory(prefix='name-dialog-') as tmp:
        binary=build(Path(tmp),extra=extra,harness=HARNESS)
        run=subprocess.run([str(binary)],input=('\n'.join(' '.join(map(str,v)) for v in inputs)+'\n').encode(),stdout=subprocess.PIPE,check=True)
        outputs=run.stdout.splitlines();assert len(outputs)==len(inputs)
        for n,(v,line) in enumerate(zip(inputs,outputs)):
            actual=json.loads(line);expected=oracle(v)
            # C argument evaluation may query width/height in either order.
            if not v[7] and actual['events'][1:3]==[expected['events'][2],expected['events'][1]]:expected['events'][1:3]=actual['events'][1:3]
            for key,value in expected.items():assert actual[key]==value,(n,v,key,actual[key],value)
    print(f'{len(inputs)} shared name-dialog/source-helper cases match geometry, calls, scratch-message copies, output tails and retry/cancel results')
    print('Scripted window/text/filesystem services; no native ABI, display, accessibility or reentrancy qualification claim.')


if __name__=='__main__':main()
