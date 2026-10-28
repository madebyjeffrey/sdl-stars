#include "fileio.h"
#include "filesystem.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define CHECK(f) do { if (!(f)) { fprintf(stderr,"line %d: %s (errno %d)\n",__LINE__,#f,errno); return 1; } } while (0)
int main(void) {
    unsigned char rgb[] = {0, 255, 26, 13, 10, 128}, out[16];
    char dir[4096], path[8192], other[8192];
    NATIVEFILEINFO info;
    CHECK(FFilesystemTempDir(dir, sizeof(dir)));
    CHECK(snprintf(path, sizeof(path), "%s/caf\xc3\xa9-XXXXXX", dir) > 0);
    NATIVEFILE *temp = PfNativeCreateTemp(path);
    CHECK(temp && CWriteNative(rgb, 1, sizeof(rgb), temp) == sizeof(rgb));
    CHECK(ISyncNativeFile(temp) == 0 && ICloseNativeFile(temp) == 0);
    NATIVEFILE *pf = PfNativeOpen(path,"r+b");
    CHECK(pf && CbNativeFile(pf) == 6 && LNativePosition(pf) == 0);
    CHECK(CReadNative(out,2,4,pf) == 3 && !memcmp(out,rgb,6) && FNativeStreamEof(pf) && !FNativeStreamError(pf));
    CHECK(CbNativeFile(pf) == 6 && LNativePosition(pf) == 6 && FNativeStreamEof(pf));
    CHECK(ISeekNativeFile(pf,2,0)==0 && !FNativeStreamEof(pf));
    CHECK(ChPutNativeFile(42,pf)==42 && ISeekNativeFile(pf,2,0)==0 && ChGetNativeFile(pf)==42);
    CHECK(ICloseNativeFile(pf)==0);
    errno=0;CHECK(!PfNativeOpen(path,"wbx") && errno==EEXIST);
    pf=PfNativeOpenFlags(path,2,0,0,0,0,1);CHECK(pf);
    CHECK(CbWriteNativeFile(pf,"Z",1)==1 && CbNativeFile(pf)==6);
    CHECK(LSeekNativeFile(pf,2,0)==2 && ITruncateNativeFile(pf,2)==0 && CbNativeFile(pf)==2);
    CHECK(LSeekNativeFile(pf,8,0)==8 && ITruncateNativeFile(pf,8)==0 && CbNativeFile(pf)==8);
    uint64_t dev,ino;CHECK(FNativeFileIdentity(pf,NULL,&dev,&ino));CHECK(ICloseNativeFile(pf)==0);
    pf=PfNativeOpen(path,"rb");CHECK(pf && CReadNative(out,1,8,pf)==8 && out[0]=='Z' && !out[7]);CHECK(ICloseNativeFile(pf)==0);
    pf=PfNativeOpen(path,"ab");CHECK(pf && ISeekNativeFile(pf,0,0)==0 && ChPutNativeFile('A',pf)=='A' && ICloseNativeFile(pf)==0);
    CHECK(IGetNativeFileInfo(path,&info,1)==0 && info.cb==9 && info.fRegular);
    pf=PfNativeOpen(path,"rb");CHECK(pf && ISeekNativeFile(pf,8,0)==0 && ChGetNativeFile(pf)=='A');CHECK(ICloseNativeFile(pf)==0);
    snprintf(other,sizeof(other),"%s/renamed",dir);
    CHECK(IRenameNativePath(path,other,1,0)==0);
    NATIVEDIR *pd=PdirNativeOpen(dir);CHECK(pd);int found=0;const char *entry;
    while((entry=PszNativeReadDir(pd)))if(!strcmp(entry,"renamed"))found++;
    CHECK(found==1 && !errno && !PszNativeReadDir(pd) && !errno && ICloseNativeDir(pd)==0);
    CHECK(IRemoveNativePath(other,0)==0 && IRemoveNativePath(dir,1)==0);
    errno=0;CHECK(!PfNativeOpen("\xff","rb") && errno==EILSEQ);
    pf=PfNativeTemporary();CHECK(pf && CchNativePrintf(pf,"2\nname\n7\n1234567890123\nnext\n")>0 && ISeekNativeFile(pf,0,0)==0);
    int version,player;long long played;char line[64];
    CHECK(CNativeScan(pf,"%d\n",&version)==1 && version==2 && PszGetNativeLine(line,sizeof(line),pf) && !strcmp(line,"name\n"));
    CHECK(CNativeScan(pf,"%d\n%lld\n",&player,&played)==2 && player==7 && played==1234567890123LL);
    CHECK(PszGetNativeLine(line,sizeof(line),pf) && !strcmp(line,"next\n"));CHECK(ICloseNativeFile(pf)==0);
    puts("File service binary, read/write transition, EOF/seek, exclusive creation, live sizes, truncate/extend, identity and metadata tests passed");
    return 0;
}
