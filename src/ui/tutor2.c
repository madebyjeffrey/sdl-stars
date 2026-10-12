#include "win.h"

int16_t CchTutorString(char *pchOut, TutorId idt) {
    int16_t  iOffset;
    int16_t  fHigh;
    int16_t  iChunk;
    uint8_t *pchLen;
    int16_t  iBuild;
    int16_t  iNibble;
    int16_t  i;
    char    *pszOut;
    int16_t  iLen;
    uint8_t *pch;

    iNibble = 0;
    if (idt == iLastTutGet) {
        return strlen(pchOut);
    }
    iChunk = idt >> 6;
    iOffset = idt & 0x3f;
    pch = &aTUTCmpr[aiTUTChunkOffset[iChunk]];
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
