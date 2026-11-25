#!/usr/bin/env python3
"""Actual research quote/preview controllers with native records and width-aware reference.

Cost getters and scalar cost body are actual source; preview resource/estimator
callbacks are explicit post-state providers, including controlled escapes.
Undefined arithmetic/index candidates are excluded before C execution.
"""
from pathlib import Path
import random
import struct
import subprocess
import tempfile
from check_native_colony_source import Boundary, checked, signed, div

TECH=[0,50,80,130,210,340,550,890,1440,2330,3770,6100,9870,13850,18040,22440,27050,31870,36900,42140,47590,53250,59120,65200,71490,77990,84700]


def mix(hash_value,*values):
    for value in values:hash_value=((hash_value*16777619)^(value&0xffffffff))&0xffffffff
    return hash_value


def reference(c):
    result=[0]*22;levels=c[4:10].copy();choices=c[10:16];hash_value=0;calls=0
    def cost(field,level):
        nonlocal hash_value,calls
        if not 0<=field<6 or not 0<=level<=26:raise Boundary('cost field/table index')
        base=signed(sum(levels)*10,16)+TECH[level]
        choice=choices[field];value=2*base-base//4 if choice<1 else base if choice==1 else div(base,2)
        value=checked(value*(2 if c[1] else 1),'cost arithmetic')
        calls+=1;hash_value=mix(hash_value,field,level,sum(levels),value)
        return value
    if c[0]==0:
        result[0]=cost(c[2],c[3])
    elif c[0]==1:
        targets=c[16:22]
        if any(level>26 for level in targets):result[0]=-1
        else:
            original=levels.copy();total=0
            for field in range(6):
                if targets[field]>levels[field]:
                    stored=signed(c[22+field],32)
                    scaled=checked(stored*(2 if c[1] else 1),'slow stored-spending multiplication')
                    current=checked(-scaled,'development spent negation')
                    while targets[field]>levels[field]:
                        current=checked(current+cost(field,levels[field]+1),'development field addition')
                        levels[field]+=1
                    total=checked(total+max(0,current),'development total addition')
            levels=original;result[0]=total
    else:
        player=c[29];alloc=[signed(c[30],8),signed(c[31],8)]+[0]*14
        if not 0<=player<16:raise Boundary('preview entry player index')
        saved=alloc[player];alloc[player]=signed(c[28],8);count=c[32];global_count=count
        if not 0<=count<=32767:raise Boundary('planet extent')
        planets=[c[38+5*(i%4):43+5*(i%4)].copy() for i in range(count)]
        gross_calls=forecast_calls=0;complete=True;total=0;mutated=False
        def callback(kind,index,argument):
            nonlocal player,calls,hash_value,mutated,global_count,gross_calls,forecast_calls,complete
            calls+=1;hash_value=mix(hash_value,kind,index,argument,alloc[0],alloc[1])
            if kind==1:gross_calls+=1
            else:forecast_calls+=1
            if (kind==1 or c[34]&128) and not mutated:
                if c[34]&1:player=c[35]
                if c[34]&2:
                    if not 0<=player<16:raise Boundary('callback player index')
                    alloc[player]=signed(c[36],8)
                if c[34]&4 and planets[index][4]!=-2:planets[index][1]=planets[index][4]
                if c[34]&8:global_count=0
                if c[34]&16 and index+1<count:planets[index+1][0]=player
                mutated=True
            if calls-1==c[33]:complete=False;return None
            return planets[index][2 if kind==1 else 3]
        for index in range(count):
            if planets[index][0]!=player:continue
            value=callback(1,index,player)
            if not complete:break
            if planets[index][1]>0:
                value=callback(2,index,player)
                if not complete:break
            total=checked(total+value,'preview total addition')
        if complete:
            if not 0<=player<16:raise Boundary('preview restore player index')
            alloc[player]=saved
        result[:10]=[total,alloc[0],alloc[1],player,calls,gross_calls,forecast_calls,hash_value,int(complete),global_count]
        for i in range(min(4,count)):
            result[10+i]=planets[i][0];result[14+i]=planets[i][1]
        result[18:22]=[alloc[i] for i in (2,3,14,15)]
        return result
    result[1:7]=levels;result[7]=calls;result[8]=hash_value
    result[9:15]=[value%4294967296 for value in c[22:28]];result[15:21]=choices
    return result


def base(mode):
    c=[0]*64;c[0]=mode;c[3]=1;c[10:16]=[1]*6;c[29]=0;c[30]=10;c[31]=20;c[32]=4;c[33]=-1;c[35]=1;c[36]=-1
    for i in range(4):c[38+5*i:43+5*i]=[i%2,(-1,0,1,1)[i],100+i,10+i,-2]
    return c


def cases():
    for choice in range(-128,128):
        for value in (-128,-1,0,26,127):
            for level in range(27):
                for slow in (0,1):
                    c=base(0);c[1]=slow;c[3]=level;c[4:10]=[value]*6;c[10]=choice;yield c
    for field in range(6):
        for initial in (-2,-1,0,1,25,26,127):
            for target in (-128,-1,0,1,6,26,27,127):
                for spent in (0,1,50,2147483647,-2147483648,-1):
                    for choice in (-128,0,1,2,127):
                        for slow in (0,1):
                            c=base(1);c[1]=slow;c[4+field]=initial;c[16+field]=target;c[22+field]=spent;c[10+field]=choice;yield c
    rng=random.Random(0xD3E3109)
    for _ in range(6000):
        c=base(rng.randrange(2));c[1]=rng.randrange(2);c[2]=rng.randrange(6);c[3]=rng.randrange(27)
        c[4:10]=[rng.randrange(27) for _ in range(6)];c[10:16]=[rng.randrange(3) for _ in range(6)]
        c[16:22]=[rng.randrange(27) for _ in range(6)];c[22:28]=[rng.randrange(300000) for _ in range(6)];yield c
    for pct in (-2147483648,-129,-128,-1,0,100,127,128,255,256,2147483647):
        for player in (0,1):
            for count in (0,1,4):
                for flags in (0,1,2,4,8,16,3,7,128|3):
                    for queue in (-1,0,1):
                        for after in (-2,-1,0,1):
                            for fail in (-1,0,1,2):
                                c=base(2);c[28]=pct;c[29]=player;c[32]=count;c[34]=flags;c[33]=fail
                                for i in range(4):c[39+5*i]=queue;c[42+5*i]=after
                                yield c
    for value in (-32768,-1,0,1,32767):
        for queue in (-1,0,1):
            c=base(2);c[32]=32767
            for i in range(4):c[38+5*i:43+5*i]=[0,queue,value,value,-2]
            yield c
    for player in (-1,16):
        c=base(2);c[29]=player;yield c
    for flags in (1,3):
        c=base(2);c[34]=flags;c[35]=-1;yield c
    for level in (-1,27):
        c=base(0);c[3]=level;yield c
    c=base(1);c[16:22]=[1,1,0,0,0,0];yield c
    for fields in (2,3):
        c=base(1);c[16:22]=[1 if i<fields else 0 for i in range(6)]
        c[22:28]=[-1073741000 if i<fields else 0 for i in range(6)];yield c
    for player in (2,15):
        c=base(2);c[29]=player;yield c
    c=base(2);c[34]=1;c[35]=15;yield c


def main():
    with tempfile.TemporaryDirectory(prefix='research-contract-') as work:
        from linked_witness import binary as linked_binary
        binary=linked_binary('research');path=Path(work);data=path/'vectors.bin';excluded={};count=0
        with data.open('wb') as stream:
            for c in cases():
                assert len(c)==64
                try:result=reference(c)
                except Boundary as why:excluded[str(why)]=excluded.get(str(why),0)+1;continue
                stream.write(struct.pack('<64i22q',*c,*result));count+=1
        print(f'Prepared {count} defined research vectors; excluded before C: {excluded}',flush=True)
        subprocess.run([str(binary),str(data)],check=True)


if __name__=="__main__":main()
