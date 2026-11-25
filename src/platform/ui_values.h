#ifndef STARS_UI_VALUES_H
#define STARS_UI_VALUES_H

// Numeric dialog/message-box values shared by the SDL-free core and desktop.
namespace win32 {
inline constexpr auto MB_OK = 0x0000;
inline constexpr auto MB_OKCANCEL = 0x0001;
inline constexpr auto MB_YESNOCANCEL = 0x0003;
inline constexpr auto MB_YESNO = 0x0004;
inline constexpr auto MB_ICONHAND = 0x0010;
inline constexpr auto MB_ICONQUESTION = 0x0020;
inline constexpr auto MB_ICONEXCLAMATION = 0x0030;
inline constexpr auto MB_ICONASTERISK = 0x0040;
inline constexpr auto MB_TASKMODAL = 0x2000;
inline constexpr auto IDOK = 1;
inline constexpr auto IDCANCEL = 2;
inline constexpr auto IDYES = 6;
inline constexpr auto IDNO = 7;
inline constexpr auto IDHELP = 9;
} // namespace win32
#endif
