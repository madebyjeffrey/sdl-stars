#include "win.h"
#include "uistate.h"

std::int16_t CchTutorString(APPSESSION &app, char *pchOut, TutorId idt) {
    std::int16_t  iOffset;
    std::int16_t  fHigh;
    std::int16_t  iChunk;
    std::uint8_t *pchLen;
    std::int16_t  iBuild;
    std::int16_t  iNibble;
    std::int16_t  i;
    char    *pszOut;
    std::int16_t  iLen;
    std::uint8_t *pch;

    iNibble = 0;
    if (idt == app.lpui->iLastTutGet) {
        return std::strlen(pchOut);
    }
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
    pszOut = pchOut;
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
            *pszOut = rgTUTLookupTable[iBuild];
            pszOut++;
            iBuild = 0;
        }
    }
    *pszOut = 0;
    return pszOut - pchOut;
}
