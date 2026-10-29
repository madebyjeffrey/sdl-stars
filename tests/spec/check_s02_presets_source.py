#!/usr/bin/env python3
"""Exercise native preset codecs extracted from init.c/mdi.c and structs.h.

INI/text operations are doubles; valid UTF-8 names fit the stored byte capacity.
This checks storage codecs, not M09/M10 preset application or native filesystem I/O.
"""
from pathlib import Path
import os
import shlex
import subprocess
import tempfile

from check_s02_source import ROOT
from check_s02_state_source import between


def main():
    init = (ROOT / "src/ui/init.c").read_text()
    mdi = (ROOT / "src/ui/mdi.c").read_text()
    types = (ROOT / "src/core/structs.h").read_text()
    declarations = r'''
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define TRUE 1
#define idsZiporders 1
#define idsDefault 2
#define mdIdleFactory 7
#define iobjMine 0
'''
    for tag, alias in [("itemaction", "ITEMACTION"), ("taskxport", "TASKXPORT"),
                       ("prodq1", "PRODQ1"), ("ziporder", "ZIPORDER"),
                       ("zipprodq1", "ZIPPRODQ1"), ("zipprodq", "ZIPPRODQ")]:
        start = types.index("struct _" + tag + " {")
        end = types.index("\n};", start) + 3
        declarations += types[start:end] + f"\ntypedef struct _{tag} {alias};\n"
    declarations += r'''
static ZIPORDER vrgZip[4];static ZIPPRODQ vrgZipProd[5];
static struct { int fChgZipOrd,fChgZipProd; } gd;
static const char *transport[4],*production[5];
static char savedTransport[4][80],savedProduction[5][80];
static int writes,checks;
static void CchGetString(int id,char *out) { strcpy(out,id==idsDefault ? "Default" : "ZipOrders"); }
static void GetPrivateProfileString(const char *section,const char *key,const char *fallback,char *out,unsigned cap,const char *file) {
    const char *s=key[9]=='P' ? production[key[10]-'1'] : transport[key[9]-'1'];size_t n=strlen(s);
    (void)section;(void)fallback;(void)file;
    if(n>=cap)n=cap-1;memcpy(out,s,n);out[n]=0;
}
static int WritePrivateProfileString(const char *section,const char *key,const char *value,const char *file) {
    (void)section;(void)file;writes++;
    strcpy(key[9]=='P' ? savedProduction[key[10]-'1'] : savedTransport[key[9]-'1'],value);
    return 0; // Source ignores even every write failing.
}
static void reset(void) {
    int i;for(i=0;i<4;i++){transport[i]="";savedTransport[i][0]=0;}
    for(i=0;i<5;i++){production[i]="";savedProduction[i][0]=0;}
    writes=0;gd.fChgZipOrd=gd.fChgZipProd=0;
}
#define CHECK(x) do { checks++;if(!(x)){fprintf(stderr,"witness %d failed: %s\n",checks,#x);return 1;} } while(0)
'''
    common = 'int16_t i,iPass,cch,cpq;char szSection[32]="ZipOrders",szEntry[32],szWork[256],szIniFile[16]="stars.ini",*psz;'
    reader = between(init, "    CchGetString(idsZiporders, szSection);\n    memset(vrgZip", "    return;\n}")
    writer = between(mdi, "    if (gd.fChgZipOrd) {", "    return;\n}")
    bodies = "static void readPresets(void) {" + common + reader + "}\n"
    bodies += "static void writePresets(void) {" + common + "(void)cch;(void)cpq;" + writer + "}\n"
    witnesses = r'''
int main(void) {
    unsigned w;char encoded[7]="abaaaa";
    CHECK(sizeof(vrgZip)==96 && sizeof(vrgZipProd)==200);
    reset();readPresets();CHECK(vrgZipProd[0].fValid && !vrgZipProd[1].fValid && !vrgZip[0].fValid && strcmp(vrgZipProd[0].szName,"Default")==0);
    reset();transport[0]="pbadaaaaaaaaaaaaaaaaName";readPresets();
    CHECK(vrgZip[0].fValid && vrgZip[0].txp.rgia[0].iAction==15 && vrgZip[0].txp.rgia[0].cQuan==769 && strcmp(vrgZip[0].szName,"Name")==0);
    gd.fChgZipOrd=1;writePresets();CHECK(writes==4 && strcmp(savedTransport[0],transport[0])==0 && savedTransport[1][0]==0);
    reset();transport[0]="qaaaaaaaaaaaaaaaaaaa";transport[1]="aaaaaaaaaaaaaaaaaaa";transport[2]="aaaaaaaaaaaaaaaaaaaa1234567890123";readPresets();
    CHECK(!vrgZip[0].fValid && !vrgZip[1].fValid && !vrgZip[2].fValid);
    reset();production[0]="pbbaaaN";production[1]="abppppx";production[2]="aax";production[3]="aa";production[4]="anx";readPresets();
    CHECK(vrgZipProd[0].fValid && vrgZipProd[0].fNoResearch && vrgZipProd[0].cpq==1 && vrgZipProd[0].rgpq[0].w==1 && strcmp(vrgZipProd[0].szName,"Default")==0);
    CHECK(vrgZipProd[1].fValid && vrgZipProd[1].rgpq[0].w==64 && !vrgZipProd[1].fNoResearch);
    CHECK(vrgZipProd[2].fValid && vrgZipProd[2].cpq==0 && strcmp(vrgZipProd[2].szName,"x")==0);
    CHECK(!vrgZipProd[3].fValid && !vrgZipProd[4].fValid);
    gd.fChgZipProd=1;writePresets();CHECK(writes==5 && strcmp(savedProduction[0],"bbbaaaDefault")==0 && strcmp(savedProduction[1],"abaeaax")==0 && savedProduction[3][0]==0);
    reset();production[0]="!";readPresets();CHECK(vrgZipProd[0].fValid && vrgZipProd[0].cpq==0 && !vrgZipProd[0].fNoResearch);
    writePresets();CHECK(writes==0);
    vrgZipProd[1].fValid=1;gd.fChgZipProd=1;writePresets();CHECK(strcmp(savedProduction[1],"aa")==0);
    production[1]=savedProduction[1];readPresets();CHECK(!vrgZipProd[1].fValid);
    // Exhaust every packed PRODQ1 word through the actual reader and writer.
    reset();production[1]=encoded;
    for(w=0;w<65536;w++) {
        encoded[2]=(char)('a'+(w&15));encoded[3]=(char)('a'+((w>>4)&15));
        encoded[4]=(char)('a'+((w>>8)&15));encoded[5]=(char)('a'+((w>>12)&15));
        readPresets();
        CHECK(vrgZipProd[1].fValid && vrgZipProd[1].rgpq[0].mdIdle==((w&63)>=7 ? 0 : (w&63)) && vrgZipProd[1].rgpq[0].cQuan==((w>>6)>1020 ? 1 : (w>>6)));
        gd.fChgZipProd=1;writePresets();
        CHECK(savedProduction[1][2]=='a'+(vrgZipProd[1].rgpq[0].w&15) && savedProduction[1][3]=='a'+((vrgZipProd[1].rgpq[0].w>>4)&15) && savedProduction[1][4]=='a'+((vrgZipProd[1].rgpq[0].w>>8)&15) && savedProduction[1][5]=='a'+((vrgZipProd[1].rgpq[0].w>>12)&15));
    }
    printf("S02 source-extracted preset witnesses: %d assertions passed (including all 65536 packed production words)\n",checks);
    return 0;
}
'''
    with tempfile.TemporaryDirectory(prefix="s02-presets-") as work:
        source = Path(work) / "witness.cpp"
        binary = Path(work) / "witness"
        source.write_text(declarations + bodies + witnesses)
        subprocess.run(shlex.split(os.environ.get("CXX", "c++")) +
                       ["-std=gnu++20", "-include", str(ROOT / "src/platform/value.h"), "-x", "c++", "-Wall", "-Wextra", "-Werror", "-Wno-misleading-indentation", "-fsanitize=undefined", "-fno-sanitize-recover=undefined",
                        str(source), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()
