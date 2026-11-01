#!/usr/bin/env python3
"""Actual six initialization arms with native layout and scripted UI/catalog data."""
import json
from pathlib import Path
import subprocess
import tempfile
from check_s02_source import ROOT, function
from check_race_workflows_source import build, prefix
from check_race_page_commands_source import signed


PRELUDE = r'''
#include <stdarg.h>
#include "resource.h"
typedef void *HFONT;typedef int32_t BOOL;
typedef uintptr_t WPARAM;typedef intptr_t LPARAM;
#define EM_LIMITTEXT 0xc5
#define CB_ADDSTRING 0x143
#define CB_SETCURSEL 0x14e
#define BM_SETCHECK 0xf1
#define SWP_NOZORDER 4
PLAYER vrgplrDef[7];GAME game;
int16_t fRCWReadOnly,dyArial8,idPlayer,cColDrop;
static int16_t viStore,crcRCW;static RaceWizardPage iPanelActive;
static RECT rects[17],*vrgrcRCW=rects;static POINT16 ptStickyRaceDlg;
static HFONT rghfontArial8[2]={(void *)10,(void *)11};char szWork[360];
static int first,inTitle,comboRequest,comboSelection,nextEnabled,defaultKind,widthKind,clientRight;
static void NativeTitle(HWND,int16_t);
static const int widths[4][4]={{77,35,280,21},{65535,65535,65535,65535},{32760,32761,1234,65530},{0,0,0,0}};
static const char *defaultText(void){return defaultKind==0?"Humanoid":defaultKind==1?"123456789012345678901":"\xe6\x98\x9f\xe6\x98\x9f\xe6\x98\x9f\xe6\x98\x9f\xe6\x98\x9f\xe6\x98\x9f";}
static void hex(const void *p,size_t n){const uint8_t *a=p;putchar('"');for(size_t i=0;i<n;i++)printf("%02x",a[i]);putchar('"');}
static void ev(const char *k,int64_t a,int64_t b,int64_t c,int64_t d,const char *text){if(!first)putchar(',');first=0;printf("[\"%s\",%lld,%lld,%lld,%lld,",k,(long long)a,(long long)b,(long long)c,(long long)d);hex(text,strlen(text));putchar(']');}
static HWND GetDlgItem(HWND hwnd,int id){assert(hwnd==(HWND)7);return (HWND)(uintptr_t)id;}
static void SetRCWTitle(HWND hwnd,int16_t page){inTitle=1;NativeTitle(hwnd,page);inTitle=0;}
int16_t CchGetString(StringId id,char *out){
    if(inTitle){ev("title",id,0,0,0,"");strcpy(out,"Title %d");}
    else if(id==idsTemperature){ev("label",0,0,0,0,"");strcpy(out,"Temperature");}
    else {assert(id==idsMaximumColonistGrowthRatePerYear);ev("label",2,0,0,0,"");strcpy(out,"Growth");}
    return strlen(out);
}
char *PszGetCompressedString(StringId id){static char text[32];ev("catalog",id,0,0,0,"");
    if(id==idsCosts75ExtraResearchFieldsStartTech)return "Tech %d";
    snprintf(text,sizeof(text),"item%u",id);return text;}
static int wsprintf(char *out,const char *fmt,...){assert(out==szWork);va_list args;va_start(args,fmt);int n=vsnprintf(out,360,fmt,args);va_end(args);return n;}
static BOOL SetWindowTextA(HWND hwnd,const char *text){ev("text",(intptr_t)hwnd,0,0,0,text);return TRUE;}
#define SetWindowText SetWindowTextA
static void SetDlgItemText(HWND hwnd,int id,const char *text){assert(hwnd==(HWND)7);ev("text",id,0,0,0,text);}
static int GetDlgItemText(HWND hwnd,int id,char *out,int size){assert(hwnd==(HWND)7 && id==IDC_RACE_HUMANOID && size==16);size_t n=CchUtf8Prefix(defaultText(),size-1);memcpy(out,defaultText(),n);out[n]=0;ev("default",size,0,0,0,out);return n;}
static void StickyDlgPos(HWND hwnd,POINT16 *pt,int init){assert(hwnd==(HWND)7 && pt==&ptStickyRaceDlg && init);ev("sticky",0,0,0,0,"");}
static void CheckRadioButton(HWND hwnd,int lo,int hi,int choice){assert(hwnd==(HWND)7);ev("radio",lo,hi,choice,0,"");}
static void SendDlgItemMessage(HWND hwnd,int id,int msg,WPARAM w,LPARAM l){assert(hwnd==(HWND)7 && msg==EM_LIMITTEXT && !l);ev("limit",id,w,0,0,"");}
static intptr_t SendMessage(HWND hwnd,int msg,WPARAM w,LPARAM l){
    if(msg==CB_ADDSTRING){assert(hwnd==(HWND)IDC_COMBOBOX);ev("add",0,0,0,0,(char *)l);return 0;}
    if(msg==CB_SETCURSEL){assert(hwnd==(HWND)IDC_COMBOBOX);comboRequest=(intptr_t)w;comboSelection=comboRequest>=0 && comboRequest<5?comboRequest:-1;ev("combo",comboRequest,0,0,0,"");return comboSelection;}
    assert(msg==BM_SETCHECK);ev("check",(intptr_t)hwnd,w,0,0,"");return 0;
}
static void EnableWindow(HWND hwnd,int enabled){assert(!enabled);assert(hwnd!=(HWND)IDC_NEXT);ev("disable",(intptr_t)hwnd,0,0,0,"");}
static HDC GetDC(HWND hwnd){assert(hwnd==(HWND)7);ev("dc",0,0,0,0,"");return (HDC)13;}
static void GetClientRect(HWND hwnd,RECT *r){assert(hwnd==(HWND)7);*r=(RECT){0,0,clientRight,600};ev("client",clientRight,0,0,0,"");}
static HFONT SelectObject(HDC dc,HFONT font){assert(dc==(HDC)13);ev("font",(intptr_t)font,0,0,0,"");return (HFONT)12;}
static void ReleaseDC(HWND hwnd,HDC dc){assert(hwnd==(HWND)7 && dc==(HDC)13);ev("release",0,0,0,0,"");}
static uint32_t GetTextExtent(HDC dc,const char *text,int size){assert(dc==(HDC)13 && size>=0);int i=!strcmp(text,"Temperature")?0:!strcmp(text,"200mR")?1:!strcmp(text,"Growth")?2:3;ev("measure",i,0,0,0,"");return widths[widthKind][i];}
static void SetRect(RECT *r,int l,int t,int right,int b){*r=(RECT){l,t,right,b};}
static void OffsetRect(RECT *r,int x,int y){r->left+=x;r->right+=x;r->top+=y;r->bottom+=y;}
static void SetWindowPos(HWND hwnd,HWND after,int x,int y,int w,int h,int flags){assert(!after && flags==SWP_NOZORDER);ev("place",(intptr_t)hwnd,x,y,w,"");ev("height",h,0,0,0,"");}
static void CheckDlgButton(HWND hwnd,int id,int on){assert(hwnd==(HWND)7);ev("check",id,on,0,0,"");}
static int ObservedCompare(const void *a,const void *b,size_t n){assert(a==&vplr && n==128);int i=(const PLAYER *)b-vrgplrDef;assert(i>=0 && i<7);ev("compare",i,vplr.iPlrBmp,0,0,"");return memcmp(a,b,n);}
#define memcmp ObservedCompare
'''
HARNESS = r'''
static void fill(PLAYER *p,int seed,int choice){
    for(int i=0;i<192;i++)((uint8_t *)p)[i]=(uint8_t)(i*37+seed);
    strcpy(p->szName,"Original");strcpy(p->szNames,"Originals");p->iPlrBmp=seed%32;p->lSalt=seed%2;
    p->grbitAttr=(seed%2?UINT32_C(0xe0001234):UINT32_C(0x40001234))^(UINT32_C(1)<<(seed%14));for(int i=0;i<3;i++)p->rgEnvVarMax[i]=i==seed%3?-1:80;
    p->rgAttr[7]=choice;p->rgAttr[14]=seed%10;
}
int main(void){
    _Static_assert(idsDefenses3-idsSurfaceMinerals==4,"five leftover entries");
    int page,profile,ro,tutorial,player,choice,seed,font,right,kind;
    while(scanf("%d %d %d %d %d %d %d %d %d %d",&page,&profile,&ro,&tutorial,&player,&choice,&seed,&font,&right,&kind)==10){
        for(int i=0;i<7;i++){fill(&vrgplrDef[i],40+i,choice);vrgplrDef[i].lSalt=seed%2;}
        if(profile<7)vplr=vrgplrDef[profile];else fill(&vplr,100+seed,choice);
        vplr.iPlrBmp=seed%32;vplr.lSalt=seed%2;
        if(kind%3==1)vplr.szName[0]=0;else if(kind%3==2)strcpy(vplr.szName,"\xc3\xa9");
        memset(szRacePass,'T',16);if(seed%2 || seed%3)strcpy(szRacePass,"pw");
        fRCWReadOnly=ro;game.fTutorial=tutorial;idPlayer=player;iPanelActive=page;dyArial8=font;
        defaultKind=kind%3;widthKind=kind%4;clientRight=right;comboRequest=-99;comboSelection=-99;nextEnabled=seed%2;
        crcRCW=-7;viStore=-9;cColDrop=-3;for(int i=0;i<17;i++)rects[i]=(RECT){-9000-i,-8000-i,-7000-i,-6000-i};
        first=1;printf("{\"events\":[");int result=0;switch(page){case 1:result=Init1();break;case 2:result=Init4();break;case 3:result=Init5();break;case 4:result=Init2();break;case 5:result=Init3();break;case 6:result=Init6();break;default:abort();}
        printf("],\"result\":%d,\"count\":%d,\"cache\":%d,\"description\":%d,\"combo\":%d,\"selection\":%d,\"next\":%d,\"race\":",result,crcRCW,viStore,cColDrop,comboRequest,comboSelection,nextEnabled);hex(&vplr,192);printf(",\"password\":");hex(szRacePass,16);printf(",\"rects\":[");for(int i=0;i<17;i++){if(i)putchar(',');printf("[%d,%d,%d,%d]",rects[i].left,rects[i].top,rects[i].right,rects[i].bottom);}puts("]}");
    }
    (void)FSameLibraryRace;(void)SaveRaceState;(void)RestoreRaceState;return 0;
}
'''


def fill(seed, choice):
    r = bytearray((i*37+seed) % 256 for i in range(192))
    for at, text in ((128, b'Original'), (160, b'Originals')):r[at:at+len(text)+1] = text+b'\0'
    r[6] = r[6] % 8+8*(seed % 32);r[12:16] = (seed % 2).to_bytes(4, 'little');r[78:82] = ((0xe0001234 if seed % 2 else 0x40001234) ^ (1 << (seed % 14))).to_bytes(4, 'little')
    r[22:25] = bytes(255 if i == seed % 3 else 80 for i in range(3));r[69] = choice % 256;r[76] = seed % 10
    return r


def oracle(v, ids):
    page, profile, ro, tutorial, player, choice, seed, font, right, kind = v
    rows = [fill(40+i, choice) for i in range(7)]
    for row in rows:row[12:16] = (seed % 2).to_bytes(4, 'little')
    r = rows[profile].copy() if profile < 7 else fill(100+seed, choice)
    r[6] = r[6] % 8+8*(seed % 32);r[12:16] = (seed % 2).to_bytes(4, 'little')
    if kind % 3 == 1:r[128] = 0
    elif kind % 3 == 2:r[128:131] = 'é'.encode()+b'\0'
    password = b'pw\0'+b'T'*13 if seed % 2 or seed % 3 else b'T'*16
    rects = [[-9000-i, -8000-i, -7000-i, -6000-i] for i in range(17)]
    count, cache, description, combo, selection = -7, -9, -3, -99, -99;events = []
    def ev(k, a=0, b=0, c=0, d=0, text=b''):events.append([k, a, b, c, d, bytes(text).hex()])
    def text(at, end):return bytes(r[at:end]).split(b'\0')[0]
    def disable(values):
        for ident in values:ev('disable', ident)
    def trait(bit):return int(bool(r[78+bit//8] & (1 << (bit % 8))))
    ev('title', (ids['idsCustomRaceWizardStepD6']+ro) % 65536);ev('text', 7, text=f'Title {page}'.encode())
    if page == 1:
        name = text(128, 160);ev('text', 268, text=name);ev('text', 2075, text=text(160, 192))
        if not name:
            default = (b'Humanoid', b'123456789012345678901', '星'*6)[kind % 3]
            if isinstance(default, str):default = default.encode()
            name = prefix(default, 15);r[128:129+len(name)] = name+b'\0';ev('default', 16, text=name);ev('text', 268, text=name)
        ev('sticky');selected = 0
        if not (tutorial and player == 0 and ro):
            selected = 7
            for i, row in enumerate(rows):
                candidate = r.copy();candidate[6] = candidate[6] % 8+8*(row[6]//8);ev('compare', i, row[6]//8)
                if candidate[:128] == row[:128]:selected = i;break
        ev('radio', 271, 278, 271+selected)
        for ident in (268, 2075, 269):ev('limit', ident, 15)
        for ident in range(ids['idsSurfaceMinerals'], ids['idsDefenses3']+1):
            ev('catalog', ident);ev('add', text=f'item{ident}'.encode())
        combo = signed(r[69], 8);selection = combo if 0 <= combo < 5 else -1;ev('combo', combo)
        if seed % 2:ev('text', 269, text=b'pw')
        if ro:disable([268, 269, 2075, *range(271, 279), 2074])
    elif page == 2:
        ev('radio', 271, 280, signed(r[76], 8)+271)
        if ro:disable(range(271, 281))
        ev('sticky')
    elif page == 3:
        description = 0
        for i in range(14):
            ident = ids['idsImprovedFuelEfficiency']+i;ev('catalog', ident);ev('text', 291+i, text=f'item{ident}'.encode());ev('check', 291+i, trait(i))
            if ro:disable([291+i])
        ev('sticky')
    elif page == 4:
        widths = ((77, 35, 280, 21), (65535,)*4, (32760, 32761, 1234, 65530), (0,)*4)[kind % 4]
        def div(n):return n//2 if n >= 0 else -((-n)//2)
        label = signed(widths[0]+10, 16);middle = signed(right-label-signed(widths[1]+10, 16), 16);dy = div(signed(3*font, 16));top = signed(3*font, 16)
        base = [[label, top, label+dy, top+dy], [label+dy+6, top, label+middle-dy-6, top+dy], [label+middle-dy, top, label+middle, top+dy],
                [label, top+dy+4, 3*dy+label, 2*dy+top+4], [label+middle-3*dy, top+dy+4, label+middle, 2*dy+top+4]]
        def offset(row, d):return [row[0], row[1]+d, row[2], row[3]+d]
        rects = base+[offset(row, 3*dy+div(font)) for row in base]+[offset(row, 6*dy+font) for row in base]
        left = label+widths[2]+widths[3]+4;y = 9*dy+top-3+3*div(font);up = [left, y, left+15, font//2+y+3]
        rects += [up, offset(up, up[3]-up[1]-1)];count, cache = 17, -1
        ev('dc');ev('client', right);ev('font', 11);ev('label');ev('measure');ev('measure', 1)
        for i in range(3):ev('place', 291+i, 3*dy+label+6, rects[5*i+3][1], middle-6*dy-12);ev('height', dy);ev('check', 291+i, int(r[22+i] >= 128))
        ev('label', 2);ev('measure', 2);ev('measure', 3);ev('font', 12);ev('release')
        if ro:disable(range(291, 294))
        ev('sticky')
    elif page == 5:
        ev('check', 291, trait(31))
        if ro or signed(r[76], 8) == 8:disable([291])
        ev('sticky')
    else:
        for i in range(6):ev('radio', 3*i+271, 3*i+273, 3*i+271+signed(r[70+i], 8))
        if ro:disable(range(271, 289))
        ev('catalog', ids['idsCosts75ExtraResearchFieldsStartTech']);ev('text', 291, text=f'Tech {4 if signed(r[76],8)==9 else 3}'.encode());ev('check', 291, trait(29))
        if ro:disable([291])
        ev('sticky')
    return dict(events=events, result=1, count=count, cache=cache, description=description, combo=combo, selection=selection,
                next=seed % 2, race=bytes(r).hex(), password=password.hex(), rects=rects)


def vectors():
    for page in range(1, 7):
        for profile in range(8):
            for ro in (0, 1, -7):
                for tutorial in (0, 1):
                    for player in (-1, 0, 1):yield page, profile, ro, tutorial, player, 3, 1, 12, 400, 0
    for choice in (-1, 0, 4, 5, 6, 127):
        for seed in (0, 1, 8, 9, 31):
            for kind in range(3):yield 1, 6, 0, 0, 0, choice, seed, 12, 400, kind
    for font in (-32768, -3, -1, 0, 1, 9, 12, 32767):
        for right in (-400, 0, 400, 32768):
            for kind in range(4):yield 4, 7, 0, 0, 0, 3, 8, font, right, kind
    for page in (3, 6):
        for seed in range(14):yield page, 7, 0, 0, 0, 3, seed, 12, 400, 0
    for page in (2, 3, 5, 6):
        for seed in (0, 8, 9, 31):yield page, 7, 0, 0, 0, 3, seed, 12, 400, 0


def main():
    import re
    extra = PRELUDE.replace('#include "resource.h"', '#include "'+str(ROOT/'res/resource.h')+'"')
    source = (ROOT/'src/core/race.c').read_text()
    extra += '\n'+next(line for line in source.splitlines() if line.startswith('int16_t GetRaceStat('))
    extra += '\n'+function('src/core/race.c', 'int16_t GetRaceGrbit(')
    extra += '\n'+function('src/ui/nativeui.c', 'BOOL FSetUtf8WindowText(')
    extra += '\n#pragma GCC diagnostic push\n#pragma GCC diagnostic ignored \"-Wunused-but-set-variable\"\n'\
        +function('src/ui/raceui.c', 'void SetRCWTitle(').replace('void SetRCWTitle(', 'static void NativeTitle(', 1)\
        +'\n#pragma GCC diagnostic pop\n'
    locals_ = {1:'int16_t i,iPlrBmp;HWND hwndCB;char *psz;', 2:'int16_t i,cch,dxLabel,dxMiddle,dy,yTop;RECT rc;HDC hdc;HFONT hfontSav;char szTemp[20];', 3:'', 4:'int16_t i;', 5:'int16_t i;HWND hwndCtl;', 6:'int16_t i;'}
    for page in range(1, 7):
        callback = function('src/ui/raceui.c', f'INT_PTR CALLBACK RaceWizardDlg{page}(')
        arm = callback.split('    case WM_INITDIALOG:\n', 1)[1].split('    case WM_', 1)[0]
        extra += f'\nstatic int Init{page}(void){{HWND hwnd=(HWND)7;{locals_[page]}\n'+arm+'}\n'
    cases = list(vectors())
    with tempfile.TemporaryDirectory(prefix='race-init-') as tmp:
        directory = Path(tmp);binary = build(directory, extra=extra, harness=HARNESS)
        ids = {name:int(value, 16) for name, value in re.findall(r'\b(ids\w+) = (0x[0-9a-f]+)', (directory/'textids.h').read_text())}
        run = subprocess.run([str(binary)], input=('\n'.join(' '.join(map(str, v)) for v in cases)+'\n').encode(), capture_output=True)
        assert run.returncode == 0, run.stderr.decode()
        lines = run.stdout.splitlines();assert len(lines) == len(cases)
        for i, (values, line) in enumerate(zip(cases, lines)):
            actual = json.loads(line);expected = oracle(values, ids)
            # The two GetTextExtent operands in one addition have unspecified order.
            events = actual['events']
            for j in range(len(events)-1):
                if events[j][0]=='measure' and events[j][1]==3 and events[j+1][0]=='measure' and events[j+1][1]==2:events[j:j+2] = events[j:j+2][::-1]
            for key, value in expected.items():assert actual[key] == value, (i, values, key, actual[key], value)
    print(f'{len(cases)} actual initialization/title/helper cases match native bytes, UI requests, five-item combo behavior and all 17 rectangles')
    print('Scripted catalog/text/metrics/window services; initialization only, with no native display/paint/mouse qualification claim.')


if __name__ == '__main__':
    main()
