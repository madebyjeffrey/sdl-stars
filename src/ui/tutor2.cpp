#include "win.h"
#include <algorithm>
#include "uistate.h"

std::optional<std::size_t> CchTutorString(APPSESSION &app, std::span<char> pchOut, TutorId idt) {
    std::int16_t  iOffset;
    std::int16_t  fHigh;
    std::int16_t  iChunk;
    std::uint8_t *pchLen;
    std::int16_t  iBuild;
    std::int16_t  iNibble;
    std::int16_t  i;
    std::int16_t  iLen;
    std::uint8_t *pch;
    std::size_t cchOut;

    iNibble = 0;
    if (idt == app.lpui->iLastTutGet) {
        const auto pchEnd = std::find(pchOut.begin(), pchOut.end(), '\0');
        if (pchEnd == pchOut.end()) return std::nullopt;
        return static_cast<std::size_t>(pchEnd - pchOut.begin());
    }
    if (pchOut.empty()) return std::nullopt;
    iChunk = idt >> 6;
    iOffset = idt & 0x3f;
    pch = reinterpret_cast<std::uint8_t *>(&aTUTCmpr[aiTUTChunkOffset[iChunk]]);
    pchLen = &acTUT[iChunk * 64];
    i = 0;
    while (i < iOffset) {
        iNibble += *pchLen;
        i++;
        pchLen++;
    }
    pch += iNibble >> 1;
    iLen = *pchLen;
    fHigh = (iNibble & 1) == 0;
    cchOut = 0;
    iBuild = 0;
    while (iLen-- != 0) {
        if (fHigh) {
            i = *pch >> 4;
        } else {
            i = *pch++ & 0xf;
        }
        fHigh = fHigh == 0;
        iBuild += i;
        if (i != 15) {
            if (cchOut + 1 >= pchOut.size()) {
                pchOut[cchOut] = 0;
                return std::nullopt;
            }
            pchOut[cchOut++] = rgTUTLookupTable[iBuild];
            iBuild = 0;
        }
    }
    pchOut[cchOut] = 0;
    return cchOut;
}
