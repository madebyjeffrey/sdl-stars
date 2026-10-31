#ifndef STARS_RECORDIO_H
#define STARS_RECORDIO_H
#include "common.h"

// Effects belong to this module. Defaults delegate to the original functions;
// scoped bindings own a snapshot and restore the enclosing thread's binding.
struct RECORDIO {
    decltype(&WriteRt)     pfnWriteRt = WriteRt;
    decltype(&ReadRt)      pfnReadRt = ReadRt;
    decltype(&FCreateFile) pfnFCreateFile = FCreateFile;
    decltype(&StreamOpen)  pfnStreamOpen = StreamOpen;
    decltype(&StreamClose) pfnStreamClose = StreamClose;
    decltype(&DirtyGame)   pfnDirtyGame = DirtyGame;
};
const RECORDIO &RecordIO() noexcept;
class RECORDIOSCOPE {
    RECORDIO        io;
    const RECORDIO *lpPrev;

  public:
    explicit RECORDIOSCOPE(const RECORDIO &io) noexcept;
    ~RECORDIOSCOPE() noexcept;
    RECORDIOSCOPE(const RECORDIOSCOPE &) = delete;
    RECORDIOSCOPE &operator=(const RECORDIOSCOPE &) = delete;
};
#endif
