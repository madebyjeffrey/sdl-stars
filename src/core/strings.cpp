#include "common.h"
#include "appstate.h"

char *PszGetCompressedString(APPSESSION &app, StringId ids) {
    std::int16_t  iChunk;
    std::uint8_t *pchLen;
    std::int16_t  iBuild;
    std::int16_t  iNibble;
    std::int16_t  i;
    std::int16_t  iLen;
    std::uint8_t *pch;
    char    *pszOut;
    std::int16_t  iOffset;
    std::int16_t  fHigh;

    iNibble = 0;
    if (ids == app.state.iLastStrGet) {
        return app.state.szLastStrGet;
    }
    iChunk = ids >> 6;
    iOffset = ids & 0x3f;
    pch = reinterpret_cast<std::uint8_t *>(&aSTRCmpr[aiSTRChunkOffset[iChunk]]);
    pchLen = &acSTR[iChunk * 64];
    i = 0;
    while (i < iOffset) {
        iNibble += *pchLen;
        i++;
        pchLen++;
    }
    pch += iNibble >> 1;
    iLen = *pchLen;
    fHigh = (iNibble & 1) == 0;
    pszOut = app.state.szLastStrGet;
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
            *pszOut = rgSTRLookupTable[iBuild];
            pszOut++;
            iBuild = 0;
        }
    }
    *pszOut = 0;
    return app.state.szLastStrGet;
}
