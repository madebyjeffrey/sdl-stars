#!/usr/bin/env python3
"""Actual Duplicate/Export/Delete controllers with independent state oracle.

Native PLAYER/snapshot/UTF-8/portable/suggestion/export-allocation bodies are
compiled unchanged. Parser, picker, copy, file-size, replacement and removal
providers expose explicit deterministic replies over literal path identities.
This does not qualify decoding/authentication, modal reentrancy or OS effects.
"""
import json
import random
from pathlib import Path
import subprocess
import tempfile
from check_s02_source import ROOT,function
from check_race_workflows_source import build,prefix
from check_library_publication_source import extract


PRELUDE=r'''
#include <stdarg.h>
#include <strings.h>
#include <errno.h>
#include <ctype.h>
#define SDL_strcasecmp compared
#define SDL_strncasecmp strncasecmp
#define SDL_tolower tolower
#define OFN_HIDEREADONLY 4
#define OFN_PATHMUSTEXIST 0x800
#define OFN_OVERWRITEPROMPT 2
typedef struct {size_t lStructSize;HWND hwndOwner;char *lpstrFilter,*lpstrFile;unsigned nMaxFile,Flags;} OPENFILENAME;
char szBase[256];
uint16_t wVersFile;
static char szRaceLibraryRoot[256],targetPath[512],sourceName[256];
static uint32_t idRaceStage;
static void SaveRaceState(RACESTATE *);
static void RestoreRaceState(const RACESTATE *,int16_t,int16_t);
static unsigned flags,startCounter;
static int mode,retries,available,binding,parseCount,firstEvent,formatCount;
static int parserAnswer(int n){return flags&(n==1?2u:16u)?(n==1?-1:0):1;}
typedef struct {char path[512];unsigned char bytes[8];size_t count;int present;} FILEENTRY;
static FILEENTRY files[1104];static int fileCount;
static void hex(const void *data,size_t n){const unsigned char *p=data;putchar('"');for(size_t i=0;i<n;i++)printf("%02x",p[i]);putchar('"');}
static void event(char kind,const char *a,const char *b,int value){
    if(!firstEvent)putchar(',');firstEvent=0;printf("[\"%c\",\"%s\",\"%s\",%d]",kind,a,b,value);
}
static int compared(const char *a,const char *b){
    int result=strcasecmp(a,b);
    if(strchr(a,chDirSep) && strchr(b,chDirSep))event('v',a,b,(result>0)-(result<0));
    return result;
}
static FILEENTRY *entry(const char *path){
    for(int i=0;i<fileCount;i++)if(!strcmp(files[i].path,path))return &files[i];
    assert(fileCount<1104);FILEENTRY *p=&files[fileCount++];memset(p,0,sizeof(*p));assert(strlen(path)<sizeof(p->path));strcpy(p->path,path);return p;
}
static void put(const char *path,const unsigned char *bytes,size_t n){FILEENTRY *p=entry(path);assert(n<=8);memcpy(p->bytes,bytes,n);p->count=n;p->present=1;}
static int16_t FInitRaceLibrary(void){event('i',"","",0);return !(flags&1);}
int CchSnprintf(char *out,size_t cap,const char *fmt,...){
    va_list a;va_start(a,fmt);int n=vsnprintf(out,cap,fmt,a);va_end(a);assert(n>=0 && n<(int)cap);formatCount++;
    if(!strcmp(fmt,"%s%s%s")){event('f',out,"",0);if(flags&4 && formatCount>1)return -1;}
    if(flags&2048 && !strcmp(fmt,"%.*s.stars-race-export-%u-%u.tmp")){event('h',out,"",-1);return -1;}
    return n;
}
static int16_t FRaceLibraryNameAvailable(const char *name){
    const char *p=strrchr(name,'.');assert(p && p[1]=='r');int yes=atoi(p+2)==available;
    event('n',name,"",yes);return yes;
}
int16_t FWasRaceFile(char *path,int16_t password){
    assert(password==TRUE);int n=++parseCount;event('p',path,"",n);
    memset(&vplr,64+n,sizeof(vplr));vplr.szName[0]=vplr.szNames[0]=0;
    memset(szRacePass,80+n,sizeof(szRacePass));const char *p=strrchr(path,chDirSep);SetSzRaceFile((char *)(p?p+1:path));
    wVersFile=(uint16_t)(100+n);return (int16_t)parserAnswer(n);
}
static int IsWindow(HWND hwnd){(void)hwnd;event('w',"","",0);return !(flags&64);}
char *PszGetCompressedString(StringId id){return id==idsLibraryRaceDeletePrompt?"Delete %s?":id==idsLibraryTitle?"Races":"failure";}
int16_t AlertSz(char *text,int16_t style){assert(!strcmp(text,"failure"));event('a',"","",style);return 0;}
static int MessageBox(HWND hwnd,const char *text,const char *title,unsigned style){
    (void)hwnd;assert(!strcmp(title,"Races") && style==(MB_YESNO|MB_ICONQUESTION|MB_TASKMODAL));
    char expected[320];snprintf(expected,sizeof(expected),"Delete %s?",sourceName);assert(!strcmp(text,expected));
    int answer=flags&16384?7:6;event('q',sourceName,"",answer);return answer;
}
int16_t FNativeFileSize(const char *path,int64_t *size){
    FILEENTRY *p=entry(path);int okay=p->present && !(flags&4096);if(okay)*size=(int64_t)p->count;
    event('b',path,"",okay);return okay;
}
int RemoveNativeFile(const char *path){
    FILEENTRY *p=entry(path);int okay=p->present && !(flags&32);if(okay)p->present=0;
    event('r',path,"",okay);return okay?0:-1;
}
int ReplaceNativeFile(const char *source,const char *target){
    FILEENTRY *p=entry(source);int okay=p->present && !(flags&1024);
    if(okay){unsigned char bytes[8];size_t n=p->count;memcpy(bytes,p->bytes,n);p->present=0;put(target,bytes,n);}
    event('x',source,target,okay);return okay?0:-1;
}
static int copyFile(const char *source,const char *target,int exclusive){
    FILEENTRY *p=entry(source),*q=entry(target);int code=mode;
    if(!p->present)code=1;
    if(exclusive && q->present){event('c',source,target,2);errno=EEXIST;return 0;}
    if(code==0){unsigned char bytes[8];size_t n=!exclusive && !strcmp(source,target)?0:p->count;memcpy(bytes,p->bytes,n);put(target,bytes,n);}
    else if(code>=3){unsigned char bytes[8];size_t wanted=!exclusive && !strcmp(source,target)?0:p->count;
        size_t n=code==4?(wanted?1:0):wanted;memcpy(bytes,p->bytes,n);put(target,bytes,n);if(code==3)entry(target)->present=0;}
    event('c',source,target,code==0?1:0);errno=code?EACCES:0;return code==0;
}
static int16_t FCopyNativeFile(const char *source,const char *target){return copyFile(source,target,0);}
static int16_t FCopyNativeFileExclusive(const char *source,const char *target){assert(errno==0);return copyFile(source,target,1);}
uint32_t DwTickCount(void){event('t',"","",0);return 7;}
static void GetLibraryFileFilter(StringId id,char *filter){assert(id==idsLibraryRaceFilesFilter);strcpy(filter,"race");event('g',"","",0);}
static int FNativeSaveFileName(OPENFILENAME *ofn){
    assert(ofn->lStructSize==sizeof(*ofn) && !ofn->hwndOwner && ofn->nMaxFile==512);
    assert(ofn->Flags==(OFN_HIDEREADONLY|OFN_PATHMUSTEXIST|OFN_OVERWRITEPROMPT));
    assert(!strcmp(ofn->lpstrFile,sourceName) && !strcmp(ofn->lpstrFilter,"race"));
    event('k',"","",0);if(flags&256)return 0;strcpy(ofn->lpstrFile,targetPath);return 1;
}
'''

HARNESS=r'''
static void prepare(char op,int nameKind){
    firstEvent=1;parseCount=formatCount=fileCount=0;idRaceStage=startCounter;
    strcpy(szRaceLibraryRoot,nameKind==2?"RRRRRRRRRRRRRR":"R");
    if(nameKind==2){memset(sourceName,'A',236);strcpy(sourceName+236,".r1");}
    else strcpy(sourceName,nameKind==1?"A.B.r1":nameKind==3?"bad":nameKind==4?"Race.R1":"Race.r1");
    char source[512];snprintf(source,sizeof(source),"%s%s%s",szRaceLibraryRoot,szDirSep,sourceName);
    strcpy(targetPath,flags&512?source:"outside/Export.r1");
    if(flags&512)for(char *p=targetPath;*p;p++)*p=(char)toupper((unsigned char)*p);
    if(flags&131072)strcpy(targetPath,"outside/.stars-race-export-7-0.tmp");
    memset(&vplr,17,sizeof(vplr));memset(szRacePass,34,16);memset(szRaceFile,51,16);strcpy(szRaceFile,"old.r7");
    memset(szRaceLibraryPath,68,512);strcpy(szRaceLibraryPath,binding==0?source:binding==1?"R/RACE.R1":binding==2?"elsewhere":"");
    memset(szRaceFileSuggest,85,256);strcpy(szRaceFileSuggest,"old-suggestion.r7");fNewLibraryRace=-7;wVersFile=99;
    const unsigned char a[]={1,2,3,4},b[]={9,8};put(source,a,flags&8192?0:4);
    char sibling[512];snprintf(sibling,sizeof(sibling),"%s%sSibling.r2",szRaceLibraryRoot,szDirSep);put(sibling,b,2);
    if(op=='E'){
        if(!(flags&262144))put(targetPath,b,2);
        const char *slash=strrchr(targetPath,chDirSep);int count=slash?(int)(slash-targetPath)+1:0;
        for(int i=0;i<retries;i++){char temp[512];snprintf(temp,sizeof(temp),"%.*s.stars-race-export-7-%u.tmp",count,targetPath,startCounter+(unsigned)i);put(temp,b,2);}
    }
}
int main(void){
    char op;int nameKind;
    while(scanf(" %c %u %d %d %d %d %d %u",&op,&flags,&mode,&retries,&available,&binding,&nameKind,&startCounter)==8){
        prepare(op,nameKind);printf("{\"events\":[");
        int okay=op=='D'?FDuplicateLibraryRace(NULL,sourceName):op=='E'?FExportLibraryRace(NULL,sourceName,flags&128?NULL:targetPath):FDeleteLibraryRace(NULL,sourceName);
        printf("],\"okay\":%d,\"version\":%u,\"counter\":%u,\"native\":[",okay,wVersFile,idRaceStage);
        hex(&vplr,192);putchar(',');hex(szRacePass,16);putchar(',');hex(szRaceFile,16);putchar(',');hex(szRaceLibraryPath,512);printf(",%d,",fNewLibraryRace);hex(szRaceFileSuggest,256);
        printf("],\"files\":{");int first=1;
        for(int i=0;i<fileCount;i++)if(files[i].present){if(!first)putchar(',');first=0;printf("\"%s\":",files[i].path);hex(files[i].bytes,files[i].count);}
        printf("}}\n");
    }
    (void)FSameLibraryRace;(void)NativeSuggestion;return 0;
}
'''


def oracle(v):
    op,flags,mode,retries,available,binding,kind,counter=v
    root='R'*14 if kind==2 else 'R'
    name='A'*236+'.r1' if kind==2 else ('A.B.r1' if kind==1 else 'bad' if kind==3 else 'Race.R1' if kind==4 else 'Race.r1')
    source=root+'/'+name
    target=source.upper() if flags&512 else 'outside/Export.r1'
    if flags&131072:target='outside/.stars-race-export-7-0.tmp'
    text=lambda s:s.encode()
    copy_string=lambda old,new:new+b'\0'+old[len(new)+1:]
    native=[bytes([17])*192,bytes([34])*16,copy_string(bytes([51])*16,b'old.r7'),
        copy_string(bytes([68])*512,text(source if binding==0 else 'R/RACE.R1' if binding==1 else 'elsewhere' if binding==2 else '')),
        -7,copy_string(bytes([85])*256,b'old-suggestion.r7')]
    saved=native.copy();version=99;events=[];parse_count=0;formats=0
    files={source:b'' if flags&8192 else bytes([1,2,3,4]),root+'/Sibling.r2':bytes([9,8])}
    export_name=lambda c:target[:target.rfind('/')+1]+f'.stars-race-export-7-{c}.tmp'
    if op=='E':
        if not flags&262144:files[target]=bytes([9,8])
        for n in range(retries):files[export_name((counter+n)%2**32)]=bytes([9,8])

    def event(k,a='',b='',n=0):events.append([k,a,b,n])
    def fmt(path):
        nonlocal formats
        formats+=1;event('f',path)
        return not (flags&4 and formats>1)
    def parse(path):
        nonlocal parse_count,version,native
        parse_count+=1;event('p',path,n=parse_count)
        race=bytearray([64+parse_count]*192);race[128]=race[160]=0
        native[0]=bytes(race);native[1]=bytes([80+parse_count])*16
        base=text(path.rsplit('/',1)[-1]);native[2]=copy_string(native[2],prefix(base,15));native[5]=copy_string(native[5],prefix(base,255))
        version=100+parse_count
        return not flags&(2 if parse_count==1 else 16)
    def finish(ok,notify=False):
        nonlocal native
        native=saved[:5]+[native[5]]
        if notify:
            event('w')
            if not flags&64:event('a',n=48)
        return ok
    def copy(source,path,exclusive=False):
        if exclusive and path in files:event('c',source,path,2);return False,True
        code=mode if source in files else 1
        wanted=b'' if source==path and not exclusive else files.get(source,b'')
        if code==0:files[path]=wanted
        elif code>=3:
            if code==3:files.pop(path,None)
            else:files[path]=wanted[:1] if code==4 else wanted
        event('c',source,path,int(code==0));return code==0,False
    def remove(path):
        okay=path in files and not flags&32
        if okay:del files[path]
        event('r',path,n=int(okay));return okay
    def portable():return kind!=3
    def run():
        nonlocal counter
        event('i')
        if flags&1 or not portable():return False
        fmt(source)
        if op=='X':
            okay=source in files and not flags&4096;event('b',source,n=int(okay))
            if not okay or not files[source]:return False
            event('q',name,n=7 if flags&16384 else 6)
            if flags&16384:return False
            if not remove(source):event('a',n=16);return False
            if binding==0:native[3]=b'\0'+native[3][1:]
            return True
        if op=='E' and flags&128:
            event('g');event('k')
            if flags&256:return False
        if not parse(source):return finish(False,op=='E')
        if op=='D':
            stem=name.rsplit('.',1)[0];stem=stem.rsplit('.',1)[0] if '.' in stem else stem
            for n in range(1,1000):
                candidate=f'{stem}.r{n}';event('n',candidate,n=int(n==available))
                if n==available:break
            else:return finish(False,True)
            destination=root+'/'+candidate
            if not fmt(destination):return finish(False,True)
            if not copy(source,destination)[0]:return finish(False,True)
            if not parse(destination):remove(destination);return finish(False,True)
            return finish(True)
        key_a,key_b=source.lower(),target.lower();event('v',source,target,(key_a>key_b)-(key_a<key_b))
        if key_a==key_b:return finish(False,True)
        event('t')
        for _ in range(1000):
            temp=export_name(counter);counter=(counter+1)%2**32
            if flags&2048:event('h',temp,n=-1);return finish(False,True)
            okay,exists=copy(source,temp,True)
            if okay:break
            if not exists:return finish(False,True)
        else:return finish(False,True)
        if not parse(temp):remove(temp);return finish(False,True)
        okay=not flags&1024
        if okay:files[target]=files.pop(temp) if temp!=target else files[temp]
        event('x',temp,target,int(okay))
        if not okay:remove(temp);return finish(False,True)
        return finish(True)
    okay=run()
    return dict(events=events,okay=int(okay),version=version,counter=counter,
        native=[b.hex() if isinstance(b,bytes) else b for b in native],files={p:b.hex() for p,b in files.items()})


def vectors():
    for op in ('D','E','X'):
        for flags in (0,1,2,4,16,32,64,128,256,512,1024,2048,4096,8192,16384,128|256,16|32,1024|32):
            for mode in range(6):
                for binding in range(4):yield op,flags,mode,1,1,binding,0,0
        for kind in range(5):
            for available in (1,999,1000):yield op,0,0,0,available,0,kind,0
    for flags in (131072,131072|262144,131072|262144|16,131072|262144|16|32,131072|262144|1024|32):
        yield 'E',flags,0,0,1,0,0,0
    for retries in (0,1,999,1000):
        for counter in (0,4294967295):yield 'E',0,0,retries,1,0,0,counter
    rng=random.Random(20261027)
    for _ in range(3000):
        yield rng.choice(('D','E','X')),rng.randrange(1<<15),rng.randrange(6),rng.randrange(3),rng.choice((1,2,999,1000)),rng.randrange(4),rng.randrange(5),rng.choice((0,4294967295))


def main():
    extra='\n'.join([PRELUDE,extract('FPortableRaceFilename'),function('src/ui/libraryui.c','static void SuggestRaceFilename('),
        extract('FCopyRaceExport'),function('src/ui/libraryui.c','int16_t FDuplicateLibraryRace('),
        function('src/ui/libraryui.c','int16_t FExportLibraryRace('),function('src/ui/libraryui.c','int16_t FDeleteLibraryRace(')])
    # SetSzRaceFile's definition follows these bodies in the shared factory;
    # common.h already supplies its native prototype.
    inputs=list(vectors());data=('\n'.join(' '.join(map(str,v)) for v in inputs)+'\n').encode()
    with tempfile.TemporaryDirectory(prefix='race-actions-') as directory:
        binary=build(Path(directory),extra=extra,harness=HARNESS)
        result=subprocess.run([str(binary)],input=data,stdout=subprocess.PIPE,check=True)
        outputs=result.stdout.splitlines();assert len(outputs)==len(inputs)
        for n,(v,line) in enumerate(zip(inputs,outputs)):
            actual=json.loads(line);expected=oracle(v)
            assert actual==expected,(n,v,actual,expected)
    print(f'{len(inputs)} extracted Duplicate/Export/Delete cases match full captured bytes, residual files, versions, counters and exact helper calls')
    print('Scripted parser/picker/filesystem replies; no full native decoder, UI or OS/concurrency/crash qualification claim.')


if __name__=='__main__':main()
