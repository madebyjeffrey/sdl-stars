#include "fileio.h"
#include "filesession.h"
#include "filesystem.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string>
#define CHECK(f) do { if (!(f)) { fprintf(stderr,"line %d: %s (errno %d)\n",__LINE__,#f,errno); return 1; } } while (0)
int main(void) {
    unsigned char rgb[] = {0, 255, 26, 13, 10, 128}, out[16];
    std::uint16_t rgwOut[4];
    char dir[4096], path[8192], other[8192];
    NATIVEFILEINFO info;
    CHECK(FFilesystemTempDir(dir, sizeof(dir)));
    CHECK(snprintf(path, sizeof(path), "%s/caf\xc3\xa9-XXXXXX", dir) > 0);
    NATIVEFILE *temp = PfNativeCreateTemp(path);
    CHECK(temp && CWriteNative(std::span{rgb}, temp) == sizeof(rgb));
    CHECK(ISyncNativeFile(temp) == 0 && ICloseNativeFile(temp) == 0);
    // The bound owner closes every legacy stream and retains token reuse.
    {
        NATIVEFILESESSION session;
        auto *previous = PBindNativeFileSession(&session);
        auto nfr = HfNativeOpenLegacy(path, {.mdShare = NATIVEFILESHARE::denyNone});
        int16_t hf = nfr.hf;
        CHECK(hf == 3 && !nfr.fMissing);
        unsigned char ch = 1;
        CHECK(CbNativeReadLegacy(hf, std::as_writable_bytes(std::span{&ch, 1})) == 1 && ch == 0);
        session.Close(); session.Close();
        errno = 0;
        CHECK(!CbNativeReadLegacy(hf, std::as_writable_bytes(std::span{&ch, 1})) && errno == EBADF);
        nfr = HfNativeOpenLegacy(path, {.mdShare = NATIVEFILESHARE::denyNone});
        CHECK(nfr.hf == 3 && !nfr.fMissing);
        CHECK(PBindNativeFileSession(previous) == &session);
        // The remaining token is released by the actual session destructor.
    }
    NATIVEFILE *pf = PfNativeOpen(path, NATIVEFILEMODE::readWrite);
    CHECK(pf && CbNativeFile(pf) == 6 && LNativePosition(pf) == 0);
    CHECK(CReadNative(std::span{rgwOut}, pf) == 3 && !memcmp(rgwOut,rgb,6) && FNativeStreamEof(pf) && !FNativeStreamError(pf));
    CHECK(CbNativeFile(pf) == 6 && LNativePosition(pf) == 6 && FNativeStreamEof(pf));
    CHECK(ISeekNativeFile(pf, 2, NATIVESEEKORIGIN::begin)==0 && !FNativeStreamEof(pf));
    CHECK(ChPutNativeFile(42,pf)==42 && ISeekNativeFile(pf, 2, NATIVESEEKORIGIN::begin)==0 && ChGetNativeFile(pf)==42);
    CHECK(ICloseNativeFile(pf)==0);
    errno=0;CHECK(!PfNativeOpen(path, NATIVEFILEMODE::writeExclusive) && errno==EEXIST);
    errno=0;CHECK(!PfNativeOpenFlags(path, {.mdAccess = static_cast<NATIVEFILEACCESS>(0)}) && errno==EINVAL);
    pf=PfNativeOpenFlags(path, {.mdAccess = NATIVEFILEACCESS::write, .fNative = true});CHECK(pf);
    CHECK(CbWriteNativeFile(pf, std::as_bytes(std::span{"Z"}).first(1))==1 && CbNativeFile(pf)==6);
    CHECK(LSeekNativeFile(pf, 2, NATIVESEEKORIGIN::begin)==2 && ITruncateNativeFile(pf,2)==0 && CbNativeFile(pf)==2);
    CHECK(LSeekNativeFile(pf, 8, NATIVESEEKORIGIN::begin)==8 && ITruncateNativeFile(pf,8)==0 && CbNativeFile(pf)==8);
    uint64_t dev,ino;CHECK(FNativeFileIdentity(pf,NULL,&dev,&ino));CHECK(ICloseNativeFile(pf)==0);
    pf=PfNativeOpen(path, NATIVEFILEMODE::read);CHECK(pf && CReadNative(std::span{out}.first(8), pf)==8 && out[0]=='Z' && !out[7]);CHECK(ICloseNativeFile(pf)==0);
    pf=PfNativeOpen(path, NATIVEFILEMODE::writeAppend);CHECK(pf && ISeekNativeFile(pf, 0, NATIVESEEKORIGIN::begin)==0 && ChPutNativeFile('A',pf)=='A' && ICloseNativeFile(pf)==0);
    CHECK(IGetNativeFileInfo(path,&info,1)==0 && info.cb==9 && info.fRegular);
    pf=PfNativeOpen(path, NATIVEFILEMODE::read);CHECK(pf && ISeekNativeFile(pf, 8, NATIVESEEKORIGIN::begin)==0 && ChGetNativeFile(pf)=='A');CHECK(ICloseNativeFile(pf)==0);
    // Open policies preserve creation, truncation, append positioning and
    // update access, including failed exclusive opens leaving bytes intact.
    snprintf(other,sizeof(other),"%s/modes",dir);
    pf=PfNativeOpen(other, NATIVEFILEMODE::readWriteTruncate);CHECK(pf && CbNativeFile(pf)==0 && LNativePosition(pf)==0);
    CHECK(CWriteNative(std::span{"ab"}.first(2),pf)==2 && ISeekNativeFile(pf,0,NATIVESEEKORIGIN::begin)==0 && ChGetNativeFile(pf)=='a');
    CHECK(ICloseNativeFile(pf)==0);
    pf=PfNativeOpen(other, NATIVEFILEMODE::readWriteAppend);CHECK(pf && LNativePosition(pf)==2);
    CHECK(ISeekNativeFile(pf,0,NATIVESEEKORIGIN::begin)==0 && ChGetNativeFile(pf)=='a');
    CHECK(ChPutNativeFile('Z',pf)=='Z' && CbNativeFile(pf)==3 && ICloseNativeFile(pf)==0);
    errno=0;CHECK(!PfNativeOpen(other, NATIVEFILEMODE::readWriteExclusive) && errno==EEXIST);
    pf=PfNativeOpen(other, NATIVEFILEMODE::read);CHECK(pf && CbNativeFile(pf)==3 && CReadNative(std::span{out}.first(3),pf)==3 && !memcmp(out,"abZ",3));
    CHECK(ICloseNativeFile(pf)==0 && IUnlinkNativePath(other)==0);
    errno=0;CHECK(!PfNativeOpen(other, NATIVEFILEMODE::read) && errno==ENOENT);
    errno=0;CHECK(!PfNativeOpen(other, NATIVEFILEMODE::readWrite) && errno==ENOENT);
    pf=PfNativeOpen(other, NATIVEFILEMODE::readWriteExclusive);CHECK(pf && CbNativeFile(pf)==0);
    CHECK(CWriteNative(std::span{"cd"}.first(2),pf)==2 && ISeekNativeFile(pf,0,NATIVESEEKORIGIN::begin)==0 && ChGetNativeFile(pf)=='c');
    CHECK(ICloseNativeFile(pf)==0 && IUnlinkNativePath(other)==0);
    pf=PfNativeOpen(other, NATIVEFILEMODE::readWriteAppend);CHECK(pf && CbNativeFile(pf)==0 && LNativePosition(pf)==0 && ChPutNativeFile('A',pf)=='A');
    CHECK(ICloseNativeFile(pf)==0 && IUnlinkNativePath(other)==0);
    errno=0;CHECK(!PfNativeOpen("\xff", static_cast<NATIVEFILEMODE>(255)) && errno==EINVAL);
    snprintf(other,sizeof(other),"%s/renamed",dir);
    CHECK(IRenameNativePath(path,other,1,0)==0);
    NATIVEDIR *pd=PdirNativeOpen(dir);CHECK(pd);int found=0;const char *entry;
    while((entry=PszNativeReadDir(pd)))if(!strcmp(entry,"renamed"))found++;
    CHECK(found==1 && !errno && !PszNativeReadDir(pd) && !errno && ICloseNativeDir(pd)==0);
    CHECK(IRemoveNativePath(other,0)==0 && IRemoveNativePath(dir,1)==0);
    errno=0;CHECK(!PfNativeOpen("\xff", NATIVEFILEMODE::read) && errno==EILSEQ);
    pf=PfNativeTemporary();CHECK(pf && CchNativePrintf(pf,"2\nname\n7\n1234567890123\nnext\n")>0 && ISeekNativeFile(pf, 0, NATIVESEEKORIGIN::begin)==0);
    int version,player;long long played;char line[64];
    CHECK(CNativeScan(pf,"%d\n",&version)==1 && version==2 && PszGetNativeLine(line,sizeof(line),pf) && !strcmp(line,"name\n"));
    CHECK(CNativeScan(pf,"%d\n%lld\n",&player,&played)==2 && player==7 && played==1234567890123LL);
    CHECK(PszGetNativeLine(line,sizeof(line),pf) && !strcmp(line,"next\n"));CHECK(ICloseNativeFile(pf)==0);
    // Byte operations preserve counts, sentinels, EOF and seek failure state.
    {
        unsigned char bytes[8];
        pf=PfNativeTemporary();CHECK(pf);
        CHECK(CbWriteNativeFile(pf, std::as_bytes(std::span{"abcd"}).first(4))==4 && LSeekNativeFile(pf, -2, NATIVESEEKORIGIN::end)==2);
        memset(bytes,0xa5,sizeof(bytes));
        CHECK(CbReadNativeFile(pf, std::as_writable_bytes(std::span{bytes}).subspan(1, 4))==2 && bytes[0]==0xa5 && bytes[1]=='c' && bytes[2]=='d' && bytes[3]==0xa5 && bytes[5]==0xa5);
        CHECK(CbReadNativeFile(pf, std::as_writable_bytes(std::span{bytes}).subspan(3, 1))==0 && bytes[3]==0xa5);
        CHECK(FNativeStreamEof(pf) && LNativePosition(pf)==4 && CbNativeFile(pf)==4 && FNativeStreamEof(pf));
        CHECK(CbReadNativeFile(pf, std::as_writable_bytes(std::span{bytes}).first(static_cast<std::size_t>(0)))==0 && FNativeStreamEof(pf));
        CHECK(LSeekNativeFile(pf, -1, NATIVESEEKORIGIN::current)==3 && !FNativeStreamEof(pf));
        errno=0;CHECK(LSeekNativeFile(pf, 0, static_cast<NATIVESEEKORIGIN>(3))==-1 && errno==EINVAL && LNativePosition(pf)==3);
        CHECK(LSeekNativeFile(pf, 0, NATIVESEEKORIGIN::begin)==0 && CbWriteNativeFile(pf, std::as_bytes(std::span{bytes}).first(static_cast<std::size_t>(0)))==0 && CbNativeFile(pf)==4);
        CHECK(ICloseNativeFile(pf)==0);
        errno=0;CHECK(!CbReadNativeFile(nullptr, std::as_writable_bytes(std::span{bytes}).first(static_cast<std::size_t>(0))) && errno==EBADF);
    }
    // Incomplete elements still expose their transferred prefix, but are not
    // counted as complete objects; neither byte position nor EOF is rolled back.
    {
        std::uint32_t word = 0xa5a5a5a5U;
        const std::uint32_t words[] = {0x01020304U, 0xf0e0d0c0U};
        pf=PfNativeTemporary();CHECK(pf);
        CHECK(CWriteNative(std::span{"abc"}.first(3), pf)==3 && ISeekNativeFile(pf,0,NATIVESEEKORIGIN::begin)==0);
        CHECK(CReadNative(std::span{&word, 1}, pf)==0 && !memcmp(&word,"abc",3));
        CHECK(reinterpret_cast<const unsigned char *>(&word)[3]==0xa5 && LNativePosition(pf)==3 && FNativeStreamEof(pf));
        CHECK(ICloseNativeFile(pf)==0);
        pf=PfNativeTemporary();CHECK(pf);
        CHECK(CWriteNative(std::span{words}, pf)==2 && LNativePosition(pf)==sizeof(words));
        CHECK(ICloseNativeFile(pf)==0);
    }
    // Stream formatting writes binary bytes and measures long output before
    // allocating its second-pass buffer; counts include bytes after a NUL.
    {
        const std::string payload(3072, 'x');
        char expected[4096], actual[4096];
        int count = -1;
        const int cb = snprintf(expected, sizeof(expected), "%ld|%llu|%c%s:end", -123L, 18446744073709551615ULL, 0, payload.c_str());
        pf=PfNativeTemporary();CHECK(pf);
        CHECK(CchNativePrintf(pf,"%ld|%llu|%c%s:end%n",std::int32_t{-123},18446744073709551615ULL,0,payload,&count)==cb && count==cb);
        CHECK(LNativePosition(pf)==cb && ISeekNativeFile(pf, 0, NATIVESEEKORIGIN::begin)==0);
        CHECK(CReadNative(std::span{actual}, pf)==static_cast<std::size_t>(cb) && !memcmp(actual,expected,cb));
        CHECK(ICloseNativeFile(pf)==0);
        pf=PfNativeTemporary();CHECK(pf);
        count=-1;
        CHECK(CchNativePrintf(pf,"ab%n%c",&count,0)==3 && count==2 && CbNativeFile(pf)==3);
        errno=0;CHECK(CchNativePrintf(pf,"prefix %d")==-1 && errno==EINVAL && CbNativeFile(pf)==3);
        errno=0;CHECK(CchNativePrintf(pf,nullptr)==-1 && errno==EINVAL && CbNativeFile(pf)==3);
        CHECK(ICloseNativeFile(pf)==0);
    }
    // Metadata scanning commits each field and leaves the first non-digit unread.
    pf=PfNativeTemporary();CHECK(pf && IPutNativeString(" \t-2147483648\n+9223372036854775807!",pf)==0 && ISeekNativeFile(pf, 0, NATIVESEEKORIGIN::begin)==0);
    CHECK(CNativeScan(pf,"%d\n%lld",&player,&played)==2 && player==INT32_MIN && played==INT64_MAX);
    CHECK(ChGetNativeFile(pf)=='!' && ICloseNativeFile(pf)==0);
    pf=PfNativeTemporary();CHECK(pf && IPutNativeString("7 2147483648;",pf)==0 && ISeekNativeFile(pf, 0, NATIVESEEKORIGIN::begin)==0);
    version=123;player=456;
    CHECK(CNativeScan(pf,"%d %d",&version,&player)==1 && version==7 && player==456 && ChGetNativeFile(pf)==';');
    CHECK(ICloseNativeFile(pf)==0);
    pf=PfNativeTemporary();CHECK(pf && IPutNativeString("7 +!",pf)==0 && ISeekNativeFile(pf, 0, NATIVESEEKORIGIN::begin)==0);
    player=456;
    CHECK(CNativeScan(pf,"%d %d",&version,&player)==1 && player==456 && ChGetNativeFile(pf)=='!');
    CHECK(ICloseNativeFile(pf)==0);
    pf=PfNativeTemporary();CHECK(pf && IPutNativeString("-9223372036854775808 9223372036854775808?",pf)==0 && ISeekNativeFile(pf, 0, NATIVESEEKORIGIN::begin)==0);
    CHECK(CNativeScan(pf,"%lld",&played)==1 && played==INT64_MIN);
    errno=0;CHECK(CNativeScan(pf,"%lld",&played)==0 && played==INT64_MIN && errno==ERANGE && ChGetNativeFile(pf)=='?');
    CHECK(ICloseNativeFile(pf)==0);
    pf=PfNativeTemporary();CHECK(pf && CNativeScan(pf,"%d",&version)==EOF && ICloseNativeFile(pf)==0);
    pf=PfNativeTemporary();CHECK(pf && IPutNativeString("7",pf)==0 && ISeekNativeFile(pf, 0, NATIVESEEKORIGIN::begin)==0);
    CHECK(CNativeScan(pf,"%d %d",&version,&player)==1 && version==7 && player==456 && FNativeStreamEof(pf));
    CHECK(ISeekNativeFile(pf, 0, NATIVESEEKORIGIN::begin)==0);
    errno=0;CHECK(CNativeScan(pf,"%lld",&player)==0 && errno==EINVAL && LNativePosition(pf)==0);
    errno=0;CHECK(CNativeScan(pf,"%d")==0 && errno==EINVAL && LNativePosition(pf)==0);
    errno=0;CHECK(CNativeScan(pf,"%d",static_cast<int *>(nullptr))==0 && errno==EINVAL && LNativePosition(pf)==0);
    CHECK(ICloseNativeFile(pf)==0);
    puts("File service binary, read/write transition, EOF/seek, exclusive creation, live sizes, truncate/extend, identity and metadata tests passed");
    return 0;
}
