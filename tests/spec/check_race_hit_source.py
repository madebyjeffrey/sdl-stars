#!/usr/bin/env python3
"""Actual race hit test: defined cases and the growth-slot array boundary.

The two growth cases must stop at a sanitizer diagnostic. Their native return
value is deliberately neither read nor modeled as a supported result.
"""
from pathlib import Path
import re
import subprocess
import tempfile
from check_s02_source import function
from check_race_workflows_source import build


PRELUDE = r'''
int16_t fRCWReadOnly;
static int16_t crcRCW;
static RaceWizardPage iPanelActive;
static RECT rectangles[17],*vrgrcRCW=rectangles;
static POINT16 PointFrom16(POINT16 p){return p;}
static int PtInRect(const RECT *r,POINT16 p){
    return r->left<=p.x && p.x<r->right && r->top<=p.y && p.y<r->bottom;
}
_Static_assert(sizeof(vplr.rgEnvVarMax)/sizeof(vplr.rgEnvVarMax[0])==3,"three habitat maxima");
'''
HARNESS = r'''
int main(int argc,char **argv){
    assert(argc==5);int hit=atoi(argv[1]),ro=atoi(argv[2]),immune=atoi(argv[3]),panel=atoi(argv[4]);
    assert(-1<=hit && hit<17);fRCWReadOnly=ro;iPanelActive=panel;crcRCW=17;
    for(int i=0;i<17;i++)rectangles[i]=(RECT){10*i,0,10*i+8,8};
    for(int i=0;i<3;i++)vplr.rgEnvVarMax[i]=immune?-1:80;
    vplr.pctIdealGrowth=15;
    POINT16 pt={(int16_t)(hit<0?200:10*hit+1),1};
    printf("%d\n",IrcRaceDlgHitTest(pt));
    (void)FSameLibraryRace;(void)SaveRaceState;(void)RestoreRaceState;return 0;
}
'''


def main():
    cases = []
    for hit in range(-1, 15):
        for ro in (0, 1, -7):
            for immune in (0, 1):
                cases.append(((hit, ro, immune, 4), -1 if ro or hit < 0 or immune else hit))
    for hit in (15, 16):
        for ro in (1, -7):
            cases.append(((hit, ro, 0, 4), -1))
        for immune in (0, 1):
            cases.append(((hit, 0, immune, 5), hit))
    extra = PRELUDE + '\n' + function('src/ui/raceui.c', 'int16_t IrcRaceDlgHitTest(')
    with tempfile.TemporaryDirectory(prefix='race-hit-') as tmp:
        binary = build(Path(tmp), extra=extra, harness=HARNESS)
        for values, expected in cases:
            run = subprocess.run([str(binary), *map(str, values)], capture_output=True, check=True)
            assert int(run.stdout) == expected, (values, run.stdout, expected)
        for hit in (15, 16):
            run = subprocess.run([str(binary), str(hit), '0', '0', '4'], capture_output=True)
            diagnostic = run.stderr.decode()
            assert run.returncode != 0, (hit, run.returncode, run.stdout)
            assert 'index 3 out of bounds' in diagnostic, diagnostic
            assert re.search(r'(?:int8_t|signed char|char)\s*\[\s*3\s*\]', diagnostic), diagnostic
            print(f'growth rectangle {hit}: actual helper stops at index 3 / int8_t[3] sanitizer diagnostic')
    print(f'{len(cases)} defined hit cases match; two growth-hit undefined array accesses reproduced')
    print('Scripted initialized rectangles; native event reachability, paint order and any repair remain separate work.')


if __name__ == '__main__':
    main()
