#!/usr/bin/env python3
"""Native capacity/resources/installation/growth differential fixtures.

Actual common.h structs, fresh IDs, and actual habitat/race/planet/growth bodies
execute against an independent width-aware numeric interpreter. A second habitat
mode supplies raw signed helper results; growth can also be a declared provider.
Undefined vectors are filtered before C, not counted as native error returns.
"""
from pathlib import Path
import math
import random
import struct
import subprocess
import tempfile


class Boundary(Exception):
    pass


def checked(n, why):
    if not -(2**31) <= n < 2**31:
        raise Boundary(why)
    return n


def signed(n, width):
    n %= 2**width
    return n if n < 2**(width-1) else n-2**width


def div(n, d):
    if not d or (n == -(2**31) and d == -1):
        raise Boundary('division')
    return (abs(n)//abs(d)) * (-1 if (n<0)!=(d<0) else 1)


def fields(c, player):
    if player not in (0,1):
        raise Boundary('player index')
    return c[8:16] if player==0 else c[29:37]


def habitat(c, stored=False):
    if c[17]:
        return c[16]
    x=c[19 if stored else 18]
    if x<0:
        return -3*min(15,-x)
    if x>100:
        return -3*min(15,x-100)
    distance=abs(x-50); q=100-distance*2; modifier=10000;penalty=distance*2-50
    if penalty>0:
        for _ in range(3):
            modifier=modifier*(100-penalty)//100
    return int(math.sqrt(float(3*q*q)/3.0)+0.9)*modifier//10000


def capacity(c, player):
    primary,_,_,_,_,_,_,obrm=fields(c,player)
    if primary==8:
        if c[27]!=player or not c[28]:
            return 0
        relative=signed(c[22],16)
        if c[21]>=10 or not 0<=relative<5:
            raise Boundary('base/population index')
        value=[2500,5000,10000,20000,30000][relative]
    else:
        hab=habitat(c,True);value=500 if hab<5 else hab*100
        if primary==0:
            value-=value//2
        elif primary==9:
            value+=value//5
    return value+(value//10 if obrm else 0)


def growth(c, update):
    pop,delta,owner=c[1],c[2],c[6]
    if owner==-1 or pop==0:
        return 0,pop,delta
    primary,_,_,_,_,ideal,_,_=fields(c,owner);hab=habitat(c)
    if hab<0:
        loss=max(1,div(checked(pop*signed(-hab,16),'hostile product'),10))
        whole=div(loss,100);fraction=loss-whole*100;outdelta=delta-fraction
        if outdelta<0:
            whole+=1;outdelta+=100
        change=-whole
    else:
        maximum=capacity(c,owner);rate=signed((ideal*2 if primary==0 else ideal)*hab,16)
        if pop>div(maximum,4):
            fullness=div(checked(pop*1000,'fullness product'),maximum)
            if pop>=maximum:
                if pop<checked(maximum+10,'dead band'):
                    return 0,pop,delta
                rate=max(-300,99-div(fullness,10))*4
            else:
                retard=checked(1000-fullness,'retardation');square=checked(retard*retard,'retardation square')
                product=checked((rate if rate<1000 else div(rate,10))*square,'crowding product')
                rate=checked(div(product,562500)*(1 if rate<1000 else 10),'crowding rescale')
        rough=checked(pop*div(rate,100),'rough growth product')
        amount=div(checked(pop*rate,'fine growth product'),100) if rough<10000000 else rough
        whole=div(amount,100);fraction=amount-whole*100
        if whole==0 and fraction==0:
            fraction=1
        outdelta=signed(delta+fraction,16)
        if outdelta>=100:
            whole+=1;outdelta-=100
        elif outdelta<0:
            whole-=1;outdelta+=100
        change=whole
    if update:
        return change,checked(pop+change,'population writeback'),outdelta%256
    return change,pop,delta


def absolute(c, player, factories):
    primary,_,_,factop,mineop,_,_,_=fields(c,player)
    count=max(10,div(checked(capacity(c,player)*(factop if factories else mineop),'absolute product'),100))
    return 0 if primary==8 else signed(count,16)


def operable(c, player, factories):
    maximum=absolute(c,player,factories);primary,_,_,factop,mineop,_,_,_=fields(c,player);pop=c[1]
    if c[23]:
        extra=c[26] if c[25] else growth(c,False)[0]
        pop=checked(pop+extra,'next population')
    current=div(checked(pop*(factop if factories else mineop),'operable product'),100)
    count=maximum if maximum<current else signed(current,16)
    return 0 if primary==8 else max(1,count)


def defense(c, player, next_year=False):
    primary=fields(c,player)[0];maximum=0 if primary==8 else min(100,max(10,habitat(c)*4))
    if not next_year:
        return maximum
    pop=c[1]
    if c[23]:
        pop=checked(pop+(c[26] if c[25] else growth(c,False)[0]),'next population')
    current=min(1000,div(checked(pop+24,'defense rounding'),25));word=signed(current,16)
    return 0 if primary==8 else min(maximum,word)


def effective(pop, maximum):
    if pop<=maximum:
        return pop
    half=checked(div(checked(pop-maximum,'effective subtraction'),2)+maximum,'effective addition')
    return min(half,checked(maximum*2,'effective cap'))


def floating(value):
    if not math.isfinite(value) or not -(2**31)<=math.trunc(value)<2**31:
        raise Boundary('float-to-int')
    return math.trunc(value),struct.unpack('<Q',struct.pack('<d',value))[0]


def reference(c):
    op,pop,delta,owner,player=c[0],c[1],c[2],c[6],c[7];bits=0
    if op==0:
        result=capacity(c,player)
    elif op in (1,2):
        result=absolute(c,player,op==2)
    elif op in (3,4):
        result=operable(c,player,op==4)
    elif op in (5,6):
        if owner==-1:
            result=0
        elif fields(c,owner)[0]==8:
            if op==6:
                result=0
            else:
                if pop<0:
                    raise Boundary('negative square root')
                value,bits=floating(math.sqrt(float(pop)));result=signed(value,16)
        else:
            now=c.copy();now[23]=0;result=min(c[3 if op==5 else 4],operable(now,owner,op==6))
    elif op in (7,8):
        result=defense(c,player,op==8)
    elif op==9:
        if pop==0:
            return 0,pop,delta,0
        primary,eff,prod,_,_,_,energy,_=fields(c,player);q=effective(pop,capacity(c,player))
        if primary==8:
            if not eff:
                raise Boundary('float division zero')
            radicand=float(q)*max(1,energy)/eff
            if radicand<0:
                raise Boundary('negative square root')
            value,bits=floating(math.sqrt(radicand)*max(25,habitat(c))/10+0.999)
            result=signed(value,16)
        else:
            colonists=signed(div(q,eff),16);now=c.copy();now[23]=0;facts=min(c[4],operable(now,player,True))
            contribution=div(signed(checked(facts*prod,'factory product')+9,32),10)
            result=signed(colonists+contribution,16)
        if not result:
            result=1
    elif op==10:
        maximum=capacity(c,player)
        if maximum<=0:
            result=0
        else:
            numerator=signed(checked(pop*100,'capacity display product')+div(maximum,2),32)
            percentage=div(numerator,maximum);result=999 if percentage>=1000 else signed(percentage,16)
    else:
        result,pop,delta=growth(c,bool(c[24]))
    return result,pop,delta,bits


def base():
    # op,pop,delta,M/F/D,owner/query, player0 eight fields, habitat/mode,
    # live/stored environment, base/slot/hull, next/update, growth mode/value,
    # lookup owner/base, player1 eight fields.
    return [0,10000,37,500,1000,50,0,0,1,10,10,10,10,10,10,0,100,0,50,50,0,0,0,0,0,0,0,0,0,
            9,7,15,12,13,15,20,1]


def cases():
    for op in range(12):
        c=base();c[0]=op;yield c
    for hab in range(-32768,32768):
        for op in (0,7,8):
            c=base();c[0]=op;c[16]=hab;c[17]=1;yield c
    for raw in range(-128,128):
        for pop in (-2147483648,-327690,-1,0,1,10000,655360,2147483647):
            for op in (1,2,3,4,9):
                c=base();c[0]=op;c[1]=pop;c[9]=raw;c[10]=raw;c[11]=raw;c[12]=raw;yield c
    for delta in range(256):
        for pop in (1,99,100,2500,10000,10005,10010,100000,3000000):
            for hab in (-45,-1,0,100):
                for update in (0,1):
                    c=base();c[0]=11;c[1]=pop;c[2]=delta;c[16]=hab;c[17]=1;c[24]=update;yield c
    for primary in (0,1,8,9):
        for pop in (0,1,2,4,100,1600,32768,65536,100000,2147483647):
            for eff in (-128,-1,0,1,10,127):
                for energy in (-128,0,1,127):
                    for op in (5,6,9):
                        c=base();c[0]=op;c[1]=pop;c[8]=primary;c[9]=eff;c[14]=energy;c[20]=c[28]=1;yield c
    for primary in (0,1,8,9):
        for owner in (-1,0,1):
            for query in (0,1):
                for slot in (0,9,10,15):
                    for hull in (-1,0,1,4,5,32767,-32768):
                        for op in (0,1,2,9):
                            c=base();c[0]=op;c[6]=c[27]=owner;c[7]=query;c[8]=c[29]=primary;c[20]=c[28]=1;c[21]=slot;c[22]=hull;yield c
    rng=random.Random(0xC0107)
    for _ in range(12000):
        c=base();c[0]=rng.randrange(12);c[1]=rng.choice([0,1,100,10000,21474836,1000000,-1000024]);c[2]=rng.randrange(256)
        c[3:6]=[rng.randrange(4096) for _ in range(3)];c[8]=rng.choice([0,1,8,9]);c[9:15]=[rng.randrange(-128,128) for _ in range(6)]
        c[18]=rng.randrange(-128,128);c[19]=rng.randrange(-128,128);c[20]=c[28]=rng.randrange(2);c[22]=rng.randrange(5);c[23]=rng.randrange(2);c[24]=rng.randrange(2);yield c
    c=base();c[0]=9;c[1]=0;c[7]=-1;yield c # source early return before player access
    c=base();c[0]=9;c[1]=1600;c[8]=8;c[9]=c[14]=1;c[16]=16384;c[17]=1;c[20]=c[28]=1;yield c
    c=base();c[0]=9;c[9]=1;c[10]=c[11]=127;c[4]=4095;yield c
    c=base();c[0]=9;c[1]=65536;c[9]=1;c[10]=0;c[4]=0;c[16]=1000;c[17]=1;yield c
    c=base();c[0]=10;c[1]=21474836;yield c
    for hab in (-32768,-32767,-100,-1,0,100,32767):
        for update in (0,1):
            c=base();c[0]=11;c[1]=1;c[16]=hab;c[17]=1;c[24]=update;yield c
    for growth_value in (-2147483648,-1,0,1,100000,2147483647):
        for op in (3,4,8):
            c=base();c[0]=op;c[23]=c[25]=1;c[26]=growth_value;yield c
    # Query-player installation rates and owner-player growth are distinct.
    for owner in (0,1):
        for player in (0,1):
            for next_year in (0,1):
                for op in (3,4,5,6,8,9,11):
                    c=base();c[0]=op;c[1]=2500;c[6]=c[27]=owner;c[7]=player;c[23]=next_year;c[24]=1;yield c


def main():
    with tempfile.TemporaryDirectory(prefix='native-colony-') as work:
        from linked_witness import binary as linked_binary
        binary=linked_binary('native-colony');path=Path(work);data=path/'vectors.bin';excluded={};count=0
        with data.open('wb') as stream:
            for c in cases():
                assert len(c)==37
                try:
                    result=reference(c)
                except Boundary as why:
                    excluded[str(why)]=excluded.get(str(why),0)+1
                    continue
                stream.write(struct.pack('<37i3qQ',*c,*result));count+=1
        print(f'Prepared {count} defined native colony vectors; excluded before C: {excluded}',flush=True)
        subprocess.run([str(binary),str(data)],check=True)


if __name__=="__main__":main()
