#ifndef STARS_PROFILETEXTIO_H
#define STARS_PROFILETEXTIO_H
#include <windows.h>

struct PROFILETEXTIO {
    decltype(&win32::GetPrivateProfileStringA)   pfnReadString = win32::GetPrivateProfileStringA;
    decltype(&win32::WritePrivateProfileStringA) pfnWriteString = win32::WritePrivateProfileStringA;
};
const PROFILETEXTIO &ProfileTextIO() noexcept;
class PROFILETEXTIOSCOPE {
    PROFILETEXTIO        io;
    const PROFILETEXTIO *lpPrev;

  public:
    explicit PROFILETEXTIOSCOPE(const PROFILETEXTIO &io) noexcept;
    ~PROFILETEXTIOSCOPE() noexcept;
    PROFILETEXTIOSCOPE(const PROFILETEXTIOSCOPE &) = delete;
    PROFILETEXTIOSCOPE &operator=(const PROFILETEXTIOSCOPE &) = delete;
};
#endif
