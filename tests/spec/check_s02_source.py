#!/usr/bin/env python3
"""Run S02 witnesses against source-extracted C functions, without game edits.

Only profile reads and layout storage are stubbed. This is a focused adapter
check, not a whole application/INI/OS test. The assertions mirror named Dafny
witness inputs; the algorithms under test are extracted from current source.
"""
from pathlib import Path
import os
import re
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def enum_source():
    # Temporary declaration selection while the main header still serves C fixtures.
    text = (ROOT / "src/core/enums.h").read_text()
    return re.sub(r"#ifdef __cplusplus\n(enum \w+ : \w+ \{)\n#else\nenum \w+ \{\n#endif", r"\1", text)


ENUM_STORAGE = dict(re.findall(r"enum (\w+) : (\w+) \{", enum_source()))


def function(path, signature):
    text = (ROOT / path).read_text()
    start = text.index(signature)
    end = text.index("\n}", start) + 2
    return text[start:end]


def main():
    declarations = r'''
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#define TRUE 1
#define FALSE 0
#define WINAPI
#define idsMain 1
typedef int StringId;
typedef uint32_t UINT;
typedef int32_t INT;
typedef int BOOL;
typedef const char *LPCSTR;
typedef struct { int32_t left,top,right,bottom; } RECT;
typedef struct { RECT rc; uint16_t fMaximized:1,fMinimized:1,fInitalized:1; } WN;
typedef struct { int other; uint16_t iCol:3,id:4,fPopped:1; } TILE;
static char szWork[512];
static const char *input;
static void CchGetString(StringId id,char *out) { (void)id; strcpy(out,"key"); }
static unsigned GetPrivateProfileStringA(const char *section,const char *key,const char *fallback,char *out,unsigned cap,const char *file) {
    size_t n=strlen(input); (void)section; (void)key; (void)fallback; (void)file;
    if(n>=cap)n=cap-1;
    memcpy(out,input,n);out[n]=0;return (unsigned)n;
}
#define GetPrivateProfileString GetPrivateProfileStringA
static int checks;
#define CHECK(x) do { checks++; if(!(x)){fprintf(stderr,"witness %d failed: %s\n",checks,#x);return 1;} } while(0)
'''
    bodies = "\n".join([
        function("src/ui/init.c", "void GetIniWinRc("),
        function("src/ui/init.c", "void ReadIniTileSettings("),
        function("compat/win32/src/profile.c", "UINT WINAPI GetPrivateProfileIntA("),
    ])
    witnesses = r'''
int main(void) {
    TILE tiles[3]={{10,0,0,0},{20,0,1,0},{30,0,2,0}};
    TILE original[3]; WN wn; unsigned i;
    static const char *strings[]={"","nonnumeric"," ","-0x10tail","077","0x","4294967296","+42x","-1"};
    static const uint32_t expected[]={7,0,0,4294967280u,77,0,0,42,4294967295u};
    memcpy(original,tiles,sizeof(tiles));
    ReadIniTileSettings(const_cast<char *>("C"),tiles,3);
    CHECK(tiles[0].id==2 && tiles[1].id==1 && tiles[2].id==0);
    CHECK(tiles[0].fPopped && tiles[0].other==30 && tiles[2].other==10);
    memcpy(tiles,original,sizeof(tiles)); ReadIniTileSettings(const_cast<char *>("Cc"),tiles,3);
    CHECK(tiles[0].id==2 && tiles[0].fPopped && tiles[1].id==1 && tiles[2].id==0);
    memcpy(tiles,original,sizeof(tiles)); ReadIniTileSettings(const_cast<char *>("*?*b"),tiles,3);
    CHECK(tiles[0].id==1 && tiles[1].id==0 && tiles[2].id==2);
    CHECK(tiles[0].iCol==1 && tiles[1].iCol==1 && tiles[2].iCol==1);
    memcpy(tiles,original,sizeof(tiles)); tiles[2].iCol=3; ReadIniTileSettings(const_cast<char *>("*"),tiles,3);
    CHECK(tiles[2].iCol==3);
    input="X";GetIniWinRc(const_cast<char *>("Windows"),const_cast<char *>("unused"),idsMain,&wn);
    CHECK(wn.rc.left==-32768 && wn.rc.right==-32768 && wn.rc.top==0 && wn.rc.bottom==0);
    CHECK(wn.fMaximized && !wn.fMinimized && !wn.fInitalized);
    input="R1-2-000000010002";GetIniWinRc(const_cast<char *>("Windows"),const_cast<char *>("unused"),0,&wn);
    CHECK(wn.rc.left==-12 && wn.rc.top==0 && wn.rc.right==1 && wn.rc.bottom==2);
    CHECK(wn.fInitalized && !wn.fMaximized && !wn.fMinimized);
    input="M0000000010000001";GetIniWinRc(const_cast<char *>("Windows"),const_cast<char *>("unused"),0,&wn);
    CHECK(wn.fMaximized && wn.fInitalized);
    input="R0000000010000001x";GetIniWinRc(const_cast<char *>("Windows"),const_cast<char *>("unused"),0,&wn);
    CHECK(!wn.fInitalized && !wn.fMaximized);
    for(i=0;i<sizeof(expected)/sizeof(expected[0]);i++) {
        input=strings[i]; CHECK(GetPrivateProfileIntA("a","b",7,"unused")==expected[i]);
    }
    printf("S02 source-extracted C witnesses: %d passed\n",checks);
    return 0;
}
'''
    with tempfile.TemporaryDirectory(prefix="s02-source-") as work:
        path = Path(work)
        source = path / "witness.cpp"
        binary = path / "witness"
        source.write_text(declarations + bodies + witnesses)
        subprocess.run(shlex.split(os.environ.get("CXX", "c++")) +
                       ["-std=gnu++20", "-x", "c++", "-Wall", "-Wextra", "-Werror", str(source), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()
