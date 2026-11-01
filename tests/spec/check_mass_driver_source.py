#!/usr/bin/env python3
"""Actual IWarpMAFromLppl plus construction/packet integration.

Native common.h structs and generated enums; a separate word/slot interpreter
supplies expectations. Reached invalid native reads are excluded before C.
Output is null or separate live storage; aliasing/lifetime is not inferred.
"""
from pathlib import Path
import random
import struct
import subprocess
import tempfile


class Boundary(Exception):
    pass


def base():
    return [0,1,0,0,1,7,16,1]+[0,0,0]*16


def reference(c):
    owner,has,index,current,roster,detail,count,output=c[:8]
    two=0 if output else 1234
    if owner==-1 or not has:return [0,two]
    if not 0<=owner<16:raise Boundary('roster owner')
    if not roster:raise Boundary('roster pointer')
    if index>=10:raise Boundary('base design read')
    if current not in (-1,owner) and detail!=7:return [0,two]
    if count>16:raise Boundary('hull slot read')
    values=[c[9+3*i]-2 for i in range(count) if c[8+3*i]==512 and c[10+3*i]>0 and 7<=c[9+3*i]<=15]
    warp=max(values,default=0)
    return [warp,int(values.count(warp)>1) if output else 1234]


def cases():
    for mask in range(65536):
        for item in (7,15,16):
            c=base();c[8:11]=[mask,item,255];yield c
    for word in range(-32768,32768):
        for has in (0,1):
            c=base();c[0]=word;c[1]=has;yield c
        for detail in (0,7):
            c=base();c[3]=word;c[5]=detail;c[8:14]=[512,7,2,512,7,1];yield c
    for item in range(256):
        for quantity in range(256):
            c=base();c[8:11]=[512,item,quantity];yield c
    for position in range(16):
        for count in range(17):
            for item in range(18):
                for output in (0,1):
                    c=base();c[6]=count;c[7]=output;c[8+3*position:11+3*position]=[512,item,2];yield c
    for count in range(256):
        for detail in (0,7):
            for current in (-1,0,1):
                for output in (0,1):
                    c=base();c[3]=current;c[5]=detail;c[6]=count;c[7]=output;yield c
    for owner in range(-2,18):
        for index in range(16):
            for roster in (0,1):
                for current in (-1,owner,17):
                    for output in (0,1):
                        c=base();c[:8]=[owner,1,index,current,roster,0,255,output];yield c
    for a in range(18):
        for b in range(18):
            for d in range(18):
                c=base();c[8:17]=[512,a,2,512,b,1,512,d,1];yield c
    rng=random.Random(80819)
    for _ in range(20000):
        c=base();c[:8]=[rng.randrange(-1,17),rng.randrange(2),rng.randrange(16),rng.randrange(-2,18),rng.randrange(2),rng.randrange(8),rng.randrange(18),rng.randrange(2)]
        for i in range(16):c[8+3*i:11+3*i]=[rng.choice([512,1536,65535,0]),rng.randrange(256),rng.randrange(256)]
        yield c


def main():
    from linked_witness import binary as linked_binary
    binary=linked_binary("mass-driver")
    with tempfile.TemporaryDirectory(prefix='mass-driver-') as work:
        path=Path(work)
        vectors=path/'vectors.bin';count=0;excluded={}
        with vectors.open('wb') as stream:
            for c in cases():
                try:out=reference(c)
                except Boundary as why:excluded[str(why)]=excluded.get(str(why),0)+1;continue
                stream.write(struct.pack('<58i',*c,*out));count+=1
        print(f'Prepared {count} cases; excluded before C: {excluded}',flush=True)
        subprocess.run([str(binary),str(vectors)],check=True)


if __name__=='__main__':main()
