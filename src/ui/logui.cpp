#include "win.h"
#include "appstate.h"
#include "uistate.h"
#include "text_io.h"
#include "record_io.h"

std::int16_t FWriteTutorialMFile(APPSESSION &app, std::int16_t iTurn) {
    std::span<const std::byte> rgbResource;
    const std::byte *lpb;
    HRSRC    hrsrc;
    char     szT[30];
    HGLOBAL  hres;
    std::int16_t  cch;
    std::int16_t  cSkip;

    cSkip = iTurn;
    StarsRecoveryScope scopeRecovery;
    try {
        if (iTurn < 32) {
            hrsrc = win32::FindResourceA(app.lpui->hInst, win32::MAKEINTRESOURCE(10003), win32::MAKEINTRESOURCE(10002));
        } else {
            hrsrc = win32::FindResourceA(app.lpui->hInst, win32::MAKEINTRESOURCE(10005), win32::MAKEINTRESOURCE(10004));
            cSkip -= 32;
        }
        hres = win32::LoadResource(app.lpui->hInst, hrsrc);
        if (!hres) {
        BailOut:
            return 0;
        }
        lpb = static_cast<const std::byte *>(win32::LockResource(hres));
        if (!lpb) {
            app.state.vlpMemStream.reset();
            goto BailOut;
        }
        rgbResource = {lpb, static_cast<std::size_t>(win32::SizeofResource(app.lpui->hInst, hrsrc))};
        if (rgbResource.empty()) {
            app.state.vlpMemStream.reset();
            goto BailOut;
        }
        app.state.vlpMemStream = rgbResource;
        if (cSkip >= std::to_integer<std::uint8_t>(app.state.vlpMemStream->front())) {
            app.state.vlpMemStream.reset();
            return 2;
        }
        *app.state.vlpMemStream = app.state.vlpMemStream->subspan(1);
        while (cSkip-- != 0) {
            do {
                *app.state.vlpMemStream = app.state.vlpMemStream->subspan(RgMemStreamRecord(app).size());
            } while (HdrMemStream(app).rt != rtBOF);
        }
        cch = TextIO().pfnCchGetString(app, idsTutorial, std::span{szT});
        std::strcpy(&szT[cch], iTurn == 37 ? ".hst" : ".m1");
        StarsFileGuard closeStream(app.state.hf);
        RecordIO().pfnStreamOpen(app, szT, mdCreate);
        do {
            rgbResource = RgMemStreamRecord(app);
            RgToStream(app, rgbResource);
            *app.state.vlpMemStream = app.state.vlpMemStream->subspan(rgbResource.size());
        } while (HdrMemStream(app).rt != rtBOF);
        RecordIO().pfnStreamClose(app);
        app.state.vlpMemStream.reset();
        return 1;
    } catch (const StarsError &) {
        if (app.state.vlpMemStream) {
            app.state.vlpMemStream.reset();
            return 0;
        }
        if (app.state.hf == -1) {
            return 1;
        }
        RecordIO().pfnStreamClose(app);
        return 0;
    }
}

// LpbLoadTutorLog returns the tutorial's recorded orders for the player,
// kept in the program's resources, or an empty span. FLoadLogFile replays them. The
// resource stays loaded for the life of the program.
std::span<const std::byte> LpbLoadTutorLog(APPSESSION &app) {
    HRSRC   hrsrc;
    HGLOBAL hres;
    const std::byte *lpb;

    hrsrc = win32::FindResourceA(app.lpui->hInst, win32::MAKEINTRESOURCE(10001), win32::MAKEINTRESOURCE(10000));
    hres = win32::LoadResource(app.lpui->hInst, hrsrc);
    if (!hres) {
        return {};
    }
    lpb = static_cast<const std::byte *>(win32::LockResource(hres));
    if (!lpb) return {};
    return {lpb, static_cast<std::size_t>(win32::SizeofResource(app.lpui->hInst, hrsrc))};
}
