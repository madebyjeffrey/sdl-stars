#!/usr/bin/env python3
"""Source-extracted profile buffers and host grammar with bounded input stubs.

UTF-8 decoding, profile line/lookup/buffer code, and host parsing come directly
from current source. Filesystem loading and localization are fixture adapters.
"""
from pathlib import Path
import os
import shlex
import subprocess
import tempfile
from check_s02_source import ROOT, function


def main():
    declarations = r'''
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <atomic>
#define TRUE 1
#define FALSE 0
#define WINAPI
typedef int BOOL;
typedef const char *LPCSTR;
typedef char *LPSTR;
typedef uint32_t DWORD;
typedef int StringId;
typedef struct COMPATPROFILE {
    char *szRaw,*szParsed,*pszSection,*pszKey,*pszValue;
    struct COMPATPROFILE *lpNext;
} COMPATPROFILE;
static NATIVEFLAGLOCK fProfileLock;
static const char *const *fixture;
static int loadOK=1;
static void CchGetString(StringId id,char *out) {strcpy(out,id==1?"A":"k");}
static char *SzProfilePath(const char *path) {return path?strdup(path):NULL;}
static int checks;
#define CHECK(x) do {checks++;if(!(x)){fprintf(stderr,"witness %d failed: %s\n",checks,#x);return 1;}}while(0)
'''
    declarations = (f'#include "{ROOT / "src/platform/locking.h"}"\n'
                    f'#include "{ROOT / "src/platform/ownership.h"}"\n') + declarations
    bodies = "\n".join(
        function("src/platform/encoding.c", signature) for signature in
        ["int CbUtf8Char(", "size_t CchUtf8Prefix(", "int FUtf8Valid("])
    bodies += "\n" + "\n".join(
        function("compat/win32/src/profile.c", signature) for signature in
        ["static void FreeProfile(", "static char *PszTrimProfile(",
         "static COMPATPROFILE *LpProfileLine(", "static COMPATPROFILE *LpProfileSection(",
         "static COMPATPROFILE *LpProfileKey(", "static const char *PszProfileValue("])
    bodies += r'''
static BOOL FLoadProfile(const char *path,COMPATPROFILE **out) {
    COMPATPROFILE **next=out; unsigned i; (void)path; *out=NULL;
    if(!loadOK)return FALSE;
    for(i=0;fixture[i];i++){*next=LpProfileLine(fixture[i]); if(!*next)abort(); next=&(*next)->lpNext;}
    return TRUE;
}
'''
    bodies += function("compat/win32/src/profile.c", "DWORD WINAPI GetPrivateProfileStringA(")
    bodies += "\n" + "\n".join(
        function("src/host/host.c", signature) for signature in
        ["static int16_t FSzEqNoCase(", "static int16_t FIniValue(", "static int32_t LIniInt("])
    witnesses = r'''
int main(void) {
    static const char *const one[]={"[AB]\n",NULL};
    static const char *const unicode[]={"[\xc3\xa9]\n",NULL};
    static const char *const duplicate[]={"[A]\n","x=first\n","[A]\n","k=second\n",NULL};
    static const char *const quoted[]={"[A]\n","k=' a '\n",NULL};
    static const char *const empty[]={NULL};
    char out[16],longline[320];
    CHECK(CchUtf8Prefix("A\xc3\xa9" "B",2)==1);
    CHECK(CchUtf8Prefix("A\xc3\xa9" "B",3)==3);
    CHECK(CchUtf8Prefix("A\xff" "B",8)==1);
    CHECK(CchUtf8Prefix("A\0B",8)==1);
    CHECK(!FUtf8Valid("A\0B",3));
    fixture=one;memset(out,9,sizeof(out));
    CHECK(GetPrivateProfileStringA(NULL,NULL,"",out,4,"x")==2 && !memcmp(out,"AB\0\0",4));
    CHECK(out[4]==9);
    fixture=unicode;memset(out,9,sizeof(out));
    CHECK(GetPrivateProfileStringA(NULL,NULL,"",out,3,"x")==1 && out[0]==0 && out[1]==0 && out[2]==0 && out[3]==9);
    fixture=empty;memset(out,9,sizeof(out));
    CHECK(GetPrivateProfileStringA(NULL,NULL,"",out,3,"x")==0 && out[0]==0 && out[1]==0 && out[2]==9);
    fixture=quoted;memset(out,9,sizeof(out));
    CHECK(GetPrivateProfileStringA("A","k","",out,3,"x")==2 && !strcmp(out," a"));
    fixture=duplicate;
    CHECK(GetPrivateProfileStringA("A","k","fallback",out,sizeof(out),"x")==8 && !strcmp(out,"fallback"));
    CHECK(FIniValue("[A]\nx=first\n[A]\nk=second",1,2,out,sizeof(out)) && !strcmp(out,"second"));
    CHECK(!FIniValue(" [A]\nk=1",1,2,out,sizeof(out)));
    CHECK(!FIniValue("[ A ]\nk=1",1,2,out,sizeof(out)));
    CHECK(!FIniValue("[A]\n k=1",1,2,out,sizeof(out)));
    CHECK(FIniValue("[A]\n[broken\nk=1",1,2,out,sizeof(out)) && !strcmp(out,"1"));
    CHECK(FIniValue("[A]\nk= '2'  ",1,2,out,sizeof(out)) && !strcmp(out,"'2'"));
    CHECK(FIniValue("[A]\nk=abcd",1,2,out,3) && !strcmp(out,"ab"));
    CHECK(LIniInt("[A]\nk=0x2",1,2,7)==0);
    CHECK(LIniInt("[A]\nk='2'",1,2,7)==0);
    CHECK(LIniInt("[A]\nk=",1,2,7)==0);
    CHECK(LIniInt("[A]\nx=",1,2,7)==7);
    CHECK(LIniInt("[A]\nk=-12suffix",1,2,7)==-12);
    strcpy(longline,"[A]\nk=");memset(longline+6,'a',270);strcpy(longline+276,"\nk=second");
    CHECK(FIniValue(longline,1,2,out,4) && !strcmp(out,"aaa"));
    loadOK=0;
    CHECK(GetPrivateProfileStringA("A","k","fallback",out,sizeof(out),"x")==8 && !strcmp(out,"fallback"));
    printf("S02 profile I/O and host source witnesses: %d passed\n",checks);
    return 0;
}
'''
    with tempfile.TemporaryDirectory(prefix="s02-io-source-") as work:
        path=Path(work)
        source=path/"witness.cpp"
        binary=path/"witness"
        source.write_text(declarations+bodies+witnesses)
        subprocess.run(shlex.split(os.environ.get('CXX', 'c++'))+
                       ["-std=gnu++20", "-DSTARS_TEST_STATE_DOUBLE", "-include", str(ROOT / "src/platform/value.h"), "-x", "c++","-D_POSIX_C_SOURCE=200809L","-Wall","-Wextra","-Werror", "-Wno-misleading-indentation",str(source),"-o",str(binary)],check=True)
        subprocess.run([str(binary)],check=True)


if __name__=="__main__":
    main()
