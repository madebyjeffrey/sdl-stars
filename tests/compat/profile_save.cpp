/* Independent S02 save-stage vectors against the linked production serializer.
 * I/O providers supply failures and receipts, never a replacement algorithm. */
#include "profile_io.h"
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>

enum {Ok,Allocate,Temporary,Mode,Bom,Rows,Newline,Flush,Sync,Close,Rename};
static int failure,published,unlinked,cleanupOK,opened,closed,syncCalled,renameCalled;
static char output[256];static size_t used;
struct PROFILEFAILURE {};
static bool fNestedFailure;
static void *TestMalloc(size_t n){return failure==Allocate?NULL:malloc(n);}
static NATIVEFILE *TestTemp(char *p){(void)p;if(failure==Temporary)return NULL;opened=1;return (NATIVEFILE *)(uintptr_t)1;}
static int TestMode(NATIVEFILE *fp,const char *p){(void)fp;(void)p;return failure==Mode?-1:0;}
static size_t TestWrite(const void *data,size_t size,size_t count,NATIVEFILE *fp){(void)fp;if(failure==Bom)return 2;memcpy(output+used,data,size*count);used+=size*count;return count;}
static int TestPuts(const char *s,NATIVEFILE *fp){
    (void)fp;
    if (fNestedFailure) {
        PROFILEIO io = ProfileIO();
        io.pfnWrite = CWriteNative;
        PROFILEIOSCOPE nested(io);
        throw PROFILEFAILURE{};
    }
    if(failure==Rows)return EOF;
    memcpy(output+used,s,strlen(s));used+=strlen(s);return 0;
}
static int TestPutc(int c,NATIVEFILE *fp){(void)fp;if(failure==Newline)return EOF;output[used++]=(char)c;return c;}
static int TestFlush(NATIVEFILE *fp){(void)fp;return failure==Flush?-1:0;}
static int TestSync(NATIVEFILE *fp){(void)fp;syncCalled++;return failure==Sync?-1:0;}
static int TestClose(NATIVEFILE *fp){(void)fp;closed=1;return failure==Close?-1:0;}
static int TestRename(const char *a,const char *b,int replace,int sync){(void)a;(void)b;(void)replace;(void)sync;renameCalled++;if(failure==Rename)return -1;published=1;return 0;}
static int TestUnlink(const char *p){(void)p;unlinked++;if(cleanupOK)return 0;errno=EACCES;return -1;}

static int checks;
#define CHECK(x) do {checks++;if(!(x)){fprintf(stderr,"stage %d witness %d failed: %s\n",failure,checks,#x);return 1;}}while(0)
int main(void){
    char szSecond[]="B\n",szFirst[]="A";
    COMPATPROFILE second={},first={};int ret;
    second.szRaw=szSecond;first.szRaw=szFirst;first.lpNext=&second;
    PROFILEIO io;
    io.pfnAllocate=TestMalloc;io.pfnCreateTemp=TestTemp;io.pfnCopyPermissions=TestMode;
    io.pfnWrite=TestWrite;io.pfnPutString=TestPuts;io.pfnPutChar=TestPutc;
    io.pfnFlush=TestFlush;io.pfnSync=TestSync;io.pfnClose=TestClose;
    io.pfnRename=TestRename;io.pfnUnlink=TestUnlink;
    for(failure=Ok;failure<=Rename;failure++){
        SetLastError(77);errno=EIO;published=unlinked=opened=closed=syncCalled=renameCalled=0;used=0;cleanupOK=1;
        ret=FSaveProfile(&first,"unused",io);
        CHECK(ret==(failure==Ok) && published==(failure==Ok));
        CHECK(unlinked==(failure!=Ok && failure!=Allocate && failure!=Temporary));
        CHECK(GetLastError()==((failure==Ok || failure==Bom)?77:failure==Allocate?8:29));
        if(failure==Ok)CHECK(used==7 && !memcmp(output,"\xef\xbb\xbf" "A\nB\n",7));
        if(failure==Flush)CHECK(!syncCalled && !renameCalled);
        if(failure==Sync || failure==Close)CHECK(!renameCalled);
        if(failure==Mode || failure==Bom || failure==Rows || failure==Newline)CHECK(closed);
    }
    failure=Sync;cleanupOK=0;SetLastError(77);errno=EIO;unlinked=published=used=0;
    CHECK(!FSaveProfile(&first,"unused",io) && unlinked==1 && !published);
    printf("S02 linked save-control witnesses: %d passed\n",checks);

    CHECK(GetLastError() == ERROR_WRITE_FAULT);
    CHECK(ProfileIO().pfnWrite == CWriteNative);
    {
        PROFILEIOSCOPE outer(io);
        bool fCaught = false, fThreadDefault = false;
        CHECK(ProfileIO().pfnWrite == TestWrite);
        io.pfnWrite = CWriteNative;
        CHECK(ProfileIO().pfnWrite == TestWrite);
        {
            PROFILEIOSCOPE inner(io);
            CHECK(ProfileIO().pfnWrite == CWriteNative);
        }
        CHECK(ProfileIO().pfnWrite == TestWrite);
        SetLastError(77);
        std::thread worker([&] {
            fThreadDefault = ProfileIO().pfnWrite == CWriteNative && GetLastError() == 0;
            SetLastError(91);
        });
        worker.join();
        CHECK(fThreadDefault && GetLastError() == 77);

        failure=Ok;cleanupOK=1;published=unlinked=closed=0;used=0;
        fNestedFailure = true;
        try { FSaveProfile(&first, "unused"); }
        catch (const PROFILEFAILURE &) { fCaught = true; }
        fNestedFailure = false;
        CHECK(fCaught && closed && unlinked == 1 && !published);
        CHECK(ProfileIO().pfnWrite == TestWrite);
        used=0;
        CHECK(FSaveProfile(&first, "unused") && used == 7);
    }
    CHECK(ProfileIO().pfnWrite == CWriteNative);
    std::puts("Profile providers: nesting, owned snapshots, callback unwinding, retry and thread isolation passed");
    return 0;
}
