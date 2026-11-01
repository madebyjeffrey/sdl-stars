#!/usr/bin/env python3
"""Compare actual lookup/technology/hidden bodies, fresh catalogs and native frames.

The independent reference preserves source short-circuit query order. Catalog
requirements/planetary ability come from an independently indexed native export;
all versioned model identities and limits bind to fresh generated enums.
No game source, catalog or baseline is modified.
"""
from pathlib import Path
import random
import re
import struct
import subprocess
import tempfile
from linked_witness import ROOT

LIMITS=[16,16,10,12,24,12,15,8,10,16,5,17,11,20,32,15]
IDENTITY_SOURCE=(ROOT/'spec/dafny/S03-lookup-identities.dfy').read_text()
IDENTITIES={name:int(value) for name,value in re.findall(r'const (\w+): nat := (\d+)',IDENTITY_SOURCE)}
assert set(re.findall(r'const (\w+)',IDENTITY_SOURCE))==set(IDENTITIES), 'unsupported model identity declaration'
I=type('Identities',(),IDENTITIES)
TRADER={(0,I.iengineEnigmaPulsar):512,(2,I.ishieldLangstonShell):4,(3,I.iarmorMegaPolyShell):8,
        (4,I.ibeamMultiContainedMunition):128,(5,I.itorpAntiMatterTorpedo):64,(6,I.ibombHushABoom):32,
        (7,I.iminingAlienMiner):16,(11,I.ispecialEMultiFunctionPod):2,(12,I.ispecialMMultiCargoPod):1,
        (12,I.ispecialMJumpGate):2048,(14,I.ihuldefMiniMorph):256,(15,I.iplanetaryGenesisDevice):1024}


def tech(have,need,current):
    missing=[i for i in range(6) if have[i]<need[i]]
    if not missing:return 1
    if missing==[current]:return 2 if need[current]==have[current]+1 else need[current]-have[current]+1
    return 99


def hidden(headless,grants,kind,item):
    flag=TRADER.get((kind,item),0)
    return not headless and flag!=0 and grants&flag==0


def policy(headless,primary,flags,grants,kind,item,ability):
    calls=[]
    def bit(index):calls.append(100+index);return bool(flags&(1<<index))
    def visible():calls.append(1);return not hidden(headless,grants,kind,item)
    ra=9 if headless else primary
    if headless and kind not in (2,3,4):return True,calls
    allowed=True
    if kind==0:
        if item==I.iengineSettlersDelight and ra!=0:allowed=False
        elif (I.iengineSubGalacticFuelScoop<=item<=I.iengineGalaxyScoop or item==I.iengineRadiatingHydroRamScoop) and bit(7):allowed=False
        elif item in (I.iengineGalaxyScoop,I.iengineFuelMizer) and not bit(0):allowed=False
        elif item==I.iengineInterspace10 and not bit(7):allowed=False
        else:allowed=visible()
    elif kind==1:
        if item in (I.iscannerFerretScanner,I.iscannerDolphinScanner,I.iscannerElephantScanner) and bit(10):allowed=False
        elif item in (I.iscannerChameleonScanner,I.iscannerPickPocketScanner,I.iscannerRobberBaronScanner) and ra!=1:allowed=False
    elif kind==2:
        allowed=not ((item==I.ishieldShadowShield and ra!=1) or (item==I.ishieldCrobySharmor and ra!=4)) and visible()
    elif kind==3:
        allowed=not ((item==I.iarmorDepletedNeutronium and ra!=1) or (item==I.iarmorFieldedKelarium and ra!=4)) and visible()
    elif kind==4:
        allowed=not ((item==I.ibeamMiniGun and ra!=4) or (item in (I.ibeamBlunderbuss,I.ibeamGatlingNeutrinoCannon) and ra!=2)) and visible()
    elif kind==5:allowed=visible()
    elif kind==6:
        allowed=not ((I.ibombSmartBomb<=item<=I.ibombAnnihilatorBomb and ra==4) or (item==I.ibombRetroBomb and ra!=3)) and visible()
    elif kind==7:
        if item in (I.iminingRoboMiner,I.iminingRoboMaxiMiner,I.iminingRoboSuperMiner,I.iminingRoboMidgetMiner,I.iminingRoboUltraMiner) and bit(9):allowed=False
        elif item in (I.iminingRoboMidgetMiner,I.iminingRoboUltraMiner) and not bit(2):allowed=False
        elif item==I.iminingOrbitalAdjuster and ra!=3:allowed=False
        else:allowed=visible()
    elif kind==8:
        if item in (I.iminesMineDispenser40,I.iminesMineDispenser80,I.iminesMineDispenser130,I.iminesHeavyDispenser50,I.iminesHeavyDispenser110,I.iminesHeavyDispenser200,I.iminesSpeedTrap30,I.iminesSpeedTrap50) and ra!=5:allowed=False
        elif item==I.iminesSpeedTrap20 and ra not in (4,5):allowed=False
        elif item==I.iminesMineDispenser50 and ra==2:allowed=False
    elif kind==9:
        if I.ispecialSBMassDriver5<=item<=I.ispecialSBUltraDriver13:allowed=item in (I.ispecialSBMassDriver7,I.ispecialSBUltraDriver10) or ra==6
        elif I.ispecialSBStargate100250<=item<=I.ispecialSBStargateAnyAny:
            allowed=not (ra!=7 and (item==I.ispecialSBStargateAny300 or item>=I.ispecialSBStargate100Any)) and ra!=0
    elif kind==10:
        if item in (I.isbhullSpaceDock,I.isbhullUltraStation) and not bit(3):allowed=False
        elif item==I.isbhullDeathStar and ra!=8:allowed=False
    elif kind==11:
        allowed=visible()
        if allowed:
            allowed=not ((item in (I.ispecialETransportCloaking,I.ispecialEUltraStealthCloak) and ra!=1) or
                         (item==I.ispecialEEnergyDampener and ra!=5) or (item==I.ispecialEAntiMatterGenerator and ra!=7) or
                         (item==I.ispecialEFluxCapacitor and ra!=0) or
                         (item in (I.ispecialEJammer10,I.ispecialEJammer50,I.ispecialETachyonDetector) and ra!=4))
    elif kind==12:
        allowed=visible()
        if allowed:allowed=not (item==I.ispecialMColonizationModule and ra==8) and (item!=I.ispecialMOrbitalConstructionModule or ra==8)
    elif kind==13:
        if I.iterraTotalTerraform3<=item<=I.iterraTotalTerraform30:allowed=bit(1)
    elif kind==14:
        if (item in (I.ihuldefMiniColonyShip,I.ihuldefMetaMorph) and ra!=0) or (item in (I.ihuldefFuelTransport,I.ihuldefSuperFreighter) and ra!=4):allowed=False
        elif item in (I.ihuldefMiner,I.ihuldefMaxiMiner,I.ihuldefMidgetMiner,I.ihuldefUltraMiner) and bit(9):allowed=False
        elif item in (I.ihuldefMidgetMiner,I.ihuldefMiner,I.ihuldefUltraMiner) and not bit(2):allowed=False
        elif ((item in (I.ihuldefDreadnought,I.ihuldefBattleCruiser) and ra!=2) or
              (item in (I.ihuldefRogue,I.ihuldefStealthBomber) and ra!=1) or
              (item in (I.ihuldefMiniMineLayer,I.ihuldefSuperMineLayer) and ra!=5)):allowed=False
        else:allowed=visible()
    elif kind==15:
        scanner=I.iplanetaryViewer50<=item<=I.iplanetarySnooper620X
        if scanner and ability<0 and bit(10):allowed=False
        elif (scanner and ra==8) or (I.iplanetarySDI<=item<=I.iplanetaryNeutronShield and ra==8) or (I.iplanetaryLaserBattery<=item<=I.iplanetaryNeutronShield and ra==2):allowed=False
        else:allowed=visible()
    return allowed,calls


def cases():
    relevant=(0,1,2,3,7,9,10)
    for primary in range(10):
        for combination in range(128):
            flags=sum(1<<bit for i,bit in enumerate(relevant) if combination&(1<<i))
            levels=[(primary+combination)%27]*6
            for grants in (0,65535):
                for kind,count in enumerate(LIMITS):
                    for item in range(count+1):yield [0,0,primary,flags,grants,(combination+primary)%16,1<<kind,item,combination%256,combination%16]+levels+[0]*6
    for primary in range(-128,128):
        for flags in (0,0xffffffff):
            for kind,count in enumerate(LIMITS):
                for item in range(count+1):yield [0,0,primary,flags,0,15,1<<kind,item,255,15]+[26]*6+[0]*6
    for mask in range(65536):
        for headless in (0,1):
            for mode in (0,1):yield [mode,headless,9,0xffffffff,0,0,mask,0,0,0]+[0]*12
    for kind in range(16):
        for item in range(256):
            for headless in (0,1):yield [0,headless,4,0,0,0,1<<kind,item,255,0]+[0]*12
    # Every trader-grant word is checked directly for every special trader part.
    for grants in range(65536):
        for (kind,item) in TRADER:yield [1,0,0,0,grants,0,1<<kind,item,0,0]+[0]*12
    for have in range(-128,128):
        for need in range(-128,128):
            for current in (0,1,15):yield [2,0,0,0,0,current,1,0,0,0]+[have,0,0,0,0,0]+[need,0,0,0,0,0]
    for missing in range(64):
        for current in range(16):yield [2,0,0,0,0,current,1,0,0,0]+[0]*6+[int(bool(missing&(1<<i))) for i in range(6)]
    rng=random.Random(110312)
    for _ in range(10000):
        kind=rng.randrange(16);yield [0,rng.randrange(2),rng.randrange(-128,128),rng.randrange(1<<32),rng.randrange(1<<16),rng.randrange(16),1<<kind,rng.randrange(256),rng.randrange(256),rng.randrange(16)]+[rng.randrange(-128,128) for _ in range(12)]


def main():
    from linked_witness import binary as linked_binary
    with tempfile.TemporaryDirectory(prefix='part-availability-') as work:
        path=Path(work);binary=linked_binary('part-availability')
        export=path/'catalog.bin';subprocess.run([str(binary),'export',str(export)],check=True)
        table={}
        for row in struct.iter_unpack('<9i',export.read_bytes()):table[row[0],row[1]]=(row[2:8],row[8])
        assert len(table)==sum(LIMITS) and all((k,i) in table for k,count in enumerate(LIMITS) for i in range(count))
        model_masks=re.search(r'function Masks\(\).*?\{\s*\[([0-9,\s]+)\]\s*\}',(ROOT/'spec/dafny/S03-research-catalog.dfy').read_text(),re.S)
        assert model_masks and [int(v) for v in model_masks[1].split(',')]==[1<<k for k in range(16)]
        model_limits=re.search(r'function Limits\(\).*?\{\s*\[([0-9,\s]+)\]\s*\}',(ROOT/'spec/dafny/S03-research-catalog.dfy').read_text(),re.S)
        assert model_limits and [int(v) for v in model_limits[1].split(',')]==LIMITS
        data=path/'vectors.bin';count=0
        with data.open('wb') as stream:
            for c in cases():
                mode,headless,primary,flags,grants,current,mask,item=c[:8];kind=(mask.bit_length()-1) if mask and mask&(mask-1)==0 else 16
                if mode==2:status=tech(c[10:16],c[16:22],current);calls=[2]
                elif mode==1:status=int(hidden(headless,grants,kind,item));calls=[1]
                else:
                    calls=[] if headless else [0]
                    if (kind,item) not in table:status=0
                    else:
                        need,ability=table[kind,item];allowed,queries=policy(headless,primary,flags,grants,kind,item,ability);calls+=queries
                        if not allowed:status=-1
                        elif headless:status=1
                        else:status=tech(c[10:16],need,current);calls+=[2]
                assert len(c)==22 and len(calls)<=8
                c[3]=flags if flags<0x80000000 else flags-0x100000000
                stream.write(struct.pack('<33i',*c,status,0,len(calls),*(calls+[-1]*(8-len(calls)))));count+=1
        print(f'Prepared {count} cases using {len(table)} actual catalog rows; {len(IDENTITIES)} model identities and native enum/bit protocols bound',flush=True)
        subprocess.run([str(binary),'verify',str(data)],check=True)


if __name__=='__main__':main()
