#!/usr/bin/env python3
"""Player labels/titles using actual helpers, generated text and native formats.

Only SetWindowText and last-error storage are scripted. Known undefined input
domains are classified by the independent oracle and excluded before C runs.
"""
import json
import os
from pathlib import Path
import random
import re
import shlex
import subprocess
import sys
import tempfile
from check_s02_source import ROOT, function


def catalog():
    rows = {}
    for line in (ROOT / 'text/strings.txt').read_text().splitlines():
        match = re.fullmatch(r'(0x[0-9a-fA-F]+) (\w+) (".*")', line)
        if match and int(match[1], 16) in (270,271,*range(1369,1375),*range(1383,1390)):
            rows[int(match[1], 16)] = (match[2], json.loads(match[3]).encode())
    return rows


PRELUDE = r'''
#include "common.h"
#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#define WINAPI
#define WINAPIV
#define ERROR_INVALID_PARAMETER 87
typedef char *LPSTR;typedef const char *LPCSTR;
PLAYER rgplr[16];char szWork[360],szLastStrGet[256],iLastStrGet;
_Static_assert(sizeof(PLAYER)==192 && offsetof(PLAYER,szName)==128 && offsetof(PLAYER,szNames)==160,"native name projection");
int16_t fRCWReadOnly;static int first,requested,errorCode;
static char title[360];
static void hex(const void *data,size_t n){const unsigned char *p=data;putchar('"');for(size_t i=0;i<n;i++)printf("%02x",p[i]);putchar('"');}
static void SetLastError(int n){errorCode=n;}
static int SetWindowText(HWND hwnd,const char *text){assert(hwnd==(HWND)1);requested=1;strcpy(title,text);return 0;}
static void unhex(const char *s,unsigned char *out,size_t cap){size_t n=!strcmp(s,"-")?0:strlen(s)/2;assert(n<cap);for(size_t i=0;i<n;i++){unsigned b;assert(sscanf(s+2*i,"%2x",&b)==1);out[i]=(unsigned char)b;}out[n]=0;}
'''

HARNESS = r'''
int main(void){int kind,index,capital,plural,article,word,explicitRow,readOnly,step;
 char a[65],b[65],fmt[2049],format[1025];
 while(scanf("%d %d %d %d %d %d %d %d %d %64s %64s %2048s",&kind,&index,&capital,&plural,&article,&word,&explicitRow,&readOnly,&step,a,b,fmt)==12){
  PLAYER row,before[16],saved;unsigned char name[33],names[33];
  unhex(a,name,sizeof(name));unhex(b,names,sizeof(names));unhex(fmt,(unsigned char *)format,sizeof(format));
  memset(&row,92,sizeof(row));memcpy(row.szName,name,32);memcpy(row.szNames,names,32);
  for(int i=0;i<16;i++)rgplr[i]=row;
  memcpy(before,rgplr,sizeof(before));saved=row;memset(szWork,170,sizeof(szWork));memset(szLastStrGet,187,sizeof(szLastStrGet));
  iLastStrGet=-1;fRCWReadOnly=(int16_t)readOnly;first=1;requested=0;errorCode=0;title[0]=0;int n=-1;
  printf("{\"lookups\":[");
  if(kind==0){char *p=PszPlayerName((int16_t)index,(int16_t)capital,(int16_t)plural,(int16_t)article,(int16_t)word,explicitRow?&row:NULL);assert(p==szWork);}
  else if(kind==1)SetRCWTitle((HWND)1,(int16_t)step);
  else if(kind==2)n=CchSprintf(szWork,format,index);
  else if(kind==3)n=wsprintfA(szWork,format,index);
  else {char *p=PszGetCompressedString((StringId)index);assert(p==szLastStrGet);}
  assert(!memcmp(before,rgplr,sizeof(before)) && !memcmp(&saved,&row,sizeof(row)));
  printf("],\"work\":");hex(szWork,sizeof(szWork));printf(",\"last\":");hex(szLastStrGet,sizeof(szLastStrGet));
  printf(",\"cache\":%d,\"requested\":%d,\"title\":",iLastStrGet,requested);hex(title,strlen(title));printf(",\"n\":%d,\"error\":%d}\n",n,errorCode);
 }
 return 0;
}
'''


def field(text):
    assert len(text) < 32 and b'\0' not in text
    return text + b'\0' + b'Z' * (31 - len(text))


def label(name, names, index, capital, plural, article, word):
    """Mirror named Dafny rules without executing invalid native reads."""
    calls = []
    if not name[0]:
        out = b'Player ' + str(index + 1).encode() + (b'' if plural else b"'s")
        calls.append(1374)
        grammar = {1: (1372, b' has'), 2: (1373, b' is')}
    else:
        selected = names if plural and names[0] else name
        if b'\0' not in selected:
            return None
        out = ((b'The ' if capital else b'the ') if article else b'') + selected.split(b'\0')[0]
        if len(out) >= 50:
            return None
        out = out.rstrip(b' ')
        if not out:
            return None
        if plural and not names[0]:
            if out == b'e':
                return None
            if out[-1:] != b's' and out[-2:] != b'se':
                out += b's'
        grammar = {1: (1370, b' have'), 2: (1371, b' are')}
    if word in grammar:
        row, suffix = grammar[word]
        calls.append(row)
        out += suffix
    return (out, calls) if len(out) < 50 else None


def vector(kind=0, name=b'Humanoid', names=b'', index=0, capital=1, plural=1,
           article=0, word=0, explicit=1, read_only=0, step=1, fmt=b'%d'):
    return (kind, index, capital, plural, article, word, explicit, read_only, step,
            field(name), field(names), fmt)


def inputs(rows):
    for name in (b'Humanoid', b'Mouse', b'horse', b's', b'S', b'e', b'E', b'a ', b'   ', b'',
                 b' Leading  ', b'X'*31, 'é🌟'.encode(), b'\xff'):
        for names in (b'', b'People ', b'e', b'   '):
            for capital, plural, article, word in ((0,0,0,0),(1,1,0,0),(-7,-7,-7,1),(0,1,1,2),(1,0,1,1),(1,1,0,3)):
                yield vector(name=name, names=names, capital=capital, plural=plural, article=article, word=word)
    for index in (-32768, -1, 0, 15, 32767):
        for plural in (0, 1, -7):
            for word in (-1, 0, 1, 2, 3):
                yield vector(name=b'', index=index, plural=plural, word=word)
    for index in range(16):
        yield vector(index=index, explicit=0)
    for index in (-1,16,32767):
        yield vector(index=index,explicit=0)
    for row in range(1383,1390):
        yield vector(name=rows[row][1])
    # Unterminated fields are only used when the source does not reach strlen.
    value = list(vector(name=b''));value[10] = b'X'*32;yield tuple(value)
    value = list(vector(plural=0));value[10] = b'X'*32;yield tuple(value)
    value = list(vector(names=b'People'));value[9] = b'X'*32;yield tuple(value)
    value = list(vector());value[9] = b'X'*32;yield tuple(value)
    value = list(vector());value[10] = b'X'*32;yield tuple(value)
    for row in (270,271,*range(1369,1375),*range(1383,1390)):
        yield vector(kind=4,index=row)
        for step in (-32768,0,1,6,32767):
            yield vector(kind=1,read_only=row-270,step=step)
    for i in (0,1,6,10,20,-1,-32768,32767,-2147483648,2147483647):
        for fmt in (b'%d',b'%d%%',b'1 in %d',b'literal %%',b'plain',b'',b'%%d %d %%'):
            for kind in (2,3):
                yield vector(kind=kind,index=i,fmt=fmt)
    rng = random.Random(1104)
    for _ in range(400):
        name=bytes(rng.choice(b'abcse SE ') for _ in range(rng.randrange(32)))
        names=bytes(rng.choice(b'xyzse ') for _ in range(rng.randrange(32)))
        yield vector(name=name,names=names,capital=rng.choice([0,1,-7]),plural=rng.choice([0,1,-7]),article=rng.choice([0,1,-7]),word=rng.randrange(-1,4))


def render(fmt, n):
    result = bytearray();at=0;used=0
    while at<len(fmt):
        if fmt[at]!=37:result.append(fmt[at]);at+=1;continue
        assert at+1<len(fmt) and fmt[at+1] in (37,100)
        if fmt[at+1]==37:result.append(37)
        else:used+=1;assert used==1;result.extend(str(n).encode())
        at+=2
    return bytes(result)


def main():
    rows=catalog();vectors=[];expected=[];excluded=0
    for value in inputs(rows):
        kind,index,capital,plural,article,word,explicit,read_only,step,name,names,fmt=value
        result=label(name,names,index,capital,plural,article,word) if kind==0 else None
        if kind==0 and (result is None or not explicit and not 0<=index<16):excluded+=1;continue
        calls=[];work=b'\xaa'*360;last=b'\xbb'*256;title=b'';n=-1
        if kind==0:out,calls=result
        elif kind==1:calls=[(270+read_only)%65536];out=render(rows[calls[0]][1],step);title=out
        elif kind in (2,3):out=render(fmt,index);n=len(out)
        else:calls=[index];out=None
        if out is not None:work=out+b'\0'+work[len(out)+1:]
        for row in calls:
            data=rows[row][1];last=data+b'\0'+last[len(data)+1:]
        expected.append(dict(lookups=calls,work=work.hex(),last=last.hex(),cache=-1,requested=int(kind==1),title=title.hex(),n=n,error=0))
        vectors.append(value)
    with tempfile.TemporaryDirectory(prefix='player-labels-') as tmp:
        path=Path(tmp)
        for generator in ('text/textgen.py','data/datagen.py'):
            subprocess.run([sys.executable,str(ROOT/generator),str(path)],check=True,stdout=subprocess.DEVNULL)
        bodies='\n'.join([
            '#define PszGetCompressedString NativePszGetCompressedString',
            function('src/core/strings.c','char *PszGetCompressedString('),
            '#undef PszGetCompressedString',
            'char *PszGetCompressedString(StringId id){if(!first)putchar(44);first=0;printf("%u",id);return NativePszGetCompressedString(id);}',
            function('src/core/utilgen.c','int16_t CchGetString('),
            function('src/platform/encoding.c','int CbUtf8Char('),
            function('src/platform/encoding.c','size_t CchUtf8Prefix('),
            function('src/platform/native.c','int CchSprintf('),
            function('compat/win32/src/services.c','int WINAPIV wsprintfA('),
            function('src/core/util.c','char *PszPlayerName('),
            '#pragma GCC diagnostic push\n#pragma GCC diagnostic ignored "-Wunused-but-set-variable"',
            function('src/ui/raceui.c','void SetRCWTitle('),
            '#pragma GCC diagnostic pop',
        ])
        source=path/'labels.c';binary=path/'labels'
        source.write_text(PRELUDE+'\n'+(path/'text_strings.c').read_text()+'\n#define wsprintf wsprintfA\n'+bodies+'\n'+HARNESS)
        subprocess.run(shlex.split(os.environ.get('CC','cc'))+['-std=gnu11','-Wall','-Wextra','-Werror','-Wno-pointer-sign','-fsanitize=address,undefined','-fno-sanitize-recover=all',
            '-I'+str(ROOT/'src/core'),'-I'+str(ROOT/'src/platform'),'-I'+str(path),str(source),'-o',str(binary)],check=True)
        data=''.join(' '.join([*(str(n) for n in v[:9]),*(b.hex() or '-' for b in v[9:])])+'\n' for v in vectors)
        run=subprocess.run([str(binary)],input=data,text=True,stdout=subprocess.PIPE,check=True)
        actual=[json.loads(line) for line in run.stdout.splitlines()]
        assert len(actual)==len(expected)
        for i,(got,want) in enumerate(zip(actual,expected)):
            assert got==want,(i,vectors[i],got,want)
    # Bind the formal catalog snapshot to the same freshly generated rows.
    formal=(ROOT/'spec/dafny/S03-player-labels.dfy').read_text()
    snapshots={int(i):json.loads('"'+s+'"').encode() for i,s in re.findall(r'id==(\d+) then Text\(B\("([^"]*)"\)\)',formal)}
    assert len(snapshots)==8
    for i,text in snapshots.items():assert rows[i][1]==text
    defaults=re.search(r'function Defaults\(\).*?\{(.*?)\n  \}',formal,re.S)[1]
    names=[s.encode() for s in re.findall(r'B\("([^"]*)"\)',defaults)]
    assert names==[rows[i][1] for i in range(1383,1390)]
    print(f'Player labels/title/source-format cases: {len(vectors)} passed; {excluded} undefined-domain inputs classified before execution')
    print('15 generated catalog rows match the formal snapshot; full player bytes, scratch tails, lookup order and ignored SetWindowText failure checked.')


if __name__=='__main__':main()
