#!/usr/bin/env python3
"""FBuildObject/Produce with actual build-check, availability and catalog bodies.

Economy, messages, warp, allocation and registration retain declared fixture
providers. Undefined owner/roster/design/check/metadata reads are filtered before
C. Gate success remains distinct from successful object application.
"""
from pathlib import Path
import random
import struct
import subprocess
import tempfile
from check_part_availability_source import LIMITS
from check_design_buildability_source import base as design_base,reference as design_reference,Boundary


def base():
    # item,base,owner,ships,bases,free,hull,count,mask,item,quantity,position,
    # primary,flags,grants,level,current,cargo,limit,saved,built,old slot,hull
    return [0,1,0,1,1,0,0,0,8,0,0,0,9,0,65535,26,0,0,0,3,1,9,32]


def cases():
    for item in range(-32768,32768):
        for has_base in (0,1):
            c=base();c[0]=item;c[1]=has_base;c[6]=32 if item>=16 else 0;yield c
    for owner in range(-32768,32768):
        c=base();c[1]=0;c[2]=owner;yield c
    for owner in range(-1,17):
        for item in (-1,0,15,16,25,26):
            for ships in (0,1):
                for bases in (0,1):
                    for free in (0,1):
                        c=base();c[0]=item;c[2]=owner;c[3]=ships;c[4]=bases;c[5]=free;c[6]=32 if item>=16 else 0;yield c
    for hull in range(65536):
        c=base();c[0]=16;c[6]=hull;yield c
    for owner in range(16):
        for item in range(26):
            for free in (0,1):
                for level in (0,26):
                    for variant in range(6):
                        c=base();c[0]=item;c[2]=owner;c[5]=free;c[6]=32 if item>=16 else 0;c[7]=16;c[11]=-1;c[12]=owner%10;c[13]=0xffffffff if variant%2 else 0;c[15]=level;c[17]=(0,1,65535)[variant%3];c[18]=variant//3;c[19]=(-1,0,1,15)[variant%4];c[20]=1+variant%2
                        if variant%3==1:c[8]=3;c[10]=1
                        elif variant%3==2:c[8]=8;c[9]=11;c[10]=255
                        yield c
    for count in range(256):
        for item in (0,16):
            for failure in (0,1,2):
                c=base();c[0]=item;c[6]=32 if item==16 else 0;c[7]=count
                if failure==1:c[6]=255
                if failure==2:c[8]=3;c[10]=1
                yield c
    rng=random.Random(190911)
    for _ in range(10000):
        c=base();c[:8]=[rng.randrange(26),rng.randrange(2),rng.randrange(16),rng.randrange(2),rng.randrange(2),rng.randrange(2),rng.randrange(37),rng.randrange(18)]
        c[8:21]=[rng.choice([0,3,65535]+[1<<k for k in range(16)]),rng.randrange(256),rng.choice([0,1,255]),rng.randrange(-1,16),rng.randrange(-128,128),rng.randrange(1<<32),rng.randrange(65536),rng.randrange(-128,128),rng.randrange(16),rng.choice([0,1,65535]),rng.randrange(2),rng.randrange(-32768,32768),rng.choice([1,2])]
        yield c


def reference(c,table):
    item,has_base,owner,ships,bases,free,hull,count=c[:8]
    empty=[-1]*230
    if item<16 and not has_base:return [0,0,0,0,0,0,0,c[19],0]+empty
    is_base=item>=16;index=item-16 if is_base else item
    if not 0<=owner<16:raise Boundary('roster owner')
    if not (bases if is_base else ships):raise Boundary('roster pointer')
    if index<0 or index>=(10 if is_base else 16):raise Boundary('design slot')
    if free:return [0,0,0,0,0,int(not is_base),0,c[19],0]+empty
    d=design_base();d[:10]=[c[19],owner,hull,count,c[12],c[13],c[14],c[16],free,0];d[10:16]=[c[15]]*6
    for at in range(16):
        if c[11]==-1 or at==c[11]:d[16+3*at:19+3*at]=c[8:11]
    check=design_reference(d,table);can=check[0]
    if can and is_base:
        signed=hull if hull<32768 else hull-65536
        if signed<0 or signed>=37:raise Boundary('base notification catalog')
    stage=0 if not can else 2 if is_base else 1
    applied=bool(can and (is_base or not c[18]))
    messages=1 if (not is_base or applied) else 0
    return [stage,int(applied),1,check[2],check[3],messages,int(is_base and can),c[19],int(applied and not is_base)]+check[4:]


def main():
    from linked_witness import binary as linked_binary
    binary=linked_binary("construction-buildability")
    with tempfile.TemporaryDirectory(prefix='construction-buildability-') as work:
        path=Path(work)
        export=path/'catalog.bin';subprocess.run([str(binary),'export',str(export)],check=True)
        table={r[:2]:(r[2:8],r[8]) for r in struct.iter_unpack('<9i',export.read_bytes())};assert len(table)==sum(LIMITS)
        vectors=path/'vectors.bin';count=0;excluded={}
        with vectors.open('wb') as stream:
            for c in cases():
                assert len(c)==23
                try:out=reference(c,table)
                except Boundary as why:excluded[str(why)]=excluded.get(str(why),0)+1;continue
                assert len(out)==239
                c[13]=c[13] if c[13]<0x80000000 else c[13]-0x100000000
                stream.write(struct.pack('<262i',*c,*out));count+=1
        print(f'Prepared {count} defined cases; excluded before C: {excluded}',flush=True)
        subprocess.run([str(binary),'verify',str(vectors)],check=True)


if __name__=='__main__':main()
