#!/usr/bin/env python3
"""S02 state witnesses against extracted C; profile/path operations are doubles.

Runs defined-domain inputs only. This checks native field widths and partial
effects; it does not establish real filesystem outcomes or whole-program refinement.
"""
from pathlib import Path
import os
import shlex
import subprocess
import tempfile

from check_s02_source import ROOT, function


def between(source, start, end):
    begin = source.index(start)
    return source[begin:source.index(end, begin)]


def main():
    init = (ROOT / "src/ui/init.c").read_text()
    selection = between(init, "    if (cch < 3) {", "    CchGetString(idsMessage, szEntry);")
    identity = between(init, "    ini.lid = 0;", "    CchGetString(idsScanzoom, szEntry);")
    recent = between(init, "    if (cch > 3) {", "    CchGetString(idsTurn, szEntry);")
    declarations = r'''
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdarg.h>
#define TRUE 1
#define FALSE 0
#define htPerm 0
#define idsStarsIni 1
#define idsFiles 2
#define idsFile1 3
#define _stricmp strcasecmp
static struct { uint16_t grobjSel:4, fStartupFile:1; int16_t idPlayer,iObjSel; int32_t lid; } ini;
static char memory[9][256], disk[9][256];
static char *vrgszMRU=(char *)memory;
static char szBase[256];
static const char *inputs[9],*startupInput;
static int attempts,failAt;
static int16_t under[9];
static void *LpAlloc(size_t n,int kind) { (void)kind; if(n!=sizeof(memory))abort(); return memory; }
template<class T> static T *LpAlloc(size_t n,int kind) { return static_cast<T *>(LpAlloc(n,kind)); }
static void CchGetString(int id,char *out) { strcpy(out,id==idsFile1 ? "File1" : id==idsFiles ? "Files" : "stars.ini"); }
static int CchSnprintf(char *out,size_t cap,const char *format,...) {
    va_list args; int n; va_start(args,format);n=vsnprintf(out,cap,format,args);va_end(args);return n;
}
static unsigned GetPrivateProfileString(const char *section,const char *key,const char *fallback,char *out,unsigned cap,const char *file) {
    const char *s=inputs[key[4]-'1']; size_t n=strlen(s);
    (void)section;(void)fallback;(void)file;
    if(n>=cap)n=cap-1;memcpy(out,s,n);out[n]=0;return (unsigned)n;
}
static int WritePrivateProfileString(const char *section,const char *key,const char *value,const char *file) {
    (void)section;(void)file;attempts++;
    if(attempts==failAt)return 0;
    strcpy(disk[key[4]-'1'],value);return 1;
}
static int FPathUnderLibrary(const char *directory,const char *path) {
    int i;(void)directory;
    for(i=0;i<9;i++)if(strcmp(path,memory[i])==0)return under[i];
    return 0;
}
static void reset(void) {
    int i;static const char *names[]={"aaaa","bbbb","cccc","dddd","eeee","ffff","gggg","hhhh","iiii"};
    for(i=0;i<9;i++){strcpy(memory[i],names[i]);strcpy(disk[i],names[i]);under[i]=0;inputs[i]="";}
    attempts=0;failAt=0;startupInput=NULL;vrgszMRU=(char *)memory;ini.grobjSel=2;ini.idPlayer=7;ini.iObjSel=90;
}
static int checks;
#define CHECK(x) do { checks++;if(!(x)){fprintf(stderr,"witness %d failed: %s\n",checks,#x);return 1;} } while(0)
'''
    bodies = (
        "static void selection(const char *szWork) { int cch=(int)strlen(szWork);\n" + selection + "}\n"
        + "static void identity(const char *szWork) { int cch=(int)strlen(szWork),i;\n" + identity + "}\n"
        + 'static void recent(void) { const char *first=startupInput ? startupInput : inputs[0]; int cch=(int)strlen(first),i,iPass; char szWork[256],szEntry[16]="File1",szSection[16]="Files",szIniFile[16]="stars.ini",*psz; strcpy(szWork,first);\n'
        + recent + "}\n"
        + function("src/ui/fileui.c", "void AddMRUFile(") + "\n"
        + function("src/ui/libraryui.c", "static int16_t FDeleteLibraryGameRecentPaths(")
    )
    witnesses = r'''
int main(void) {
    int k;
    reset(); selection("N"); CHECK(ini.grobjSel==0 && ini.idPlayer==7 && ini.iObjSel==90);
    reset(); selection("NQx"); CHECK(ini.grobjSel==0 && ini.idPlayer==15 && ini.iObjSel==90);
    reset(); selection("PA3"); CHECK(ini.grobjSel==0 && ini.idPlayer==7 && ini.iObjSel==90);
    reset(); selection("SB-1x"); CHECK(ini.grobjSel==2 && ini.idPlayer==0 && ini.iObjSel==-1);
    reset(); selection("PBx"); CHECK(ini.grobjSel==1 && ini.idPlayer==0 && ini.iObjSel==0);
    reset(); selection("PB65536"); CHECK(ini.grobjSel==1 && ini.iObjSel==0);
    identity("1A"); CHECK(ini.lid==16);
    identity("1a"); CHECK(ini.lid==26);
    identity("7fffffff"); CHECK(ini.lid==2147483647);
    reset();strcpy(szBase,"prior");inputs[0]=".";inputs[1]="bbbb";recent();
    CHECK(!ini.fStartupFile && strcmp(szBase,"prior")==0 && strcmp(memory[0],"bbbb")==0 && memory[1][0]==0);
    reset();startupInput="aaaa";inputs[0]=".";inputs[1]="bbbb";recent();
    CHECK(ini.fStartupFile && strcmp(szBase,"aaaa")==0 && strcmp(memory[0],"bbbb")==0);
    reset();inputs[0]="aaaa";inputs[2]="cc";inputs[8]="iiii";recent();
    CHECK(ini.fStartupFile && strcmp(szBase,"aaaa")==0 && strcmp(memory[1],"iiii")==0 && memory[8][0]==0);
    reset();strcpy(memory[0],"a.m1");AddMRUFile(const_cast<char *>("A"),const_cast<char *>("M1")); CHECK(attempts==0 && strcmp(memory[0],"a.m1")==0);
    reset();strcpy(memory[2],"c.m1");AddMRUFile(const_cast<char *>("c"),const_cast<char *>("m1"));
    CHECK(attempts==9 && strcmp(memory[0],"c.m1")==0 && strcmp(memory[2],"bbbb")==0 && strcmp(memory[8],"iiii")==0);
    reset();failAt=2;AddMRUFile(const_cast<char *>("j"),const_cast<char *>("m1"));
    CHECK(attempts==9 && strcmp(memory[0],"j.m1")==0 && strcmp(memory[8],"hhhh")==0 && strcmp(disk[1],"bbbb")==0 && strcmp(disk[2],"bbbb")==0);
    reset();vrgszMRU=NULL;AddMRUFile(const_cast<char *>("j"),const_cast<char *>("m1"));CHECK(attempts==0);
    reset();under[0]=1; CHECK(FDeleteLibraryGameRecentPaths("unused"));
    CHECK(attempts==10 && memory[0][0]==0 && strcmp(memory[1],"bbbb")==0 && disk[0][0]==0);
    reset();under[2]=1; CHECK(FDeleteLibraryGameRecentPaths("unused"));
    CHECK(attempts==9 && strcmp(memory[2],"dddd")==0 && strcmp(memory[7],"iiii")==0 && memory[8][0]==0);
    for(k=1;k<=10;k++) {
        reset();under[0]=1;failAt=k;
        CHECK(!FDeleteLibraryGameRecentPaths("unused") && attempts==k && memory[0][0]==0);
        CHECK(k==1 ? strcmp(disk[0],"aaaa")==0 : disk[0][0]==0);
    }
    reset();under[2]=1;failAt=3;
    CHECK(!FDeleteLibraryGameRecentPaths("unused") && attempts==3 && strcmp(memory[2],"dddd")==0 && strcmp(disk[2],"cccc")==0 && strcmp(memory[3],"dddd")==0);
    reset();vrgszMRU=NULL; CHECK(FDeleteLibraryGameRecentPaths("unused") && attempts==0);
    printf("S02 source-extracted state witnesses: %d passed\n",checks);
    return 0;
}
'''
    with tempfile.TemporaryDirectory(prefix="s02-state-") as work:
        source = Path(work) / "witness.cpp"
        binary = Path(work) / "witness"
        source.write_text(declarations + bodies + witnesses)
        subprocess.run(shlex.split(os.environ.get("CXX", "c++")) +
                       ["-std=gnu++20", "-DSTARS_TEST_STATE_DOUBLE", "-include", str(ROOT / "src/platform/value.h"), "-x", "c++", "-Wall", "-Wextra", "-Werror", "-Wno-misleading-indentation", "-fsanitize=undefined", "-fno-sanitize-recover=undefined",
                        str(source), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()
