#!/usr/bin/env python3
"""Actual FCanBuildShdef composed with actual catalog/trait/technology helpers.

Checks hull signed-word/byte mapping, early failure, occupied-slot order,
headless/player context, restoration and whole native input frames. Candidates
that reach invalid player/slot reads are excluded before C, not recovered by it.
"""
from pathlib import Path
import random
import struct
import subprocess
import tempfile
from check_part_availability_source import LIMITS,policy,tech


class Boundary(Exception):pass


def base():
    return [7,0,0,0,9,0,65535,0,0,0]+[26]*6+[value for _ in range(16) for value in (8,0,0)]


def cases():
    for hull in range(65536):
        c=base();c[2]=hull;yield c
    for saved in range(-32768,32768):
        c=base();c[0]=saved;yield c
    for selected in range(-32768,32768):
        c=base();c[1]=selected;yield c
    for count in range(256):
        for failure in (-2,-1,0,7,15):
            c=base();c[3]=count
            if failure==-2:c[2]=255 # unavailable hull exits before slot count matters
            elif failure>=0:c[16+3*failure:19+3*failure]=[3,255,1]
            yield c
    # Every category row in every slot: slot traversal reuses the actual lookup.
    for primary in range(10):
        for level in (0,26):
            for kind,n in enumerate(LIMITS):
                for item in range(n+1):
                    for at in range(16):
                        c=base();c[1]=primary;c[3]=at+1;c[4]=primary;c[5]=0xffffffff if at%2 else 0;c[6]=65535 if item%2 else 0;c[7]=at;c[10:16]=[level]*6
                        c[16+3*at:19+3*at]=[1<<kind,item,255];yield c
    # All occupancy masks distinguish skipped garbage from requested parts.
    for occupied in range(65536):
        c=base();c[3]=16;c[8]=occupied%2;c[9]=(occupied//2)%2
        for at in range(16):c[16+3*at:19+3*at]=[8 if occupied&(1<<at) else 65535,0 if occupied&(1<<at) else 255,1 if occupied&(1<<at) else 0]
        yield c
    for selected in (-1,0,1,15):
        for flags in (0,0xffffffff):
            for failure in range(-1,16):
                c=base();c[1]=selected;c[3]=16;c[5]=flags;c[8]=1;c[9]=1
                for at in range(16):c[16+3*at:19+3*at]=[8,0,1+(at*17)%255]
                if failure>=0:c[16+3*failure:19+3*failure]=[3,0,1]
                yield c
    rng=random.Random(110416)
    for _ in range(20000):
        c=base();c[:10]=[rng.randrange(-32768,32768),rng.choice([-1]+list(range(16))),rng.randrange(65536),rng.randrange(18),rng.randrange(-128,128),rng.randrange(1<<32),rng.randrange(65536),rng.randrange(16),rng.randrange(2),rng.randrange(2)]
        c[10:16]=[rng.randrange(-128,128) for _ in range(6)]
        for at in range(16):c[16+3*at:19+3*at]=[rng.choice([3,0]+[1<<k for k in range(16)]),rng.randrange(256),rng.choice([0,1,255])]
        yield c


def reference(c,table):
    saved,selected,hull,count,primary,flags,grants,current=c[:8]
    if selected!=-1 and not 0<=selected<16:raise Boundary('lookup player index')
    queries=[];requests=[];headless=selected==-1
    def lookup(mask,item,quantity):
        kind=mask.bit_length()-1 if mask and mask&(mask-1)==0 else 16
        start=len(queries);calls=[] if headless else [0]
        if (kind,item) not in table:status=0
        else:
            need,ability=table[kind,item];allowed,extra=policy(headless,primary,flags,grants,kind,item,ability);calls+=extra
            if not allowed:status=-1
            elif headless:status=1
            else:status=tech(c[10:16],need,current);calls+=[2]
        requests.append([selected,mask,item,quantity,status,start]);queries.extend(calls)
        return status==1
    signed=hull if hull<32768 else hull-65536
    result=lookup(1024 if signed>=32 else 16384,(hull-(32 if signed>=32 else 0))%256,-1)
    if result:
        for at in range(count):
            if at>=16:raise Boundary('design slot read')
            mask,item,quantity=c[16+3*at:19+3*at]
            if quantity and not lookup(mask,item,quantity):result=False;break
    assert len(requests)<=17 and len(queries)<=128
    return [int(result),saved,len(requests),len(queries)]+[v for row in requests+[[-1]*6]*(17-len(requests)) for v in row]+queries+[-1]*(128-len(queries))


def main():
    from linked_witness import binary as linked_binary
    with tempfile.TemporaryDirectory(prefix='design-buildability-') as work:
        path=Path(work);binary=linked_binary('design-buildability')
        export=path/'catalog.bin';subprocess.run([str(binary),'export',str(export)],check=True)
        table={row[:2]:(row[2:8],row[8]) for row in struct.iter_unpack('<9i',export.read_bytes())}
        assert len(table)==sum(LIMITS) and all((k,i) in table for k,n in enumerate(LIMITS) for i in range(n))
        data=path/'vectors.bin';count=0;excluded={}
        with data.open('wb') as stream:
            for c in cases():
                assert len(c)==64
                try:expected=reference(c,table)
                except Boundary as why:excluded[str(why)]=excluded.get(str(why),0)+1;continue
                assert len(expected)==234
                c[5]=c[5] if c[5]<0x80000000 else c[5]-0x100000000
                stream.write(struct.pack('<298i',*c,*expected));count+=1
        print(f'Prepared {count} defined cases; excluded before C: {excluded}',flush=True)
        subprocess.run([str(binary),'verify',str(data)],check=True)


if __name__=='__main__':main()
