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
from check_s02_source import function


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
enum {Ok,Allocate,Temporary,Mode,Stream,Bom,Rows,Newline,Flush,Sync,Close,Rename};
static int failure,lastError,published,unlinked,cleanupOK,opened,closed,syncCalled,renameCalled;
static char output[256];static size_t used;
static void SetLastError(int e){lastError=e;}
static int DwProfileError(void){return 29;}
static void *TestMalloc(size_t n){return failure==Allocate?NULL:malloc(n);}
static int TestTemp(char *p){(void)p;if(failure==Temporary)return -1;opened=1;return 9;}
static int TestStat(const char *p,struct stat *s){(void)p;s->st_mode=0640;return 0;}
static int TestChmod(int fd,mode_t mode){if(fd!=9 || mode!=0640)abort();return failure==Mode?-1:0;}
static int TestCloseFd(int fd){(void)fd;closed=1;return 0;}
static FILE *TestOpen(int fd,const char *mode){if(fd!=9 || strcmp(mode,"wb"))abort();return failure==Stream?NULL:(FILE *)(uintptr_t)1;}
static size_t TestWrite(const void *data,size_t size,size_t count,FILE *fp){(void)fp;if(failure==Bom)return 2;memcpy(output+used,data,size*count);used+=size*count;return count;}
static int TestPuts(const char *s,FILE *fp){(void)fp;if(failure==Rows)return EOF;memcpy(output+used,s,strlen(s));used+=strlen(s);return 0;}
static int TestPutc(int c,FILE *fp){(void)fp;if(failure==Newline)return EOF;output[used++]=(char)c;return c;}
static int TestFlush(FILE *fp){(void)fp;return failure==Flush?-1:0;}
static int TestSync(int fd){(void)fd;syncCalled++;return failure==Sync?-1:0;}
static int TestClose(FILE *fp){(void)fp;closed=1;return failure==Close?-1:0;}
static int TestRename(const char *a,const char *b){(void)a;(void)b;renameCalled++;if(failure==Rename)return -1;published=1;return 0;}
static int TestUnlink(const char *p){(void)p;unlinked++;return cleanupOK?0:-1;}
#define malloc TestMalloc
#define mkstemp TestTemp
#define stat TestStat
#define fchmod TestChmod
#define close TestCloseFd
#define fdopen TestOpen
#define fwrite TestWrite
#define fputs TestPuts
#define fputc TestPutc
#define fflush TestFlush
#define fsync TestSync
#define fclose TestClose
#define rename TestRename
#define unlink TestUnlink
'''
    # A function-like macro avoids rewriting the struct stat type.
    stubs=stubs.replace("#define stat TestStat", "#define stat(path,st) TestStat(path,st)")
    body=function("compat/win32/src/profile.c", "static BOOL FSaveProfile(")
    witnesses=r'''
static int checks;
#define CHECK(x) do {checks++;if(!(x)){fprintf(stderr,"stage %d witness %d failed: %s\n",failure,checks,#x);return 1;}}while(0)
int main(void){
    COMPATPROFILE second={"B\n",NULL},first={"A",&second};int ret;
    for(failure=Ok;failure<=Rename;failure++){
        lastError=77;published=unlinked=opened=closed=syncCalled=renameCalled=0;used=0;cleanupOK=1;
        ret=FSaveProfile(&first,"unused");
        CHECK(ret==(failure==Ok) && published==(failure==Ok));
        CHECK(unlinked==(failure!=Ok && failure!=Allocate));
        CHECK(lastError==((failure==Ok || failure==Bom)?77:failure==Allocate?8:29));
        if(failure==Ok)CHECK(used==7 && !memcmp(output,"\xef\xbb\xbf" "A\nB\n",7));
        if(failure==Flush)CHECK(!syncCalled && !renameCalled);
        if(failure==Sync || failure==Close)CHECK(!renameCalled);
        if(failure==Mode || failure==Stream || failure==Bom || failure==Rows || failure==Newline)CHECK(closed);
    }
    failure=Sync;cleanupOK=0;lastError=77;unlinked=published=used=0;
    CHECK(!FSaveProfile(&first,"unused") && unlinked==1 && !published);
    printf("S02 save-control source witnesses: %d passed\n",checks);
    return 0;
}
'''
    with tempfile.TemporaryDirectory(prefix="s02-save-source-") as work:
        path=Path(work);source=path/"witness.c";binary=path/"witness"
        source.write_text(stubs+body+witnesses)
        subprocess.run(shlex.split(os.environ.get("CC","cc"))+
                       ["-std=c11","-D_POSIX_C_SOURCE=200809L","-Wall","-Wextra","-Werror",str(source),"-o",str(binary)],check=True)
        subprocess.run([str(binary)],check=True)


if __name__=="__main__":
    main()
