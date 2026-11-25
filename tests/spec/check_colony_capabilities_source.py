#!/usr/bin/env python3
"""Actual terraforming totals, first-slot gate range and scanner fallback.

Reuses real lookup/catalog/terraform/native structs, compares independent
numeric/selection references, and runs production/application handoffs. Native
undefined reads and message-provider domains are excluded before C separately.
"""
from pathlib import Path
import random
import struct
import subprocess
import tempfile
from check_terraform_source import base as terra_base,cases as terra_cases,reference as terra_reference
from check_part_availability_source import policy,tech
from check_native_colony_source import signed,Boundary


def base(op):
    result=terra_base()[:54]+[0,1,0,1,16,0,1,255]+[0,0,0]*16
    result[0]=op
    return result


def reference(c,table,special,planetary):
    op=c[0];current=c[3];owner=c[4];extra=[0,-1,-1,-1,-1]+[-1]*64
    if op==0:
        count_input=c[:54];count_input[6]=1  # Ipct always requests helpful bounds.
        state=terra_reference(count_input,table)
        state[0]=signed(sum((c[8+i]-state[5+i] if state[5+i]!=-1 else 0)+(state[8+i]-c[8+i] if state[8+i]!=-1 else 0) for i in range(3)),16) if state[0] else 0
        return state+extra
    state=[0,current]+c[8:11]+[9876]*9+[0,0,0]+[-1]*128+[-999]*64
    assert len(state)==209
    trace=[];chosen=-1
    def lookup(mask,item,quantity):
        if current!=-1 and not 0<=current<16:raise Boundary('capability lookup player')
        row=(special if mask==512 else planetary)[item]
        if c[1]:status=1 if c[2]&(1<<item) else -1
        else:
            headless=current==-1;at=38 if current==1 else 32;primary=c[51] if current==1 else c[50]
            flags=(2 if (c[45] if current==1 else c[44]) else 0)|((1<<10) if c[52] else 0)
            allowed,_=policy(headless,primary,flags,0,9 if mask==512 else 15,item,row[6] if mask==32768 else 0)
            status=-1 if not allowed else 1 if headless else tech(c[at:at+6],row[:6],c[53])
        trace.append([mask,item,status,quantity]);return status
    if op==1:
        direct,has,index,present,count=c[54:59];index=index if direct else index%16
        if not direct and (owner==-1 or not has):return state+extra
        if not 0<=owner<16:raise Boundary('gate roster owner')
        if not present:raise Boundary('gate roster pointer')
        if not 0<=index<10:raise Boundary('gate design index')
        for at in range(count):
            if at==16:raise Boundary('gate hull slot')
            mask,item,quantity=c[62+3*at:65+3*at]
            if mask==512 and quantity>0 and 0<=item<=6:
                lookup(mask,item,quantity);value=c[47] if c[1] and item==c[46] else special[item][6]
                state[0]=10000 if value==-1 else value;break
    else:
        if op==3:
            if not 0<=owner<16:raise Boundary('message provider owner')
            current=owner
        for item in range(8,-1,-1):
            status=lookup(32768,item,-1 if op==3 else c[61]);chosen=item
            if status==1:break
        state[0]=1 if op==3 else chosen;state[1]=-1 if op==3 else current;state[16]=1 if op==3 else 0
    extra=[len(trace),chosen,trace[-1][0] if trace else -1,trace[-1][3] if trace else -1,current if trace else -1]
    return state+extra+sum(trace,[])+[-1]*(64-4*len(trace))


def cases():
    for op in (3,2,1,0):yield base(op)
    c=base(0);c[8]=c[11]=-23;c[14]=-50;c[17]=-75;c[20]=-25;yield c
    for c in terra_cases():
        if c[0]==0:yield c+[0,1,0,1,16,0,1,255]+[0,0,0]*16
    for mask in range(65536):
        for item in (0,6,7):
            c=base(1);c[62:65]=[mask,item,255];yield c
    for word in range(-32768,32768):
        c=base(1);c[1]=1;c[46:48]=[0,word];c[62:68]=[512,0,2,512,6,1];yield c
        for op in (1,2,3):
            c=base(op);c[3]=word;yield c
        c=base(1);c[4]=word;c[55]=0;yield c
        c=base(1);c[54]=1;c[56]=word;yield c
    for count in range(256):
        for candidate in (-1,0,15):
            for detail in (0,7):
                c=base(1);c[58]=count;c[59]=detail
                if candidate>=0:c[62+3*candidate:65+3*candidate]=[512,1,255]
                yield c
    for owner in (-1,0,1,15,16):
        for direct in (0,1):
            for index in (-1,0,9,10,15):
                for roster in (0,1):
                    c=base(1);c[4]=owner;c[54]=direct;c[56]=index;c[57]=roster;c[62:65]=[512,0,1];yield c
    for mask in range(512):
        for op in (2,3):
            for qty in (0,1,255):
                c=base(op);c[1]=1;c[2]=mask;c[61]=qty;yield c
    for level in range(-128,128):
        for field in range(6):
            for primary in (0,7,8,9):
                for op in (1,2,3):
                    c=base(op);c[3]=0;c[32+field]=level;c[50]=primary;c[62:65]=[512,6,2];yield c
    for primary in range(-128,128):
        for current in (-1,0,1,15):
            for no_scanners in (0,1):
                for op in (1,2,3):
                    c=base(op);c[3]=current;c[50]=c[51]=primary;c[52]=no_scanners;c[62:65]=[512,4,1];yield c
    rng=random.Random(8082027)
    for _ in range(20000):
        op=rng.choice([1,2,3]);c=base(op);c[1]=rng.randrange(2);c[2]=rng.randrange(65536);c[3]=rng.randrange(-1,17);c[4]=rng.randrange(-1,17)
        c[32:44]=[rng.randrange(-128,128) for _ in range(12)];c[50:54]=[rng.randrange(-128,128),rng.randrange(-128,128),rng.randrange(2),rng.randrange(16)]
        c[54:62]=[rng.randrange(2),rng.randrange(2),rng.randrange(-1,17),rng.randrange(2),rng.randrange(18),rng.randrange(8),rng.randrange(2),rng.randrange(256)]
        for at in range(16):c[62+3*at:65+3*at]=[rng.choice([0,512,1536]),rng.randrange(256),rng.randrange(256)]
        yield c


def main():
    from linked_witness import binary as linked_binary
    binary=linked_binary("colony-capabilities")
    with tempfile.TemporaryDirectory(prefix='colony-capabilities-') as work:
        path=Path(work)
        export=path/'catalog.bin';subprocess.run([str(binary),'export',str(export)],check=True);raw=export.read_bytes();table=list(struct.iter_unpack('<8i',raw[:640]));special=list(struct.iter_unpack('<7i',raw[640:1088]));planetary=list(struct.iter_unpack('<7i',raw[1088:]));assert (len(table),len(special),len(planetary))==(20,16,15)
        count=0;excluded={};process=subprocess.Popen([str(binary),'verify','-'],stdin=subprocess.PIPE)
        try:
            with process.stdin as stream:
                for c in cases():
                    assert len(c)==110
                    try:out=reference(c,table,special,planetary)
                    except Boundary as why:excluded[str(why)]=excluded.get(str(why),0)+1;continue
                    assert len(out)==278
                    stream.write(struct.pack('<388i',*c,*out));count+=1
        except BrokenPipeError:process.wait();raise
        result=process.wait();print(f'Prepared {count} cases; pre-C exclusions: {excluded}',flush=True)
        if result:raise subprocess.CalledProcessError(result,process.args)


if __name__=='__main__':main()
