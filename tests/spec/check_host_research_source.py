#!/usr/bin/env python3
"""Native UpdateResearchStatus with actual cost/race bodies and declared part/message services.

Independent interpreter preserves signed arithmetic guards, unsigned progress,
redo/recursive passes, all equipment lookup/message ordering and packed policy.
No game source or baseline is changed. Invalid numeric/pointer candidates stop
before C. Missing-return fixtures use an explicit lookup longjmp provider.
"""
from pathlib import Path
import random
import struct
import subprocess
import tempfile
from check_native_colony_source import Boundary,checked,signed,div
from check_research_source import TECH,mix


class Escape(Exception):pass


def reference(c):
    n,use_pool,slow,mode,message_return,escape,initial=c[:7]
    players=[]
    for i in range(16):
        p=c[8+26*i:34+26*i]
        players.append(dict(primary=p[0],general=p[1],tt=p[2],dead=p[3],crippled=p[4],now=p[5],next=p[6],pool=p[7],
                            levels=p[8:14].copy(),choices=p[14:20],spent=[x%4294967296 for x in p[20:26]]))
    if n>16:raise Boundary('player array extent')
    global_player=initial;hash_value=0;cost_calls=lookup_calls=msg_calls=general_calls=tt_calls=primary_calls=entry_calls=0
    last_field=last_level=0;type_calls=[0]*16
    def event(*values):
        nonlocal hash_value
        hash_value=mix(hash_value,*values)
    def trait(player,kind):
        nonlocal general_calls,tt_calls,primary_calls
        event(30,player,kind)
        if kind==0:general_calls+=1;return players[player]['general']
        if kind==1:tt_calls+=1;return players[player]['tt']
        primary_calls+=1;return players[player]['primary']
    def message(player,tag,target,a,b,d):
        nonlocal msg_calls,last_field,last_level,type_calls
        a,b,d=signed(a,16),signed(b,16),signed(d,16)
        event(10,player,tag,signed(target,16),a,b,d);msg_calls+=1
        if tag in (0,1):last_field=b;last_level=a;type_calls=[0]*16
    def cost(player,field):
        nonlocal cost_calls
        p=players[player];level=p['levels'][field]+1
        if not 0<=level<=26:raise Boundary('technology cost table')
        total=sum(p['levels']);base=signed(total*10,16)+TECH[level];choice=p['choices'][field]
        value=(2*base-base//4 if choice<1 else base if choice==1 else div(base,2))*(2 if slow else 1)
        cost_calls+=1;event(20,player,field,level,total,value)
        return checked(value,'technology cost')
    def lowest(levels):
        best=0
        for i in range(1,6):
            if levels[i]<levels[best]%256:best=i
        return best
    def stored(value):
        return checked(value+1,'slow spending round addition')//2 if slow else value
    def part(player,kind,item):
        nonlocal lookup_calls
        slot=item%256;event(50,player,kind,slot);lookup_calls+=1
        if lookup_calls-1==escape:raise Escape()
        at=type_calls[kind];type_calls[kind]+=1
        limit=0 if mode==0 else 18 if mode in (1,2,6) else (kind%3+1) if mode==3 else 32769 if mode==5 else 258
        if at>=limit:return 0,0
        if mode==6:raise Boundary('part requirement pointer')
        if mode==1:
            availability=(1,-1,2,99,1)[slot%5];requirement=last_level if slot%5==0 else signed(last_level-1,8)
        else:availability=1;requirement=last_level
        return availability,requirement
    def announce(player,field,level):
        for kind in range(16):
            item=0
            while True:
                availability,requirement=part(player,kind,item)
                if availability==0:break
                if availability==1 and level==requirement%256:
                    if kind==10:tag,target=2,-3
                    elif kind==14:tag,target=3,-3
                    else:
                        if kind==13 and trait(player,1) and item in (8,12,16):item=signed(item+1,16);continue
                        tag=4 if kind==15 and 9<=item<=13 else 5 if kind==15 and 0<=item<=8 else 6
                        target=signed((kind<<8)|0xc000|item,16)
                    message(player,tag,target,field,signed(1<<kind,16),item)
                item=signed(item+1,16)
    def update(pool_flag):
        nonlocal entry_calls,global_player
        entry_calls+=1;event(40,int(pool_flag),global_player)
        alive=0;totals=[0]*6
        for player in range(n):
            p=players[player];using_pool=pool_flag;general=trait(player,0);current=p['now'];next_field=p['next'];global_player=player
            if not p['dead']:alive+=1
            while True:
                redo=False
                for field in range(6):
                    spent=signed(p['spent'][field],32);change=False
                    if slow:spent=checked(spent*2,'slow stored research multiplication')
                    if field==current and using_pool and general<2:
                        if general:
                            redo=True;general=2;amount=div(checked(p['pool']+1,'general primary round addition'),2)
                            spent=checked(spent+amount,'primary research addition');totals[field]=checked(totals[field]+amount,'shared primary field total')
                            for other in range(6):
                                if other!=field:
                                    amount=div(signed(checked(p['pool']*3,'general research pool multiplication')+19,32),20)
                                    p['spent'][other]=(p['spent'][other]+(div(amount,2) if slow else amount))%4294967296
                                    totals[other]=checked(totals[other]+amount,'shared secondary field total')
                        else:
                            spent=checked(spent+p['pool'],'primary research addition');totals[field]=checked(totals[field]+p['pool'],'shared primary field total')
                    while p['levels'][field]<26 and (not p['crippled'] or p['levels'][field]<10):
                        price=cost(player,field)
                        if price>spent:p['spent'][field]=stored(spent)%4294967296;break
                        target=current;spent=checked(spent-price,'research cost subtraction');p['levels'][field]+=1;level=p['levels'][field]
                        if level==26 and next_field==6:next_field=7
                        if current==field and next_field!=6:
                            target=next_field if next_field!=7 else lowest(p['levels']);change=True
                        message(player,0 if not general else 1,-2,level,field,target)
                        announce(player,field,level)
                        if (using_pool or change or next_field==7) and next_field!=6 and field==current:
                            if next_field==7:
                                target=lowest(p['levels']);p['now']=target%16;p['spent'][field]=0;current=target;next_field=7
                            else:
                                p['next']=6;p['now']=next_field%16;p['spent'][field]=0;current=next_field;next_field=6
                            remaining=stored(spent)
                            if not 0<=current<6:raise Boundary('redirected research field index')
                            p['spent'][current]=(p['spent'][current]+remaining)%4294967296;using_pool=False;redo=True
                            break
                if not redo:break
        global_player=-1;redo=False
        if pool_flag and alive>1:
            for player in range(n):
                if trait(player,2)==1:
                    for field in range(6):
                        if totals[field]>0:
                            amount=div(div(totals[field],alive),2)
                            if amount>1:
                                redo=True;message(player,7,-2,field,signed(amount,16),0)
                                players[player]['spent'][field]=(players[player]['spent'][field]+stored(amount))%4294967296
            if redo:update(False)
    complete=True
    try:update(bool(use_pool))
    except Escape:complete=False
    result=[]
    for p in players:result+=p['levels']+p['spent']+[(p['now']%16)+16*(p['next']%16),p['pool']]
    return result+[global_player,hash_value,cost_calls,lookup_calls,msg_calls,general_calls,tt_calls,primary_calls,entry_calls,int(complete)]


def base(n=1):
    c=[n,1,0,0,1,-1,5,0]
    for _ in range(16):c += [0,0,0,0,0,0,6,0]+[0]*6+[1]*6+[0]*6
    return c


def cases():
    for policy in range(256):
        for pool in (0,49,50,1000):
            for general in (0,1):
                for slow in (0,1):
                    c=base();c[2]=slow;c[8+1]=general;c[8+5]=policy%16;c[8+6]=signed(policy//16,4);c[8+7]=pool;yield c
    for level in (-128,-2,-1,0,9,10,25,26,127):
        for raw_spent in (0,1,49,50,100000,2147483647,-2147483648,-1):
            for pool in (-2147483648,-1,0,101,715827882,2147483647):
                for general in (0,1):
                    for slow in (0,1):
                        for use_pool in (0,1):
                            c=base();c[1]=use_pool;c[2]=slow;c[9]=general;c[15]=pool;c[16]=level;c[28]=raw_spent;yield c
    for n in (0,1,2,4,16):
        for alive in (0,1,2,16):
            for slow in (0,1):
                for general in (0,1):
                    c=base(n);c[2]=slow
                    for i in range(16):at=8+26*i;c[at]=1 if i%2==0 else 0;c[at+1]=general;c[at+3]=int(i>=alive);c[at+7]=101
                    yield c
    for mode in (0,1,2,3,4,6):
        for tt in (0,1):
            for message_return in (0,1):
                for escape in (-1,0,3,18):
                    c=base();c[3]=mode;c[4]=message_return;c[5]=escape;c[10]=tt;c[15]=50;yield c
    rng=random.Random(0xA12C0DE)
    for _ in range(5000):
        n=rng.randrange(1,5);c=base(n);c[1]=rng.randrange(2);c[2]=rng.randrange(2);c[3]=rng.choice([0,0,0,3]);c[4]=rng.randrange(2)
        for i in range(n):
            at=8+26*i;c[at:at+8]=[rng.choice([0,1,9]),rng.randrange(2),rng.randrange(2),rng.randrange(2),rng.randrange(2),rng.randrange(6),rng.choice([0,1,2,3,4,5,6,7]),rng.randrange(1500)]
            c[at+8:at+14]=[rng.randrange(27) for _ in range(6)];c[at+14:at+20]=[rng.randrange(3) for _ in range(6)];c[at+20:at+26]=[rng.randrange(200000) for _ in range(6)]
        yield c
    c=base();c[8+4]=1;c[8+8]=9;c[8+7]=100000;yield c
    c=base();c[8+8]=26;c[8+7]=101;yield c
    for tt in (0,1):
        c=base();c[3]=5;c[10]=tt;c[15]=50;yield c
    c=base(5)
    for i in range(5):c[8+26*i+7]=600000000
    yield c
    c=base();c[2]=1;c[14]=1;c[15]=1;c[16:22]=[-1,1,0,0,0,0];c[28]=1073741823;yield c
    for n in (-32768,-1):
        c=base(n);yield c
    for flag in (-32768,2):
        c=base();c[1]=flag;c[15]=50;yield c


def main():
    with tempfile.TemporaryDirectory(prefix='host-research-') as work:
        from linked_witness import binary as linked_binary
        binary=linked_binary("host-research");path=Path(work);data=path/'vectors.bin';excluded={};count=0
        with data.open('wb') as stream:
            for c in cases():
                assert len(c)==424
                try:result=reference(c)
                except Boundary as why:excluded[str(why)]=excluded.get(str(why),0)+1;continue
                stream.write(struct.pack('<424i234q',*c,*result));count+=1
        print(f'Prepared {count} host research vectors; excluded before C: {excluded}',flush=True)
        subprocess.run([str(binary),str(data)],check=True)
        subprocess.run([str(binary),"--catalog"],check=True)

if __name__=="__main__":main()
