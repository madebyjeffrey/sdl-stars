#!/usr/bin/env python3
"""Source-extracted M02 stage/export allocation and numbered suggestions.

Clock, formatter-error, directory, availability and exclusive-copy replies are
scripted. Python independently predicts names, truncation, calls and counters.
This does not qualify OS races, directory enumeration or the race decoder.
"""
import json
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile
from check_s02_source import ROOT, function
from check_library_publication_source import extract, check_exclusive_copy
from check_race_workflows_source import prefix


PRELUDE = r'''
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdarg.h>
#include <ctype.h>
#include <assert.h>
#include <errno.h>
#include <limits.h>
#define TRUE 1
#define FALSE 0
#define SDL_strcasecmp strcasecmp
#define SDL_strncasecmp strncasecmp
#define SDL_tolower tolower
static char szRaceLibraryRoot[256],szBase[256],szDirSep[2]="/",chDirSep='/';
static uint32_t idRaceStage,tick;
static int calls,formatFailure,collisions,kind,removeOkay,available;
static int makes,copies,queries;
static int firstEvent;
static void hex(const char *text){
    putchar('"');for(const unsigned char *p=(const unsigned char *)text;*p;p++)printf("%02x",*p);putchar('"');
}
static void event(char type,const char *text,int value){
    if(!firstEvent)putchar(',');firstEvent=0;
    printf("[\"%c\",",type);hex(text);printf(",%d]",value);
}
static void unhex(char *out,const char *in){
    if(!strcmp(in,"-")){*out=0;return;}
    size_t n=strlen(in);assert(n%2==0 && n<2048);
    for(size_t i=0;i<n;i+=2){unsigned v;assert(sscanf(in+i,"%2x",&v)==1);out[i/2]=(char)v;}out[n/2]=0;
}
static uint32_t DwTickCount(void){event('t',"",0);return tick;}
static int CchSnprintf(char *out,size_t cap,const char *fmt,...){
    char full[4096];va_list a,b;va_start(a,fmt);va_copy(b,a);
    int n=vsnprintf(full,sizeof(full),fmt,b);va_end(b);assert(n>=0 && n<(int)sizeof(full));
    int result=vsnprintf(out,cap,fmt,a);va_end(a);assert(result==n);
    calls++;if(calls==formatFailure)result=-1;event('f',full,result);return result;
}
static int CreateNativeDir(const char *path){
    int okay=makes++>=collisions && kind==0;event('m',path,okay);return okay?0:-1;
}
static int16_t FNativeDirectoryExists(const char *path){
    int okay=makes<=collisions;event('d',path,okay);return okay;
}
static int RemoveNativePath(const char *path){event('r',path,removeOkay);return removeOkay?0:-1;}
static int16_t FCopyNativeFileExclusive(const char *source,const char *target){
    assert(!strcmp(source,"source"));assert(errno==0);
    int okay=copies++>=collisions && kind==0;
    errno=okay?0:(copies<=collisions || kind==2)?EEXIST:EACCES;
    event('c',target,okay?1:errno==EEXIST?2:0);return okay;
}
static int16_t FRaceLibraryNameAvailable(const char *name){
    int okay=++queries==available;event('a',name,okay);return okay;
}
typedef int16_t (*FNativeDirectoryEntryProc)(void *,const char *);
typedef struct {FNativeDirectoryEntryProc pfnEntry;void *lpData;} LIBRARYRACEENUM;
static int16_t FNativeFileSize(const char *path,int64_t *size){*size=1;event('b',path,1);return 1;}
static int16_t listed(void *data,const char *name){assert(!data);event('l',name,1);return 1;}
'''

HARNESS = r'''
int main(void){
    char op,rootHex[2048],nameHex[2048],name[1024],dir[512],file[512];
    unsigned counter,ticks,cap1,cap2,sep;
    while(scanf(" %c %u %u %u %u %d %d %d %d %d %u %2047s %2047s",&op,&counter,&ticks,&cap1,&cap2,
          &collisions,&kind,&formatFailure,&removeOkay,&available,&sep,rootHex,nameHex)==13){
        assert(cap1<=512 && cap2<=512 && (sep==47 || sep==92));
        assert(strlen(rootHex)<512);unhex(szRaceLibraryRoot,rootHex);unhex(name,nameHex);assert(strlen(name)<256 || op!='N');
        idRaceStage=counter;tick=ticks;szDirSep[0]=chDirSep=(char)sep;
        calls=makes=copies=queries=0;firstEvent=1;memset(dir,0xa5,sizeof(dir));memset(file,0xa5,sizeof(file));
        dir[0]=file[0]=0;printf("{\"events\":[");int result;
        if(op=='S')result=FCreateRaceStagePath(dir,cap1,file,cap2);
        else if(op=='E')result=FCopyRaceExport("source",name,file,cap2);
        else {assert(op=='N');SuggestRaceFilename(name);result=FPortableRaceFilename(name);
            LIBRARYRACEENUM enumeration={listed,NULL};assert(EnumLibraryRace(&enumeration,name));}
        printf("],\"result\":%d,\"counter\":%u,\"directory\":",result,idRaceStage);hex(dir);
        printf(",\"file\":");hex(file);printf(",\"name\":");hex(name);printf("}\n");
    }
    (void)szBase;return 0;
}
'''


def oracle(v):
    op, counter, tick, dcap, fcap, collisions, kind, error, removed, available, sep, root, name = v
    events, directory, file = [], b'', b''
    formats = 0

    def event(kind, path=b'', value=0):
        events.append([kind, path.hex(), value])

    def formatted(value, cap):
        nonlocal formats
        formats += 1
        n = -1 if formats == error else len(value)
        event('f', value, n)
        return (value[:cap-1] if cap else b''), n > 0 and n < cap

    def suffix(value):
        return value.rsplit(b'.', 1)[0] if b'.' in value else value

    result = 0
    if op == 'N':
        stem = prefix(suffix(name), 239)
        for i in range(1, 1000):
            name, _ = formatted(stem+b'.r'+str(i).encode(), 256)
            event('a', name, int(i == available))
            if i == available:
                break
        else:
            name = b''
        # Vectors use non-reserved ASCII stems or well-formed UTF-8; malformed
        # UTF-8 is stopped by the actual prefix routine before a suggestion.
        stem, dot, digits = name.rpartition(b'.')
        result = int(bool(stem and dot and digits[:1] in (b'r', b'R') and
            digits[1:].isdigit() and 1 <= len(digits[1:]) <= 3 and digits[1:2] != b'0') and
            len(name) < 240 and len(root)+len(name)+2 < 256 and
            name[:1] not in (b' ', b'.') and name[-1:] not in (b' ', b'.') and
            not any(c < 32 or c == 127 or c in b'<>:"/\\|?*' for c in name) and
            name.split(b'.', 1)[0].lower() not in (b'con', b'prn', b'aux', b'nul', b'clock$') and
            not (len(name.split(b'.', 1)[0]) == 4 and name[:3].lower() in (b'com', b'lpt') and name[3:4] in b'123456789'))
        if result:
            path, okay = formatted(root+bytes([sep])+name, 512)
            assert okay
            event('b', path, 1)
            event('l', name, 1)
    else:
        event('t')
        for i in range(1000):
            old, counter = counter, (counter+1) % 2**32
            if op == 'S':
                raw = root+bytes([sep])+b'.stars-race-stage-'+str(tick).encode()+b'-'+str(old).encode()
                directory, okay = formatted(raw, dcap)
                if not okay:
                    break
                made = i >= collisions and kind == 0
                event('m', directory, int(made))
                if not made:
                    event('d', directory, int(i < collisions))
                    if i < collisions:
                        continue
                    break
                file, okay = formatted(directory+bytes([sep])+b'design.r1', fcap)
                if not okay:
                    event('r', directory, removed)
                    break
                result = 1
                break
            else:
                prefix_name = name[:name.rfind(bytes([sep]))+1]
                raw = prefix_name+b'.stars-race-export-'+str(tick).encode()+b'-'+str(old).encode()+b'.tmp'
                file, okay = formatted(raw, fcap)
                if not okay:
                    break
                copied = i >= collisions and kind == 0
                exists = i < collisions or kind == 2
                event('c', file, 1 if copied else 2 if exists else 0)
                if copied:
                    result = 1
                    break
                if not exists:
                    break
    return dict(events=events, result=result, counter=counter, directory=directory.hex(), file=file.hex(), name=name.hex())


def vectors():
    for op in ('S', 'E'):
        for counter in (0, 4294967295, 4294967294):
            for tick in (0, 4294967295):
                for collisions in (0, 1, 2, 999, 1000):
                    for kind in (0, 1, 2):
                        yield (op,counter,tick,512,512,collisions,kind,0,0,1,47,b'R',b'outside/Race.r1')
        for dcap in (0, 1, 2, 28, 29, 30, 36, 40, 511, 512):
            for fcap in (0, 1, 2, 29, 30, 31, 40, 511, 512):
                for error in (0, 1, 2, 3):
                    for removed in (0, 1):
                        yield (op,0,0,dcap,fcap,1,0,error,removed,1,47,b'R',b'outside/Race.r1')
        for sep in (47, 92):
            for target in (b'Race.r1',b'/a/b.r1',b'a\\b.r1',b'a/b\\c.r1',b'/'):
                yield (op,7,23,512,512,2,0,0,1,1,sep,b'R',target)
    for length in (0, 1, 14, 120, 233, 234, 235, 236, 237, 238, 239, 240, 250):
        for cp in (b'', 'é'.encode(), '🌟'.encode(), b'\xfftail'):
            name=(b'A'*length+cp+b'.r1')[:255]
            for available in (1, 9, 10, 99, 100, 999, 1000):
                for root in (b'R', b'R'*14):
                    yield ('N',7,23,512,512,0,0,0,1,available,47,root,name)
    for name in (b'A.B.r1', b'A.B', b'.r1', b'CON.r1', b'COM1.r1', b'CLOCK$.r9', b'Name .r1', b'bad/name.r1'):
        yield ('N',0,0,512,512,0,0,0,1,999,47,b'R',name)


def check_direct_copy(tmp, compiler):
    prelude=r'''
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>
#define TRUE 1
#define FALSE 0
#define PfOpenNativeFile fopen
#define RemoveNativeFile remove
'''
    harness=r'''
static void writeFile(const char *path,int length){
    FILE *f=fopen(path,"wb");assert(f);
    for(int i=0;i<length;i++)assert(fputc(i%251,f)!=EOF);
    assert(fclose(f)==0);
}
static void checkFile(const char *path,int length){
    FILE *f=fopen(path,"rb");assert(f);
    for(int i=0;i<length;i++)assert(fgetc(f)==i%251);
    assert(fgetc(f)==EOF && !ferror(f));assert(fclose(f)==0);
}
int main(void){
    const int lengths[]={0,1,8191,8192,8193,16384,16385};int cases=0;
    for(unsigned i=0;i<sizeof(lengths)/sizeof(lengths[0]);i++){
        writeFile("source",lengths[i]);writeFile("direct",37);
        assert(FCopyNativeFile("source","direct"));checkFile("direct",lengths[i]);checkFile("source",lengths[i]);cases++;
        assert(FCopyNativeFile("source","source"));checkFile("source",0);cases++;
    }
    remove("missing");writeFile("direct",37);
    assert(!FCopyNativeFile("missing","direct"));checkFile("direct",37);cases++;
    assert(mkdir("directory",0700)==0);writeFile("source",1);
    assert(!FCopyNativeFile("source","directory"));checkFile("source",1);cases++;
    printf("%d nonexclusive real-stdio cases: overwrite, literal self-alias truncation and open failures\n",cases);return 0;
}
'''
    c=tmp/'direct.c';binary=tmp/'direct'
    c.write_text('\n'.join([prelude,extract('FCopyNativeFileContents'),extract('FCopyNativeFile'),harness]))
    subprocess.run(compiler+['-std=gnu11','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',
        '-fno-sanitize-recover=all',str(c),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],cwd=tmp,check=True)


def main():
    ui=(ROOT/'src/ui/libraryui.c').read_text()
    globals_header=(ROOT/'src/core/globals.h').read_text()
    assert re.search(r'static char szRaceLibraryRoot\[256\];',ui)
    assert re.search(r'static uint32_t idRaceStage;',ui)
    assert re.search(r'extern char\s+szBase\[256\];',globals_header)
    compiler=shlex.split(os.environ.get('CC','cc'))
    flags=['-std=gnu11','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-sanitize-recover=all']
    source='\n'.join([PRELUDE, function('src/platform/encoding.c','int CbUtf8Char('),
        function('src/platform/encoding.c','size_t CchUtf8Prefix('),extract('FPortableRaceFilename'),
        extract('FCreateRaceStagePath'),function('src/ui/libraryui.c','static void SuggestRaceFilename('),
        extract('FCopyRaceExport'),extract('EnumLibraryRace'),HARNESS])
    inputs=list(vectors())
    lines=[' '.join(map(str,v[:11]))+' '+(v[11].hex() or '-')+' '+(v[12].hex() or '-') for v in inputs]
    with tempfile.TemporaryDirectory(prefix='race-allocation-') as temp:
        tmp=Path(temp);c=tmp/'allocation.c';c.write_text(source);binary=tmp/'allocation'
        subprocess.run(compiler+flags+[str(c),'-o',str(binary)],check=True)
        run=subprocess.run([str(binary)],input=('\n'.join(lines)+'\n').encode(),stdout=subprocess.PIPE,check=True)
        outputs=run.stdout.splitlines();assert len(outputs)==len(inputs)
        for i,(v,line) in enumerate(zip(inputs,outputs)):
            actual=json.loads(line);expected=oracle(v)
            assert actual==expected,(i,v,actual,expected)
        print(f'{len(inputs)} native allocator/suggestion cases match independent names, counters, output prefixes and exact calls')
        check_exclusive_copy(tmp,compiler)
        check_direct_copy(tmp,compiler)
    print('Scripted services and quiet path identities; full OS/concurrency/UI and outer import/export controllers remain separately qualified.')


if __name__=='__main__':
    main()
