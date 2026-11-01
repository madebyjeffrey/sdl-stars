#!/usr/bin/env python3
"""Extracted import/name-choice/save/load/edit/wrappers with scripted services.

Native PLAYER, snapshot, prefix, allocation, writer/checksum/close bodies run
under sanitizers. Parser, dialog, BOF/create and file services are providers;
there is no whole decoder, UI, OS or crash qualification claim.
"""
import json
import os
from pathlib import Path
import random
import shlex
import subprocess
import sys
import tempfile
from check_s02_source import ROOT,function
from check_race_workflows_source import declarations,bodies,prefix
from check_library_publication_source import extract
from check_race_save_source import player_record,checksum,record


PRELUDE=r'''
#include <stdarg.h>
#include <strings.h>
#include <ctype.h>
#include <errno.h>
#define SDL_strcasecmp strcasecmp
#define SDL_strncasecmp strncasecmp
#define SDL_tolower tolower
#define OFN_HIDEREADONLY 4
#define OFN_PATHMUSTEXIST 0x800
#define OFN_FILEMUSTEXIST 0x1000
#define IDD_LIBRARY_NAME 123
#define MAKEINTRESOURCE(n) ((void *)(uintptr_t)(n))
typedef intptr_t LPARAM;
typedef struct {size_t lStructSize;HWND hwndOwner;char *lpstrFilter,*lpstrFile;unsigned nMaxFile,nFilterIndex,Flags;} OPENFILENAME;
static void *hInst;
static int16_t LibraryNameDlg(void){return 0;}
char szBase[256];uint16_t wVersFile;int16_t hf=-1;
_Static_assert(sizeof(HDR)==2 && sizeof(VERS)==2,"native header/version projection");
static char szRaceLibraryRoot[256]="R",szRaceLibraryName[256],szRaceLibraryTemp[512],sourcePath[1024],activeStage[512];
static const char *pszRaceLibraryNameOverride;
static int16_t fRaceLibraryNameDialog;
static uint32_t idRaceStage;
static unsigned flags,startCounter;
static int minor,sourceKind,initialNew,mode,firstFree,overrideKind,firstEvent,parseCount,insideEdit,insideSave,physicalOpen,suggestQuery;
static char directory[512];static int dirPresent;
static jmp_buf escape;
static void SaveRaceState(RACESTATE *);
static void RestoreRaceState(const RACESTATE *,int16_t,int16_t);
static int16_t NativeSave(HWND,const char *,PLAYER *);
static int16_t NativeEdit(HWND,const char *);
static void NativeWritePlayer(PLAYER *,uint8_t *);
typedef struct {char path[1024];unsigned char bytes[1024];size_t count;int present;} FILEENTRY;
static FILEENTRY files[16];static int fileCount;
static int bit(unsigned n){return !!(flags&(1u<<n));}
static void hex(const void *data,size_t n){const unsigned char *p=data;putchar('"');for(size_t i=0;i<n;i++)printf("%02x",p[i]);putchar('"');}
static void event(char k,const char *a,const char *b,int n){if(!firstEvent)putchar(',');firstEvent=0;printf("[\"%c\",",k);hex(a,strlen(a));putchar(',');hex(b,strlen(b));printf(",%d]",n);}
static FILEENTRY *entry(const char *path){for(int i=0;i<fileCount;i++)if(!strcmp(files[i].path,path))return &files[i];assert(fileCount<16);FILEENTRY *p=&files[fileCount++];memset(p,0,sizeof(*p));assert(strlen(path)<1024);strcpy(p->path,path);return p;}
static void put(const char *path,const unsigned char *data,size_t n){FILEENTRY *p=entry(path);assert(n<=1024);memcpy(p->bytes,data,n);p->count=n;p->present=1;}
static int16_t FInitRaceLibrary(void){event('i',"","",insideSave);return !bit(0) && !(insideSave && bit(23));}
int CchSnprintf(char *out,size_t cap,const char *fmt,...){va_list a;va_start(a,fmt);int n=vsnprintf(out,cap,fmt,a);va_end(a);
    assert(n>=0);suggestQuery=!strcmp(fmt,"%s.r%d");
    if(!strcmp(fmt,"%s%s%s")){event('f',out,"",0);if(bit(14) && !insideSave)return -1;}return n;}
static int16_t FRaceLibraryNameAvailable(const char *name){const char *p=strrchr(name,'.');int okay=suggestQuery?(p && atoi(p+2)==firstFree):!bit(12);event('n',name,"",okay);return okay;}
int16_t FWasRaceFile(char *path,int16_t password){assert(password);int n=++parseCount;event('p',path,"",n);
    memset(&vplr,64+n,192);vplr.szName[0]=vplr.szNames[0]=0;if(bit(25))vplr.fDead=FALSE;
    memset(szRacePass,80+n,16);const char *p=strrchr(path,chDirSep);SetSzRaceFile((char *)(p?p+1:path));
    VERS version={0};version.verMajor=2;version.verMinor=n==1?minor:85;memcpy(&wVersFile,&version,2);
    hf=-1;return insideEdit?(bit(22)?-1:1):n==1?(bit(1)?-1:1):(bit(2)?0:1);}
static int IsWindow(HWND hwnd){(void)hwnd;event('w',"","",0);return !bit(19);}
char *PszGetCompressedString(StringId id){(void)id;return "failure";}
char *PszFormatIds(StringId id,int16_t *args){(void)id;(void)args;return "failure";}
int16_t AlertSz(char *text,int16_t style){assert(!strcmp(text,"failure"));event('a',"","",style);return 0;}
static int MessageBox(HWND hwnd,const char *text,const char *title,unsigned style){(void)hwnd;assert(!strcmp(text,"failure") && !strcmp(title,"failure") && style==(MB_OK|MB_ICONHAND));event('m',"","",style);return 1;}
static void GetLibraryFileFilter(StringId id,char *filter){assert(id==idsLibraryRaceFilesFilter);strcpy(filter,"race");event('g',"","",0);}
static int FNativeOpenFileName(OPENFILENAME *ofn){assert(ofn->lStructSize==sizeof(*ofn) && !ofn->hwndOwner && ofn->nMaxFile==512 && ofn->nFilterIndex==1);
    assert(ofn->Flags==(OFN_HIDEREADONLY|OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST) && ofn->lpstrFile[0]==0);event('k',"","",0);
    if(bit(9))return 0;assert(strlen(sourcePath)<512);strcpy(ofn->lpstrFile,sourcePath);return 1;}
static intptr_t DialogBoxParam(void *instance,void *id,HWND hwnd,int16_t (*proc)(void),LPARAM param){
    assert(instance==hInst && id==MAKEINTRESOURCE(IDD_LIBRARY_NAME) && !hwnd && proc==LibraryNameDlg && (char *)param==szRaceLibraryName && fRaceLibraryNameDialog==TRUE);
    event('d',szRaceLibraryName,"",0);if(bit(24))return -1;if(bit(13))return 0;strcpy(szRaceLibraryName,"Chosen.r5");return 1;}
uint32_t DwTickCount(void){event('t',"","",0);return 7;}
int CreateNativeDir(const char *path){event('j',path,"",!bit(3));if(bit(3))return -1;assert(!dirPresent);strcpy(directory,path);dirPresent=1;return 0;}
int16_t FNativeDirectoryExists(const char *path){event('z',path,"",0);return 0;}
int RemoveNativeFile(const char *path){FILEENTRY *p=entry(path);int okay=p->present && !bit(6);if(okay)p->present=0;event('r',path,"",okay);return okay?0:-1;}
int RemoveNativePath(const char *path){int okay=dirPresent && !strcmp(directory,path) && !bit(7);size_t n=strlen(path);
    for(int i=0;i<fileCount;i++)if(files[i].present && !strncmp(files[i].path,path,n) && files[i].path[n]=='/')okay=0;
    if(okay)dirPresent=0;event('b',path,"",okay);return okay?0:-1;}
int ReplaceNativeFile(const char *source,const char *target){FILEENTRY *p=entry(source);int okay=p->present && !bit(5);if(okay){unsigned char data[1024];size_t n=p->count;memcpy(data,p->bytes,n);p->present=0;put(target,data,n);}event('x',source,target,okay);return okay?0:-1;}
static int16_t FCopyNativeFile(const char *source,const char *target){FILEENTRY *p=entry(source);int code=p->present?mode:1;unsigned char data[1024];size_t n=strcmp(source,target)?p->count:0;memcpy(data,p->bytes,n);
    if(code==0)put(target,data,n);else if(code>=3){put(target,data,code==4?(n?1:0):n);if(code==3)entry(target)->present=0;}
    event('c',source,target,code);return code==0;}
int16_t FCreateFile(DtFileType type,int16_t player,char *path){assert(type==dtRace && player==iplrNone);event('o',path,"",!bit(15));
    if(bit(15))return 0;const unsigned char prefix[]={1};put(path,prefix,1);strcpy(activeStage,path);hf=5;physicalOpen=1;return 1;}
int16_t FCompressUserString(char *a,char *b,int16_t *count){(void)a;(void)b;(void)count;return 0;}
void WriteRt(RecordType type,int16_t count,void *data){HDR header={0};header.rt=type;header.cb=count;unsigned char bytes[512];memcpy(bytes,&header,2);memcpy(bytes+2,data,count);
    FILEENTRY *p=entry(activeStage);size_t n=(size_t)count+2;int fails=bit(type==rtPlr?16:17);if(fails)n=type==rtPlr?1:3;
    assert(p->present && p->count+n<=1024);memcpy(p->bytes+p->count,bytes,n);p->count+=n;event('v',activeStage,"",type);if(fails)longjmp(escape,1);}
void WriteRtPlr(PLAYER *player,uint8_t *buffer){assert(player==&vplr);NativeWritePlayer(player,buffer);}
void CloseFile(int16_t handle){assert(handle==5);event('h',"","",bit(18));if(!bit(18))physicalOpen=0;}
int16_t FSaveLibraryRace(HWND hwnd,const char *name,PLAYER *player){event('s',name,"",0);insideSave=1;int16_t result=NativeSave(hwnd,name,player);insideSave=0;return result;}
int16_t FEditLibraryRace(HWND hwnd,const char *name){event('e',name,"",0);insideEdit=1;int16_t result=NativeEdit(hwnd,name);insideEdit=0;return result;}
int16_t RaceCreationWizard(HWND hwnd,int16_t a,int16_t b){assert(!hwnd && !a && !b);event('q',"","",0);
    memset(&vplr,102,192);memset(szRacePass,103,16);SetSzRaceFile("edited.r9");strcpy(szRaceLibraryPath,"R/edited.r9");fNewLibraryRace=1;
    const unsigned char saved[]={7};put("R/independent.r9",saved,1);
    if(bit(21))longjmp(escape,1);return !bit(20);}
'''

HARNESS=r'''
static const char *overrideName(void){return overrideKind==0?NULL:overrideKind==1?"Import.r8":overrideKind==2?"":"bad";}
static void prepare(void){firstEvent=1;parseCount=fileCount=dirPresent=insideEdit=insideSave=physicalOpen=0;hf=-1;idRaceStage=startCounter;suggestQuery=0;
    memset(&vplr,17,192);memset(szRacePass,34,16);memset(szRaceFile,51,16);strcpy(szRaceFile,"old.r7");
    memset(szRaceLibraryPath,68,512);strcpy(szRaceLibraryPath,"old/location.r7");memset(szRaceFileSuggest,85,256);strcpy(szRaceFileSuggest,"old-suggestion.r7");
    memset(szRaceLibraryName,86,256);strcpy(szRaceLibraryName,"prior-name.r8");fNewLibraryRace=initialNew;fRaceLibraryNameDialog=-3;wVersFile=99;
    strcpy(sourcePath,sourceKind==1?"mixed\\folder/Imported.r4":sourceKind==5?"folder.with.dot/name":sourceKind==6?"folder/":
        sourceKind==7?"external/folder\\Imported.r4":sourceKind==8?"mixed\\folder\\Imported.r4":"external/Imported.r4");
    if(sourceKind==2){strcpy(sourcePath,"external/");memset(sourcePath+9,'A',236);strcpy(sourcePath+245,".r4");}
    if(sourceKind==3){strcpy(sourcePath,"external/");memset(sourcePath+9,'A',254);strcpy(sourcePath+263,"\xc3\xa9.r4");}
    if(sourceKind==4){memset(sourcePath,'A',512);sourcePath[512]=0;}
    const unsigned char a[]={1,2,3,4},b[]={9,8};put(sourcePath,a,4);put("R/Sibling.r2",b,2);pszRaceLibraryNameOverride=overrideName();
}
int main(void){char op;
    while(scanf(" %c %u %d %d %d %d %d %d %u",&op,&flags,&minor,&sourceKind,&initialNew,&mode,&firstFree,&overrideKind,&startCounter)==9){
        prepare();if(op=='A')pszRaceLibraryNameOverride="prior.r8";printf("{\"events\":[");int result=-2;
        if(!setjmp(escape)){const char *path=bit(8)?NULL:sourcePath;
            result=op=='A'?FImportLibraryRaceAs(NULL,path,overrideName()):op=='E'?FImportLibraryRaceAndEdit(NULL,path):FImportLibraryRace(NULL,path);}
        printf("],\"result\":%d,\"version\":%u,\"counter\":%u,\"open\":%d,\"physicalOpen\":%d,\"dialog\":%d,\"override\":",result,wVersFile,idRaceStage,hf!=-1,physicalOpen,fRaceLibraryNameDialog);
        if(pszRaceLibraryNameOverride)hex(pszRaceLibraryNameOverride,strlen(pszRaceLibraryNameOverride));else printf("null");
        printf(",\"native\":[");hex(&vplr,192);putchar(',');hex(szRacePass,16);putchar(',');hex(szRaceFile,16);putchar(',');hex(szRaceLibraryPath,512);printf(",%d,",fNewLibraryRace);hex(szRaceFileSuggest,256);printf("],\"library\":");hex(szRaceLibraryName,256);
        printf(",\"directories\":[");if(dirPresent)hex(directory,strlen(directory));printf("],\"files\":{");int first=1;
        for(int i=0;i<fileCount;i++)if(files[i].present){if(!first)putchar(',');first=0;hex(files[i].path,strlen(files[i].path));putchar(':');hex(files[i].bytes,files[i].count);}printf("}}\n");
    }
    (void)FSameLibraryRace;(void)NativeSuggestion;return 0;
}
'''


def vectors():
    masks=[0]+[1<<bit for bit in range(26)]+[(1<<2)|(1<<6),(1<<5)|(1<<6)|(1<<7),(1<<16)|(1<<10)]
    for op in ('I','A','E'):
        for minor in (49,84,85):
            for flags in masks:
                for override in (0,1):yield op,flags,minor,0,-7,0,1,override,0
        for source in range(9):
            for override in range(4):yield op,0,85,source,1,0,999,override,4294967295
        for mode in range(6):
            for flags in (0,1<<6,1<<7):yield op,flags,85,0,0,mode,2,1,0
    rng=random.Random(20261028)
    for _ in range(2500):
        flags=rng.randrange(1<<24);source=rng.randrange(9)
        if source==4:flags&=~(1<<8)
        yield rng.choice(('I','A','E')),flags,rng.choice((49,84,85)),source,rng.choice((-7,0,1)),rng.randrange(6),rng.choice((1,2,999,1000)),rng.randrange(4),rng.choice((0,4294967295))


def compile_probe(path):
    for generator in ('text/textgen.py','data/datagen.py'):
        subprocess.run([sys.executable,str(ROOT/generator),str(path)],check=True,stdout=subprocess.DEVNULL)
    actual=bodies()+'\n'+function('src/core/save.c','void WriteRtPlr(').replace('void WriteRtPlr(','static void NativeWritePlayer(',1)
    actual+='\n'+function('src/core/file.c','void STARS_TEST_IMPL(StreamClose)(')
    actual+='\n'+extract('FPortableRaceFilename')+'\n'+function('src/ui/libraryui.c','static void SuggestRaceFilename(')
    actual+='\n'+extract('FCreateRaceStagePath')+'\n'+function('src/ui/libraryui.c','static int16_t FChooseLibraryRaceName(')
    actual+='\n'+function('src/ui/libraryui.c','int16_t FSaveLibraryRace(').replace('int16_t FSaveLibraryRace(','static int16_t NativeSave(',1)
    actual+='\n'+function('src/ui/libraryui.c','int16_t FLoadLibraryRace(')
    actual+='\n'+function('src/ui/libraryui.c','int16_t FEditLibraryRace(').replace('int16_t FEditLibraryRace(','static int16_t NativeEdit(',1)
    actual+='\n'+'\n'.join(function('src/ui/libraryui.c','int16_t '+name+'(') for name in ('FImportLibraryRace','FImportLibraryRaceAs','FImportLibraryRaceAndEdit'))
    c=path/'import.c';c.write_text(declarations()+'\n'+PRELUDE+'\n'+actual+'\n'+HARNESS);binary=path/'import'
    subprocess.run(shlex.split(os.environ.get('CC','cc'))+['-std=gnu11','-Wall','-Wextra','-Werror','-Wno-pointer-sign','-fsanitize=address,undefined','-fno-sanitize-recover=all',
        '-I'+str(ROOT/'src/core'),'-I'+str(ROOT/'src/platform'),'-I'+str(path),str(c),'-lm','-o',str(binary)],check=True)
    return binary


def oracle(v):
    op,flags,minor,kind,new_flag,mode,first_free,override_kind,counter=v
    bit=lambda n:bool(flags&(1<<n))
    source=(b'external/'+b'A'*236+b'.r4' if kind==2 else b'external/'+b'A'*254+'é.r4'.encode() if kind==3 else
        b'A'*512 if kind==4 else b'mixed\\folder/Imported.r4' if kind==1 else b'folder.with.dot/name' if kind==5 else b'folder/' if kind==6 else
        b'external/folder\\Imported.r4' if kind==7 else b'mixed\\folder\\Imported.r4' if kind==8 else b'external/Imported.r4')
    overwrite=lambda old,new:new+b'\0'+old[len(new)+1:]
    native=[bytes([17])*192,bytes([34])*16,overwrite(bytes([51])*16,b'old.r7'),overwrite(bytes([68])*512,b'old/location.r7'),new_flag,overwrite(bytes([85])*256,b'old-suggestion.r7')]
    library=overwrite(bytes([86])*256,b'prior-name.r8');dialog=-3;version=99;handle=False;physical=False;directory=None
    override=(None,b'Import.r8',b'',b'bad')[override_kind]
    files={source:bytes([1,2,3,4]),b'R/Sibling.r2':bytes([9,8])};events=[];parses=0
    class Escape(Exception):pass
    def event(k,a=b'',b=b'',n=0):events.append([k,a.hex(),b.hex(),n])
    def set_file(name):
        native[2]=overwrite(native[2],prefix(name,15));native[5]=overwrite(native[5],prefix(name,255))
    def parse(path,editing=False):
        nonlocal parses,version,handle
        parses+=1;event('p',path,n=parses)
        raw=bytearray([64+parses]*192);raw[128]=raw[160]=0
        if bit(25):raw[84]&=~1
        native[0]=bytes(raw);native[1]=bytes([80+parses])*16;set_file(path.rsplit(b'/',1)[-1]);version=(2<<12)+((minor if parses==1 else 85)<<5)
        handle=False;return not bit(22 if editing else 1 if parses==1 else 2)
    def restore(saved,race=True,path=True):
        if race:native[:3]=saved[:3]
        if path:native[3:5]=saved[3:5]
    def init(saving=False):event('i',n=int(saving));return not bit(0) and not (saving and bit(23))
    def fmt(path,saving=False):event('f',path);return not bit(14) or saving
    def available(name,suggest=False):
        okay=(int(name.rsplit(b'.',1)[-1][1:])==first_free) if suggest else not bit(12)
        event('n',name,n=int(okay));return okay
    def choose(hint):
        nonlocal library,dialog
        if not init():return None
        library=overwrite(library,hint[:255]);text=library.split(b'\0')[0]
        stem=prefix(text.rsplit(b'.',1)[0] if b'.' in text else text,239)
        for n in range(1,1000):
            name=stem+b'.r'+str(n).encode();library=overwrite(library,name)
            if available(name,True):break
        else:library=b'\0'+library[1:];return None
        dialog=1;event('d',name)
        if bit(13) or bit(24):dialog=0;return None
        library=overwrite(library,b'Chosen.r5');dialog=0;path=b'R/Chosen.r5'
        if not fmt(path) or not available(b'Chosen.r5'):event('m',n=16);return None
        return path
    def allocate():
        nonlocal counter,directory
        event('t');name=b'R/.stars-race-stage-7-'+str(counter).encode();counter=(counter+1)%2**32
        event('j',name,n=int(not bit(3)))
        if bit(3):event('z',name);return None
        directory=name;return name+b'/design.r1'
    def remove(path):
        okay=path in files and not bit(6)
        if okay:del files[path]
        event('r',path,n=int(okay));return okay
    def remove_directory(path):
        nonlocal directory
        okay=directory==path and not bit(7) and not any(p.startswith(path+b'/') for p in files)
        if okay:directory=None
        event('b',path,n=int(okay));return okay
    def replace(stage,target):
        okay=stage in files and not bit(5)
        if okay:files[target]=files.pop(stage)
        event('x',stage,target,int(okay));return okay
    def copy(stage):
        code=mode if source in files else 1;wanted=files.get(source,b'')
        if code==0:files[stage]=wanted
        elif code>=3:
            if code==3:files.pop(stage,None)
            else:files[stage]=wanted[:1] if code==4 else wanted
        event('c',source,stage,code);return code==0
    def save(target):
        nonlocal handle,physical
        name=library.split(b'\0')[0];event('s',name)
        if not init(True):return False
        fmt(target,True);stage=allocate()
        if stage is None:return False
        folder=directory;event('o',stage,n=int(not bit(15)))
        if bit(15):remove(stage);remove_directory(folder);return False
        files[stage]=b'\x01';handle=physical=True
        raw=bytearray(native[0])
        if raw[84]&1:raw[6]=(raw[6]&~7)|7
        native[0]=bytes(raw)
        for tag,wanted,failed in ((6,player_record(native[0]),bit(16)),(0,record(0,checksum(native[0]).to_bytes(2,'little')),bit(17))):
            files[stage]+=wanted[:1 if tag==6 else 3] if failed else wanted
            event('v',stage,n=tag)
            if failed:raise Escape()
        event('h',n=int(bit(18)));handle=False
        if not bit(18):physical=False
        if not replace(stage,target):
            remove(stage);remove_directory(folder);event('w')
            if not bit(19):event('a',n=16)
            return False
        remove_directory(folder);native[3]=overwrite(native[3],target);set_file(name);native[4]=0;return True
    def imported():
        nonlocal library
        if bit(8):
            event('g');event('k')
            if bit(9):return False
        if len(source)>=512:return False
        saved=native.copy()
        if not parse(source):restore(saved);return False
        legacy=minor<85
        base=source.replace(b'\\',b'/').rsplit(b'/',1)[-1][:255]
        stem=base.rsplit(b'.',1)[0] if b'.' in base else base
        hint=(stem+b'.r1')[:255]
        if override is not None:
            if not init() or override not in (b'Import.r8',) or not available(override):restore(saved);return False
            library=overwrite(library,override);target=b'R/'+override;fmt(target)
        else:
            target=choose(hint)
            if target is None:restore(saved);return False
        if legacy:
            okay=save(target)
            if not okay:restore(saved)
            return okay
        stage=allocate()
        if stage is None:restore(saved);return False
        folder=directory
        if not copy(stage) or not replace(stage,target):remove(stage);remove_directory(folder);restore(saved);return False
        remove_directory(folder)
        if not parse(target):remove(target);restore(saved);return False
        native[3]=overwrite(native[3],target);set_file(target.rsplit(b'/',1)[-1]);return True
    def edit():
        path=native[3].split(b'\0')[0];name=path.rsplit(b'/',1)[-1];event('e',name);saved=native.copy()
        if not init():return False
        target=b'R/'+name;fmt(target)
        if not parse(target,True):
            restore(saved);event('w')
            if not bit(19):event('a',n=48)
            return False
        native[3]=overwrite(native[3],target);set_file(name);native[4]=0;event('q')
        native[0]=bytes([102])*192;native[1]=bytes([103])*16;set_file(b'edited.r9');native[3]=overwrite(native[3],b'R/edited.r9');native[4]=1
        files[b'R/independent.r9']=b'\x07'
        if bit(21):raise Escape()
        done=not bit(20);restore(saved,not done,True);return done
    result=-2
    try:
        okay=imported()
        if op=='E' and okay:edit()
        result=int(okay)
        if op=='A':override=None
    except Escape:pass
    return dict(events=events,result=result,version=version,counter=counter,open=int(handle),physicalOpen=int(physical),dialog=dialog,
        override=None if override is None else override.hex(),native=[b.hex() if isinstance(b,bytes) else b for b in native],library=library.hex(),
        directories=[] if directory is None else [directory.hex()],files={p.hex():b.hex() for p,b in files.items()})


def main():
    inputs=list(vectors())
    with tempfile.TemporaryDirectory(prefix='race-import-') as temp:
        binary=compile_probe(Path(temp))
        run=subprocess.run([str(binary)],input=('\n'.join(' '.join(map(str,v)) for v in inputs)+'\n').encode(),stdout=subprocess.PIPE,check=True)
        outputs=run.stdout.splitlines();assert len(outputs)==len(inputs)
        for n,(v,line) in enumerate(zip(inputs,outputs)):
            actual=json.loads(line);expected=oracle(v)
            for key,value in expected.items():assert actual[key]==value,(n,v,key,actual[key],value)
    print(f'{len(inputs)} source import/name-choice/save/load/edit/wrapper cases match native arrays, residual bytes, counters, dialog/override and exact calls')
    print('Parser/dialog/BOF/I/O providers; no full UI, decoder, native OS, alias/concurrency or crash qualification claim.')


if __name__=='__main__':main()
