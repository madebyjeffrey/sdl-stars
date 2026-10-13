#include "win.h"
#include <SDL3/SDL.h>

/* Desktop metadata is deliberately outside the legacy game records. A folder
 * is listed only after its complete generated family and metadata are saved. */
static char szLibraryRoot[256];
static char szLibraryPending[256];
static char szLibraryName[256];

static int16_t FInitLibrary(void) {
    const char *pszOverride;
    char *pszRoot;
    int cch;

    pszOverride = getenv("STARS_LIBRARY_DIR");
    if (!pszOverride) pszOverride = getenv("STARS_SETTINGS_DIR");
    pszRoot = pszOverride ? NULL : SDL_GetPrefPath("", "Stars!");
    if (!pszOverride && !pszRoot) return FALSE;
    cch = CchSnprintf(szLibraryRoot, sizeof(szLibraryRoot), "%s%sGames", pszOverride ? pszOverride : pszRoot,
                   pszOverride ? szDirSep : "");
    SDL_free(pszRoot);
    return cch > 0 && cch < (int)sizeof(szLibraryRoot) && SDL_CreateDirectory(szLibraryRoot);
}

static SDL_EnumerationResult SDLCALL FindLibraryName(void *lpName, const char *pszDir, const char *pszFile) {
    (void)pszDir;
    if (!SDL_strcasecmp(lpName, pszFile)) return SDL_ENUM_FAILURE;
    return SDL_ENUM_CONTINUE;
}

static int16_t FLibraryNameAvailable(const char *pszName) {
    return SDL_EnumerateDirectory(szLibraryRoot, FindLibraryName, (void *)pszName);
}

static int16_t FPortableLibraryName(const char *pszName) {
    const unsigned char *pch;
    char szDevice[256];
    size_t cch, i;

    cch = strlen(pszName);
    if (cch == 0 || cch >= sizeof(szDevice) || pszName[0] == ' ' || pszName[0] == '.' || pszName[cch - 1] == ' ' || pszName[cch - 1] == '.') return FALSE;
    for (pch = (const unsigned char *)pszName; *pch; pch++)
        if (*pch < 32 || *pch == 127 || strchr("<>:\"/\\|?*", *pch)) return FALSE;
    /* Windows reserves these stems even when followed by an extension. */
    for (i = 0; i < cch && pszName[i] != '.'; i++) szDevice[i] = pszName[i];
    szDevice[i] = 0;
    if (!SDL_strcasecmp(szDevice, "CON") || !SDL_strcasecmp(szDevice, "PRN") ||
        !SDL_strcasecmp(szDevice, "AUX") || !SDL_strcasecmp(szDevice, "NUL") ||
        !SDL_strcasecmp(szDevice, "CLOCK$") ||
        (i == 4 && (!SDL_strncasecmp(szDevice, "COM", 3) || !SDL_strncasecmp(szDevice, "LPT", 3)) &&
         szDevice[3] >= '1' && szDevice[3] <= '9')) return FALSE;
    /* Longest backup path: root/name/backup.999/name.m16 plus NUL. */
    return strlen(szLibraryRoot) + 2 * cch + 18 < sizeof(szBase);
}

static void SuggestLibraryName(char *pszName) {
    char szOriginal[256];
    char szSuffix[16];
    int i, cch, cchMax;

    strcpy(szOriginal, pszName);
    cchMax = ((int)sizeof(szBase) - (int)strlen(szLibraryRoot) - 19) / 2;
    for (i = 2; i < 10000 && !FLibraryNameAvailable(pszName); i++) {
        CchSnprintf(szSuffix, sizeof(szSuffix), " (%d)", i);
        cch = min((int)strlen(szOriginal), max(0, cchMax - (int)strlen(szSuffix)));
        /* A shortened suggestion must not split a UTF-8 character. */
        while (cch > 0 && ((unsigned char)szOriginal[cch] & 0xc0) == 0x80) cch--;
        CchSnprintf(pszName, 256, "%.*s%s", cch, szOriginal, szSuffix);
    }
}

INT_PTR CALLBACK LibraryNameDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    char *pszName;
    char szName[256];
    StringId ids;

    switch (message) {
    case WM_INITDIALOG:
        SetWindowLongPtr(hwnd, DWLP_USER, lParam);
        SendDlgItemMessage(hwnd, IDC_EDIT1, EM_LIMITTEXT, 255, 0);
        SetDlgItemText(hwnd, IDC_EDIT1, (char *)lParam);
        return TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDCANCEL) { EndDialog(hwnd, FALSE); return TRUE; }
        if (LOWORD(wParam) != IDOK) break;
        pszName = (char *)GetWindowLongPtr(hwnd, DWLP_USER);
        GetDlgItemText(hwnd, IDC_EDIT1, szName, sizeof(szName));
        if (!FPortableLibraryName(szName)) ids = idsLibraryInvalidName;
        else if (!FLibraryNameAvailable(szName)) {
            SuggestLibraryName(szName);
            SetDlgItemText(hwnd, IDC_EDIT1, szName);
            ids = idsLibraryNameTaken;
        } else {
            strcpy(pszName, szName);
            EndDialog(hwnd, TRUE);
            return TRUE;
        }
        MessageBox(hwnd, PszGetCompressedString(ids), PszGetCompressedString(idsLibraryGames), MB_OK | MB_ICONEXCLAMATION);
        return TRUE;
    }
    return FALSE;
}

int16_t FChooseLibraryGameName(HWND hwnd, char *pszFile) {
    char *pch;

    if (!FInitLibrary() || strlen(szLibraryRoot) + 20 >= sizeof(szBase)) goto LFail;
    pch = strrchr(pszFile, chDirSep);
    CchSnprintf(szLibraryName, sizeof(szLibraryName), "%s", pch ? pch + 1 : pszFile);
    pch = strrchr(szLibraryName, '.');
    if (pch) *pch = 0;
    if (!FPortableLibraryName(szLibraryName)) strcpy(szLibraryName, "Game");
    SuggestLibraryName(szLibraryName);
    if (DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_LIBRARY_NAME), hwnd, LibraryNameDlg, (LPARAM)szLibraryName) != TRUE) return FALSE;
    CchSnprintf(szLibraryPending, sizeof(szLibraryPending), "%s%s%s", szLibraryRoot, szDirSep, szLibraryName);
    /* Reserve only a newly created folder; never write into an existing copy. */
    if (!FLibraryNameAvailable(szLibraryName) || CreateNativeDir(szLibraryPending) != 0) {
        szLibraryPending[0] = 0;
        goto LFail;
    }
    CchSnprintf(pszFile, 256, "%s%s%s.xy", szLibraryPending, szDirSep, szLibraryName);
    return TRUE;
LFail:
    MessageBox(hwnd, PszGetCompressedString(idsLibraryStorageFailed), PszGetCompressedString(idsLibraryGames), MB_OK | MB_ICONHAND);
    return FALSE;
}

static int16_t FWriteLibraryMetadata(const char *pszDir, const char *pszName, int iplr) {
    char szFile[256], szTemp[256];
    FILE *pf;
    int fWritten, fClosed;

    CchSnprintf(szFile, sizeof(szFile), "%s%s.library.tmp", pszDir, szDirSep);
    pf = PfOpenNativeFile(szFile, "wb");
    if (!pf) return FALSE;
    fWritten = fprintf(pf, "1\n%s\n%d\n%lld\n", pszName, iplr, (long long)LNativeTime()) > 0;
    fClosed = fclose(pf) == 0;
    if (!fWritten || !fClosed) { RemoveNativeFile(szFile); return FALSE; }
    CchSnprintf(szFile, sizeof(szFile), "%s%s.library", pszDir, szDirSep);
    /* Publish complete metadata, preserving the previous session on failure. */
    CchSnprintf(szTemp, sizeof(szTemp), "%s%s.library.tmp", pszDir, szDirSep);
    return ReplaceNativeFile(szTemp, szFile) == 0;
}

static SDL_EnumerationResult SDLCALL RemovePendingFile(void *lpUnused, const char *pszDir, const char *pszFile) {
    char szFile[256];
    (void)lpUnused;
    CchSnprintf(szFile, sizeof(szFile), "%s%s", pszDir, pszFile);
    /* Failed generation can leave an empty directory at a file destination. */
    SDL_RemovePath(szFile);
    return SDL_ENUM_CONTINUE;
}

/* GenerateWorld's original companion-write loop ignores writer return values.
 * Verify complete record framing without replacing the loaded player state. */
static int16_t FCompleteGeneratedFile(const char *pszFile, DtFileType dt, int iplr) {
    FILE *pf;
    HDR hdr;
    RTBOF rtbof;
    long cbFile, ib;
    int16_t fComplete;

    pf = PfOpenNativeFile(pszFile, "rb");
    if (!pf) return FALSE;
    fComplete = FALSE;
    if (fseek(pf, 0, SEEK_END) != 0 || (cbFile = ftell(pf)) < 22 || fseek(pf, 0, SEEK_SET) != 0) goto LDone;
    if (fread(&hdr, 2, 1, pf) != 1 || hdr.rt != rtBOF || hdr.cb != 16 ||
        fread(&rtbof, 16, 1, pf) != 1 || memcmp(rtbof.rgid, "J3J3", 4) ||
        rtbof.lidGame != game.lid || rtbof.turn != game.turn || rtbof.dt != dt || rtbof.iPlayer != iplr) goto LDone;
    /* XY stores raw four-byte star packs between its GAME and EOF records. */
    if (dt == dtXY) {
        if (cbFile != 18 + 66 + 4L * game.cPlanMax + 4 || fseek(pf, -4, SEEK_END) != 0) goto LDone;
        fComplete = fread(&hdr, 2, 1, pf) == 1 && hdr.rt == rtEOF && hdr.cb == 2;
        goto LDone;
    }
    while (fread(&hdr, 2, 1, pf) == 1) {
        ib = ftell(pf);
        if (ib < 0 || ib + hdr.cb > cbFile) break;
        if (hdr.rt == rtEOF) {
            fComplete = hdr.cb == 2 && ib + hdr.cb == cbFile;
            break;
        }
        if (fseek(pf, hdr.cb, SEEK_CUR) != 0) break;
    }
LDone:
    fclose(pf);
    return fComplete;
}

int16_t FFinishLibraryGame(HWND hwnd, int16_t fGenerated) {
    char szFile[256];
    int i;

    if (!szLibraryPending[0]) return FALSE;
    for (i = -2; fGenerated && i < game.cPlayer; i++) {
        if (i == -2) CchSnprintf(szFile, sizeof(szFile), "%s.xy", szBase);
        else if (i == -1) CchSnprintf(szFile, sizeof(szFile), "%s.hst", szBase);
        else CchSnprintf(szFile, sizeof(szFile), "%s.m%d", szBase, i + 1);
        if (!FCompleteGeneratedFile(szFile, i == -2 ? dtXY : i == -1 ? dtHost : dtTurn, i < 0 ? iplrNone : i)) fGenerated = FALSE;
    }
    if (fGenerated) fGenerated = FWriteLibraryMetadata(szLibraryPending, szLibraryName, idPlayer);
    if (!fGenerated) {
        SDL_EnumerateDirectory(szLibraryPending, RemovePendingFile, NULL);
        SDL_RemovePath(szLibraryPending);
        MessageBox(IsWindow(hwnd) ? hwnd : hwndFrame, PszGetCompressedString(idsLibraryStorageFailed), PszGetCompressedString(idsLibraryGames), MB_OK | MB_ICONHAND);
    }
    szLibraryPending[0] = 0;
    return fGenerated;
}

static SDL_EnumerationResult SDLCALL ListLibraryGame(void *lpList, const char *pszDir, const char *pszName) {
    char szFile[256], szName[256];
    FILE *pf;
    int iVersion, iplr, fRead;
    long long lPlayed;
    SDL_PathInfo info;
    LRESULT i;

    if (!FPortableLibraryName(pszName)) return SDL_ENUM_CONTINUE;
    CchSnprintf(szFile, sizeof(szFile), "%s%s%s.library", pszDir, pszName, szDirSep);
    pf = PfOpenNativeFile(szFile, "rb");
    if (!pf) return SDL_ENUM_CONTINUE;
    iVersion = 0; iplr = -1;
    fRead = fscanf(pf, "%d\n", &iVersion) == 1 && fgets(szName, sizeof(szName), pf) &&
            fscanf(pf, "%d\n%lld", &iplr, &lPlayed) == 2;
    fclose(pf);
    if (!fRead || iVersion != 1 || iplr < -1 || iplr >= 16) return SDL_ENUM_CONTINUE;
    CchSnprintf(szFile, sizeof(szFile), "%s%s%s%s.%s", pszDir, pszName, szDirSep, pszName, "xy");
    if (!SDL_GetPathInfo(szFile, &info) || info.size == 0) return SDL_ENUM_CONTINUE;
    CchSnprintf(szFile, sizeof(szFile), "%s%s%s%s.hst", pszDir, pszName, szDirSep, pszName);
    if (iplr >= 0) CchSnprintf(szFile, sizeof(szFile), "%s%s%s%s.m%d", pszDir, pszName, szDirSep, pszName, iplr + 1);
    if (SDL_GetPathInfo(szFile, &info) && info.size > 0) {
        i = SendMessage(lpList, LB_ADDSTRING, 0, (LPARAM)pszName);
        if (i != LB_ERR && i != LB_ERRSPACE) SendMessage(lpList, LB_SETITEMDATA, i, iplr);
    }
    return SDL_ENUM_CONTINUE;
}

void InitLibraryGames(HWND hwnd, const RECT *lprc) {
    HWND hwndList;
    int y;

    y = max(100, lprc->bottom - 240);
    CreateWindow("STATIC", PszGetCompressedString(idsLibraryGames), WS_CHILD | WS_VISIBLE, 24, y - 20, 200, 20,
                 hwnd, NULL, hInst, NULL);
    hwndList = CreateWindow("LISTBOX", "", WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | WS_VSCROLL | LBS_NOTIFY | LBS_SORT,
                            24, y, lprc->right - 48, 120, hwnd, (HMENU)(uintptr_t)IDC_LIBRARY_GAMES, hInst, NULL);
    if (FInitLibrary()) SDL_EnumerateDirectory(szLibraryRoot, ListLibraryGame, hwndList);
    SendMessage(hwndList, LB_SETCURSEL, 0, 0);
    EnableWindow(rghwndBtnSplash[1], SendMessage(hwndList, LB_GETCOUNT, 0, 0) > 0);
}

int16_t FSelectLibraryGame(HWND hwnd) {
    HWND hwndList;
    LRESULT i, iplr;
    char szName[256];

    hwndList = GetDlgItem(hwnd, IDC_LIBRARY_GAMES);
    i = SendMessage(hwndList, LB_GETCURSEL, 0, 0);
    if (i == LB_ERR) return FALSE;
    SendMessage(hwndList, LB_GETTEXT, i, (LPARAM)szName);
    iplr = SendMessage(hwndList, LB_GETITEMDATA, i, 0);
    if (iplr >= 0) CchSnprintf(szBase, sizeof(szBase), "%s%s%s%s%s.m%d", szLibraryRoot, szDirSep, szName, szDirSep, szName, (int)iplr + 1);
    else CchSnprintf(szBase, sizeof(szBase), "%s%s%s%s%s.hst", szLibraryRoot, szDirSep, szName, szDirSep, szName);
    ini.fStartupFile = TRUE;
    return TRUE;
}

void RememberLibraryGame(void) {
    char szDir[256];
    char *pch;

    if (!FInitLibrary() || strncmp(szBase, szLibraryRoot, strlen(szLibraryRoot)) || szBase[strlen(szLibraryRoot)] != chDirSep) return;
    strcpy(szDir, szBase);
    pch = strrchr(szDir, chDirSep);
    if (!pch) return;
    *pch = 0;
    pch = strrchr(szDir, chDirSep);
    if (pch) FWriteLibraryMetadata(szDir, pch + 1, idPlayer);
}
