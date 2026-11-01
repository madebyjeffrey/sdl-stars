#!/usr/bin/env python3
"""Actual race-page color/background arms with explicit child-handle receipts."""
import json
from pathlib import Path
import subprocess
import tempfile
from check_s02_source import function
from check_race_workflows_source import build


PRELUDE = r'''
typedef uintptr_t WPARAM;typedef intptr_t LPARAM;typedef intptr_t INT_PTR;
#define WM_CTLCOLORSTATIC 0x138
static void *hbrButtonFace=(void *)31;static uint32_t crButtonFace=0xc0c0c0;
static int first,profile,page;
static void ev(const char *k,int64_t value){if(!first)putchar(',');first=0;printf("[\"%s\",%lld]",k,(long long)value);}
static int startId(void){return page==3 || page==4 || page==5?291:271;}
static uintptr_t child(int id){if(profile==1)return 0;if(profile==2 && (id==startId() || id==startId()+1))return 8000;if(profile==3 && id==startId())return 0;return 1000+id;}
static HWND GetDlgItem(HWND hwnd,int id){assert(hwnd==(HWND)7);ev("get",id);return (HWND)child(id);}
static uint32_t SetBkColor(HDC dc,uint32_t color){assert(dc==(HDC)9 && color==crButtonFace);ev("background",color);return 0;}
static void GetClientRect(HWND hwnd,RECT *r){assert(hwnd==(HWND)7);*r=(RECT){0,0,400,300};ev("client",0);}
static int FillRect(HDC dc,const RECT *r,void *brush){assert(dc==(HDC)9 && brush==hbrButtonFace && r->right==400 && r->bottom==300);ev("fill",0);return 0;}
'''
HARNESS = r'''
int main(void){int message,wanted,erase;while(scanf("%d %d %d %d %d",&page,&message,&wanted,&profile,&erase)==5){
    first=1;printf("{\"events\":[");intptr_t result=0;LPARAM target=wanted<0?(profile==1?0:9999):(LPARAM)child(wanted);
    switch(page){case 1:result=erase?Erase1():Color1(message,target);break;case 2:result=erase?Erase4():Color4(message,target);break;
    case 3:result=erase?Erase5():Color5(message,target);break;case 4:result=erase?Erase2():Color2(message,target);break;
    case 5:result=erase?Erase3():Color3(message,target);break;case 6:result=erase?Erase6():Color6(message,target);break;default:abort();}
    printf("],\"result\":%lld}\n",(long long)result);
}(void)FSameLibraryRace;(void)SaveRaceState;(void)RestoreRaceState;return 0;}
'''


def ids(page):
    return list(range(271, 279)) if page == 1 else list(range(271, 281)) if page == 2 else list(range(291, 305)) if page == 3 else list(range(291, 294)) if page == 4 else [291] if page == 5 else [*range(271, 289), 291]


def oracle(values):
    page, message, wanted, profile, erase = values
    if erase:return dict(events=[['client', 0], ['fill', 0]], result=1)
    def handle(i):
        if profile == 1:return 0
        if profile == 2 and i in (ids(page)[0], ids(page)[0]+1):return 8000
        if profile == 3 and i == ids(page)[0]:return 0
        return 1000+i
    target = (0 if profile == 1 else 9999) if wanted < 0 else handle(wanted)
    events = [];matched = False
    for ident in ids(page):
        events.append(['get', ident])
        if handle(ident) == target:matched = True;break
    result = 31 if matched or message == 0x138 else 0
    if result:events.append(['background', 0xc0c0c0])
    return dict(events=events, result=result)


def main():
    extra = PRELUDE
    for callback in range(1, 7):
        source = function('src/ui/raceui.c', f'INT_PTR CALLBACK RaceWizardDlg{callback}(')
        arm = source.split('    case WM_CTLCOLORSTATIC:\n', 1)[1].split('    case WM_', 1)[0]
        locals_ = '' if callback == 3 else 'int16_t i;'
        extra += f'\nstatic INT_PTR Color{callback}(int message,LPARAM lParam){{HWND hwnd=(HWND)7;WPARAM wParam=9;{locals_}switch(0){{case 0:\n'+arm+'}return 0;}\n'
        erase = source.split('    case WM_ERASEBKGND:\n', 1)[1].split('    case WM_', 1)[0]
        extra += f'\nstatic INT_PTR Erase{callback}(void){{HWND hwnd=(HWND)7;WPARAM wParam=9;RECT rc;\n'+erase+'}\n'
    cases = [(page, message, wanted, profile, 0) for page in range(1, 7) for message in range(0x132, 0x139)
             for wanted in (-1, *ids(page), 2075) for profile in range(4)]
    cases += [(page, 0, -1, 0, 1) for page in range(1, 7)]
    with tempfile.TemporaryDirectory(prefix='race-appearance-') as tmp:
        binary = build(Path(tmp), extra=extra, harness=HARNESS)
        run = subprocess.run([str(binary)], input=('\n'.join(' '.join(map(str, v)) for v in cases)+'\n').encode(), capture_output=True)
        assert run.returncode == 0, run.stderr.decode()
        lines = run.stdout.splitlines();assert len(lines) == len(cases)
        for i, (values, line) in enumerate(zip(cases, lines)):
            actual = json.loads(line);assert actual == oracle(values), (i, values, actual, oracle(values))
    print(f'{len(cases)} actual color/erase cases match child-query order, null/duplicate identities, static fallback and brush/numeric returns')
    print('Scripted window/DC services; source returns are distinct from successful drawing or complete native appearance qualification.')


if __name__ == '__main__':
    main()
