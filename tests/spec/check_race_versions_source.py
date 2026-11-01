#!/usr/bin/env python3
"""Actual version reader/scan/current-save controller with native race state.

Current BOFs use real temporary stdio files. Full parsing/authentication/staged
saving are providers; actual comparison and snapshot bodies are composed. Native
Unicode comparison beyond ASCII keys and parser/OS/GUI qualification remain open.
"""
from pathlib import Path
import os
import shlex
import subprocess
import sys
import tempfile
from check_s02_source import ROOT,function
from check_race_workflows_source import declarations,bodies

PRELUDE=r'''
#include <strings.h>
#include <stdarg.h>
typedef intptr_t LPARAM;
#define SDL_strcasecmp strcasecmp
#define SDL_strncasecmp strncasecmp
#define SDL_tolower tolower
#define MAKEINTRESOURCE(x) (x)
#define IDD_LIBRARY_RACE_SAVE 123
static int hInst;
static void *LibraryRaceSaveDlg;
static char szRaceLibraryRoot[256]="R";
char szBase[256];
int16_t fFileErrSilent,hf;
uint16_t wVersFile;
static int pathInfo=1,pathDirectory,pathLink,canOpen=1,headerValid=1,headerCloseFailed,parserResult=1;
static int parserCalls,opens,choice=2,available=1,saveResult=1,saveCalls,authCalls,authResult=1;
static char savedName[256],readName[256];
static PLAYER decoded;
static int initResult=1,enumerationResult=1;
static int enumVersion,enumSame,enumReadable=1,enumStem,selectedReadable,selectedSame,currentMode;
static unsigned long checks,cases;
#define CHECK(c) do{checks++;if(!(c)){fprintf(stderr,"case %lu assertion %lu: %s\n",cases,checks,#c);abort();}}while(0)
int CchSnprintf(char *out,size_t cap,const char *fmt,...){va_list args;va_start(args,fmt);int n=vsnprintf(out,cap,fmt,args);va_end(args);return n;}
static int16_t FInitRaceLibrary(void){return initResult;}
static int16_t FRaceLibraryNameAvailable(const char *name){(void)name;return available;}
static int IsWindow(HWND hwnd){(void)hwnd;return 0;}
char *PszFormatIds(StringId id,int16_t *p){(void)id;(void)p;return "message";}
int16_t AlertSz(char *s,int16_t f){(void)s;(void)f;return 0;}
char *PszGetCompressedString(StringId id){(void)id;return "Default";}
static int DialogBoxParam(int inst,int resource,HWND hwnd,void *dialog,intptr_t data){
    (void)inst;(void)resource;(void)hwnd;(void)dialog;(void)data;return choice;
}
int16_t FNativePathInfo(const char *path,int16_t *directory,int16_t *link){
    const char *name=strrchr(path,'/');snprintf(readName,sizeof(readName),"%s",name?name+1:path);
    if(currentMode && !strcmp(readName,"Selected.r1")){decoded=vplr;decoded.lSalt=selectedSame?vplr.lSalt:vplr.lSalt+1;parserResult=selectedReadable;}
    *directory=pathDirectory;*link=pathLink;return pathInfo;
}
FILE *PfOpenNativeFile(const char *path,const char *mode){
    (void)path;assert(!strcmp(mode,"rb"));opens++;if(!canOpen)return NULL;
    FILE *f=tmpfile();assert(f);HDR h={0};h.rt=rtBOF;h.cb=sizeof(RTBOF);
    RTBOF b={0};memcpy(b.rgid,"J3J3",4);b.verMajor=2;b.verMinor=headerValid?85:84;b.dt=dtRace;
    assert(fwrite(&h,2,1,f)==1 && fwrite(&b,sizeof(b),1,f)==1);rewind(f);return f;
}
static int BofClose(FILE *f){int status=fclose(f);return headerCloseFailed?EOF:status;}
int16_t FWasRaceFile(char *path,int16_t ask){
    (void)path;CHECK(!ask && fFileErrSilent==TRUE);parserCalls++;vplr=decoded;
    memset(szRacePass,'X',16);SetSzRaceFile("Decoded-long-design-name.r1");memset(szRaceLibraryPath,'Y',512);fNewLibraryRace=9;
    wVersFile=249;hf=-1;return parserResult;
}
static int16_t FLoadLibraryRace(HWND hwnd,const char *name){
    (void)hwnd;(void)name;authCalls++;memset(&vplr,99,sizeof(vplr));SetSzRaceFile("Authenticated-target-name.r1");
    memset(szRaceLibraryPath,'Q',512);fNewLibraryRace=0;return authResult;
}
int16_t FSaveLibraryRace(HWND hwnd,const char *name,PLAYER *player){
    (void)hwnd;(void)player;saveCalls++;strcpy(savedName,name);return saveResult;
}
'''

HARNESS=r'''
static void reset(void){
    pathInfo=canOpen=headerValid=parserResult=initResult=enumerationResult=available=saveResult=authResult=1;
    pathDirectory=pathLink=headerCloseFailed=parserCalls=opens=saveCalls=authCalls=currentMode=0;choice=2;
    memset(&vplr,0,sizeof(vplr));strcpy(vplr.szName,"Race");strcpy(vplr.szNames,"Races");decoded=vplr;
    memset(szRacePass,'P',16);memset(szRaceFile,'F',16);memset(szRaceLibraryPath,0,512);memset(szRaceFileSuggest,'S',256);
    fNewLibraryRace=1;fFileErrSilent=7;wVersFile=123;hf=10;savedName[0]=readName[0]=0;
}
static void frame(RACESTATE *s){
    CHECK(!memcmp(&vplr,&s->plr,192));CHECK(!memcmp(szRacePass,s->szPass,16));CHECK(!memcmp(szRaceFile,s->szFile,16));
    CHECK(!memcmp(szRaceLibraryPath,s->szPath,512));CHECK(fNewLibraryRace==s->fNew);
}
static void readerTests(void){
    const int returns[]={-32768,-1,0,1,2,32767};
    for(int bits=0;bits<64;bits++)for(unsigned r=0;r<6;r++){
        reset();pathInfo=bits&1;pathDirectory=(bits>>1)&1;pathLink=(bits>>2)&1;canOpen=(bits>>3)&1;
        headerValid=(bits>>4)&1;headerCloseFailed=(bits>>5)&1;parserResult=returns[r];
        decoded.lSalt=123;strcpy(decoded.szName,"Decoded");RACESTATE saved;SaveRaceState(&saved);
        PLAYER out;memset(&out,0x88,sizeof(out));PLAYER prior=out;
        int called=pathInfo && !pathDirectory && !pathLink && canOpen && headerValid;
        CHECK(FReadManagedRace("R/Race.r1",&out)==(called && parserResult>0));
        CHECK(!memcmp(&out,called && parserResult>0?&decoded:&prior,192));frame(&saved);
        CHECK(fFileErrSilent==7 && wVersFile==123);CHECK(parserCalls==called);CHECK(hf==(called?-1:10));
        CHECK(opens==(pathInfo && !pathDirectory && !pathLink));
        if(called)CHECK(!strcmp(szRaceFileSuggest,"Decoded-long-design-name.r1"));else CHECK(szRaceFileSuggest[0]=='S');
        cases++;
    }
}
static void scanTests(void){
    const char *files[]={"Race.r1","Race.r2","Imported.r8","Other.r999","Case.r8","Bad.r01","Other.r3"};
    for(int first=0;first<7;first++)for(int second=0;second<7;second++)for(int same=0;same<4;same++)for(int unread=0;unread<4;unread++){
        reset();PLAYER working=vplr;RACEVERSIONS versions={0};strcpy(versions.szStem,"Race");versions.pplr=&working;
        int maximum=0,latest=0,identical=0;const char *latestName="",*sameName="";
        const int indices[]={first,second};
        for(int at=0;at<2;at++){
            const char *file=files[indices[at]];decoded=working;decoded.lSalt=(same&(1<<at))?0:1;
            if(indices[at]==3 || indices[at]==6)strcpy(decoded.szName,"Other");
            if(indices[at]==4)strcpy(decoded.szName,"rAcE");
            parserResult=(unread&(1<<at))?0:1;RACESTATE before;SaveRaceState(&before);
            CHECK(FindRaceVersion(&versions,file));frame(&before);
            if(indices[at]==5)continue;
            int version=atoi(strrchr(file,'.')+2),stem=indices[at]==0 || indices[at]==1;
            if(stem && version>maximum)maximum=version;
            if(!parserResult || indices[at]==3 || indices[at]==6)continue;
            if(version>maximum)maximum=version;
            if(version>latest || (version==latest && strcasecmp(file,latestName)<0)){latest=version;latestName=file;}
            if(indices[at]!=4 && (same&(1<<at)) && version>identical){identical=version;sameName=file;}
        }
        CHECK(versions.iMax==maximum && versions.iLatest==latest && versions.iSame==identical);
        CHECK(!strcmp(versions.szLatest,latestName) && !strcmp(versions.szSame,sameName));cases++;
    }
}
int16_t FNativeEnumerateDirectory(const char *dir,FNativeDirectoryEntryProc callback,void *data){
    assert(!strcmp(dir,"R"));
    if(enumVersion){char file[256];snprintf(file,sizeof(file),"%s.r%d",enumStem?"Race":"Imported",enumVersion);decoded=vplr;
        decoded.lSalt=enumSame?vplr.lSalt:vplr.lSalt+1;parserResult=enumReadable;callback(data,file);}
    return enumerationResult;
}
static void currentTests(void){
    for(int maximum=0;maximum<=999;maximum+=999)for(int same=0;same<2;same++)for(int fresh=0;fresh<2;fresh++)
    for(int stem=0;stem<2;stem++)for(int chosen=0;chosen<4;chosen++)for(int can=0;can<2;can++)
    for(int auth=0;auth<2;auth++)for(int eRead=0;eRead<2;eRead++)for(int sRead=0;sRead<2;sRead++)for(int sSame=0;sSame<2;sSame++)for(int alias=0;alias<2;alias++){
        reset();enumVersion=maximum;enumSame=same;enumReadable=eRead;enumStem=stem;
        selectedReadable=sRead;selectedSame=sSame;currentMode=1;
        fNewLibraryRace=fresh;choice=chosen;available=can;authResult=auth;
        if(!fresh)strcpy(szRaceLibraryPath,"R/Selected.r1");
        PLAYER working=vplr;working.lSalt=4;vplr=working;decoded=working;
        int result=FSaveCurrentLibraryRace(0,alias?&vplr:&working);
        int maxSeen=maximum && (stem || eRead)?maximum:0;
        int latest=maximum && eRead,identical=latest && same;
        int existing=!fresh || latest,read=fresh?eRead:sRead,targetSame=fresh?same:sSame;
        int unreadable=existing && !read;
        int expectedReuse=!unreadable && ((existing && targetSame) || identical);
        int newVersion=!existing || chosen==2;
        int attempted=!unreadable && !expectedReuse && (!existing || chosen==1 || chosen==2);
        if(attempted && newVersion && (maxSeen==999 || !can))attempted=0;
        int authenticate=attempted && !newVersion && fresh;
        int expectedSave=attempted && (!authenticate || auth);
        CHECK(result==(expectedReuse || expectedSave));CHECK(saveCalls==expectedSave);CHECK(authCalls==authenticate);
        CHECK(!memcmp(&working,&vplr,192));CHECK(fFileErrSilent==7 && wVersFile==123);
        if(expectedSave){
            if(newVersion)CHECK(!strcmp(savedName,"Race.r1"));
            else CHECK(!strcmp(savedName,fresh?(stem?"Race.r999":"Imported.r999"):"Selected.r1"));
        }
        if(expectedReuse){
            const char *name=existing && targetSame && !fresh?"Selected.r1":stem?"Race.r999":"Imported.r999";
            char path[512];snprintf(path,sizeof(path),"R/%s",name);CHECK(!strcmp(szRaceLibraryPath,path));CHECK(!fNewLibraryRace);
        }
        cases++;
    }
    for(int early=0;early<3;early++){
        reset();enumVersion=999;enumSame=enumReadable=1;enumStem=0;
        if(early==0)fNewLibraryRace=0;else if(early==1)initResult=0;else enumerationResult=0;
        PLAYER working=vplr;CHECK(!FSaveCurrentLibraryRace(0,&working));CHECK(!saveCalls && !authCalls);
        if(early==2)CHECK(!strcmp(szRaceFileSuggest,"Decoded-long-design-name.r1"));else CHECK(!parserCalls);
        cases++;
    }
}
int main(void){readerTests();scanTests();currentTests();printf("native version reader/scan/current-save: %lu cases, %lu assertions\n",cases,checks);return 0;}
'''


def main():
    source=(ROOT/'src/ui/libraryui.c').read_text();begin=source.index('typedef struct {\n    char szStem[240];');end=source.index('} RACEVERSIONS;',begin)+len('} RACEVERSIONS;')
    extra='\n'.join([
        function('src/ui/libraryui.c','static int16_t FPortableRaceFilename('),
        function('src/ui/libraryui.c','static void GetRaceLibraryStem('),
        '#define fclose BofClose\n'+function('src/ui/libraryui.c','static int16_t FReadManagedRace(')+'\n#undef fclose',
        function('src/ui/libraryui.c','static int16_t FindRaceVersion('),
        function('src/ui/libraryui.c','int16_t FHasCurrentLibraryRace('),
        function('src/ui/libraryui.c','const char *PszCurrentLibraryRaceName('),
        function('src/ui/libraryui.c','int16_t FSaveCurrentLibraryRace('),
    ])
    with tempfile.TemporaryDirectory(prefix='race-versions-') as tmp:
        path=Path(tmp)
        for generator in ('text/textgen.py','data/datagen.py'):
            subprocess.run([sys.executable,str(ROOT/generator),str(path)],check=True,stdout=subprocess.DEVNULL)
        # Native enumeration provider is referenced by source before its harness definition.
        c=path/'version.c';c.write_text(declarations()+'\n'+PRELUDE+'\n'+source[begin:end]+'\n'+bodies()+'\n'+extra+'\n'+HARNESS);binary=path/'version'
        subprocess.run(shlex.split(os.environ.get('CC','cc'))+['-std=gnu11','-Wall','-Wextra','-Werror','-Wno-pointer-sign','-fsanitize=address,undefined','-fno-sanitize-recover=all',
            '-I'+str(ROOT/'src/core'),'-I'+str(ROOT/'src/platform'),'-I'+str(path),str(c),'-lm','-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True)
        print('Stable current-BOF admission, parser-result/restore/close handling, version ties/occupancy and save selection checked under explicit providers; OS, full parser and GUI remain open.')


if __name__=='__main__':main()
