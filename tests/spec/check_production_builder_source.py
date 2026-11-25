#!/usr/bin/env python3
"""CBuildProdItem differential fixtures over native packed PROD/PLANET.

The normal production implementation is linked. Quotes/capacity/environment/message services
are explicit fixtures. The independent numeric interpreter rejects undefined
signed operations before C execution; no sanitizer failure is an expected pass.
This checks bounded vectors, not a proof of C/Dafny refinement or host closure.
"""
from pathlib import Path
import random
import struct
import subprocess
import tempfile


class Boundary(Exception):
    pass


def i32(n):
    if not -(2**31) <= n < 2**31:
        raise Boundary('signed overflow')
    return n


def signed(n, width):
    n %= 2**width
    return n if n < 2**(width-1) else n-2**width


def div(n, d):
    return (abs(n)//d) * (-1 if n < 0 else 1)


def word(row):
    count, item, kind, pct, unused = row
    return count + (item << 10) + (kind << 17) + (pct << 20) + (unused << 27)


def reference(row, partial, cost, resources, limits, flags):
    """Source-ordered numeric trace with every signed intermediate checked."""
    row=list(row); original=row.copy(); resources=list(resources)
    operable, installed, terra, growth, desire, warp, destination=limits
    alchemy, cheap, calc, generating, has_partial, has_status=flags
    partial=list(partial); automatic=int(row[2]==1 and row[1]<7); manual=0
    calls=[0]  # quote precedes every auto helper, encoded as decimal digits
    if automatic:
        item=row[1]; cap=1000
        if item in (0,1,2):
            calls.append(item+1); cap=signed(operable-((installed+(0,17,29)[item])%4096),16)
            manual=(8,7,9)[item]
        elif item==3:
            manual=11
        elif item in (4,5):
            calls.append(4); cap=terra; manual=12
            if cap>0 and item==4:
                calls.append(5)
                if growth>=0:
                    calls.append(6)
                    if desire>0:
                        cap=0
        else:
            manual=17; calls.append(7)
            if warp==0 or destination==0:
                cap=0
        cap=max(0,cap)
        if row[0]>cap or item==3:
            row[0]=cap%1024
    paid=[(i32(c*row[3]) % 2**32)//100 for c in cost]
    initial=row[3]; built=conversions=0; need=None; turns=0
    while row[0]>0:
        turns+=1
        if turns>12000:
            raise Boundary('trace budget')
        affordable=True
        for i in range(4):
            if i32(cost[i]-paid[i])>resources[i]:
                affordable=False
                break
        if affordable:
            built+=1; row[0]-=1; row[3]=0
            for i in range(4):
                resources[i]=i32(resources[i]-i32(cost[i]-paid[i])); paid[i]=0
            continue
        mineral=resource=False; pct=100
        for i in range(4):
            if cost[i]>0:
                if resources[i]>=cost[i]:
                    t=100
                else:
                    n=i32(resources[i]+paid[i]); high=i32(n+1)
                    t=max(div(i32(n*100),cost[i]),div(i32(high*100),cost[i])-1)
                if t<pct:
                    need=i32(i32(cost[i]-paid[i])-resources[i]); pct=t
                    if i==3:
                        resource=True
                    else:
                        mineral=True
        if mineral and automatic:
            if not alchemy:
                automatic=2
                break
        else:
            for i in range(4):
                charge=i32(div(i32(cost[i]*pct),100)-paid[i])
                resources[i]=i32(resources[i]-charge); paid[i]=i32(paid[i]+charge)
            row[3]=pct%128
            if not alchemy or resource:
                break
        if need is None:
            raise Boundary('uninitialized need')
        price=25 if cheap else 100
        amount=min(div(resources[3],price),need)
        if amount>0:
            for i in range(3):
                resources[i]=i32(resources[i]+amount)
            product=i32(price*amount)
            resources[3]=signed(resources[3]-(product%2**32),32)
            conversions=signed(conversions+amount%65536,16)
        if amount==need:
            continue
        if resources[3]>0 and has_partial:
            pct=max(div(i32(resources[3]*100),price),div(i32(i32(resources[3]+1)*100),price)-1)
            partial=[1,11,1,pct%128,0]
            resources[3]=i32(resources[3]-div(i32(pct*price),100))
        break
    if built>0 and row[2]==1 and row[1] in (3,11):
        conversions=signed(conversions+built,16)
        for i in range(3):
            resources[i]=i32(resources[i]+built)
    message=int(conversions!=0 and not calc and generating)
    if automatic==2:
        status=4 if built==0 else 3
    elif automatic and row[0]==0:
        status=2 if built==0 else 1
    elif built==0:
        status=7 if initial==row[3] else 6
    elif row[0]==0:
        status=0
    else:
        status=5
    if automatic and has_partial and partial[0]==0 and row[3]>0:
        partial=row.copy(); partial[0]=1; partial[1]=manual
    output=original if calc or automatic else row
    if message:
        calls.append(8)
    call_word=0
    for call in calls:
        call_word=call_word*10+call
    return [built,status if has_status else -123,word(output),word(partial),*resources,message,
            conversions if message else 0,call_word]


def cases():
    ordinary=(50,0,0,-1,0,10,5)
    # Exhaust every packed percentage, four quote/stock dimensions, and the
    # automatic/manual, alchemy, preview and partial-pointer branches.
    for pct in range(128):
        for item in range(18):
            for mode in range(16):
                cost=[7,11,13,10]
                if item in (3,11):
                    cost=[0,0,0,25 if mode&2 else 100]
                yield ([3,item,1,pct,31],[0,63,7,127,30],cost,
                       [5,9,4,55],ordinary,[mode&1,(mode>>1)&1,(mode>>2)&1,(mode>>3)&1,1,1])
    for item in range(7):
        for operable in (-32768,-1,0,1,1000,32767):
            for installed in (0,1,4095):
                for terra in (-1,0,1,1023,32767):
                    for growth in (-1,0):
                        for desire in (-1,0,1):
                            for warp in (0,10):
                                for dest in (0,5):
                                    yield ([3,item,1,0,7],[0,0,0,0,31],[0,0,0,1],[0,0,0,10],
                                           (operable,installed,terra,growth,desire,warp,dest),[0,0,0,0,1,1])
    rng=random.Random(0xC8B017)
    for _ in range(16000):
        item=rng.randrange(128); kind=rng.randrange(8)
        cost=[rng.choice([0,1,3,10,25,100,65535]) for _ in range(4)]
        yield ([rng.choice([0,1,2,10,1023]),item,kind,rng.randrange(128),rng.randrange(32)],
               [rng.choice([0,1,2]),rng.randrange(128),rng.randrange(8),rng.randrange(128),rng.randrange(32)],
               cost,[rng.choice([-10,-1,0,5,100,10000]) for _ in range(4)],ordinary,
               [rng.randrange(2) for _ in range(6)])
    # Actual input domain boundaries: defined signed/unsigned casts versus
    # signed-intermediate and missing-need boundaries, never executing UB.
    for c in (-(2**31),-100,-1,0,1,2**31-1):
        for r in (-(2**31),-1,0,1,2**31-1):
            for pct in (0,1,100,127):
                yield ([1,7,1,pct,0],[0,0,0,0,0],[c,c,c,c],[r,r,r,r],ordinary,[1,0,0,1,1,1])
    # Target alchemy/direct-manual/preservation and status vectors.
    for item in (0,1,2,3,4,5,6,7,8,9,11,12,17):
        for partial_count in (0,1):
            for cheap in (0,1):
                for res in (0,1,24,25,26,99,100,101,500):
                    yield ([3,item,1,0,31],[partial_count,63,7,127,30],[10,0,0,0],[0,0,0,res],
                           ordinary,[1,cheap,0,1,1,1])
    # Large but defined deficits exercise LOWORD and signed-16 conversion
    # payloads without overflowing the signed resource-price product.
    for amount in (32767,32768,40000,65535,65536,70000):
        for cheap in (0,1):
            yield ([1,0,2,0,31],[0,0,0,0,0],[amount,0,0,0],
                   [0,0,0,amount*(25 if cheap else 100)],ordinary,[1,cheap,0,1,1,1])


def main():
    with tempfile.TemporaryDirectory(prefix='m09-builder-') as work:
        from linked_witness import binary as linked_binary
        binary=linked_binary("production-builder");path=Path(work);data=path/'vectors.bin'
        excluded={}; count=0; statuses=set()
        with data.open('wb') as output:
            for row,partial,cost,resources,limits,flags in cases():
                try:
                    expected=reference(row,partial,cost,resources,limits,flags)
                except Boundary as why:
                    excluded[str(why)]=excluded.get(str(why),0)+1
                    continue
                values=[signed(word(row),32),signed(word(partial),32),*cost,*resources,*limits,*flags]
                output.write(struct.pack('<23i11q',*values,*expected)); count+=1;statuses.add(expected[1])
        assert set(range(8))<=statuses, statuses
        # Prove the fixture filter exercises both named undefined boundaries.
        assert excluded.get('signed overflow',0)>0 and excluded.get('uninitialized need',0)>0
        print(f'Prepared {count} defined vectors; excluded before C execution: {excluded}',flush=True)
        subprocess.run([str(binary),str(data)],check=True)

if __name__=="__main__":main()
