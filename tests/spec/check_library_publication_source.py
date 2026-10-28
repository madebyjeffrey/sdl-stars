#!/usr/bin/env python3
"""Check extracted M01 publication controllers against an independent interpreter.

File/path/metadata/inspection helpers are scripted providers over disjoint role
objects. This tests source control flow and projected partial effects, not real
filesystem I/O, recursive helper algorithms, admission, UI or crash durability.
"""
from pathlib import Path
import json
import os
import re
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def extract(name):
    source = (ROOT / 'src/ui/libraryui.c').read_text()
    marker = 'static int16_t ' + name + '('
    at = source.rfind(marker)
    assert at >= 0
    end = source.index('\n}', at) + 2
    assert '{' in source[at:end]
    return source[at:end]


def later():
    body = extract('FImportSubsequentTurn')
    tail = body[body.index('    cch = CchSnprintf(szOrders,'):]
    return r'''
static int16_t NativeLater(int16_t fHasTurn) {
    HWND hwnd=0; const char *pszSource="source"; int iplr=0, cch;
    char szOrders[512], szStage[512], szPrevious[512], szMessage[1024], szTarget[512]="target";
    struct { char szSelected[256]; } lm={"Game"};
    int32_t lidGame=123,lidCurrent; uint16_t turn=11,turnCurrent;
    int16_t fMultiCurrent,fHasOrders; int64_t cbFile; uint32_t dwTick;
''' + tail


DECLARATIONS = r'''
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#define TRUE 1
#define FALSE 0
#define MB_ICONHAND 1
#define MB_ICONEXCLAMATION 2
#define dtTurn 3
#define idsLibraryTurnStageFailed 1
#define idsLibraryTurnInstallFailed 2
#define idsLibraryTurnPreserveFailed 3
#define idsLibraryTurnRestoreFailed 4
#define idsLibraryGameRestoreFailed 4
#define idsLibraryTurnOrdersRetireFailed 5
#define idsLibraryTurnReplaceFailed 6
#define idsLibraryTurnCleanupFailed 7
#define idsLibrarySeparateCopyFailed 8
#define idsLibraryGameImportFailed 9
#define iplrNone (-1)
typedef intptr_t HWND;
typedef struct { const char *pszTarget,*pszName; int16_t fCopy,fBad; uint32_t grfPlayers; } LIBRARYIMPORT;
static char szLibraryRoot[256]="root",szLibraryName[256]="Copy",szLibraryPending[256]="pending";
char szBase[256];
static const char *szDirSep="/";
// Zero is absent; files are 1..3; directories are bit sets plus 100.
static int objects[8],count,profile,variant,cleared;
static unsigned mask;
static const char *names[8]={"source","target","root/Game/Game.x1","stage","previous","root/Game","hold","pending"};
static int role(const char *p) {
    if(strstr(p,".previous."))return 4;
    if(strstr(p,".importing."))return 3;
    if(!strcmp(p,"root/Copy"))return 1;
    for(int i=0;i<8;i++)if(!strcmp(p,names[i]))return i;
    fprintf(stderr,"unmapped path %s\n",p);abort();
}
static int decision(void) { assert(count<24);return !(mask&(1u<<count)); }
static void event(const char *kind,int a,int b,int ok,int residue) {
    printf("[\"%s\",%d,%d,%d,%d,[",kind,a,b,ok,residue);
    for(int i=0;i<8;i++)printf("%s%d",i?",":"",objects[i]);
    printf("]],");count++;
}
static void clear(void) { if(profile==4 && !szLibraryPending[0] && !cleared){printf("[\"clear\"],");cleared=1;} }
static const char *PszGetCompressedString(int id) {static const char *s[]={"0","1","2","3","4","5","6","7","8","9"};return s[id];}
static void AlertSz(const char *s,int flags) {(void)flags;clear();printf("[\"notice\",%d],",atoi(s));}
static int CchSnprintf(char *out,size_t cap,const char *format,...) {
    va_list args;va_start(args,format);int result=vsnprintf(out,cap,format,args);va_end(args);
    int tag=0;
    if(!strcmp(format,"%s%s%s%s%s.x%d"))tag=1;
    else if(!strcmp(format,"%s.importing.%u"))tag=2;
    else if(!strcmp(format,"%s.previous.%u"))tag=3;
    else if(!strcmp(format,"%s%s%s"))tag=(profile==3 && !strcmp(out,"root/Copy"))?5:4;
    // New-family stage formatting is unchecked in the source and is not a guard.
    if(tag){int ok=decision();event("format",tag,0,ok,0);if(!ok)return (int)cap;}
    return result;
}
static uint32_t DwTickCount(void){return 7;}
static int64_t LNativeTime(void){return 77;}
static int FCopyNativeFileExclusive(const char *source,const char *target) {
    int s=role(source),t=role(target),ok=decision() && objects[s]>0 && objects[s]<100 && !objects[t],residue=0;
    if(ok)objects[t]=objects[s];
    else if(variant==1 && !objects[t] && objects[s])objects[t]=residue=3;
    event("copy",s,t,ok,residue);return ok;
}
static int FInspectImportFile(const char *path,int type,int player,int32_t *id,uint16_t *turn,void *gen,int16_t *multi) {
    (void)type;(void)player;assert(!gen);int p=role(path),inspected=decision() && objects[p]>0 && objects[p]<100;
    // Probe 2 includes inspection and the two subsequent source comparisons.
    *id=variant==22?999:123;*turn=variant==23?12:11;*multi=variant==24;
    event("probe",2,p,inspected && variant!=22 && variant!=23,0);return inspected;
}
static int FNativeFileSize(const char *path,int64_t *size) {int p=role(path),ok=decision() && objects[p]>0 && objects[p]<100;*size=1;event("probe",1,p,ok,0);return ok;}
static int move(const char *source,const char *target) {int s=role(source),t=role(target),ok=decision() && objects[s];if(ok){objects[t]=objects[s];objects[s]=0;}event("replace",s,t,ok,0);return ok?0:-1;}
#define ReplaceNativeFile move
#define RenameNativeFile move
static int removeObject(const char *path,int tree) {
    int p=role(path),ok=decision(),residue=0;
    if(ok)objects[p]=0;
    else if(variant==1 && objects[p]>=100){objects[p]=100+((objects[p]-100)&1);residue=objects[p];}
    event("remove",p,tree,ok,residue);return ok;
}
static int RemoveNativeFile(const char *path){return removeObject(path,0)?0:-1;}
static int RemoveNativePath(const char *path){return removeObject(path,0)?0:-1;}
static int RemoveLibraryGameTree(const char *path){return removeObject(path,1);}
static int FNativeDirectoryExists(const char *path){int p=role(path),ok=decision() && objects[p]>=100;event("probe",3,p,ok,0);return ok;}
static int FValidManagedLibraryGame(const char *name,const char *dir,int *remember,long long *played,char *exchange,size_t cap){
    assert(!strcmp(name,"Game"));int p=role(dir),ok=decision() && objects[p]>=100;
    *remember=4;*played=321;assert(cap>9);strcpy(exchange,"external");event("probe",5,p,ok,0);return ok;
}
static int FCreateLibraryGameStagePath(char *out,size_t cap){assert(cap>5);strcpy(out,"stage");int ok=decision() && !objects[3];if(ok)objects[3]=100;event("allocate",1,3,ok,0);return ok;}
static int FCreateLibraryGameHoldPath(char *out,size_t cap){assert(cap>4);strcpy(out,"hold");int ok=decision();event("probe",4,6,ok,0);return ok;}
static int build(int tag){int ok=decision();assert(objects[3]>=100);objects[3]=100+((objects[3]-100)|(1<<(tag-1)));event("build",tag,3,ok,objects[3]);return ok;}
static int FCreateLibraryGameStage(const char *source,const char *old,const char *name,const char *stage,int player,long long played,const char *exchange){
    assert(role(source)==5 && role(stage)==3 && !strcmp(old,"Game") && !strcmp(exchange,"external"));
    if(profile==2){assert(!strcmp(name,"Game") && player==4 && played==321);return build(1);}
    assert(profile==3 && !strcmp(name,"Copy") && player==0 && played==0);return build(4);
}
static int FBackupConflictingPlayerFiles(const char *stage,const char *name,int player,char *backup,size_t cap){assert(role(stage)==3 && !strcmp(name,"Game") && player==0 && cap>6);strcpy(backup,"backup");return build(2);}
static int FStageConflictingTurn(const char *stage,const char *name,const char *source,int player,int32_t id,uint16_t turn){assert(role(stage)==3 && !strcmp(name,profile==2?"Game":"Copy") && role(source)==0 && player==0 && id==123 && turn==11);return build(3);}
static int FChooseSeparateCopyName(HWND hwnd,const char *name){(void)hwnd;assert(!strcmp(name,"Game"));int ok=decision();event("probe",6,1,ok,0);return ok;}
static int FLibraryNameAvailable(const char *root,const char *name){assert(root==szLibraryRoot && !strcmp(name,"Copy"));int ok=decision();event("probe",7,1,ok,0);return ok;}
static size_t NativeStrlen(const char *s){if(s==szLibraryRoot){int ok=decision();event("probe",8,1,ok,0);if(!ok)return sizeof(szBase);}return strlen(s);}
// Family enumeration is a declared stage-only provider; cleanup aggregates the
// source enumeration plus final RemoveNativePath, including possible remnants.
static int CopyImportEntry,CopyBackupDirectory,RemoveImportFile;
static int FNativeEnumerateDirectory(const char *path,void *callback,void *context){
    (void)path;
    if(callback==&CopyImportEntry || callback==&CopyBackupDirectory){
        int ok=build(callback==&CopyImportEntry?5:6);
        if(variant==4 && !ok){((LIBRARYIMPORT *)context)->fBad=1;return 1;}
        return ok;
    }
    assert(callback==&RemoveImportFile);
    // Removal itself is performed by the immediately following source call.
    return 1;
}
static int FWriteLibraryMetadata(const char *path,const char *name,int16_t player){
    int expected=variant>=5 && variant<=20?variant-5:variant==21?-1:0;
    assert(role(path)==3 && !strcmp(name,"Copy") && player==expected);return build(7);
}
static int CreateNativeDir(const char *path){int p=role(path),ok=decision() && !objects[p];if(ok)objects[p]=100;event("allocate",2,p,ok,0);return ok?0:-1;}
#define CopyImportEntry (&CopyImportEntry)
#define CopyBackupDirectory (&CopyBackupDirectory)
#define RemoveImportFile (&RemoveImportFile)
#define strlen NativeStrlen
'''


def family():
    source = (ROOT / 'src/ui/libraryui.c').read_text()
    start = source.index('    CchSnprintf(szStage, sizeof(szStage), "%s%s.importing.%lld"')
    end = source.index('\nLInvalidSource:', start)
    return r'''
static int16_t NativeFamily(void) {
    char szStage[256],szDir[512]="source";
    LIBRARYIMPORT li={0};li.grfPlayers=variant>=5 && variant<=20?1u<<(variant-5):variant==21?0:1;
''' + source[start:end] + '\n}'


HARNESS = r'''
#undef strlen
int main(void){
    unsigned inputMask;int inputProfile,inputVariant;
    while(scanf("%d %u %d",&inputProfile,&inputMask,&inputVariant)==3){
        profile=inputProfile;mask=inputMask;variant=inputVariant;count=cleared=0;
        memset(objects,0,sizeof(objects));objects[0]=1;strcpy(szLibraryPending,"pending");
        if(profile==0)objects[1]=2;
        if(profile<=1)objects[2]=2;
        if(profile==2 || profile==3)objects[5]=115;
        if(profile==4)objects[7]=100;
        if(variant==2)objects[3]=2;
        if(variant==3)objects[4]=2;
        printf("{\"profile\":%d,\"mask\":%u,\"variant\":%d,\"events\":[",profile,mask,variant);
        int result=profile<=1?NativeLater(profile==0):profile==2?
            FReplaceConflictingLibraryTurn(0,"Game","source","target",0,123,11):profile==3?
            FImportConflictingTurnAsCopy(0,"Game","source",0,123,11):NativeFamily();
        clear();printf("[\"end\"]],\"result\":%d,\"calls\":%d,\"store\":[",result,count);
        for(int i=0;i<8;i++)printf("%s%d",i?",":"",objects[i]);
        printf("]}\n");
    }
    return 0;
}
'''


def ret(value): return ('return', value)
def ask(kind, a, b, yes, no): return ('ask', (kind, a, b), yes, no)
def notice(n, nxt): return ('notice', n, nxt)
def ignore(kind, a, b, nxt): return ask(kind, a, b, nxt, nxt)
def guard(tag, nxt): return ask('format', tag, 0, nxt, ret(0))


def program(profile):
    fail_stage = ignore('remove', 3, 0, notice(1, ret(0)))
    install_fail = lambda prior: ignore('remove', 3, 0,
        ignore('remove', 4, 0, notice(2, ret(0))) if prior else notice(2, ret(0)))
    if profile <= 1:
        retire_failed = ask('replace', 4, 1, ignore('remove', 4, 0, notice(5, ret(0))), notice(4, ret(0)))
        def prior(orders):
            installed = ask('remove', 2, 0, ignore('remove', 4, 0, ret(1)), retire_failed) if orders else ignore('remove', 4, 0, ret(1))
            return ask('copy', 1, 4, ask('replace', 3, 1, installed, install_fail(True)), ignore('remove', 3, 0, notice(3, ret(0))))
        installed = ask('probe', 1, 2, prior(True), prior(False)) if profile == 0 else ask('replace', 3, 1, ret(1), install_fail(False))
        return guard(1, guard(2, guard(3, ask('copy', 0, 3, ask('probe', 2, 3, installed, fail_stage), fail_stage))))
    if profile == 2:
        def done(held=False):
            end = ask('replace', 6, 5, notice(6, ret(0)), notice(4, ret(0))) if held else notice(6, ret(0))
            return ask('probe', 3, 3, ignore('remove', 3, 1, end), end)
        publish = ask('replace', 3, 5, ask('remove', 6, 1, ret(1), notice(7, ret(1))), ask('replace', 6, 5, done(), done(True)))
        hold = ask('replace', 5, 6, publish, done())
        prepare = ask('build', 1, 3, ask('build', 2, 3, ask('build', 3, 3, ask('probe', 4, 6, hold, done()), done()), done()), done())
        return guard(4, ask('probe', 5, 5, ask('allocate', 1, 3, prepare, ret(0)), ret(0)))
    if profile == 3:
        fail = ask('probe', 3, 3, ignore('remove', 3, 1, notice(8, ret(0))), notice(8, ret(0)))
        staged = ask('build', 4, 3, ask('build', 3, 3, ask('replace', 3, 1, ret(1), fail), fail), fail)
        return guard(4, ask('probe', 5, 5, ask('probe', 6, 1, guard(5,
            ask('probe', 7, 1, ask('probe', 8, 1, ask('allocate', 1, 3, staged, ret(0)), ret(0)), ret(0))), ret(0)), ret(0)))
    clear = lambda nxt: ('clear', nxt)
    fail = ignore('remove', 3, 1, ignore('remove', 7, 0, clear(notice(9, ret(0)))))
    return ask('allocate', 2, 3, ask('build', 5, 3, ask('build', 6, 3, ask('build', 7, 3,
        ask('remove', 7, 0, ask('replace', 3, 7, clear(ret(1)), fail), fail), fail), fail), fail),
        ignore('remove', 7, 0, clear(ret(0))))


def check(case):
    p, variant = case['profile'], case['variant']
    assert case['calls'] <= (11,7,14,12,12)[p], case
    store = [1, 2 if p == 0 else 0, 2 if p <= 1 else 0, 0, 0, 115 if p in (2,3) else 0, 0, 100 if p == 4 else 0]
    if variant == 2: store[3] = 2
    if variant == 3: store[4] = 2
    node, at = program(p), 0
    while node[0] != 'return':
        event = case['events'][at]; at += 1
        if node[0] == 'notice':
            assert event == ['notice', node[1]], (case, node)
            node = node[2]; continue
        if node[0] == 'clear':
            assert event == ['clear'], (case, node)
            node = node[1]; continue
        kind, a, b = node[1]
        # Family cleanup groups shallow enumeration and final stage removal.
        assert event[:3] == [kind, a, 0 if p == 4 and kind == 'remove' and a == 3 else b], (case, node)
        ok, residue, after = event[3:]
        if kind == 'copy':
            if ok:
                assert 0 < store[a] < 100 and store[b] == 0
                store[b] = store[a]
            elif residue:
                assert store[b] == 0 and residue == 3 and store[a] in (1,2)
                store[b] = residue
        elif kind == 'replace' and ok:
            assert store[a]
            store[b], store[a] = store[a], 0
        elif kind == 'remove':
            if ok: store[a] = 0
            elif residue:
                assert store[a] >= 100 and residue >= 100 and ((residue-100) & (store[a]-100)) == residue-100
                store[a] = residue
        elif kind == 'allocate' and ok:
            assert store[b] == 0
            store[b] = 100
        elif kind == 'build':
            assert residue >= 100
            store[b] = residue
        elif kind == 'probe' and ok and a in (1,2,3,5):
            assert store[b] >= 100 if a in (3,5) else 0 < store[b] < 100
        assert store == after, (case, node, store, after)
        node = node[2 if ok else 3]
    assert case['events'][at:] == [['end']], (case,node)
    assert case['result'] == node[1] and case['store'] == store, (case,node,store)
    return tuple(tuple(e[:5]) for e in case['events'])


def check_exclusive_copy(tmp, compiler):
    """Real stdio/path checks for the extracted copy helpers, separately scoped."""
    prelude = r'''
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#define TRUE 1
#define FALSE 0
static int faults;
static FILE *copySource,*copyTarget;
static FILE *copyOpen(const char *path,const char *mode){
    FILE *f=fopen(path,mode);
    if(!strcmp(mode,"rb"))copySource=f;else copyTarget=f;
    return f;
}
static size_t copyRead(void *data,size_t size,size_t count,FILE *f){
    return f==copySource && (faults&1)?0:fread(data,size,count,f);
}
static int copyError(FILE *f){return ferror(f) || (f==copySource && (faults&1));}
static size_t copyWrite(const void *data,size_t size,size_t count,FILE *f){
    return fwrite(data,size,faults&2?count/2:count,f);
}
static int copyClose(FILE *f){
    int forced=(f==copySource && (faults&4)) || (f==copyTarget && (faults&8));
    if(f==copySource)copySource=NULL;
    if(f==copyTarget)copyTarget=NULL;
    int status=fclose(f);if(forced){errno=EIO;return EOF;}return status;
}
static int copyRemove(const char *path){if(faults&16){errno=EACCES;return -1;}return remove(path);}
#define PfOpenNativeFile copyOpen
#define RemoveNativeFile copyRemove
#define fread copyRead
#define ferror copyError
#define fwrite copyWrite
#define fclose copyClose
'''
    harness = r'''
#undef fread
#undef ferror
#undef fwrite
#undef fclose
static void writeSource(int n){
    FILE *f=fopen("source","wb");assert(f);
    for(int i=0;i<n;i++)assert(fputc((i*31)%251,f)!=EOF);
    assert(fclose(f)==0);
}
static void verifyTarget(int n){
    FILE *f=fopen("target","rb");assert(f);
    for(int i=0;i<n;i++)assert(fgetc(f)==(i*31)%251);
    assert(fgetc(f)==EOF && !ferror(f));assert(fclose(f)==0);
}
int main(void){
    const int lengths[]={0,1,8191,8192,8193,16384,16385};int cases=0;
    for(unsigned i=0;i<sizeof(lengths)/sizeof(lengths[0]);i++){
        remove("target");writeSource(lengths[i]);
        assert(FCopyNativeFileExclusive("source","target"));verifyTarget(lengths[i]);cases++;
        writeSource(lengths[i]+1);errno=0;
        assert(!FCopyNativeFileExclusive("source","target") && errno==EEXIST);
        verifyTarget(lengths[i]);cases++;
    }
    remove("source");remove("target");
    assert(!FCopyNativeFileExclusive("source","target"));
    assert(!fopen("target","rb"));cases++;
    for(int bits=0;bits<32;bits++){
        remove("target");writeSource(8193);faults=bits;copySource=copyTarget=NULL;
        int ok=FCopyNativeFileExclusive("source","target");assert(ok==!(bits&15));
        int retained=ok || (bits&16),length=(bits&1)?0:(bits&2)?4096:8193;
        if(retained)verifyTarget(length);else assert(!fopen("target","rb"));
        FILE *f=fopen("source","rb");assert(f);
        for(int i=0;i<8193;i++)assert(fgetc(f)==(i*31)%251);
        assert(fgetc(f)==EOF && !ferror(f));assert(fclose(f)==0);cases++;
    }
    faults=0;remove("target");writeSource(1);
    assert(FCopyNativeFileExclusive("source","target"));
    faults=4;errno=0;
    assert(!FCopyNativeFileExclusive("source","target") && errno==EEXIST);
    verifyTarget(1);cases++;faults=0;
    printf("%d extracted-copy cases: 15 real-stdio boundaries plus 33 read/write/close/cleanup receipt combinations\n",cases);
    return 0;
}
'''
    c, binary = tmp/'copy.c', tmp/'copy'
    body=extract('FCopyNativeFileContents')+'\n'+extract('FCopyNativeFileExclusive')
    for old,new in {'NATIVEFILE':'FILE','CReadNative':'fread','CWriteNative':'fwrite','FNativeStreamError':'ferror','ICloseNativeFile':'fclose'}.items():
        body=body.replace(old,new)
    c.write_text(prelude+'\n'+body+'\n'+harness)
    subprocess.run(compiler+['-std=c11','-Wall','-Wextra','-Werror',str(c),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],cwd=tmp,check=True)


def main():
    source=(ROOT/'src/ui/libraryui.c').read_text()
    for name in ('szLibraryRoot','szLibraryPending','szLibraryName'):
        assert f'static char {name}[256];' in source
    assert re.search(r'char\s+szBase\[256\]',(ROOT/'src/core/globals.c').read_text())
    assert 'char szUniverse[512], szTarget[512], szOrders[512], szStage[512], szPrevious[512], szMessage[1024];' in extract('FImportSubsequentTurn')
    # Enumerate every reached Boolean reply combination, including repeated
    # restore/cleanup calls, plus partial residue and stage/previous collisions.
    profiles = [(p,v,bits) for p,bits in ((0,11),(1,7),(2,14),(3,12),(4,12)) for v in (0,1)]
    profiles += [(p,v,11 if p==0 else 7) for p in (0,1) for v in (2,3)]
    profiles += [(4,4,12)]+[(4,v,0) for v in range(5,22)]
    profiles += [(p,v,0) for p in (0,1) for v in (22,23,24)]
    inputs = ''.join(f'{p} {m} {v}\n' for p,v,bits in profiles for m in range(1<<bits))
    with tempfile.TemporaryDirectory(prefix='stars-library-publication-') as tmp:
        tmp = Path(tmp); c = tmp/'controller.c'; binary = tmp/'controller'; output = tmp/'cases.jsonl'
        bodies = later()+'\n'+extract('FReplaceConflictingLibraryTurn')+'\n'+extract('FImportConflictingTurnAsCopy')+'\n'+family()
        c.write_text(DECLARATIONS+'\n'+bodies+'\n'+HARNESS)
        compiler = shlex.split(os.environ.get('CC','cc'))
        subprocess.run(compiler+['-std=c11','-O0','-Wall','-Wextra','-Werror',str(c),'-o',str(binary)],check=True)
        with output.open('w') as stream:
            subprocess.run([str(binary)],input=inputs,text=True,stdout=stream,check=True)
        count=0;unique=[set() for _ in range(5)];witnesses={}
        for line in output.open():
            case=json.loads(line);trace=check(case);count+=1;unique[case['profile']].add(trace)
            witnesses[case['profile'],case['mask'],case['variant']]=case
        # Explicit outcomes prevent all-failure/short-circuit coverage masquerading
        # as successful publication or ordinary restoration coverage.
        for p in range(5): assert witnesses[p,0,0]['result']==1
        q=witnesses[0,1<<5,0];assert q['result']==1 and q['store'][1]==1 and q['store'][2]==2
        q=witnesses[1,0,2];assert q['result']==0 and q['store'][3]==0
        q=witnesses[0,1<<6,1];assert q['result']==0 and q['store'][4]==3 and q['store'][1]==2
        q=witnesses[0,(1<<8)|(1<<9),0];assert q['result']==0 and q['store'][1]==1 and q['store'][4]==2
        q=witnesses[2,(1<<8)|(1<<9),0];assert q['result']==0 and q['store'][5]==115 and ['notice',6] in q['events']
        q=witnesses[2,1<<9,1];assert q['result']==1 and q['store'][6]==101 and ['notice',7] in q['events']
        q=witnesses[4,1<<3,1];assert q['result']==0 and ['clear'] in q['events']
        for p in (0,1):
            for v in (22,23):assert witnesses[p,0,v]['result']==0
            assert witnesses[p,0,24]['result']==1
        print(f'{count} extracted-controller cases; distinct traces by later/first/replace/separate/family: '+','.join(str(len(s)) for s in unique))
        print('Scripted helper receipts only; recursive copy/validation, native filesystem semantics, admission, UI and crash qualification remain open.')
        check_exclusive_copy(tmp,compiler)


if __name__=='__main__':main()
