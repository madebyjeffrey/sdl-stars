#!/usr/bin/env python3
"""M02 actual comparison, full snapshot and filename-prefix adapters.

Native PLAYER layout and source bodies are compiled unchanged. Comparison tests
exclude reached unterminated-name reads; the model uses a bounded-name domain.
UI/global frames, byte offsets, and ASCII/UTF-8 truncation are checked separately
from serialization, authentication, filesystem or crash qualification.
"""
from pathlib import Path
import os
import random
import re
import shlex
import struct
import subprocess
import sys
import tempfile
from check_s02_source import ROOT,function


def declarations():
    source=(ROOT/'src/ui/libraryui.c').read_text()
    begin=source.index('typedef struct {\n    PLAYER plr;')
    end=source.index('} RACESTATE;',begin)+len('} RACESTATE;')
    return r'''
#include "common.h"
#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
PLAYER vplr;
char szRacePass[16],szRaceFile[16];
static char szRaceLibraryPath[512],szRaceFileSuggest[256];
static int16_t fNewLibraryRace;
_Static_assert(sizeof(PLAYER)==192 && offsetof(PLAYER,szName)==128 && offsetof(PLAYER,szNames)==160,"native design byte projection");
_Static_assert(offsetof(PLAYER,wFlags)==84 && offsetof(PLAYER,wMdPlr)==6 && offsetof(PLAYER,lSalt)==12,"native flag/detail/credential projection");
_Static_assert(sizeof(szRacePass)==16 && sizeof(szRaceFile)==16 && sizeof(szRaceLibraryPath)==512 && sizeof(szRaceFileSuggest)==256,"snapshot and UI filename capacities");
'''+'\n'+source[begin:end]


def bodies():
    save=function('src/ui/raceui.c','int16_t FSaveRace(')
    start=save.index('    if (szFileSuggest == szRaceFile')
    end=save.index('    if (szFileSuggest) {',start)
    selector='char *NativeSuggestion(char *szFileSuggest) {\n'+save[start:end]+'    return szFileSuggest;\n}'
    return selector+'\n'+'\n'.join([
        function('src/ui/libraryui.c','static int16_t FSameLibraryRace('),
        function('src/ui/libraryui.c','static void SaveRaceState('),
        function('src/ui/libraryui.c','static void RestoreRaceState('),
        function('src/ui/raceui.c','void SetSzRaceFile('),
        function('src/platform/encoding.c','int CbUtf8Char('),
        function('src/platform/encoding.c','size_t CchUtf8Prefix('),
        function('src/core/race.c','uint16_t IRaceChecksum('),
    ])


HARNESS=r'''
static unsigned long checks,cases,excluded;
#define CHECK(c) do{checks++;if(!(c)){fprintf(stderr,"case %lu assertion %lu: %s\n",cases,checks,#c);abort();}}while(0)
static unsigned rnd=17;
static unsigned randomByte(void){rnd=rnd*1664525u+1013904223u;return rnd>>24;}
static void fill(void *p,size_t n){for(size_t i=0;i<n;i++)((unsigned char *)p)[i]=(unsigned char)randomByte();}
static void memoryTests(void){
    for(int length=0;length<=31;length++)for(int plural=0;plural<=31;plural+=31){
        PLAYER a,b;fill(&a,sizeof(a));memset(a.szName,0,32);memset(a.szNames,0,32);
        memset(a.szName,'A',length);memset(a.szNames,'B',plural);
        for(int at=0;at<192;at++)for(int bit=0;bit<8;bit++){
            b=a;((unsigned char *)&b)[at]^=1u<<bit;
            if(!memchr(b.szName,0,32) || !memchr(b.szNames,0,32)){excluded++;continue;}
            int expected=at<128?(at==84 && bit==3):at<160?(at-128>length):(at-160>plural);
            PLAYER savedA=a,savedB=b;CHECK(FSameLibraryRace(&a,&b)==expected);
            CHECK(memcmp(&a,&savedA,sizeof(a))==0 && memcmp(&b,&savedB,sizeof(b))==0);cases++;
        }
    }
    PLAYER a={0},b={0};a.lSalt=1;b.lSalt=2;CHECK(!FSameLibraryRace(&a,&b));cases++;
    b=a;((unsigned char *)&b)[0]^=1;((unsigned char *)&b)[2]^=1;
    CHECK(IRaceChecksum(&a)==IRaceChecksum(&b) && !FSameLibraryRace(&a,&b));cases++;
    b=a;b.fLearned=!a.fLearned;CHECK(FSameLibraryRace(&a,&b));
    CHECK(IRaceChecksum(&a)!=IRaceChecksum(&b));cases++;
    const int values[]={-32768,-1,0,1,32767};
    for(unsigned n=0;n<5;n++)for(int race=-1;race<=2;race++)for(int path=-1;path<=2;path++){
        fill(&vplr,sizeof(vplr));fill(szRacePass,16);fill(szRaceFile,16);fill(szRaceLibraryPath,512);fill(szRaceFileSuggest,256);fNewLibraryRace=values[n];
        RACESTATE saved;SaveRaceState(&saved);PLAYER old=vplr;
        unsigned char pw[16],file[16],binding[512],newPw[16],newFile[16],newBinding[512],suggest[256];
        memcpy(pw,szRacePass,16);memcpy(file,szRaceFile,16);memcpy(binding,szRaceLibraryPath,512);
        fill(&vplr,sizeof(vplr));fill(szRacePass,16);fill(szRaceFile,16);fill(szRaceLibraryPath,512);fill(szRaceFileSuggest,256);fNewLibraryRace=123;
        PLAYER changed=vplr;memcpy(newPw,szRacePass,16);memcpy(newFile,szRaceFile,16);memcpy(newBinding,szRaceLibraryPath,512);memcpy(suggest,szRaceFileSuggest,256);
        RestoreRaceState(&saved,race,path);
        CHECK(memcmp(&vplr,race?&old:&changed,192)==0);CHECK(memcmp(szRacePass,race?pw:newPw,16)==0);
        CHECK(memcmp(szRaceFile,race?file:newFile,16)==0);CHECK(memcmp(szRaceLibraryPath,path?binding:newBinding,512)==0);
        CHECK(fNewLibraryRace==(path?values[n]:123));CHECK(memcmp(szRaceFileSuggest,suggest,256)==0);cases++;
    }
    // Snapshot restores the short cache, leaving the full UI suggestion from a
    // later edit. The actual FSaveRace selection expression can then use it.
    SetSzRaceFile("ABCDEFGHIJKLMNO-old.r1");RACESTATE saved;SaveRaceState(&saved);
    SetSzRaceFile("ABCDEFGHIJKLMNO-new.r1");RestoreRaceState(&saved,TRUE,TRUE);
    CHECK(!strcmp(szRaceFile,"ABCDEFGHIJKLMNO"));CHECK(!strcmp(szRaceFileSuggest,"ABCDEFGHIJKLMNO-new.r1"));
    CHECK(NativeSuggestion(szRaceFile)==szRaceFileSuggest);char separate[16];memcpy(separate,szRaceFile,16);CHECK(NativeSuggestion(separate)==separate);cases++;
    printf("native race memory: %lu cases, %lu assertions; %lu unterminated-name cases excluded before C comparison\n",cases,checks,excluded);
}
int main(int argc,char **argv){
    assert(argc==2);
    if(!strcmp(argv[1],"memory")){memoryTests();return 0;}
    assert(!strcmp(argv[1],"prefix"));uint32_t length;unsigned long count=0;
    while(fread(&length,4,1,stdin)==1){
        assert(length<=4096);char bytes[4097];assert(fread(bytes,1,length,stdin)==length);bytes[length]=0;
        memset(szRaceFile,0xaa,16);memset(szRaceFileSuggest,0xbb,256);SetSzRaceFile(bytes);
        assert(fwrite(szRaceFile,1,16,stdout)==16 && fwrite(szRaceFileSuggest,1,256,stdout)==256);count++;
    }
    assert(feof(stdin));fprintf(stderr,"native filename prefixes: %lu cases\n",count);return 0;
}
'''


def prefix(value,limit):
    value=value.split(b'\0')[0]
    for size in range(min(limit,len(value)),-1,-1):
        try:value[:size].decode('utf-8');return value[:size]
        except UnicodeDecodeError:pass
    raise AssertionError('empty prefix is valid')


def vectors():
    for a in range(256):
        for b in range(256):yield b'A'*14+bytes([a,b])+b'tail'
    for cp in (0x7f,0x80,0x7ff,0x800,0xd7ff,0xe000,0xffff,0x10000,0x10ffff):
        for length in range(258):yield b'A'*length+chr(cp).encode('utf-8')+b'end'
    rng=random.Random(200217)
    for _ in range(10000):yield bytes(rng.randrange(256) for _ in range(rng.randrange(301)))


def build(path,extra='',harness=HARNESS):
    for generator in ('text/textgen.py','data/datagen.py'):
        subprocess.run([sys.executable,str(ROOT/generator),str(path)],check=True,stdout=subprocess.DEVNULL)
    c=path/'race.c';c.write_text(declarations()+'\n'+extra+'\n'+bodies()+'\n'+harness);binary=path/'race'
    subprocess.run(shlex.split(os.environ.get('CC','cc'))+['-std=gnu11','-Wall','-Wextra','-Werror','-Wno-pointer-sign',
        '-fsanitize=address,undefined','-fno-sanitize-recover=all','-I'+str(ROOT/'src/core'),'-I'+str(ROOT/'src/platform'),'-I'+str(path),str(c),'-lm','-o',str(binary)],check=True)
    return binary


def main():
    model=(ROOT/'spec/dafny/M02-native-race-state.dfy').read_text()
    constants={k:int(v) for k,v in re.findall(r'const (\w+): nat := (\d+)',model)}
    assert constants==dict(PlayerBytes=192,NameAt=128,PluralAt=160,FlagAt=84,DetailAt=6)
    with tempfile.TemporaryDirectory(prefix='race-workflows-') as tmp:
        path=Path(tmp);binary=build(path);subprocess.run([str(binary),'memory'],check=True)
        inputs=list(vectors());data=b''.join(struct.pack('<I',len(b))+b for b in inputs)
        result=subprocess.run([str(binary),'prefix'],input=data,stdout=subprocess.PIPE,check=True).stdout
        assert len(result)==272*len(inputs)
        for i,value in enumerate(inputs):
            short,full=prefix(value,15),prefix(value,255)
            expected=short+b'\0'+b'\xaa'*(15-len(short))+full+b'\0'+b'\xbb'*(255-len(full))
            assert result[i*272:(i+1)*272]==expected,(i,value,result[i*272:(i+1)*272],expected)
        print(f'{len(inputs)} prefix outputs agree with the independent maximal-valid-prefix oracle; stale tails preserved')
        print('Native filesystem, complete record serialization, authentication, full Unicode comparison and UI workflows remain separately qualified.')


if __name__=='__main__':main()
