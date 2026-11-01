#ifndef STARS_SHARED_SDK_H
#define STARS_SHARED_SDK_H
// The same vectors use native C++ callbacks here and SDK conventions in C.
#ifdef STARS_UI_SDL
#ifdef CALLBACK
#undef CALLBACK
#endif
#define CALLBACK
#endif
#endif
