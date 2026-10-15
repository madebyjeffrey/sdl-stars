#include "win.h"
#include <stars_compat.h>
#include <SDL3/SDL.h>

/* Desktop metadata is deliberately outside the legacy game records. A folder
 * is listed only after its complete generated family and metadata are saved. */
static char szLibraryRoot[256];
static char szLibraryPending[256];
static char szLibraryName[256];
static char szLibraryExchangeName[256];
static char szRaceLibraryRoot[256];
static char szRaceLibraryPath[512];
static char szRaceLibraryName[256];
static char szRaceLibraryTemp[512];
static uint32_t idRaceStage;
static int16_t fRaceLibraryNameDialog;
static const char *pszRaceLibraryNameOverride;
static int16_t FCopyNativeFile(const char *pszSource, const char *pszTarget);

typedef struct {
    PLAYER plr;
    char szPass[sizeof(szRacePass)];
    char szFile[sizeof(szRaceFile)];
    char szPath[sizeof(szRaceLibraryPath)];
} RACESTATE;

typedef struct {
    const char *pszDirectory;
    const char *pszName;
    int16_t fFound;
} LIBRARYNAMECHECK;

static void SaveRaceState(RACESTATE *prs) {
    prs->plr = vplr;
    memcpy(prs->szPass, szRacePass, sizeof(prs->szPass));
    memcpy(prs->szFile, szRaceFile, sizeof(prs->szFile));
    memcpy(prs->szPath, szRaceLibraryPath, sizeof(prs->szPath));
}

static void RestoreRaceState(const RACESTATE *prs, int16_t fRestoreRace, int16_t fRestorePath) {
    if (fRestoreRace) {
        vplr = prs->plr;
        memcpy(szRacePass, prs->szPass, sizeof(prs->szPass));
        memcpy(szRaceFile, prs->szFile, sizeof(prs->szFile));
    }
    if (fRestorePath) memcpy(szRaceLibraryPath, prs->szPath, sizeof(prs->szPath));
}

static int16_t FindLibraryName(void *lpData, const char *pszFile) {
    LIBRARYNAMECHECK *plnc = lpData;
    if (!SDL_strcasecmp(plnc->pszName, pszFile)) {
        plnc->fFound = TRUE;
        return FALSE;
    }
    return TRUE;
}

static int16_t FLibraryNameAvailable(const char *pszDirectory, const char *pszName) {
    LIBRARYNAMECHECK lnc;
    lnc.pszDirectory = pszDirectory;
    lnc.pszName = pszName;
    lnc.fFound = FALSE;
    if (!FNativeEnumerateDirectory(lnc.pszDirectory, FindLibraryName, &lnc)) return FALSE;
    return !lnc.fFound;
}

static int16_t FRaceLibraryNameAvailable(const char *pszName) {
    return FLibraryNameAvailable(szRaceLibraryRoot, pszName);
}

static int16_t FPortableRaceFilename(const char *pszName) {
    const unsigned char *pch;
    char szDevice[256];
    size_t cch = strlen(pszName);
    size_t i;
    const char *pchExt = strrchr(pszName, '.');
    const char *pchDigit;
    if (!pchExt || pchExt == pszName || SDL_tolower(pchExt[1]) != 'r' || !pchExt[2] ||
        pchExt[2] == '0' || pchExt[2] < '1' || pchExt[2] > '9' || cch >= 240 || pszName[0] == ' ' || pszName[0] == '.' ||
        pszName[cch - 1] == ' ' || pszName[cch - 1] == '.') return FALSE;
    /* Windows reserves these stems even when followed by an extension. */
    for (i = 0; i < cch && pszName[i] != '.'; i++) szDevice[i] = pszName[i];
    szDevice[i] = 0;
    if (!SDL_strcasecmp(szDevice, "CON") || !SDL_strcasecmp(szDevice, "PRN") ||
        !SDL_strcasecmp(szDevice, "AUX") || !SDL_strcasecmp(szDevice, "NUL") ||
        !SDL_strcasecmp(szDevice, "CLOCK$") ||
        (i == 4 && (!SDL_strncasecmp(szDevice, "COM", 3) || !SDL_strncasecmp(szDevice, "LPT", 3)) &&
         szDevice[3] >= '1' && szDevice[3] <= '9')) return FALSE;
    for (pchDigit = pchExt + 3; *pchDigit; pchDigit++)
        if (*pchDigit < '0' || *pchDigit > '9') return FALSE;
    if (strlen(pchExt + 2) > 3) return FALSE;
    for (pch = (const unsigned char *)pszName; *pch; pch++)
        if (*pch < 32 || *pch == 127 || strchr("<>:\"/\\|?*", *pch)) return FALSE;
    if (strlen(szRaceLibraryRoot) + cch + 2 >= sizeof(szBase)) return FALSE;
    return TRUE;
}

static int16_t FInitRaceLibrary(void) {
    const char *pszOverride = getenv("STARS_LIBRARY_DIR");
    const char *pszSettings = getenv("STARS_SETTINGS_DIR");
    char *pszRoot = pszOverride || pszSettings ? NULL : SDL_GetPrefPath("", "Stars!");
    int cch;
    if (!pszOverride && !pszSettings && !pszRoot) return FALSE;
    cch = CchSnprintf(szRaceLibraryRoot, sizeof(szRaceLibraryRoot), "%s%sRace Designs",
                      pszOverride ? pszOverride : pszSettings ? pszSettings : pszRoot,
                      pszOverride || pszSettings ? szDirSep : "");
    SDL_free(pszRoot);
    return cch > 0 && cch < (int)sizeof(szRaceLibraryRoot) && FEnsureNativeDir(szRaceLibraryRoot);
}

static int16_t FCreateRaceStagePath(char *pszDirectory, size_t cchDirectory,
                                    char *pszFile, size_t cchFile) {
    uint32_t dwTick = DwTickCount();
    int i;
    int cch;
    for (i = 0; i < 1000; i++) {
        cch = CchSnprintf(pszDirectory, cchDirectory, "%s%s.stars-race-stage-%u-%u",
                          szRaceLibraryRoot, szDirSep, dwTick, idRaceStage++);
        if (cch <= 0 || (size_t)cch >= cchDirectory) return FALSE;
        if (CreateNativeDir(pszDirectory) != 0) {
            if (FNativeDirectoryExists(pszDirectory)) continue;
            return FALSE;
        }
        cch = CchSnprintf(pszFile, cchFile, "%s%sdesign.r1", pszDirectory, szDirSep);
        if (cch <= 0 || (size_t)cch >= cchFile) {
            RemoveNativePath(pszDirectory);
            return FALSE;
        }
        return TRUE;
    }
    return FALSE;
}

static void SuggestRaceFilename(char *pszName) {
    char szStem[240];
    const char *pch = strrchr(pszName, '.');
    size_t cchStem = pch ? (size_t)(pch - pszName) : strlen(pszName);
    int i;
    if (cchStem >= sizeof(szStem)) cchStem = sizeof(szStem) - 1;
    cchStem = CchUtf8Prefix(pszName, cchStem);
    memcpy(szStem, pszName, cchStem);
    szStem[cchStem] = 0;
    for (i = 1; i < 1000; i++) {
        CchSnprintf(pszName, 256, "%s.r%d", szStem, i);
        if (FRaceLibraryNameAvailable(pszName)) return;
    }
    pszName[0] = 0;
}

static void FormatLibraryFamilyPath(char *pszPath, size_t cchPath, const char *pszDirectory,
                                    const char *pszName, DtFileType dt, int iplr) {
    const char *pszExtension;
    if (dt == dtXY) pszExtension = "xy";
    else if (dt == dtHost) pszExtension = "hst";
    else pszExtension = dt == dtTurn ? "m" : dt == dtHist ? "h" : "x";
    if (dt == dtXY || dt == dtHost) CchSnprintf(pszPath, cchPath, "%s%s%s.%s", pszDirectory, szDirSep, pszName, pszExtension);
    else CchSnprintf(pszPath, cchPath, "%s%s%s.%s%d", pszDirectory, szDirSep, pszName, pszExtension, iplr + 1);
}

static int16_t FInitLibrary(void) {
    const char *pszOverride;
    const char *pszSettings;
    char *pszRoot;
    int cch;

    pszOverride = getenv("STARS_LIBRARY_DIR");
    pszSettings = getenv("STARS_SETTINGS_DIR");
    pszRoot = pszOverride || pszSettings ? NULL : SDL_GetPrefPath("", "Stars!");
    if (!pszOverride && !pszSettings && !pszRoot) return FALSE;
    cch = CchSnprintf(szLibraryRoot, sizeof(szLibraryRoot), "%s%sGames",
                      pszOverride ? pszOverride : pszSettings ? pszSettings : pszRoot,
                      pszOverride || pszSettings ? szDirSep : "");
    SDL_free(pszRoot);
    return cch > 0 && cch < (int)sizeof(szLibraryRoot) && FEnsureNativeDir(szLibraryRoot);
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
    for (i = 2; i < 10000 && !FLibraryNameAvailable(szLibraryRoot, pszName); i++) {
        CchSnprintf(szSuffix, sizeof(szSuffix), " (%d)", i);
        cch = min((int)strlen(szOriginal), max(0, cchMax - (int)strlen(szSuffix)));
        /* A shortened suggestion must not split a UTF-8 character. */
        while (cch > 0 && ((unsigned char)szOriginal[cch] & 0xc0) == 0x80) cch--;
        CchSnprintf(pszName, 256, "%.*s%s", cch, szOriginal, szSuffix);
    }
}

INT_PTR CALLBACK LibraryNameDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    char *pszName;
    char szName[256], szMessage[256];
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
        if (fRaceLibraryNameDialog) {
            if (!FPortableRaceFilename(szName)) ids = idsLibraryInvalidName;
            else if (!FRaceLibraryNameAvailable(szName)) {
                SuggestRaceFilename(szName);
                SetDlgItemText(hwnd, IDC_EDIT1, szName);
                ids = idsLibraryNameTaken;
            } else {
                strcpy(pszName, szName);
                EndDialog(hwnd, TRUE);
                return TRUE;
            }
        } else if (!FPortableLibraryName(szName)) ids = idsLibraryInvalidName;
        else if (!FLibraryNameAvailable(szLibraryRoot, szName)) {
            SuggestLibraryName(szName);
            SetDlgItemText(hwnd, IDC_EDIT1, szName);
            ids = idsLibraryNameTaken;
        } else {
            strcpy(pszName, szName);
            EndDialog(hwnd, TRUE);
            return TRUE;
        }
        /* Each lookup reuses szLastStrGet; retain the body before fetching the caption. */
        strcpy(szMessage, PszGetCompressedString(ids));
        MessageBox(hwnd, szMessage, PszGetCompressedString(idsLibraryGames), MB_OK | MB_ICONEXCLAMATION);
        return TRUE;
    }
    return FALSE;
}

int16_t FChooseLibraryGameName(HWND hwnd, char *pszFile) {
    char *pch;
    char szMessage[256];

    if (!FInitLibrary() || strlen(szLibraryRoot) + 20 >= sizeof(szBase)) goto LFail;
    szLibraryExchangeName[0] = 0;
    pch = strrchr(pszFile, chDirSep);
    CchSnprintf(szLibraryName, sizeof(szLibraryName), "%s", pch ? pch + 1 : pszFile);
    pch = strrchr(szLibraryName, '.');
    if (pch) *pch = 0;
    if (!FPortableLibraryName(szLibraryName)) strcpy(szLibraryName, "Game");
    SuggestLibraryName(szLibraryName);
    if (DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_LIBRARY_NAME), hwnd, LibraryNameDlg, (LPARAM)szLibraryName) != TRUE) return FALSE;
    CchSnprintf(szLibraryPending, sizeof(szLibraryPending), "%s%s%s", szLibraryRoot, szDirSep, szLibraryName);
    /* Reserve only a newly created folder; never write into an existing copy. */
    if (!FLibraryNameAvailable(szLibraryRoot, szLibraryName) || CreateNativeDir(szLibraryPending) != 0) {
        szLibraryPending[0] = 0;
        goto LFail;
    }
    CchSnprintf(pszFile, 256, "%s%s%s.xy", szLibraryPending, szDirSep, szLibraryName);
    return TRUE;
LFail:
    strcpy(szMessage, PszGetCompressedString(idsLibraryStorageFailed));
    MessageBox(hwnd, szMessage, PszGetCompressedString(idsLibraryGames), MB_OK | MB_ICONHAND);
    return FALSE;
}

static int16_t FChooseLibraryRaceName(HWND hwnd, const char *pszSuggested, char *pszPath, size_t cchPath) {
    char szMessage[256];
    if (!FInitRaceLibrary()) return FALSE;
    CchSnprintf(szRaceLibraryName, sizeof(szRaceLibraryName), "%s", pszSuggested);
    SuggestRaceFilename(szRaceLibraryName);
    if (!szRaceLibraryName[0]) return FALSE;
    fRaceLibraryNameDialog = TRUE;
    if (DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_LIBRARY_NAME), hwnd, LibraryNameDlg,
                       (LPARAM)szRaceLibraryName) != TRUE) {
        fRaceLibraryNameDialog = FALSE;
        return FALSE;
    }
    fRaceLibraryNameDialog = FALSE;
    if (CchSnprintf(pszPath, cchPath, "%s%s%s", szRaceLibraryRoot, szDirSep, szRaceLibraryName) <= 0 ||
        !FRaceLibraryNameAvailable(szRaceLibraryName)) {
        strcpy(szMessage, PszGetCompressedString(idsLibraryStorageFailed));
        MessageBox(hwnd, szMessage, PszGetCompressedString(idsLibraryGames), MB_OK | MB_ICONHAND);
        return FALSE;
    }
    return TRUE;
}

static int16_t FWriteLibraryMetadata(const char *pszDir, const char *pszName, int iplr) {
    char szFile[256], szTemp[256];
    FILE *pf;
    int fWritten, fClosed;

    CchSnprintf(szFile, sizeof(szFile), "%s%s.library.tmp", pszDir, szDirSep);
    pf = PfOpenNativeFile(szFile, "wb");
    if (!pf) return FALSE;
    fWritten = fprintf(pf, "2\n%s\n%d\n%lld\n%s\n", pszName, iplr, (long long)LNativeTime(),
                        szLibraryExchangeName[0] ? szLibraryExchangeName : pszName) > 0;
    fClosed = fclose(pf) == 0;
    if (!fWritten || !fClosed) { RemoveNativeFile(szFile); return FALSE; }
    CchSnprintf(szFile, sizeof(szFile), "%s%s.library", pszDir, szDirSep);
    /* Publish complete metadata, preserving the previous session on failure. */
    CchSnprintf(szTemp, sizeof(szTemp), "%s%s.library.tmp", pszDir, szDirSep);
    return ReplaceNativeFile(szTemp, szFile) == 0;
}

static int16_t RemovePendingFile(void *lpUnused, const char *pszFile) {
    char szFile[256];
    (void)lpUnused;
    CchSnprintf(szFile, sizeof(szFile), "%s%s%s", szLibraryPending, szDirSep, pszFile);
    /* Failed generation can leave an empty directory at a file destination. */
    RemoveNativePath(szFile);
    return TRUE;
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

typedef struct {
    const char *pszStem;
    const char *pszSource;
    const char *pszTarget;
    const char *pszName;
    int32_t lidGame;
    uint32_t grfPlayers;
    int16_t rgturn[16];
    int16_t fValid;
    int16_t fSawXY;
    int16_t fBad;
    int16_t fCopied;
    int16_t fBackup;
    int16_t fCopy;
} LIBRARYIMPORT;

static int16_t FCopyNativeFile(const char *pszSource, const char *pszTarget) {
    FILE *pfSource, *pfTarget;
    char rgb[8192];
    size_t cb;
    int fOkay;

    pfSource = PfOpenNativeFile(pszSource, "rb");
    if (!pfSource) return FALSE;
    pfTarget = PfOpenNativeFile(pszTarget, "wb");
    if (!pfTarget) { fclose(pfSource); return FALSE; }
    fOkay = TRUE;
    while ((cb = fread(rgb, 1, sizeof(rgb), pfSource)) != 0) {
        if (fwrite(rgb, 1, cb, pfTarget) != cb) { fOkay = FALSE; break; }
    }
    if (ferror(pfSource) || fclose(pfSource) != 0) fOkay = FALSE;
    if (fclose(pfTarget) != 0) fOkay = FALSE;
    if (!fOkay) RemoveNativeFile(pszTarget);
    return fOkay;
}

int16_t FSaveLibraryRace(HWND hwnd, const char *pszName, PLAYER *pplr) {
    char szPath[512];
    char szTempDir[512];
    uint16_t icksum;
    int16_t fOkay;
    if (!FInitRaceLibrary() || !FPortableRaceFilename(pszName)) return FALSE;
    CchSnprintf(szPath, sizeof(szPath), "%s%s%s", szRaceLibraryRoot, szDirSep, pszName);
    if (!FCreateRaceStagePath(szTempDir, sizeof(szTempDir), szRaceLibraryTemp, sizeof(szRaceLibraryTemp))) return FALSE;
    if (!FCreateFile(dtRace, iplrNone, szRaceLibraryTemp)) {
        RemoveNativeFile(szRaceLibraryTemp);
        RemoveNativePath(szTempDir);
        return FALSE;
    }
    WriteRtPlr(pplr, NULL);
    icksum = IRaceChecksum(pplr);
    WriteRt(rtEOF, 2, &icksum);
    StreamClose();
    fOkay = ReplaceNativeFile(szRaceLibraryTemp, szPath) == 0;
    if (!fOkay) {
        RemoveNativeFile(szRaceLibraryTemp);
        RemoveNativePath(szTempDir);
        if (IsWindow(hwnd)) AlertSz(PszFormatIds(idsStarsUnableSaveRaceDataFilePlease, NULL), MB_ICONHAND);
        return FALSE;
    }
    RemoveNativePath(szTempDir);
    CchSnprintf(szRaceLibraryPath, sizeof(szRaceLibraryPath), "%s", szPath);
    SetSzRaceFile(pszName);
    return TRUE;
}

int16_t FSaveCurrentLibraryRace(HWND hwnd, PLAYER *pplr) {
    const char *pszName;
    if (!szRaceLibraryPath[0]) return FALSE;
    pszName = strrchr(szRaceLibraryPath, chDirSep);
    return FSaveLibraryRace(hwnd, pszName ? pszName + 1 : szRaceLibraryPath, pplr);
}

int16_t FHasCurrentLibraryRace(void) {
    return szRaceLibraryPath[0] != 0;
}

int16_t FLoadLibraryRace(HWND hwnd, const char *pszName) {
    char szPath[512];
    RACESTATE rsSav;
    int16_t fResult;
    if (!FInitRaceLibrary() || !FPortableRaceFilename(pszName)) return FALSE;
    CchSnprintf(szPath, sizeof(szPath), "%s%s%s", szRaceLibraryRoot, szDirSep, pszName);
    SaveRaceState(&rsSav);
    fResult = FWasRaceFile(szPath, TRUE);
    if (!fResult) {
        RestoreRaceState(&rsSav, TRUE, TRUE);
        if (IsWindow(hwnd)) AlertSz("The selected race design could not be opened.", MB_ICONEXCLAMATION);
        return FALSE;
    }
    CchSnprintf(szRaceLibraryPath, sizeof(szRaceLibraryPath), "%s", szPath);
    SetSzRaceFile(pszName);
    return TRUE;
}

int16_t FBeginLibraryRace(HWND hwnd) {
    char szPath[512];
    RACESTATE rsSav;
    int16_t fResult;
    SaveRaceState(&rsSav);
    if (!FChooseLibraryRaceName(hwnd, "Custom Race.r1", szPath, sizeof(szPath))) return FALSE;
    CchSnprintf(szRaceLibraryPath, sizeof(szRaceLibraryPath), "%s", szPath);
    vplr = vrgplrDef[0];
    strcpy(vplr.szName, "Humanoid");
    strcpy(vplr.szNames, "Humanoids");
    szRacePass[0] = 0;
    SetSzRaceFile(strrchr(szPath, chDirSep) + 1);
    fResult = RaceCreationWizard(hwnd, FALSE, FALSE);
    if (!fResult) {
        RestoreRaceState(&rsSav, TRUE, TRUE);
    }
    else RestoreRaceState(&rsSav, FALSE, TRUE);
    return fResult;
}

int16_t FEditLibraryRace(HWND hwnd, const char *pszName) {
    RACESTATE rsSav;
    int16_t fResult;
    SaveRaceState(&rsSav);
    if (!FLoadLibraryRace(hwnd, pszName)) return FALSE;
    fResult = RaceCreationWizard(hwnd, FALSE, FALSE);
    RestoreRaceState(&rsSav, !fResult, TRUE);
    return fResult;
}

int16_t FDuplicateLibraryRace(HWND hwnd, const char *pszName) {
    char szSource[512], szName[256], szPath[512] = "";
    const char *pch;
    RACESTATE rsSav;
    if (!FInitRaceLibrary() || !FPortableRaceFilename(pszName)) return FALSE;
    CchSnprintf(szSource, sizeof(szSource), "%s%s%s", szRaceLibraryRoot, szDirSep, pszName);
    SaveRaceState(&rsSav);
    if (!FWasRaceFile(szSource, TRUE)) {
        RestoreRaceState(&rsSav, TRUE, TRUE);
        return FALSE;
    }
    CchSnprintf(szName, sizeof(szName), "%s", pszName);
    pch = strrchr(szName, '.');
    if (pch) szName[pch - szName] = 0;
    SuggestRaceFilename(szName);
    if (!szName[0] || CchSnprintf(szPath, sizeof(szPath), "%s%s%s", szRaceLibraryRoot, szDirSep, szName) <= 0) {
        RestoreRaceState(&rsSav, TRUE, TRUE);
        if (IsWindow(hwnd)) AlertSz("The selected race design could not be duplicated.", MB_ICONEXCLAMATION);
        return FALSE;
    }
    if (!FCopyNativeFile(szSource, szPath)) {
        RestoreRaceState(&rsSav, TRUE, TRUE);
        if (IsWindow(hwnd)) AlertSz("The selected race design could not be duplicated.", MB_ICONEXCLAMATION);
        return FALSE;
    }
    if (!FWasRaceFile(szPath, TRUE)) {
        RemoveNativeFile(szPath);
        RestoreRaceState(&rsSav, TRUE, TRUE);
        if (IsWindow(hwnd)) AlertSz("The selected race design could not be duplicated.", MB_ICONEXCLAMATION);
        return FALSE;
    }
    RestoreRaceState(&rsSav, TRUE, TRUE);
    return TRUE;
}

int16_t FExportLibraryRace(HWND hwnd, const char *pszName, const char *pszTarget) {
    OPENFILENAME ofn;
    char szSource[512], szTarget[512], szValidate[512];
    RACESTATE rsSav;
    if (!FInitRaceLibrary() || !FPortableRaceFilename(pszName)) return FALSE;
    CchSnprintf(szSource, sizeof(szSource), "%s%s%s", szRaceLibraryRoot, szDirSep, pszName);
    if (!pszTarget) {
        memset(&ofn, 0, sizeof(ofn));
        memset(szTarget, 0, sizeof(szTarget));
        CchSnprintf(szTarget, sizeof(szTarget), "%s", pszName);
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = hwnd;
        ofn.lpstrFilter = "Race designs (*.r1;*.r2;*.r3)\0*.r1;*.r2;*.r3\0All files (*.*)\0*.*\0\0";
        ofn.lpstrFile = szTarget;
        ofn.nMaxFile = sizeof(szTarget);
        ofn.Flags = OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
        if (!FNativeSaveFileName(&ofn)) return FALSE;
        pszTarget = szTarget;
    }
    SaveRaceState(&rsSav);
    if (!FWasRaceFile(szSource, TRUE) || SDL_strcasecmp(szSource, pszTarget) == 0) {
        RestoreRaceState(&rsSav, TRUE, TRUE);
        if (IsWindow(hwnd)) AlertSz("The race design could not be exported.", MB_ICONEXCLAMATION);
        return FALSE;
    }
    if (!FCopyNativeFile(szSource, pszTarget)) {
        RestoreRaceState(&rsSav, TRUE, TRUE);
        if (IsWindow(hwnd)) AlertSz("The race design could not be exported.", MB_ICONEXCLAMATION);
        return FALSE;
    }
    CchSnprintf(szValidate, sizeof(szValidate), "%s", pszTarget);
    if (!FWasRaceFile(szValidate, TRUE)) {
        RemoveNativeFile(pszTarget);
        RestoreRaceState(&rsSav, TRUE, TRUE);
        if (IsWindow(hwnd)) AlertSz("The race design could not be exported.", MB_ICONEXCLAMATION);
        return FALSE;
    }
    RestoreRaceState(&rsSav, TRUE, TRUE);
    return TRUE;
}

int16_t FDeleteLibraryRace(HWND hwnd, const char *pszName) {
    char szPath[512], szMessage[320];
    int64_t cbFile;
    if (!FInitRaceLibrary() || !FPortableRaceFilename(pszName)) return FALSE;
    CchSnprintf(szPath, sizeof(szPath), "%s%s%s", szRaceLibraryRoot, szDirSep, pszName);
    if (!FNativeFileSize(szPath, &cbFile) || cbFile <= 0) return FALSE;
    CchSnprintf(szMessage, sizeof(szMessage), "Delete the managed race design %s?", pszName);
    if (MessageBox(hwnd, szMessage, "Stars!", MB_YESNO | MB_ICONQUESTION | MB_TASKMODAL) != IDYES) return FALSE;
    if (RemoveNativeFile(szPath) != 0) {
        AlertSz("The selected race design could not be deleted.", MB_ICONHAND);
        return FALSE;
    }
    if (!strcmp(szRaceLibraryPath, szPath)) szRaceLibraryPath[0] = 0;
    return TRUE;
}

int16_t FImportLibraryRace(HWND hwnd, const char *pszSource) {
    OPENFILENAME ofn;
    char szSource[512], szName[256], szStem[256], szPath[512], szStage[512], szStageDir[512];
    char *pch, *pchSlash, *pchExt;
    RACESTATE rsSav;
    int16_t fValid;
    if (!pszSource) {
        memset(&ofn, 0, sizeof(ofn));
        memset(szSource, 0, sizeof(szSource));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = hwnd;
        ofn.lpstrFilter = "Race designs (*.r1;*.r2;*.r3)\0*.r1;*.r2;*.r3\0All files (*.*)\0*.*\0\0";
        ofn.nFilterIndex = 1;
        ofn.lpstrFile = szSource;
        ofn.nMaxFile = sizeof(szSource);
        ofn.Flags = OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
        if (!FNativeOpenFileName(&ofn)) return FALSE;
        pszSource = szSource;
    }
    if (strlen(pszSource) >= sizeof(szSource)) return FALSE;
    if (pszSource != szSource) strcpy(szSource, pszSource);
    SaveRaceState(&rsSav);
    fValid = FWasRaceFile(szSource, TRUE);
    if (!fValid) {
        RestoreRaceState(&rsSav, TRUE, TRUE);
        return FALSE;
    }
    pchSlash = strrchr(szSource, '/');
    pch = strrchr(szSource, '\\');
    if (pch && (!pchSlash || pch > pchSlash)) pchSlash = pch;
    pch = pchSlash ? pchSlash + 1 : szSource;
    CchSnprintf(szStem, sizeof(szStem), "%s", pch);
    pchExt = strrchr(szStem, '.');
    if (pchExt) *pchExt = 0;
    CchSnprintf(szName, sizeof(szName), "%s.r1", szStem);
    if (pszRaceLibraryNameOverride) {
        if (!FInitRaceLibrary() || !FPortableRaceFilename(pszRaceLibraryNameOverride) ||
            !FRaceLibraryNameAvailable(pszRaceLibraryNameOverride)) goto LRestore;
        CchSnprintf(szRaceLibraryName, sizeof(szRaceLibraryName), "%s", pszRaceLibraryNameOverride);
        CchSnprintf(szPath, sizeof(szPath), "%s%s%s", szRaceLibraryRoot, szDirSep, szRaceLibraryName);
    } else if (!FChooseLibraryRaceName(hwnd, szName, szPath, sizeof(szPath))) goto LRestore;
    if (!FCreateRaceStagePath(szStageDir, sizeof(szStageDir), szStage, sizeof(szStage))) goto LRestore;
    if (!FCopyNativeFile(szSource, szStage) || ReplaceNativeFile(szStage, szPath) != 0) {
        RemoveNativeFile(szStage);
        RemoveNativePath(szStageDir);
        goto LRestore;
    }
    RemoveNativePath(szStageDir);
    if (!FWasRaceFile(szPath, TRUE)) {
        RemoveNativeFile(szPath);
        goto LRestore;
    }
    CchSnprintf(szRaceLibraryPath, sizeof(szRaceLibraryPath), "%s", szPath);
    SetSzRaceFile(strrchr(szPath, chDirSep) + 1);
    return TRUE;
LRestore:
    RestoreRaceState(&rsSav, TRUE, TRUE);
    return FALSE;
}

int16_t FImportLibraryRaceAs(HWND hwnd, const char *pszSource, const char *pszName) {
    int16_t fResult;
    pszRaceLibraryNameOverride = pszName;
    fResult = FImportLibraryRace(hwnd, pszSource);
    pszRaceLibraryNameOverride = NULL;
    return fResult;
}

int16_t FImportLibraryRaceAndEdit(HWND hwnd, const char *pszSource) {
    const char *pszName;
    if (!FImportLibraryRace(hwnd, pszSource)) return FALSE;
    pszName = strrchr(szRaceLibraryPath, chDirSep);
    if (pszName) pszName++;
    else pszName = szRaceLibraryPath;
    /* Import remains successful when the user cancels customization. */
    FEditLibraryRace(hwnd, pszName);
    return TRUE;
}

/* Inspect record framing and BOF metadata without calling FOpenFile, which
 * binds validation to the active game globals. */
static int16_t FInspectImportFile(const char *pszFile, DtFileType dt, int iplr,
                                 int32_t *plidGame, uint16_t *pturn, int16_t *pfMulti) {
    FILE *pf;
    HDR hdr;
    RTBOF rtbof;
    int64_t cbFile;
    long ib;
    int16_t fComplete;

    pf = PfOpenNativeFile(pszFile, "rb");
    if (!pf) return FALSE;
    fComplete = FALSE;
    if (fseek(pf, 0, SEEK_END) != 0 || (cbFile = ftell(pf)) < 22 || fseek(pf, 0, SEEK_SET) != 0) goto LDone;
    if (fread(&hdr, 2, 1, pf) != 1 || hdr.rt != rtBOF || hdr.cb != sizeof(RTBOF) ||
        fread(&rtbof, sizeof(rtbof), 1, pf) != 1 || memcmp(rtbof.rgid, "J3J3", 4) ||
        rtbof.verMajor != 2 || rtbof.verMinor != 85 || rtbof.dt != dt ||
        rtbof.iPlayer != iplr || rtbof.lidGame == 0) goto LDone;
    if (dt == dtXY) {
        uint32_t cPlanets;
        if (cbFile < 88 || (cbFile - 88) % 4 != 0 || (cPlanets = (uint32_t)((cbFile - 88) / 4)) > 999 ||
            fseek(pf, 18, SEEK_SET) != 0 || fread(&hdr, 2, 1, pf) != 1 || hdr.rt != rtGame || hdr.cb != 64 ||
            fseek(pf, -4, SEEK_END) != 0 || fread(&hdr, 2, 1, pf) != 1 || hdr.rt != rtEOF || hdr.cb != 2) goto LDone;
        fComplete = TRUE;
        *plidGame = rtbof.lidGame;
        *pturn = rtbof.turn;
        *pfMulti = rtbof.fMulti;
        goto LDone;
    }
    while (fread(&hdr, 2, 1, pf) == 1) {
        ib = ftell(pf);
        if (ib < 0 || ib + hdr.cb > cbFile) break;
        if (hdr.rt == rtEOF) {
            if (hdr.cb != 2 || ib + hdr.cb != cbFile) break;
            fComplete = TRUE;
            break;
        }
        if (fseek(pf, hdr.cb, SEEK_CUR) != 0) break;
    }
    if (fComplete) {
        *plidGame = rtbof.lidGame;
        *pturn = rtbof.turn;
        *pfMulti = rtbof.fMulti;
        if (rtbof.fMulti) {
            uint16_t turnFinal;
            if (cbFile < 4 || fseek(pf, -2, SEEK_END) != 0 || fread(&turnFinal, 2, 1, pf) != 1) fComplete = FALSE;
            else *pturn = turnFinal;
        }
    }
LDone:
    fclose(pf);
    return fComplete;
}

static int16_t FImportFamilyMember(const char *pszName, const char *pszStem, int *piplr, DtFileType *pdt) {
    size_t cch = strlen(pszStem);
    const char *pszExt;
    int iPlayer;
    if (SDL_strncasecmp(pszName, pszStem, cch) || pszName[cch] != '.') return FALSE;
    pszExt = pszName + cch + 1;
    if (!SDL_strcasecmp(pszExt, "xy")) { *piplr = -1; *pdt = dtXY; return TRUE; }
    if (!SDL_strcasecmp(pszExt, "hst")) { *piplr = -1; *pdt = dtHost; return TRUE; }
    if (pszExt[0] == 'm' || pszExt[0] == 'M' || pszExt[0] == 'h' || pszExt[0] == 'H' || pszExt[0] == 'x' || pszExt[0] == 'X') {
        if (pszExt[1] < '1' || pszExt[1] > '9' || (pszExt[2] && (pszExt[2] < '0' || pszExt[2] > '9')) || (pszExt[2] && pszExt[3])) return FALSE;
        iPlayer = pszExt[1] - '0';
        if (pszExt[2]) iPlayer = iPlayer * 10 + pszExt[2] - '0';
        if (iPlayer > 16) return FALSE;
        *piplr = iPlayer - 1;
        *pdt = SDL_tolower(pszExt[0]) == 'm' ? dtTurn : SDL_tolower(pszExt[0]) == 'h' ? dtHist : dtLog;
        return TRUE;
    }
    return FALSE;
}

static int16_t CopyImportEntry(void *lpData, const char *pszName) {
    LIBRARYIMPORT *pli = lpData;
    char szSource[512], szTarget[512];
    int iplr;
    DtFileType dt;
    int32_t lidGame;
    uint16_t turn;
    int16_t fMulti;

    if (!FImportFamilyMember(pszName, pli->pszStem, &iplr, &dt)) {
        size_t cch = strlen(pli->pszStem);
        if (!SDL_strncasecmp(pszName, pli->pszStem, cch) && pszName[cch] == '.' &&
            (SDL_tolower(pszName[cch + 1]) == 'm' || SDL_tolower(pszName[cch + 1]) == 'h' || SDL_tolower(pszName[cch + 1]) == 'x')) pli->fBad = TRUE;
        if (!strncmp(pszName, "backup.", 7) && strlen(pszName) == 10) pli->fBackup = TRUE;
        return TRUE;
    }
    CchSnprintf(szSource, sizeof(szSource), "%s%s%s", pli->pszSource, szDirSep, pszName);
    FormatLibraryFamilyPath(szTarget, sizeof(szTarget), pli->pszTarget, pli->pszName, dt, iplr);
    if (pli->fCopy) {
        if (!FCopyNativeFile(szSource, szTarget)) { pli->fBad = TRUE; return FALSE; }
        return TRUE;
    }
    if (!FInspectImportFile(szSource, dt, iplr < 0 ? iplrNone : iplr, &lidGame, &turn, &fMulti)) {
        pli->fBad = TRUE;
        return TRUE;
    }
    if (dt == dtXY) {
        if (pli->fSawXY) { pli->fBad = TRUE; return TRUE; }
        pli->fSawXY = TRUE;
        if (pli->lidGame && pli->lidGame != lidGame) { pli->fBad = TRUE; return TRUE; }
        pli->lidGame = lidGame;
        pli->fValid = TRUE;
    } else if (dt == dtTurn) {
        if ((pli->lidGame && lidGame != pli->lidGame) || iplr >= 16 || (pli->grfPlayers & (1U << iplr))) { pli->fBad = TRUE; return TRUE; }
        if (!pli->lidGame) pli->lidGame = lidGame;
        pli->grfPlayers |= 1U << iplr;
        pli->rgturn[iplr] = (int16_t)turn;
    } else if (dt == dtHost || dt == dtHist || dt == dtLog) {
        if (pli->lidGame && lidGame != pli->lidGame) { pli->fBad = TRUE; return TRUE; }
        if (!pli->lidGame) pli->lidGame = lidGame;
    }
    /* The first pass validates every recognized file; copying happens only
     * after the entire family has passed validation. */
    pli->fCopied++;
    return TRUE;
}

typedef struct {
    const char *pszSource;
    const char *pszTarget;
    const char *pszStem;
    const char *pszName;
    const char *pszBackup;
    int16_t fBad;
} LIBRARYBACKUP;

static int16_t CopyBackupEntry(void *lpData, const char *pszName) {
    LIBRARYBACKUP *plb = lpData;
    int iplr;
    DtFileType dt;
    char szSource[512], szTarget[512], szDirectory[512];
    if (!FImportFamilyMember(pszName, plb->pszStem, &iplr, &dt)) return TRUE;
    CchSnprintf(szSource, sizeof(szSource), "%s%s%s%s%s", plb->pszSource, szDirSep, plb->pszBackup, szDirSep, pszName);
    CchSnprintf(szDirectory, sizeof(szDirectory), "%s%s%s", plb->pszTarget, szDirSep, plb->pszBackup);
    FormatLibraryFamilyPath(szTarget, sizeof(szTarget), szDirectory, plb->pszName, dt, iplr);
    if (!FCopyNativeFile(szSource, szTarget)) { plb->fBad = TRUE; return FALSE; }
    return TRUE;
}

static int16_t CopyBackupDirectory(void *lpData, const char *pszName) {
    LIBRARYIMPORT *pli = lpData;
    LIBRARYBACKUP lb;
    char szSource[512], szTarget[512];
    size_t cch = strlen(pszName);
    if (strncmp(pszName, "backup.", 7) || cch != 10 ||
        pszName[7] < '0' || pszName[7] > '9' || pszName[8] < '0' || pszName[8] > '9' ||
        pszName[9] < '0' || pszName[9] > '9') return TRUE;
    CchSnprintf(szSource, sizeof(szSource), "%s%s%s", pli->pszSource, szDirSep, pszName);
    if (!FNativeDirectoryExists(szSource)) return TRUE;
    CchSnprintf(szTarget, sizeof(szTarget), "%s%s%s", pli->pszTarget, szDirSep, pszName);
    if (CreateNativeDir(szTarget) != 0) { pli->fBad = TRUE; return FALSE; }
    lb.pszSource = pli->pszSource;
    lb.pszTarget = pli->pszTarget;
    lb.pszStem = pli->pszStem;
    lb.pszName = pli->pszName;
    lb.pszBackup = pszName;
    lb.fBad = FALSE;
    if (!FNativeEnumerateDirectory(szSource, CopyBackupEntry, &lb) || lb.fBad) { pli->fBad = TRUE; return FALSE; }
    return TRUE;
}

static int16_t RemoveImportChild(void *lpData, const char *pszFile) {
    char szFile[512];
    CchSnprintf(szFile, sizeof(szFile), "%s%s%s", (char *)lpData, szDirSep, pszFile);
    RemoveNativePath(szFile);
    return TRUE;
}

static int16_t RemoveImportFile(void *lpData, const char *pszFile) {
    char szFile[512];
    const char *pszRoot = lpData;
    CchSnprintf(szFile, sizeof(szFile), "%s%s%s", pszRoot, szDirSep, pszFile);
    if (FNativeDirectoryExists(szFile)) {
        FNativeEnumerateDirectory(szFile, RemoveImportChild, szFile);
        RemoveNativePath(szFile);
    } else RemoveNativePath(szFile);
    return TRUE;
}

static int16_t FindImportUniverse(void *lpData, const char *pszName) {
    char *pszStem = lpData;
    const char *pch = strrchr(pszName, '.');
    if (!pch || strcmp(pch, ".xy")) return TRUE;
    if (pszStem[0]) { pszStem[0] = '!'; return TRUE; }
    if ((size_t)(pch - pszName) >= 256) { pszStem[0] = '!'; return TRUE; }
    memcpy(pszStem, pszName, pch - pszName);
    pszStem[pch - pszName] = 0;
    return TRUE;
}

static void SDLCALL LibraryImportFolderSelected(void *lpData, const char * const *rgpszPath, int iFilter) {
    HWND hwnd = lpData;
    char *pszPath;
    (void)iFilter;
    if (!rgpszPath || !rgpszPath[0]) return;
    pszPath = SDL_strdup(rgpszPath[0]);
    if (!pszPath || !StarsCompatPostMessageAsync(hwnd, WM_STARS_IMPORT_FOLDER, (WPARAM)pszPath, 0)) SDL_free(pszPath);
}

void ShowLibraryImportFolderPicker(HWND hwnd) {
    SDL_ShowOpenFolderDialog(LibraryImportFolderSelected, hwnd, NULL, NULL, FALSE);
}

int16_t FImportLibraryGame(HWND hwnd, const char *pszSource) {
    OPENFILENAME ofn;
    char szChosen[512], szTitle[256], szFilter[128];
    char szSource[512], szDir[512], szStem[256], szStage[256], *pch, *pchSlash;
    LIBRARYIMPORT li;
    int cchDir;

    if (!pszSource) {
        memset(&ofn, 0, sizeof(ofn));
        memset(szChosen, 0, sizeof(szChosen));
        CchGetString(idsStarsGameFilesMHstRStars, szFilter);
        pch = strstr(szFilter, "*.M*;");
        if (pch) {
            memmove(pch + 5, pch, strlen(pch) + 1);
            memcpy(pch, "*.XY;", 5);
        }
        for (pch = szFilter; *pch; pch++) if (*pch == '|') *pch = 0;
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = hwnd;
        ofn.lpstrFilter = szFilter;
        ofn.nFilterIndex = 1;
        ofn.lpstrFile = szChosen;
        ofn.nMaxFile = sizeof(szChosen);
        szTitle[0] = 0;
        ofn.lpstrFileTitle = szTitle;
        ofn.nMaxFileTitle = sizeof(szTitle);
        ofn.Flags = OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
        if (!FNativeOpenFileName(&ofn)) return FALSE;
        pszSource = szChosen;
    }
    if (strlen(pszSource) >= sizeof(szSource)) return FALSE;
    strcpy(szSource, pszSource);
    if (FNativeDirectoryExists(szSource)) {
        strcpy(szDir, szSource);
        szStem[0] = 0;
        FNativeEnumerateDirectory(szDir, FindImportUniverse, szStem);
        if (!szStem[0] || szStem[0] == '!') {
            AlertSz("Choose a folder containing exactly one game universe file (.xy).", MB_ICONEXCLAMATION);
            return FALSE;
        }
        CchSnprintf(szSource, sizeof(szSource), "%s%s%s.xy", szDir, szDirSep, szStem);
        pchSlash = strrchr(szSource, chDirSep);
    } else {
    pchSlash = strrchr(szSource, '/');
    pch = strrchr(szSource, '\\');
    if (pch && (!pchSlash || pch > pchSlash)) pchSlash = pch;
    if (!pchSlash) return FALSE;
    cchDir = (int)(pchSlash - szSource);
    if (cchDir == 0) cchDir = 1;
    memcpy(szDir, szSource, cchDir);
    szDir[cchDir] = 0;
    pch = strrchr(pchSlash + 1, '.');
    if (!pch || pch == pchSlash + 1 || (size_t)(pch - (pchSlash + 1)) >= sizeof(szStem)) return FALSE;
    memcpy(szStem, pchSlash + 1, pch - (pchSlash + 1));
    szStem[pch - (pchSlash + 1)] = 0;
    }
    if (!FInitLibrary()) return FALSE;
    memset(&li, 0, sizeof(li));
    li.pszStem = szStem;
    li.pszSource = szDir;
    li.fValid = FALSE;
    if (!FNativeEnumerateDirectory(szDir, CopyImportEntry, &li) || li.fBad || !li.fSawXY || !li.fValid || li.grfPlayers == 0) {
        AlertSz("This folder does not contain a complete supported player game.", MB_ICONEXCLAMATION);
        return FALSE;
    }
    if (!FChooseLibraryGameName(hwnd, szSource)) return FALSE;
    strcpy(szLibraryExchangeName, szStem);
    CchSnprintf(szStage, sizeof(szStage), "%s%s.importing.%lld", szLibraryRoot, szDirSep, (long long)LNativeTime());
    if (CreateNativeDir(szStage) != 0) {
        RemoveNativePath(szLibraryPending);
        szLibraryPending[0] = 0;
        return FALSE;
    }
    li.pszTarget = szStage;
    li.pszName = szLibraryName;
    li.fCopy = TRUE;
    if (!FNativeEnumerateDirectory(szDir, CopyImportEntry, &li) || li.fBad ||
        !FNativeEnumerateDirectory(szDir, CopyBackupDirectory, &li) || li.fBad ||
        !FWriteLibraryMetadata(szStage, szLibraryName, (int16_t)__builtin_ctz(li.grfPlayers)) ||
        RemoveNativePath(szLibraryPending) != 0 || RenameNativeFile(szStage, szLibraryPending) != 0) {
        FNativeEnumerateDirectory(szStage, RemoveImportFile, szStage);
        RemoveNativePath(szStage);
        RemoveNativePath(szLibraryPending);
        szLibraryPending[0] = 0;
        AlertSz("The game could not be copied into the library.", MB_ICONHAND);
        return FALSE;
    }
    szLibraryPending[0] = 0;
    return TRUE;
}


int16_t FFinishLibraryGame(HWND hwnd, int16_t fGenerated) {
    char szFile[256], szMessage[256];
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
        FNativeEnumerateDirectory(szLibraryPending, RemovePendingFile, NULL);
        RemoveNativePath(szLibraryPending);
        strcpy(szMessage, PszGetCompressedString(idsLibraryStorageFailed));
        MessageBox(IsWindow(hwnd) ? hwnd : hwndFrame, szMessage, PszGetCompressedString(idsLibraryGames), MB_OK | MB_ICONHAND);
    }
    szLibraryPending[0] = 0;
    return fGenerated;
}

static int16_t ListLibraryGame(void *lpList, const char *pszName) {
    char szFile[256], szMetadataName[256], szDisplay[320];
    FILE *pf;
    int iVersion, iplr, fRead, iPlayer, cPlayers;
    int64_t cbFile;
    long long lPlayed;
    int32_t lidGame;
    uint16_t turn;
    int16_t fMulti;
    LRESULT i;

    if (!FPortableLibraryName(pszName)) return TRUE;
    CchSnprintf(szFile, sizeof(szFile), "%s%s%s%s.library", szLibraryRoot, szDirSep, pszName, szDirSep);
    pf = PfOpenNativeFile(szFile, "rb");
    if (!pf) return TRUE;
    iVersion = 0; iplr = -1;
    fRead = fscanf(pf, "%d\n", &iVersion) == 1 && fgets(szMetadataName, sizeof(szMetadataName), pf) &&
            fscanf(pf, "%d\n%lld", &iplr, &lPlayed) == 2;
    fclose(pf);
    if (!fRead || (iVersion != 1 && iVersion != 2) || iplr < -1 || iplr >= 16) return TRUE;
    CchSnprintf(szFile, sizeof(szFile), "%s%s%s%s%s.xy", szLibraryRoot, szDirSep, pszName, szDirSep, pszName);
    if (!FNativeFileSize(szFile, &cbFile) || cbFile == 0) return TRUE;
    cPlayers = 0;
    for (iPlayer = 0; iPlayer < 16; iPlayer++) {
        CchSnprintf(szFile, sizeof(szFile), "%s%s%s%s%s.m%d", szLibraryRoot, szDirSep, pszName, szDirSep, pszName, iPlayer + 1);
        if (!FNativeFileSize(szFile, &cbFile) || cbFile <= 0 ||
            !FInspectImportFile(szFile, dtTurn, iPlayer, &lidGame, &turn, &fMulti)) continue;
        CchSnprintf(szDisplay, sizeof(szDisplay), "%s - Player %d (%u)", pszName, iPlayer + 1, turn + 2400);
        i = SendMessage(lpList, LB_ADDSTRING, 0, (LPARAM)szDisplay);
        if (i != LB_ERR && i != LB_ERRSPACE) SendMessage(lpList, LB_SETITEMDATA, i, iPlayer);
        cPlayers++;
    }
    CchSnprintf(szFile, sizeof(szFile), "%s%s%s%s%s.hst", szLibraryRoot, szDirSep, pszName, szDirSep, pszName);
    if (cPlayers == 0 && FNativeFileSize(szFile, &cbFile) && cbFile > 0) {
        CchSnprintf(szDisplay, sizeof(szDisplay), "%s - Host game", pszName);
        i = SendMessage(lpList, LB_ADDSTRING, 0, (LPARAM)szDisplay);
        if (i != LB_ERR && i != LB_ERRSPACE) SendMessage(lpList, LB_SETITEMDATA, i, iplrNone);
    }
    return TRUE;
}

void InitLibraryGames(HWND hwnd, const RECT *lprc) {
    int y;
    Uint64 msStart = SDL_GetTicks();

    if (getenv("STARS_DEBUG_LIBRARY_STARTUP"))
        fprintf(stderr, "[DEBUG-library-startup] InitLibraryGames begin hwnd=%p\n", (void *)hwnd);
    y = max(80, lprc->bottom - 330);
    CreateWindow("STATIC", PszGetCompressedString(idsLibraryGames), WS_CHILD | WS_VISIBLE, 24, y - 20, 200, 20,
                 hwnd, NULL, hInst, NULL);
    CreateWindow("BUTTON", "Import Game...", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                 lprc->right - 128, y - 22, 104, 18, hwnd,
                 (HMENU)(uintptr_t)IDC_LIBRARY_IMPORT, hInst, NULL);
    CreateWindow("BUTTON", "Import Folder...", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                 lprc->right - 240, y - 22, 104, 18, hwnd,
                 (HMENU)(uintptr_t)IDC_LIBRARY_IMPORT_FOLDER, hInst, NULL);
    CreateWindow("LISTBOX", "", WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | WS_VSCROLL | LBS_NOTIFY | LBS_SORT,
                 24, y, lprc->right - 48, 90, hwnd, (HMENU)(uintptr_t)IDC_LIBRARY_GAMES, hInst, NULL);
    RefreshLibraryGames(hwnd);
    if (getenv("STARS_DEBUG_LIBRARY_STARTUP"))
        fprintf(stderr, "[DEBUG-library-startup] InitLibraryGames end elapsed=%llu ms\n",
                (unsigned long long)(SDL_GetTicks() - msStart));
}

static int16_t ListLibraryRace(void *lpList, const char *pszName) {
    size_t cch = strlen(pszName);
    char szPath[512];
    int64_t cbFile;
    LRESULT i;
    if (cch < 4 || !FPortableRaceFilename(pszName)) return TRUE;
    CchSnprintf(szPath, sizeof(szPath), "%s%s%s", szRaceLibraryRoot, szDirSep, pszName);
    if (!FNativeFileSize(szPath, &cbFile) || cbFile <= 0) return TRUE;
    i = SendMessage(lpList, LB_ADDSTRING, 0, (LPARAM)pszName);
    return i != LB_ERR && i != LB_ERRSPACE;
}

void InitLibraryRaces(HWND hwnd, const RECT *lprc) {
    int y = max(160, lprc->bottom - 200);
    CreateWindow("STATIC", "Race Designs", WS_CHILD | WS_VISIBLE, 24, y - 18, 160, 18,
                 hwnd, NULL, hInst, NULL);
    CreateWindow("BUTTON", "New", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                 lprc->right - 436, y - 20, 48, 18, hwnd, (HMENU)(uintptr_t)IDC_LIBRARY_RACE_NEW, hInst, NULL);
    CreateWindow("BUTTON", "Edit", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                 lprc->right - 384, y - 20, 48, 18, hwnd, (HMENU)(uintptr_t)IDC_LIBRARY_RACE_EDIT, hInst, NULL);
    CreateWindow("BUTTON", "Duplicate", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                 lprc->right - 332, y - 20, 62, 18, hwnd, (HMENU)(uintptr_t)IDC_LIBRARY_RACE_DUPLICATE, hInst, NULL);
    CreateWindow("BUTTON", "Export", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                 lprc->right - 266, y - 20, 48, 18, hwnd, (HMENU)(uintptr_t)IDC_LIBRARY_RACE_EXPORT, hInst, NULL);
    CreateWindow("BUTTON", "Delete", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                 lprc->right - 214, y - 20, 56, 18, hwnd, (HMENU)(uintptr_t)IDC_LIBRARY_RACE_DELETE, hInst, NULL);
    CreateWindow("BUTTON", "Import...", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                 lprc->right - 152, y - 20, 112, 18, hwnd, (HMENU)(uintptr_t)IDC_LIBRARY_RACE_IMPORT, hInst, NULL);
    CreateWindow("LISTBOX", "", WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | WS_VSCROLL | LBS_NOTIFY | LBS_SORT,
                 24, y, lprc->right - 48, 80, hwnd, (HMENU)(uintptr_t)IDC_LIBRARY_RACES, hInst, NULL);
    RefreshLibraryRaces(hwnd);
}

void RefreshLibraryRaces(HWND hwnd) {
    HWND hwndList = GetDlgItem(hwnd, IDC_LIBRARY_RACES);
    if (!hwndList) return;
    SendMessage(hwndList, LB_RESETCONTENT, 0, 0);
    if (FInitRaceLibrary()) FNativeEnumerateDirectory(szRaceLibraryRoot, ListLibraryRace, hwndList);
    SendMessage(hwndList, LB_SETCURSEL, 0, 0);
    EnableWindow(GetDlgItem(hwnd, IDC_LIBRARY_RACE_EDIT), SendMessage(hwndList, LB_GETCOUNT, 0, 0) > 0);
    EnableWindow(GetDlgItem(hwnd, IDC_LIBRARY_RACE_DUPLICATE), SendMessage(hwndList, LB_GETCOUNT, 0, 0) > 0);
    EnableWindow(GetDlgItem(hwnd, IDC_LIBRARY_RACE_EXPORT), SendMessage(hwndList, LB_GETCOUNT, 0, 0) > 0);
    EnableWindow(GetDlgItem(hwnd, IDC_LIBRARY_RACE_DELETE), SendMessage(hwndList, LB_GETCOUNT, 0, 0) > 0);
}

int16_t FSelectLibraryRace(HWND hwnd, char *pszName) {
    HWND hwndList = GetDlgItem(hwnd, IDC_LIBRARY_RACES);
    LRESULT i = SendMessage(hwndList, LB_GETCURSEL, 0, 0);
    if (i == LB_ERR) return FALSE;
    SendMessage(hwndList, LB_GETTEXT, i, (LPARAM)pszName);
    return TRUE;
}

void RefreshLibraryGames(HWND hwnd) {
    HWND hwndList;
    int16_t fLibrary;
    int16_t fEnumerated;
    hwndList = GetDlgItem(hwnd, IDC_LIBRARY_GAMES);
    if (!hwndList) return;
    SendMessage(hwndList, LB_RESETCONTENT, 0, 0);
    fLibrary = FInitLibrary();
    if (fLibrary) {
        fEnumerated = FNativeEnumerateDirectory(szLibraryRoot, ListLibraryGame, hwndList);
        (void)fEnumerated;
    }
    SendMessage(hwndList, LB_SETCURSEL, 0, 0);
    EnableWindow(rghwndBtnSplash[1], SendMessage(hwndList, LB_GETCOUNT, 0, 0) > 0);
}

int16_t FSelectLibraryGame(HWND hwnd) {
    HWND hwndList;
    LRESULT i, iplr;
    char szName[256], *pch, *pchLast;

    hwndList = GetDlgItem(hwnd, IDC_LIBRARY_GAMES);
    i = SendMessage(hwndList, LB_GETCURSEL, 0, 0);
    if (i == LB_ERR) return FALSE;
    SendMessage(hwndList, LB_GETTEXT, i, (LPARAM)szName);
    iplr = SendMessage(hwndList, LB_GETITEMDATA, i, 0);
    pchLast = NULL;
    for (pch = strstr(szName, " - Player "); pch; pch = strstr(pch + 1, " - Player ")) pchLast = pch;
    if (!pchLast) for (pch = strstr(szName, " - Host game"); pch; pch = strstr(pch + 1, " - Host game")) pchLast = pch;
    if (pchLast) *pchLast = 0;
    if (iplr >= 0) CchSnprintf(szBase, sizeof(szBase), "%s%s%s%s%s.m%d", szLibraryRoot, szDirSep, szName, szDirSep, szName, (int)iplr + 1);
    else CchSnprintf(szBase, sizeof(szBase), "%s%s%s%s%s.hst", szLibraryRoot, szDirSep, szName, szDirSep, szName);
    ini.fStartupFile = TRUE;
    return TRUE;
}

void RememberLibraryGame(void) {
    char szDir[256];
    char szFile[512], szLine[256];
    char *pch;
    FILE *pf;
    int iVersion;
    int iPlayer;
    long long lPlayed;

    if (!FInitLibrary() || strncmp(szBase, szLibraryRoot, strlen(szLibraryRoot)) || szBase[strlen(szLibraryRoot)] != chDirSep) return;
    strcpy(szDir, szBase);
    pch = strrchr(szDir, chDirSep);
    if (!pch) return;
    *pch = 0;
    pch = strrchr(szDir, chDirSep);
    if (!pch) return;
    szLibraryExchangeName[0] = 0;
    CchSnprintf(szFile, sizeof(szFile), "%s%s%s%s.library", szDir, szDirSep, pch + 1, szDirSep);
    pf = PfOpenNativeFile(szFile, "rb");
    if (pf) {
        if (fscanf(pf, "%d\n", &iVersion) == 1 && fgets(szLine, sizeof(szLine), pf) &&
            fscanf(pf, "%d\n%lld\n", &iPlayer, &lPlayed) == 2) {
            if (iVersion >= 2 && fgets(szLibraryExchangeName, sizeof(szLibraryExchangeName), pf)) {
                szLibraryExchangeName[strcspn(szLibraryExchangeName, "\r\n")] = 0;
            } else szLibraryExchangeName[0] = 0;
        }
        fclose(pf);
    }
    FWriteLibraryMetadata(szDir, pch + 1, idPlayer);
}
