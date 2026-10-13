#include "common.h"

char *PszGetCompressedString(StringId ids) {
    int16_t  iChunk;
    uint8_t *pchLen;
    int16_t  iBuild;
    int16_t  iNibble;
    int16_t  i;
    int16_t  iLen;
    uint8_t *pch;
    char    *pszOut;
    int16_t  iOffset;
    int16_t  fHigh;

    iNibble = 0;
    if (ids == iLastStrGet) {
        return szLastStrGet;
    }
    iChunk = ids >> 6;
    iOffset = ids & 0x3f;
    pch = &aSTRCmpr[aiSTRChunkOffset[iChunk]];
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
    pszOut = szLastStrGet;
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
    return szLastStrGet;
}
