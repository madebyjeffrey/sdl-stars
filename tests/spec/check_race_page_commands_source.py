#!/usr/bin/env python3
"""Actual six WM_COMMAND arms and race helpers with scripted UI/catalog replies."""
import json
from pathlib import Path
import subprocess
import tempfile
from check_s02_source import ROOT, function
from check_race_workflows_source import build


PRELUDE = r'''
#include "resource.h"
typedef uintptr_t WPARAM;typedef intptr_t LPARAM;typedef intptr_t INT_PTR;
typedef struct {int32_t x,y;} POINT;
#define CB_GETCURSEL 0x147
#define CB_SETCURSEL 0x14e
#define BM_GETCHECK 0xf0
#define HELP_CONTEXT 1
PLAYER vrgplrDef[7];
_Static_assert(offsetof(PLAYER,rgAttr)==62 && offsetof(PLAYER,grbitAttr)==78,"native attribute and trait offsets");
_Static_assert(offsetof(PLAYER,rgEnvVar)==16 && offsetof(PLAYER,rgEnvVarMin)==19 && offsetof(PLAYER,rgEnvVarMax)==22,"native habitat offsets");
ControlId rgidRaceBtn[5]={IDCANCEL,IDC_BACK,IDC_NEXT,IDC_FINISH,IDC_HELP};
int16_t fRCWReadOnly,cColDrop;
static POINT16 ptStickyRaceDlg;static RECT rcCargo={10,20,200,220};static char szHelpFile[]="help";
static char nameText[64],pluralText[64],pwText[16];
static const char *defaults[]={"Human","Rabbit","Insect","Robo","Bird","Ant","Random"};
static const char *automatic="AutoPlural";
static int rawCheck,checkedMask,rejected,closed,endCode,combo,nextEnabled,first;
static int32_t NativeHash(char *);
static void hex(const void *p,size_t n){const uint8_t *a=p;putchar('"');for(size_t i=0;i<n;i++)printf("%02x",a[i]);putchar('"');}
static void ev(const char *kind,int64_t n,const char *text){if(!first)putchar(',');first=0;printf("[\"%s\",%lld,",kind,(long long)n);hex(text,strlen(text));putchar(']');}
static void bounded(char *out,const char *text,int size){size_t n=strlen(text);if(n>=(size_t)size)n=size-1;memcpy(out,text,n);out[n]=0;}
static HWND GetDlgItem(HWND hwnd,int id){assert(hwnd==(HWND)7);return (HWND)(uintptr_t)id;}
static int GetDlgItemText(HWND hwnd,int id,char *out,int size){
    assert(hwnd==(HWND)7);const char *text=id==IDC_RACE_NAME?nameText:id==IDC_RACE_PLURAL_NAME?pluralText:id<=277?defaults[id-271]:"Custom";
    ev("T",id,text);bounded(out,text,size);return strlen(out);
}
static void SetDlgItemText(HWND hwnd,int id,const char *text){assert(hwnd==(HWND)7);ev("S",id,text);strcpy(id==IDC_RACE_NAME?nameText:pluralText,text);}
static int StarsCompatTextInputRejected(HWND hwnd){assert(hwnd==(HWND)IDC_RACE_PASSWORD);ev("X",rejected,"");return rejected;}
static int GetWindowTextA(HWND hwnd,char *out,int size){assert(hwnd==(HWND)IDC_RACE_PASSWORD);ev("P",size,pwText);bounded(out,pwText,size);return strlen(out);}
static intptr_t SendMessage(HWND hwnd,int message,WPARAM w,LPARAM l){
    (void)l;if(message==CB_GETCURSEL){assert(hwnd==(HWND)IDC_COMBOBOX);ev("G",combo,"");return combo;}
    if(message==CB_SETCURSEL){assert(hwnd==(HWND)IDC_COMBOBOX);combo=(intptr_t)w;ev("C",combo,"");return combo;}
    assert(message==BM_GETCHECK);ev("K",(intptr_t)hwnd,"");return rawCheck;
}
static int IsDlgButtonChecked(HWND hwnd,int id){assert(hwnd==(HWND)7);ev("R",id,"");return (checkedMask>>(id-271))&1;}
static void StickyDlgPos(HWND hwnd,POINT16 *pt,int init){assert(hwnd==(HWND)7 && pt==&ptStickyRaceDlg && !init);ev("Y",0,"");}
static void EndDialog(HWND hwnd,int result){assert(hwnd==(HWND)7);closed=1;endCode=result;ev("E",result,"");}
static void WinHelp(HWND hwnd,char *file,int mode,int topic){assert(hwnd==(HWND)7 && file==szHelpFile && mode==HELP_CONTEXT);ev("H",topic,"");}
char *PszGetCompressedString(StringId id){
    if(id==idsNativePasswordReenter){ev("A",0,"");return "reenter";}
    assert(id>=idsHumanoid && id<idsHumanoid+7);ev("M",id-idsHumanoid,"");return (char *)defaults[id-idsHumanoid];
}
int16_t CchGetString(StringId id,char *out){assert(id>=idsHumanoid && id<idsHumanoid+7);ev("L",id-idsHumanoid,"");strcpy(out,defaults[id-idsHumanoid]);return strlen(out);}
char *PszPlayerName(int16_t a,int16_t b,int16_t c,int16_t d,int16_t e,PLAYER *p){assert(!a && b && c && !d && !e && p==&vplr && p->szNames[0]==0);ev("U",0,"");return (char *)automatic;}
int16_t AlertSz(char *text,int16_t style){assert(style==MB_ICONHAND);ev("!",style,text);return 0;}
int32_t LSaltFromSz(char *text){int32_t n=NativeHash(text);ev("#",n,text);return n;}
static void InvalidateAdvPtsRect(HWND hwnd){assert(hwnd==(HWND)7);ev("I",0,"");}
static void InvalidateRect(HWND hwnd,const RECT *rect,int erase){assert(hwnd==(HWND)7 && rect);ev("V",erase,"");}
static void EnableWindow(HWND hwnd,int enabled){assert(hwnd==(HWND)IDC_NEXT);nextEnabled=enabled;ev("N",enabled,"");}
static void GetWindowRect(HWND hwnd,RECT *r){assert(hwnd==(HWND)IDC_COMBOBOX);*r=(RECT){0,0,100,30};}
static void ScreenToClient(HWND hwnd,POINT *p){assert(hwnd==(HWND)7 && p);}
static void draw(int which,HWND hwnd,void *dc,int mask){
    assert(hwnd==(HWND)7 && dc==NULL);if(!first)putchar(',');first=0;
    printf("[\"D%d\",%d,",which,mask);hex(&vplr,192);putchar(']');BoundsCheckPlayer(&vplr);
}
static void DrawRace2(HWND hwnd,void *dc,int16_t mask){draw(2,hwnd,dc,mask);}
static void DrawRace3(HWND hwnd,void *dc,int16_t mask){draw(3,hwnd,dc,mask);}
'''
HARNESS = r'''
static void initialize(PLAYER *p,int seed){
    for(int i=0;i<192;i++)((uint8_t *)p)[i]=(uint8_t)(i*37+seed);
    strcpy(p->szName,"Original");strcpy(p->szNames,"Originals");p->iPlrBmp=seed%32;p->lSalt=123+seed;
    for(int i=0;i<3;i++){p->rgEnvVar[i]=50;p->rgEnvVarMin[i]=20;p->rgEnvVarMax[i]=80;}
    p->pctIdealGrowth=seed%5==0?25:15;
    for(int i=0;i<16;i++)p->rgAttr[i]=(int8_t)(i%7+5);
    p->rgAttr[0]=seed%3==0?0:10;p->rgAttr[7]=seed%7;p->rgAttr[14]=seed%10;
    p->grbitAttr=UINT32_C(0xe0001234);
}
int main(void){
    int page,id,notification,nameKind,seed,ro;
    while(scanf("%d %d %d %d %d %d %d %d %d %d",&page,&id,&notification,&rawCheck,&checkedMask,&rejected,&combo,&nameKind,&seed,&ro)==10){
        initialize(&vplr,seed);for(int i=0;i<7;i++){initialize(&vrgplrDef[i],40+i);if(i!=6)vrgplrDef[i].grbitAttr&=~grbitRaceAIPlayer;}
        memset(szRacePass,'T',16);strcpy(szRacePass,"old");
        const char *name=nameKind==0?"CustomName":nameKind<=7?defaults[nameKind-1]:nameKind==8?"1234567890123456789012345678901":"\xc3\xa9Race";
        strcpy(nameText,name);strcpy(pluralText,"FormPlural");strcpy(pwText,seed%11==0?"":seed%2?"\xc3\xa9":"abcdefghijklmno");
        fRCWReadOnly=ro;cColDrop=-7;nextEnabled=0;closed=0;endCode=-1;first=1;
        uint32_t param=(uint16_t)id|((uint32_t)(uint16_t)notification<<16);
        printf("{\"events\":[");int handled=0;
        switch(page){case 1:handled=Command1(param);break;case 2:handled=Command4(param);break;case 3:handled=Command5(param);break;
        case 4:handled=Command2(param);break;case 5:handled=Command3(param);break;case 6:handled=Command6(param);break;default:abort();}
        printf("],\"handled\":%d,\"closed\":%d,\"code\":%d,\"description\":%d,\"combo\":%d,\"next\":%d,\"race\":",handled,closed,endCode,cColDrop,combo,nextEnabled);
        hex(&vplr,192);printf(",\"password\":");hex(szRacePass,16);printf(",\"name\":");hex(nameText,strlen(nameText));printf(",\"plural\":");hex(pluralText,strlen(pluralText));puts("}");
    }
    (void)FSameLibraryRace;(void)SaveRaceState;(void)RestoreRaceState;return 0;
}
'''

DEFAULTS = [x.encode() for x in ('Human', 'Rabbit', 'Insect', 'Robo', 'Bird', 'Ant', 'Random')]
MINIMUM = [7, 5, 5, 5, 5, 2, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0]
MAXIMUM = [25, 15, 25, 25, 25, 15, 25, 6, 2, 2, 2, 2, 2, 2, 9, 0]


def signed(n, bits):
    n %= 2**bits
    return n if n < 2**(bits-1) else n-2**bits


def initial(seed):
    r = bytearray((i*37+seed) % 256 for i in range(192))
    def put(at, text):r[at:at+len(text)+1] = text+b'\0'
    put(128, b'Original');put(160, b'Originals')
    r[6] = r[6] % 8+8*(seed % 32);r[12:16] = (123+seed).to_bytes(4, 'little')
    r[16:25] = bytes([50]*3+[20]*3+[80]*3);r[25] = 25 if seed % 5 == 0 else 15
    r[62:78] = bytes(i % 7+5 for i in range(16));r[62] = 0 if seed % 3 == 0 else 10
    r[69] = seed % 7;r[76] = seed % 10;r[78:82] = (0xe0001234).to_bytes(4, 'little')
    return r


def oracle(v):
    page, ident, notification, check, radios, rejected, combo, kind, seed, readonly = v
    r = initial(seed);password = bytearray(b'old\0'+b'T'*12);description = -7;enabled = 0;closed = 0;code = -1;handled = 0;events = []
    name = b'CustomName' if kind == 0 else DEFAULTS[kind-1] if kind <= 7 else b'1234567890123456789012345678901' if kind == 8 else 'éRace'.encode()
    plural = b'FormPlural';pw = b'' if seed % 11 == 0 else 'é'.encode() if seed % 2 else b'abcdefghijklmno'
    def event(k, n=0, text=b''):events.append([k, n, bytes(text).hex()])
    def put(at, text, zero=False):
        if zero:r[at:at+32] = bytes(32)
        r[at:at+len(text)+1] = text+b'\0'
    def trait(bit, on):
        at = 78+bit//8;mask = 1 << (bit % 8);r[at] = r[at] | mask if on else r[at] & ~mask
    def stat(index, value):r[62+index] = max(MINIMUM[index], min(MAXIMUM[index], value))
    def draw(which, mask):
        event('D'+str(which), mask, r)
        for i in range(3):
            lo, mid, hi = signed(r[19+i], 8), signed(r[16+i], 8), signed(r[22+i], 8)
            if lo == -1:
                if hi != -1 or mid != -1:r[16+i] = r[22+i] = 255;r[84] |= 16
            else:
                new_lo = max(0, min(100, lo));new_hi = max(new_lo, min(100, hi));new_mid = new_lo+(new_hi-new_lo)//2
                if (lo, mid, hi) != (new_lo, new_mid, new_hi):r[84] |= 16
                r[19+i], r[16+i], r[22+i] = new_lo, new_mid, new_hi
        if signed(r[25], 8) > 20:r[25] = 20;r[84] |= 16
        for i in range(16):
            value = signed(r[62+i], 8);bounded = max(MINIMUM[i], min(MAXIMUM[i], value))
            if value != bounded:r[84] |= 16
            r[62+i] = bounded
    nav = {2: 0, 1070: 1, 1071: 2, 1072: 3}.get(ident)
    if ident == 118:
        handled = 1;event('H', (1023, 1032, 1041, 1053, 1056, 1057)[page-1])
    elif nav is not None:
        handled = 1
        if page == 1 and nav:
            portrait = r[6]//8;selected = 8
            for i in range(8):
                event('R', i+271)
                if radios & (1 << i):selected = i;break
            if selected < 7:r = initial(40+selected);trait(30, selected == 6)
            else:trait(30, False)
            event('T', 268, name);put(128, name);event('T', 2075, plural);put(160, plural);event('X', rejected)
            if rejected:password[0] = 0;event('A');event('!', 16, b'reenter')
            else:
                event('P', 16, pw);password[:len(pw)+1] = pw+b'\0';event('G', combo);stat(7, signed(combo, 16))
                salt = 0
                for i in range(0, len(pw), 2):
                    salt = (salt+signed(pw[i], 8)) % 2**32
                    if i+1 < len(pw):salt = salt*signed(pw[i+1], 8) % 2**32
                if pw and salt == 0:salt = 1
                event('#', signed(salt, 32), pw);r[12:16] = salt.to_bytes(4, 'little');r[6] = r[6] % 8+portrait*8
        if not (page == 1 and nav and rejected):closed = 1;code = nav;event('Y');event('E', nav)
    elif page == 1 and notification == 0 and 271 <= ident <= 278:
        event('T', 268, name);put(128, name, True);event('T', 2075, plural);put(160, plural, True)
        event('T', ident, DEFAULTS[ident-271] if ident < 278 else b'Custom')
        matched = 7
        for i, default in enumerate(DEFAULTS):
            event('M', i)
            if name == default:matched = i;break
        if matched < 7 and ident < 278:
            name = DEFAULTS[ident-271];event('L', ident-271);put(128, name, True);event('S', 268, name)
            r[160:192] = bytes(32);event('U');plural = b'AutoPlural';put(160, plural);event('S', 2075, plural)
        source = initial(40+ident-271) if ident < 278 else r
        event('I');combo = signed(source[69], 8);event('C', combo);enabled = int(ident != 277);event('N', enabled)
        r[6] = r[6] % 8+8*(source[6]//8);event('V', 1)
    elif page == 4 and 291 <= ident <= 293:
        event('K', ident);i = ident-291;immune = signed(check, 16) == 1
        r[16+i], r[19+i], r[22+i] = (255, 255, 255) if immune else (50, 20, 80);draw(2, signed((1 << i) | 0xff00, 16))
    elif page == 5 and ident == 291:
        event('K', ident);trait(31, signed(check, 16) != 0);draw(3, 99)
    elif page == 2 and notification == 0 and 271 <= ident <= 280:
        stat(14, ident-271)
        if ident-271 == 8:
            for i, value in enumerate((10, 10, 10, 10, 5, 10), 1):stat(i, value)
            trait(31, False)
        event('I');event('V')
    elif page == 3 and 291 <= ident <= 304:
        event('K', ident);description = ident-291;trait(description, signed(check, 16) != 0);event('I');event('V')
    elif page == 6 and notification == 0 and 271 <= ident <= 288:
        i = ident-271;stat(i//3+8, i % 3);event('I')
    elif page == 6 and ident == 291:
        event('K', ident);trait(29, signed(check, 16) != 0);event('I')
    return dict(events=events, handled=handled, closed=closed, code=code, description=description, combo=combo, next=enabled,
                race=bytes(r).hex(), password=bytes(password).hex(), name=name.hex(), plural=plural.hex())


def vectors():
    for page in range(1, 7):
        for ident in (0, 1, 2, 118, 1070, 1071, 1072, 270, *range(271, 281), 288, 290, *range(291, 305), 305):
            for notification in (0, 1, 65535):
                for ro in (0, -7):yield page, ident, notification, 1, 128, 0, 3, 0, 15, ro
    for ident in (1070, 1071, 1072):
        for radios in (0, 1, 2, 4, 8, 16, 32, 64, 128, 255):
            for rejected in (0, 1, -7):
                for combo in (-65537, -1, 0, 6, 7, 65535, 65542):
                    for kind in (0, 8, 9):yield 1, ident, 65535, 0, radios, rejected, combo, kind, 26, 0
    for ident in range(271, 279):
        for kind in range(10):yield 1, ident, 0, 1, 128, 0, 3, kind, 15, 0
    for seed in (0, 1, 7, 31, 32, 63):
        for radios in (1, 64, 128):
            for rejected in (0, 1, -7):yield 1, 1071, 0, 0, radios, rejected, 6, 9, seed, 0
    for page, ident in ((4, 291), (4, 293), (5, 291), (3, 291), (3, 304), (6, 291)):
        for check in (-65536, -32768, -1, 0, 1, 2, 32768, 65535, 65536, 65537):yield page, ident, 65535, check, 128, 0, 3, 0, 15, -7


def main():
    extra = PRELUDE.replace('#include "resource.h"', '#include "'+str(ROOT/'res/resource.h')+'"')
    race_source = (ROOT/'src/core/race.c').read_text()
    for line in race_source.splitlines():
        if line.startswith('char    rgRaceStatMax') or line.startswith('char    rgRaceStatMin'):extra += '\n'+line
    extra += '\n'+next(line for line in race_source.splitlines() if line.startswith('int16_t GetRaceStat('))
    for signature in ('int16_t SetRaceStat(', 'void SetRaceGrbit(', 'void BoundsCheckPlayer('):extra += '\n'+function('src/core/race.c', signature)
    extra += '\n'+function('src/core/utilgen.c', 'int32_t LSaltFromSz(').replace('int32_t LSaltFromSz(', 'static int32_t NativeHash(', 1)
    extra += '\n'+function('src/ui/nativeui.c', 'int CchGetUtf8WindowText(')
    for page in range(1, 7):
        source = function('src/ui/raceui.c', f'INT_PTR CALLBACK RaceWizardDlg{page}(')
        command = source.split('    case WM_COMMAND:\n', 1)[1]
        locals_ = 'int16_t i;'
        if page == 1:locals_ += 'int16_t j,k,iPlrBmp;PLAYER *pplr;char *psz,szBuf[32];RECT rc;'
        if page == 2:locals_ += 'int16_t iVar;'
        extra += f'\nstatic int Command{page}(uint32_t wParam){{HWND hwnd=(HWND)7;{locals_}\nswitch(0){{case 0:\n'+command
    cases = list(vectors())
    with tempfile.TemporaryDirectory(prefix='race-page-commands-') as tmp:
        binary = build(Path(tmp), extra=extra, harness=HARNESS)
        run = subprocess.run([str(binary)], input=('\n'.join(' '.join(map(str, v)) for v in cases)+'\n').encode(), capture_output=True)
        assert run.returncode == 0, run.stderr.decode()
        lines = run.stdout.splitlines();assert len(lines) == len(cases), (len(lines), len(cases))
        for i, (values, line) in enumerate(zip(cases, lines)):
            actual = json.loads(line);expected = oracle(values)
            for key, value in expected.items():assert actual[key] == value, (i, values, key, actual[key], value)
    print(f'{len(cases)} actual page-command/helper cases match returns, call order, full PLAYER/password tails and UI choices')
    print('Actual command arms, stat/trait/hash/password helpers and BoundsCheckPlayer; scripted catalog/text/geometry and drawing services.')
    print('Initialization, mouse/paint/tracking and complete native UI/pricing refinement remain separately owned.')


if __name__ == '__main__':
    main()
