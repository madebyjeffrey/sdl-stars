#!/usr/bin/env python3
"""Extracted FSaveLibraryRace + actual player writer/checksum/stream close.

Directory/create/write/replace/remove providers are scripted over disjoint role
paths. Player payloads use the source's uncompressed branch; encryption and BOF
creation are explicit providers. No actual OS faults or valid native race files
are inferred. Short writes escape through a live outer jump buffer.
"""
from pathlib import Path
import json
import os
import shlex
import struct
import subprocess
import sys
import tempfile
from check_s02_source import ROOT,function
from check_race_workflows_source import declarations,bodies


PRELUDE=r'''
#include <stdarg.h>
static unsigned mask;
static int variant,dead,detail,aliasPlayer,directory,stagePresent,targetPresent;
static unsigned char stageBytes[1024],targetBytes[1024];
static size_t stageCount,targetCount;
static int events[32],eventCount,phase,physicalOpen;
static char szRaceLibraryRoot[256]="R",szRaceLibraryTemp[512];
static jmp_buf escape;
int16_t hf=-1;
static void event(int n){assert(eventCount<32);events[eventCount++]=n;}
static int allowed(unsigned bit){return !(mask&(1u<<bit));}
static int16_t FInitRaceLibrary(void){event(0);return allowed(0);}
static int16_t FPortableRaceFilename(const char *name){assert(!strcmp(name,"Race.r1"));event(1);return allowed(1);}
int CchSnprintf(char *out,size_t cap,const char *fmt,...){
    va_list a;va_start(a,fmt);int n=vsnprintf(out,cap,fmt,a);va_end(a);
    if(!strcmp(fmt,"%s%s%s"))event(2);else event(15);return n;
}
static int16_t FCreateRaceStagePath(char *dir,size_t dirCap,char *file,size_t fileCap){
    event(3);assert(dirCap>1 && fileCap>3);strcpy(dir,"D");strcpy(file,"D/F");
    if(!allowed(2)){if(variant==2){directory=stagePresent=1;stageCount=1;stageBytes[0]=7;}return 0;}
    directory=1;stagePresent=0;stageCount=0;return 1;
}
int16_t FCreateFile(DtFileType type,int16_t player,char *file){
    event(4);assert(type==dtRace && player==iplrNone && !strcmp(file,"D/F"));
    stagePresent=allowed(3) || variant==1;stageCount=stagePresent?1:0;stageBytes[0]=1;
    hf=stagePresent?5:-1;physicalOpen=stagePresent;return allowed(3);
}
int RemoveNativeFile(const char *file){assert(!strcmp(file,"D/F"));event(11);if(!allowed(8) || !stagePresent)return -1;stagePresent=0;stageCount=0;return 0;}
int RemoveNativePath(const char *dir){assert(!strcmp(dir,"D"));event(12);if(!allowed(9) || stagePresent || !directory)return -1;directory=0;return 0;}
int ReplaceNativeFile(const char *from,const char *to){
    assert(!strcmp(from,"D/F") && !strcmp(to,"R/Race.r1"));event(10);
    if(!allowed(7))return -1;assert(stagePresent);memcpy(targetBytes,stageBytes,stageCount);targetCount=stageCount;targetPresent=1;stagePresent=0;stageCount=0;return 0;
}
static int IsWindow(HWND hwnd){(void)hwnd;event(13);return allowed(10);}
char *PszFormatIds(StringId id,int16_t *args){(void)id;(void)args;return "failure";}
int16_t AlertSz(char *text,int16_t flags){assert(!strcmp(text,"failure") && flags==MB_ICONHAND);event(14);return 0;}
int16_t FCompressUserString(char *in,char *out,int16_t *count){(void)in;(void)out;(void)count;return 0;}
void WriteRt(RecordType type,int16_t count,void *data){
    HDR header={0};header.cb=count;header.rt=type;unsigned char record[512];assert(count>=0 && count<510);
    memcpy(record,&header,2);memcpy(record+2,data,count);size_t length=(size_t)count+2;
    unsigned bit=phase==5?4:5;assert(phase==5 || phase==7);
    size_t written=allowed(bit)?length:(variant==3?1:2+(size_t)count/2);
    assert(stagePresent && stageCount+written<sizeof(stageBytes));memcpy(stageBytes+stageCount,record,written);stageCount+=written;
    if(!allowed(bit))longjmp(escape,1);
}
void CloseFile(int16_t descriptor){assert(descriptor==5);event(9);if(allowed(6))physicalOpen=0;}
static void NativeWritePlayer(PLAYER *,uint8_t *);
static uint16_t NativeChecksum(PLAYER *);
void WriteRtPlr(PLAYER *player,uint8_t *store){phase=5;event(5);NativeWritePlayer(player,store);}
uint16_t IRaceChecksum(PLAYER *player){event(6);phase=7;event(7);return NativeChecksum(player);}
'''

HARNESS=r'''
static void hex(const void *data,size_t length){const unsigned char *p=data;putchar('"');for(size_t i=0;i<length;i++)printf("%02x",p[i]);putchar('"');}
int main(void){
    unsigned inMask;int inVariant,inDead,inDetail,inAlias;
    while(scanf("%u %d %d %d %d",&inMask,&inVariant,&inDead,&inDetail,&inAlias)==5){
        mask=inMask;variant=inVariant;dead=inDead;detail=inDetail;aliasPlayer=inAlias;
        eventCount=phase=physicalOpen=directory=stagePresent=0;hf=-1;stageCount=0;targetCount=1;targetBytes[0]=9;targetPresent=1;
        memset(szRacePass,'w',16);memset(szRaceFile,'c',16);memset(szRaceLibraryPath,'p',512);memset(szRaceFileSuggest,'s',256);fNewLibraryRace=1;
        PLAYER candidate={0};candidate.det=detail;candidate.fDead=dead;candidate.fLearned=variant==4;
        strcpy(candidate.szName,"Race");strcpy(candidate.szNames,"Races");candidate.rgmdRelation[0]=1;
        if(variant==5)candidate.rgmdRelation[15]=2;
        memset(&vplr,0,sizeof(vplr));strcpy(vplr.szName,"Global");if(aliasPlayer)vplr=candidate;
        PLAYER *chosen=aliasPlayer?&vplr:&candidate;int result=-2;
        if(!setjmp(escape))result=FSaveLibraryRace(0,"Race.r1",chosen);
        printf("{\"mask\":%u,\"variant\":%d,\"dead\":%d,\"detail\":%d,\"alias\":%d,\"result\":%d,\"events\":[",mask,variant,dead,detail,aliasPlayer,result);
        for(int i=0;i<eventCount;i++)printf("%s%d",i?",":"",events[i]);
        printf("],\"directory\":%d,\"open\":%d,\"physicalOpen\":%d,\"stage\":",directory,hf!=-1,physicalOpen);
        if(stagePresent)hex(stageBytes,stageCount);else printf("null");printf(",\"target\":");hex(targetBytes,targetCount);
        printf(",\"candidate\":");hex(chosen,192);printf(",\"global\":");hex(&vplr,192);
        printf(",\"cache\":");hex(szRaceFile,16);printf(",\"path\":");hex(szRaceLibraryPath,512);printf(",\"suggestion\":");hex(szRaceFileSuggest,256);printf("}\n");
    }
    return 0;
}
'''


def record(tag,payload):return struct.pack('<H',len(payload)+(tag<<10))+payload

def checksum(raw):
    value=bytearray(raw)
    for a,b in ((128,160),(160,192)):
        at=value.find(0,a,b)
        if at>=0:value[at:b]=bytes(b-at)
    result=0
    for (word,) in struct.iter_unpack('<H',value):result^=word
    return result

def raw(variant,dead,detail):
    p=bytearray(192);p[6]=detail;p[84]=dead+(8 if variant==4 else 0);p[128:132]=b'Race';p[160:165]=b'Races';p[112]=1
    if variant==5:p[127]=2
    return bytes(p)

def player_record(candidate):
    if candidate[6]&7==7:
        rel=candidate[112:128].rstrip(b'\0');payload=candidate[:112]+bytes([len(rel)])+rel
    else:payload=candidate[:8]
    for a,b in ((128,160),(160,192)):payload+=b'\0'+candidate[a:b].split(b'\0')[0]+b'\0'
    return record(6,payload)

def expected(c):
    mask,v,dead,detail,alias=(c[k] for k in ('mask','variant','dead','detail','alias'))
    ok=lambda bit:not mask&(1<<bit)
    e=[];candidate=raw(v,dead,detail);global_raw=candidate if alias else bytes(128)+b'Global'+bytes(58)
    state=dict(result=0,directory=0,open=0,physicalOpen=0,stage=None,target='09',candidate=candidate.hex(),global_=global_raw.hex(),
               cache=(b'c'*16).hex(),path=(b'p'*512).hex(),suggestion=(b's'*256).hex())
    def done():
        out=state.copy();out['events']=e;out['global']=out.pop('global_');return out
    def remove_file():
        e.append(11)
        if ok(8) and state['stage'] is not None:state['stage']=None
    def remove_dir():
        e.append(12)
        if ok(9) and state['stage'] is None and state['directory']:state['directory']=0
    e.append(0)
    if not ok(0):return done()
    e.append(1)
    if not ok(1):return done()
    e.extend((2,3))
    if not ok(2):
        if v==2:state['directory']=1;state['stage']='07'
        return done()
    state['directory']=1;e.append(4);present=ok(3) or v==1
    state['stage']='01' if present else None;state['open']=state['physicalOpen']=int(present)
    if not ok(3):remove_file();remove_dir();return done()
    e.append(5);candidate=bytearray(candidate)
    if dead:candidate[6]=(candidate[6]&~7)|7
    candidate=bytes(candidate);state['candidate']=candidate.hex()
    if alias:state['global_']=candidate.hex()
    wanted=player_record(candidate);written=wanted if ok(4) else wanted[:1 if v==3 else 2+(len(wanted)-2)//2]
    state['stage']+=written.hex()
    if not ok(4):state['result']=-2;return done()
    e.extend((6,7));wanted=record(0,struct.pack('<H',checksum(candidate)))
    written=wanted if ok(5) else wanted[:1 if v==3 else 3];state['stage']+=written.hex()
    if not ok(5):state['result']=-2;return done()
    e.append(9);state['open']=0
    if ok(6):state['physicalOpen']=0
    e.append(10)
    if not ok(7):
        remove_file();remove_dir();e.append(13)
        if ok(10):e.append(14)
        return done()
    state['target']=state['stage'];state['stage']=None;remove_dir();e.append(15)
    name=b'Race.r1';path=b'R/Race.r1'
    state.update(result=1,cache=(name+b'\0'+b'c'*(15-len(name))).hex(),path=(path+b'\0'+b'p'*(511-len(path))).hex(),suggestion=(name+b'\0'+b's'*(255-len(name))).hex())
    return done()


def cases():
    for variant in (0,1):
        for alias in (0,1):
            for mask in range(2048):yield mask,variant,0,0,alias
    for variant in range(6):
        for alias in (0,1):
            for dead in (0,1):
                for detail in range(8):
                    for mask in (0,4,8,16,32,64,128,256,512):yield mask,variant,dead,detail,alias


def main():
    with tempfile.TemporaryDirectory(prefix='race-save-') as tmp:
        path=Path(tmp)
        for generator in ('text/textgen.py','data/datagen.py'):
            subprocess.run([sys.executable,str(ROOT/generator),str(path)],check=True,stdout=subprocess.DEVNULL)
        actual=bodies().replace('uint16_t IRaceChecksum(','static uint16_t NativeChecksum(',1)
        actual+='\n'+function('src/core/save.c','void WriteRtPlr(').replace('void WriteRtPlr(','static void NativeWritePlayer(',1)
        actual+='\n'+function('src/core/file.c','void STARS_TEST_IMPL(StreamClose)(')
        actual+='\n'+function('src/ui/libraryui.c','int16_t FSaveLibraryRace(')
        # Keep base snapshot/comparison adapters available without artificial calls.
        harness=HARNESS.replace('unsigned inMask;', '(void)FSameLibraryRace;(void)SaveRaceState;(void)RestoreRaceState;unsigned inMask;')
        c=path/'save.c';c.write_text(declarations()+'\n'+PRELUDE+'\n'+actual+'\n'+harness);binary=path/'save'
        subprocess.run(shlex.split(os.environ.get('CC','cc'))+['-std=gnu11','-Wall','-Wextra','-Werror','-Wno-pointer-sign','-fsanitize=address,undefined','-fno-sanitize-recover=all',
            '-I'+str(ROOT/'src/core'),'-I'+str(ROOT/'src/platform'),'-I'+str(path),str(c),'-lm','-o',str(binary)],check=True)
        data=''.join(' '.join(map(str,c))+'\n' for c in cases())
        out=path/'output.jsonl'
        with out.open('w') as stream:subprocess.run([str(binary)],input=data,text=True,stdout=stream,check=True)
        count=0;statuses=set();traces=set()
        for line in out.open():
            case=json.loads(line);want=expected(case)
            for key,value in want.items():assert case[key]==value,(count,key,case, want)
            statuses.add(case['result']);traces.add(tuple(case['events']));count+=1
        assert statuses=={-2,0,1}
        print(f'{count} actual staged-save/writer/checksum/close cases; {len(traces)} controller call traces; alias/global/file/binding frames checked')
        print('Escaped=-2 is a harness observation, not a native FALSE return; close failure resets hf and still permits publication. Native crypto/BOF, allocator, OS and crash qualification remain open.')


if __name__=='__main__':main()
