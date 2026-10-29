#!/usr/bin/env python3
"""Inject failures into the actual FSaveProfile body; no OS writes are performed.

This validates control order, temporary cleanup attempts, error assignment and
serialization. It does not validate real syscall failure/crash durability.
"""
from pathlib import Path
import os
import shlex
import subprocess
import tempfile
from check_s02_source import ROOT, function


def main():
    stubs = r'''
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#define TRUE 1
#define FALSE 0
#define ERROR_NOT_ENOUGH_MEMORY 8
typedef int BOOL;
typedef struct COMPATPROFILE {char *szRaw;struct COMPATPROFILE *lpNext;} COMPATPROFILE;
enum {Ok,Allocate,Temporary,Mode,Bom,Rows,Newline,Flush,Sync,Close,Rename};
static int failure,lastError,published,unlinked,cleanupOK,opened,closed,syncCalled,renameCalled;
static char output[256];static size_t used;
static void SetLastError(int e){lastError=e;}
static int DwProfileError(void){return 29;}
static void *TestMalloc(size_t n){return failure==Allocate?NULL:malloc(n);}
typedef FILE NATIVEFILE;
static NATIVEFILE *TestTemp(char *p){(void)p;if(failure==Temporary)return NULL;opened=1;return (NATIVEFILE *)(uintptr_t)1;}
static int TestMode(NATIVEFILE *fp,const char *p){(void)fp;(void)p;return failure==Mode?-1:0;}
static size_t TestWrite(const void *data,size_t size,size_t count,FILE *fp){(void)fp;if(failure==Bom)return 2;memcpy(output+used,data,size*count);used+=size*count;return count;}
static int TestPuts(const char *s,FILE *fp){(void)fp;if(failure==Rows)return EOF;memcpy(output+used,s,strlen(s));used+=strlen(s);return 0;}
static int TestPutc(int c,FILE *fp){(void)fp;if(failure==Newline)return EOF;output[used++]=(char)c;return c;}
static int TestFlush(FILE *fp){(void)fp;return failure==Flush?-1:0;}
static int TestSync(NATIVEFILE *fp){(void)fp;syncCalled++;return failure==Sync?-1:0;}
static int TestClose(FILE *fp){(void)fp;closed=1;return failure==Close?-1:0;}
static int TestRename(const char *a,const char *b,int replace,int sync){(void)a;(void)b;(void)replace;(void)sync;renameCalled++;if(failure==Rename)return -1;published=1;return 0;}
static int TestUnlink(const char *p){(void)p;unlinked++;return cleanupOK?0:-1;}
#define malloc TestMalloc
#define PfNativeCreateTemp TestTemp
#define ICopyNativePermissions TestMode
#define CWriteNative TestWrite
#define IPutNativeString TestPuts
#define ChPutNativeFile TestPutc
#define IFlushNativeFile TestFlush
#define ISyncNativeFile TestSync
#define ICloseNativeFile TestClose
#define IRenameNativePath TestRename
#define IUnlinkNativePath TestUnlink
'''
    stubs = f'#include "{ROOT / "src/platform/ownership.h"}"\n' + stubs
    # A function-like macro avoids rewriting the struct stat type.
    stubs=stubs.replace("#define stat TestStat", "#define stat(path,st) TestStat(path,st)")
    body=function("compat/win32/src/profile.c", "static BOOL FSaveProfile(")
    witnesses=r'''
static int checks;
#define CHECK(x) do {checks++;if(!(x)){fprintf(stderr,"stage %d witness %d failed: %s\n",failure,checks,#x);return 1;}}while(0)
int main(void){
    COMPATPROFILE second={const_cast<char *>("B\n"),NULL},first={const_cast<char *>("A"),&second};int ret;
    for(failure=Ok;failure<=Rename;failure++){
        lastError=77;published=unlinked=opened=closed=syncCalled=renameCalled=0;used=0;cleanupOK=1;
        ret=FSaveProfile(&first,"unused");
        CHECK(ret==(failure==Ok) && published==(failure==Ok));
        CHECK(unlinked==(failure!=Ok && failure!=Allocate && failure!=Temporary));
        CHECK(lastError==((failure==Ok || failure==Bom)?77:failure==Allocate?8:29));
        if(failure==Ok)CHECK(used==7 && !memcmp(output,"\xef\xbb\xbf" "A\nB\n",7));
        if(failure==Flush)CHECK(!syncCalled && !renameCalled);
        if(failure==Sync || failure==Close)CHECK(!renameCalled);
        if(failure==Mode || failure==Bom || failure==Rows || failure==Newline)CHECK(closed);
    }
    failure=Sync;cleanupOK=0;lastError=77;unlinked=published=used=0;
    CHECK(!FSaveProfile(&first,"unused") && unlinked==1 && !published);
    printf("S02 save-control source witnesses: %d passed\n",checks);
    return 0;
}
'''
    with tempfile.TemporaryDirectory(prefix="s02-save-source-") as work:
        path=Path(work);source=path/"witness.cpp";binary=path/"witness"
        source.write_text(stubs+body+witnesses)
        subprocess.run(shlex.split(os.environ.get('CXX', 'c++'))+
                       ["-std=gnu++20", "-include", str(ROOT / "src/platform/value.h"), "-x", "c++","-D_POSIX_C_SOURCE=200809L","-Wall","-Wextra","-Werror", "-Wno-misleading-indentation",str(source),"-o",str(binary)],check=True)
        subprocess.run([str(binary)],check=True)


if __name__=="__main__":
    main()
