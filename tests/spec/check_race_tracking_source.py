#!/usr/bin/env python3
"""Actual hit/track/mouse/cursor bodies, with scripted UI and redraw services."""
import json
from pathlib import Path
import subprocess
import tempfile
from check_s02_source import ROOT, function
from check_race_workflows_source import build
from check_race_page_commands_source import MINIMUM, MAXIMUM, signed


PRELUDE = r'''
#include <setjmp.h>
#include <limits.h>
#define WINAPI
typedef int32_t BOOL;typedef intptr_t LPARAM;typedef uintptr_t WPARAM;
int16_t fRCWReadOnly;
static int16_t crcRCW;static RaceWizardPage iPanelActive;
static RECT rectangles[17],*vrgrcRCW=rectangles,rgrcBuildSpin[2];
static void *hcurCloseGrab=(void *)20,*hcurHand=(void *)21,*vhpal=(void *)22,*hdibRaces=(void *)23;
static char *vrgszRCWWidth[2]={"wider","narrower"};
static int mode,target,keyboard,steps,initialLow,initialHigh,growth,profile,ro,immune,panel,count;
static int first,lastHit,iteration,draws;static POINT16 mousePoint;static jmp_buf jump;
static int16_t NativeHit(POINT16);
static void hex(const void *p,size_t n){const uint8_t *b=p;putchar('"');for(size_t i=0;i<n;i++)printf("%02x",b[i]);putchar('"');}
static void ev(const char *k,int64_t a,int64_t b,int64_t c,const void *data,size_t n){if(!first)putchar(',');first=0;printf("[\"%s\",%lld,%lld,%lld,",k,(long long)a,(long long)b,(long long)c);hex(data,n);putchar(']');}
static POINT16 PointFrom16(POINT16 p){return p;}
static int PtInRect(const RECT *r,POINT16 p){
    int yes=r->left<=p.x && p.x<r->right && r->top<=p.y && p.y<r->bottom;
    if(r==&rgrcBuildSpin[0] || r==&rgrcBuildSpin[1])ev("spin",r-rgrcBuildSpin,yes,0,"",0);
    else {int i;for(i=0;i<17 && r!=&rectangles[i];i++){}assert(i<17);ev("box",i,yes,0,"",0);}return yes;
}
static int16_t IrcRaceDlgHitTest(POINT16 p){int16_t result=NativeHit(p);lastHit=result;ev("hit",result,0,0,"",0);return result;}
static void startMark(void){
    if(profile==1){((uint8_t *)&vplr)[1]++;vplr.iPlrBmp=(vplr.iPlrBmp+7)%32;
        if(mode==1 && lastHit>=0 && lastHit<15 && lastHit%5==1)vplr.rgEnvVarMin[lastHit/5]++;}
}
static void tickMark(void){
    if(profile==1){((uint8_t *)&vplr)[0]++;vplr.iPlrBmp=(vplr.iPlrBmp+5)%32;}
    if(profile==8 && mode==3)rgrcBuildSpin[0]=(RECT){40000,-40000,40014,-39986};
    if((profile==9 || profile==10) && mode==1 && lastHit>=0 && lastHit<15 && lastHit%5==1)rectangles[lastHit].right=rectangles[lastHit].left-(profile==10?100:0);
}
static void endMark(void){if(profile==1)((uint8_t *)&vplr)[2]++;}
static void InitBtnTrack(BTNT *b,HWND hwnd,HDC dc,RECT *r,int16_t button,int16_t timer,int16_t down,int16_t noRedraw,char *text){
    assert(hwnd==(HWND)7 && !dc && timer==80 && !down && !noRedraw);
    int i=mode==3?(int)(r-rgrcBuildSpin):(int)(r-rectangles);assert(i>=0 && i<(mode==3?2:17));
    int label=text==NULL?-1:text==vrgszRCWWidth[0]?0:1;assert(!text || text==vrgszRCWWidth[label]);
    ev("init",i,button,label,"",0);b->hdc=(HDC)14;b->rc=*r;startMark();if(profile==3)longjmp(jump,1);
}
static int16_t FTrackBtn(BTNT *b){assert(b->hdc==(HDC)14);int more=iteration++<steps?(profile==2?-7:1):0;tickMark();ev("tick",more,0,0,"",0);if(profile==4)longjmp(jump,1);return more;}
static int16_t FGetMouseMove(POINT16 *p){
    static const int offsets[]={-500,0,20,80,500};int i=iteration++;int more=i<steps?(profile==2?-7:1):0;tickMark();
    if(more){p->x=(int16_t)(rectangles[lastHit].left+offsets[i%5]);p->y=1;}
    ev("move",more,more?p->x:0,more?p->y:0,"",0);if(profile==4)longjmp(jump,1);return more;
}
static void SetCapture(HWND hwnd){assert(hwnd==(HWND)7);ev("capture",0,0,0,"",0);startMark();if(profile==3)longjmp(jump,1);}
static void SetCursor(void *cursor){assert(cursor==hcurCloseGrab || cursor==hcurHand);ev(cursor==hcurCloseGrab?"grab":"hand",0,0,0,"",0);}
static void ReleaseCapture(void){endMark();ev("releaseCapture",0,0,0,"",0);if(profile==6)longjmp(jump,1);}
static HDC GetDC(HWND hwnd){assert(hwnd==(HWND)7);ev("dc",0,0,0,"",0);return (HDC)13;}
static void SelectPalette(HDC dc,void *palette,int force){assert(dc==(HDC)13 && palette==vhpal && !force);ev("palette",0,0,0,"",0);}
static void RealizePalette(HDC dc){assert(dc==(HDC)13);ev("realize",0,0,0,"",0);}
static void ReleaseDC(HWND hwnd,HDC dc){assert(hwnd==(HWND)7 && dc==(HDC)13);endMark();ev("releaseDC",0,0,0,"",0);if(profile==6)longjmp(jump,1);}
static int16_t DibBlt(HDC dc,int16_t x,int16_t y,int16_t dx,int16_t dy,void *dib,int16_t sx,int16_t sy,int16_t sw,int16_t sh,int32_t op){
    assert(dc==(HDC)13 && dib==hdibRaces && dx==32 && dy==32 && sw==32 && sh==32 && op==13369376);
    ev("bitmap",x,y,sx,"",0);ev("sourceY",sy,vplr.iPlrBmp,0,"",0);return 1;
}
static void draw(int which,HWND hwnd,HDC dc,int16_t mask){
    assert(hwnd==(HWND)7 && (!dc || dc==(HDC)14));ev("draw",which,mask,(intptr_t)dc,&vplr,192);draws++;
    BoundsCheckPlayer(&vplr);if(which==3)crcRCW=GetRaceStat(&vplr,rsMajorAdv)==raMacintosh?2:14;
    if(profile==8 && lastHit>=0){rectangles[lastHit].left+=3;rectangles[lastHit].right-=2;}
    if(profile==5)longjmp(jump,1);
}
static void DrawRace2(HWND hwnd,HDC dc,int16_t mask){draw(2,hwnd,dc,mask);}
static void DrawRace3(HWND hwnd,HDC dc,int16_t mask){draw(3,hwnd,dc,mask);}
static void GetCursorPos16(POINT16 *p){ev("cursor",0,0,0,"",0);*p=mousePoint;startMark();if(profile==3)longjmp(jump,1);}
static void ScreenToClient16(HWND hwnd,POINT16 *p){assert(hwnd==(HWND)7 && p);ev("client",0,0,0,"",0);}
'''
HARNESS = r'''
int main(void){
    while(scanf("%d %d %d %d %d %d %d %d %d %d %d %d",&mode,&target,&keyboard,&steps,&initialLow,&initialHigh,&growth,&profile,&ro,&immune,&panel,&count)==12){
        for(int i=0;i<192;i++)((uint8_t *)&vplr)[i]=(uint8_t)(i*37+17);
        strcpy(vplr.szName,"Original");strcpy(vplr.szNames,"Originals");vplr.iPlrBmp=13;
        for(int i=0;i<3;i++){vplr.rgEnvVarMin[i]=initialLow;vplr.rgEnvVarMax[i]=initialHigh;vplr.rgEnvVar[i]=37;}
        if(immune)vplr.rgEnvVarMax[0]=-1;
        vplr.pctIdealGrowth=growth;for(int i=0;i<16;i++)vplr.rgAttr[i]=initialLow;vplr.grbitAttr=0;
        memset(szRacePass,'P',16);strcpy(szRacePass,"N");
        for(int i=0;i<17;i++)rectangles[i]=(RECT){i*100,0,i*100+80,80};
        if(profile==7)rectangles[5]=rectangles[0];
        rgrcBuildSpin[0]=(RECT){300,500,314,514};rgrcBuildSpin[1]=(RECT){314,500,328,514};
        if(profile==7)rgrcBuildSpin[1]=rgrcBuildSpin[0];
        fRCWReadOnly=ro;iPanelActive=panel;crcRCW=count;lastHit=-1;iteration=draws=0;
        RECT r=mode==3?(target<0?(RECT){25000,-500,25000,-500}:rgrcBuildSpin[target%2]):target<0?(RECT){25000,-500,25000,-500}:rectangles[target];
        mousePoint=(POINT16){(int16_t)(r.left+1),(int16_t)(r.top+1)};
        uint32_t position=LOWORD(mousePoint.x)|((uint32_t)LOWORD(mousePoint.y)<<16);
        int result=-2;first=1;printf("{\"events\":[");if(!setjmp(jump)){
            if(mode==0)result=IrcRaceDlgHitTest(mousePoint);else if(mode==1)result=Mouse2(position,keyboard);
            else if(mode==2)result=Mouse3(position,keyboard);else if(mode==3)result=Mouse1(position);
            else if(mode==4)result=Cursor2();else if(mode==5)result=Cursor3();else abort();
        }
        printf("],\"result\":%d,\"count\":%d,\"draws\":%d,\"race\":",result,crcRCW,draws);hex(&vplr,192);printf(",\"password\":");hex(szRacePass,16);
        printf(",\"rects\":[");for(int i=0;i<17;i++){if(i)putchar(',');printf("[%d,%d,%d,%d]",rectangles[i].left,rectangles[i].top,rectangles[i].right,rectangles[i].bottom);}puts("]}");
    }
    (void)FSameLibraryRace;(void)SaveRaceState;(void)RestoreRaceState;return 0;
}
'''


def oracle(v):
    mode, target, kbd, steps, low, high, growth, profile, ro, immune, panel, count = v
    r = bytearray((i*37+17) % 256 for i in range(192))
    for at, text in ((128, b'Original'), (160, b'Originals')):r[at:at+len(text)+1] = text+b'\0'
    def portrait(n):r[6] = r[6] % 8+8*(n % 32)
    portrait(13);r[16:25] = bytes([37]*3+[low % 256]*3+[high % 256]*3)
    if immune:r[22] = 255
    r[25] = growth % 256;r[62:78] = bytes([low % 256]*16);r[78:82] = bytes(4)
    rects = [[i*100, 0, i*100+80, 80] for i in range(17)];spins = [[300, 500, 314, 514], [314, 500, 328, 514]]
    if profile == 7:rects[5] = rects[0].copy();spins[1] = spins[0].copy()
    chosen = spins[target % 2] if mode == 3 and target >= 0 else rects[target] if target >= 0 else [25000, -500, 25000, -500]
    point = [signed(chosen[0]+1, 16), signed(chosen[1]+1, 16)];events = [];draws = 0;last_hit = -1;iteration = 0
    def ev(k, a=0, b=0, c=0, data=b''):events.append([k, a, b, c, bytes(data).hex()])
    def inside(box, p):return box[0] <= p[0] < box[2] and box[1] <= p[1] < box[3]
    def box_test(i):
        yes = int(inside(rects[i], point));ev('box', i, yes);return yes
    def spin_test(i):
        yes = int(inside(spins[i], point));ev('spin', i, yes);return yes
    def hit():
        nonlocal last_hit
        result = -1
        if not ro:
            for i in range(max(0, count)):
                assert i < 17
                if box_test(i):
                    assert panel != 4 or i//5 < 3, ('excluded known growth-hit undefined access', v)
                    if panel != 4 or signed(r[22+i//5], 8) >= 0:result = i
                    break
        last_hit = result;ev('hit', result);return result
    class Escape(Exception):pass
    def start_mark():
        if profile == 1:
            r[1] = (r[1]+1) % 256;portrait(r[6]//8+7)
            if mode == 1 and 0 <= last_hit < 15 and last_hit % 5 == 1:r[19+last_hit//5] = (r[19+last_hit//5]+1) % 256
    def tick_mark():
        if profile == 1:r[0] = (r[0]+1) % 256;portrait(r[6]//8+5)
        if profile == 8 and mode == 3:spins[0] = [40000, -40000, 40014, -39986]
        if profile in (9, 10) and mode == 1 and 0 <= last_hit < 15 and last_hit % 5 == 1:rects[last_hit][2] = rects[last_hit][0]-(100 if profile == 10 else 0)
    def end_mark():
        if profile == 1:r[2] = (r[2]+1) % 256
    def center(axis):r[16+axis] = (signed(r[19+axis], 8)+div(signed(r[22+axis], 8)-signed(r[19+axis], 8), 2)) % 256
    def div(a, b):return (abs(a)//abs(b))*(-1 if (a < 0) != (b < 0) else 1)
    def normalize():
        for i in range(3):
            lo, mid, hi = signed(r[19+i], 8), signed(r[16+i], 8), signed(r[22+i], 8)
            if lo == -1:
                if hi != -1 or mid != -1:r[16+i] = r[22+i] = 255;r[84] |= 16
            else:
                a = max(0, min(100, lo));b = max(a, min(100, hi));c = a+(b-a)//2
                if (lo, mid, hi) != (a, c, b):r[84] |= 16
                r[19+i], r[16+i], r[22+i] = a, c, b
        if signed(r[25], 8) > 20:r[25] = 20;r[84] |= 16
        for i in range(16):
            prior = signed(r[62+i], 8);value = max(MINIMUM[i], min(MAXIMUM[i], prior))
            if prior != value:r[84] |= 16
            r[62+i] = value
    def draw(which, mask, dc):
        nonlocal count, draws
        ev('draw', which, mask, dc, r);draws += 1;normalize()
        if which == 3:count = 2 if signed(r[76], 8) == 8 else 14
        if profile == 8 and last_hit >= 0:rects[last_hit][0] += 3;rects[last_hit][2] -= 2
        if profile == 5:raise Escape()
    def init(index, button, text=-1):
        ev('init', index, button, text);start_mark()
        if profile == 3:raise Escape()
    def tick(move=False):
        nonlocal iteration
        i = iteration;iteration += 1;more = (-7 if profile == 2 else 1) if i < steps else 0;tick_mark()
        p = [signed(rects[last_hit][0]+(-500, 0, 20, 80, 500)[i % 5], 16), 1] if move and more else [0, 0]
        ev('move' if move else 'tick', more, *p if move else (0, 0))
        if profile == 4:raise Escape()
        return more, p
    result = -2
    try:
        if mode == 0:result = hit()
        elif mode in (4, 5):
            ev('cursor');start_mark()
            if profile == 3:raise Escape()
            ev('client');h = hit();result = int(h >= 0)
            if result:ev('hand')
        elif mode == 3:
            if ro:result = 0
            else:
                first_hit = spin_test(0)
                second_hit = False if first_hit else spin_test(1)
                if not first_hit and not second_hit:result = 0
                else:
                    direction = -1 if spin_test(0) else 1;cur = r[6]//8
                    ev('dc');ev('palette');ev('realize');init(0 if direction < 0 else 1, 34 if direction < 0 else 35)
                    while tick()[0]:
                        cur = (cur+32+direction) % 32
                        ev('bitmap', signed(spins[0][0]-2, 16), signed(spins[0][1]-35, 16), (cur % 8)*32)
                        ev('sourceY', (3-cur//8)*32, r[6]//8)
                    portrait(cur);end_mark();ev('releaseDC')
                    if profile == 6:raise Escape()
                    result = 1
        else:
            h = hit()
            if h < 0:result = 0
            elif mode == 1 and h < 15 and h % 5 == 1:
                ev('capture');start_mark()
                if profile == 3:raise Escape()
                ev('grab');axis = h//5;half = div(signed(r[22+axis], 8)-signed(r[19+axis], 8), 2)
                while True:
                    more, p = tick(True)
                    if not more:break
                    box = rects[h];x = signed(box[0], 16) if p[0] < box[0] else p[0]
                    if x > box[2]:x = signed(box[2], 16)
                    a, b = x-box[0], box[2]-box[0]
                    if b == 0:scaled = -1
                    else:
                        n = (abs(a*100)+abs(b)//2)//abs(b)
                        if (a*100 < 0) != (b < 0):n = -n
                        scaled = n if -2147483647 <= n <= 2147483647 else -1
                    shift = signed(scaled, 16)
                    if half > (100-half if shift >= 100-half else shift):shift = half
                    elif shift >= 100-half:shift = 100-half
                    if signed(r[22+axis], 8) != shift+half:
                        r[19+axis], r[22+axis] = (shift-half) % 256, (shift+half) % 256;center(axis);draw(2, 1 << axis, 0)
                end_mark();ev('releaseCapture')
                if profile == 6:raise Escape()
                result = 1
            else:
                factor = 10 if signed(kbd, 16) % 8 >= 4 else 1
                if mode == 2:width, shift, button, label = 0, (1 if h % 2 == 0 else -1)*(3 if factor == 10 else 1), 160 if h % 2 == 0 else 161, -1
                else:
                    width, shift, button, label = (0, 1, 160, -1) if h == 15 else (0, -1, 161, -1) if h == 16 else {0:(0,-1,2,-1),2:(0,1,3,-1),3:(1,0,8,0),4:(-1,0,8,1)}[h % 5]
                    width *= factor;shift *= factor
                init(h, button, label)
                while tick()[0]:
                    if mode == 2:
                        assert h < 14, ('excluded undefined economy map read', v)
                        stat = h//2;prior = signed(r[62+stat], 8);value = max(MINIMUM[stat], min(MAXIMUM[stat], prior+shift));r[62+stat] = value
                        if value != prior:draw(3, stat, 14)
                    elif h >= 15:
                        prior = signed(r[25], 8);value = max(1, min(20, signed(prior+shift, 8)))
                        if value != prior:r[25] = value;draw(2, 8, 14)
                    else:
                        axis = h//5;lo = signed(signed(r[19+axis], 8)-(width-shift), 8);hi = signed(signed(r[22+axis], 8)+(width+shift), 8)
                        if hi > 100:lo = signed(lo-(hi-100), 8);hi = 100
                        if lo < 0:hi = signed(min(100, hi-lo), 8);lo = 0
                        difference = hi-lo
                        if difference < 20:delta = (20-difference)//2;lo = signed(lo-delta, 8);hi = signed(hi+delta, 8)
                        if signed(r[19+axis], 8) != lo or signed(r[22+axis], 8) != hi:
                            r[19+axis], r[22+axis] = lo % 256, hi % 256;center(axis);draw(2, 1 << axis, 14)
                if mode == 1 and h < 15:center(h//5)
                result = 1
    except Escape:pass
    return dict(events=events, result=result, count=count, draws=draws, race=bytes(r).hex(), password=(b'N\0'+b'P'*14).hex(), rects=rects)


def vectors():
    for mode in (0, 1, 2, 4, 5):
        indices = range(-1, 17) if mode in (0, 2, 5) else range(-1, 15)
        for target in indices:
            for ro in (0, -7):
                for immune in (0, 1):
                    for kbd in (0, 4, 8, 65540):
                        yield mode, target, kbd, 0 if mode == 2 and target >= 14 else 2, 20, 80, 15, 0, ro, immune, 5 if mode in (0,2,5) else 4, 17
    for mode, target in ((1, 0), (1, 1), (1, 3), (1, 4), (1, 15), (1, 16), (2, 0), (2, 13), (3, 0), (3, 1), (4, 1), (5, 2)):
        for profile in range(11):
            for steps in (0, 1, 5, 33):yield mode, target, 4, steps, 0, 19, 127, profile, 0, 0, 5 if target >= 15 or mode in (2,5) else 4, 17
    for low, high in ((0,20),(0,19),(0,100),(20,80),(80,100),(-128,127),(99,100)):
        for target in (0,2,3,4,5,7,8,9,10,12,13,14):
            for kbd in (0,4):yield 1,target,kbd,3,low,high,1,0,0,0,4,17
    for mode in (1, 2):
        for old in (-128,-1,0,1,7,10,15,20,25,100,127):
            for target in ((15,16) if mode == 1 else range(14)):
                yield mode,target,4,2,old,80,old,0,0,0,5,17
    for target in (0,1,-1):
        for ro in (0,-7):yield 3,target,0,33,20,80,15,1,ro,0,1,17
    for count in (-7,0,1,14,17,100):yield 0,0,0,0,20,80,15,0,0,0,5,count
    yield 0,5,0,0,20,80,15,7,0,1,4,17


def main():
    extra = PRELUDE
    source = (ROOT/'src/core/race.c').read_text()
    for line in source.splitlines():
        if line.startswith(('char    rgRaceStatMax', 'char    rgRaceStatMin', 'char    rgRW3IStat')):extra += '\n'+line
    extra += '\n'+next(line for line in source.splitlines() if line.startswith('int16_t GetRaceStat('))
    for signature in ('int16_t SetRaceStat(', 'void BoundsCheckPlayer('):extra += '\n'+function('src/core/race.c', signature)
    extra += '\n'+function('compat/win32/src/services.c', 'int WINAPI MulDiv(')
    extra += '\n'+function('src/ui/raceui.c', 'int16_t IrcRaceDlgHitTest(').replace('int16_t IrcRaceDlgHitTest(', 'static int16_t NativeHit(', 1)
    for name in ('FTrackRaceDlg2', 'FTrackRaceDlg3'):extra += '\n'+function('src/ui/raceui.c', f'int16_t {name}(')
    for page in (1, 2, 3):
        callback = function('src/ui/raceui.c', f'INT_PTR CALLBACK RaceWizardDlg{page}(')
        arm = callback.split('    case WM_LBUTTONDBLCLK:\n', 1)[1].split('    case WM_', 1)[0]
        if page == 1:
            locals_ = 'int16_t iDir,bt,iCur;RECT *prc;HDC hdc;BTNT btnt;'
            extra += '\nstatic int Mouse1(LPARAM lParam){HWND hwnd=(HWND)7;POINT16 pt;'+locals_+'switch(0){case 0:\n'+arm+'}return 0;}\n'
        else:
            extra += f'\nstatic int Mouse{page}(LPARAM lParam,WPARAM wParam){{HWND hwnd=(HWND)7;POINT16 pt;\n'+arm+'}\n'
            cursor = callback.split('    case WM_SETCURSOR:\n', 1)[1].split('    case WM_', 1)[0]
            extra += f'\nstatic int Cursor{page}(void){{HWND hwnd=(HWND)7;POINT16 pt;switch(0){{case 0:\n'+cursor+'}return 0;}\n'
    cases = list(vectors())
    with tempfile.TemporaryDirectory(prefix='race-tracking-') as tmp:
        binary = build(Path(tmp), extra=extra, harness=HARNESS)
        run = subprocess.run([str(binary)], input=('\n'.join(' '.join(map(str, v)) for v in cases)+'\n').encode(), capture_output=True)
        assert run.returncode == 0, run.stderr.decode()
        lines = run.stdout.splitlines();assert len(lines) == len(cases), (len(lines), len(cases))
        for i, (values, line) in enumerate(zip(cases, lines)):
            actual = json.loads(line);expected = oracle(values)
            for key, value in expected.items():assert actual[key] == value, (i, values, key, actual[key], value)
    print(f'{len(cases)} actual hit/track/mouse/cursor cases match native bytes, redraw/capture/repeat order and ordinary/escaping outcomes')
    print('Actual MulDiv/stat/bounds helpers; scripted event/button/capture/redraw services. Known invalid growth/map reads are excluded before C execution.')
    print('No full native timer, pointer-lifetime, display or complete drawing/pricing qualification claim.')


if __name__ == '__main__':
    main()
