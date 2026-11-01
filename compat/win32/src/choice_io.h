#ifndef STARS_COMPATCHOICEIO_H
#define STARS_COMPATCHOICEIO_H
#include <windows.h>

// Effects belong to this module. Defaults delegate to the original functions;
// scoped bindings own a snapshot and restore the enclosing thread's binding.
struct COMPATCHOICEIO {
    decltype(&win32::TrackPopupMenu) pfnTrackPopupMenu = win32::TrackPopupMenu;
    decltype(&win32::MessageBoxA)    pfnMessageBoxA = win32::MessageBoxA;
};
const COMPATCHOICEIO &CompatChoiceIO() noexcept;
class COMPATCHOICEIOSCOPE {
    COMPATCHOICEIO        io;
    const COMPATCHOICEIO *lpPrev;

  public:
    explicit COMPATCHOICEIOSCOPE(const COMPATCHOICEIO &io) noexcept;
    ~COMPATCHOICEIOSCOPE() noexcept;
    COMPATCHOICEIOSCOPE(const COMPATCHOICEIOSCOPE &) = delete;
    COMPATCHOICEIOSCOPE &operator=(const COMPATCHOICEIOSCOPE &) = delete;
};
#endif
