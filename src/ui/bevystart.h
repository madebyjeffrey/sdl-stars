#ifndef STARS_BEVYSTART_H
#define STARS_BEVYSTART_H

#include <stdint.h>

/* Native window handles never enter game state or saved records. */
typedef struct {
    uint32_t grBackend;
    uintptr_t hWindow;
    uintptr_t hDisplay;
    int32_t iScreen;
    uint32_t dx;
    uint32_t dy;
    float rScale;
} BEVYWINDOW;

typedef struct {
    uint32_t dx;
    uint32_t dy;
    float rScale;
    float x;
    float y;
    int32_t fPress;
    int32_t fRelease;
    int32_t idKey;
} BEVYINPUT;

void *StarsBevyCreate(const BEVYWINDOW *lpWindow, const char *const *rgpsz, const char *pszVersion, int32_t fContinue);
int32_t StarsBevyUpdate(void *lpScreen, const BEVYINPUT *lpInput);
void StarsBevyDestroy(void *lpScreen);
void ShowBevyStartupScreen(void);
void SetStartupScreen(void (*lpfnScreen)(void));

#endif
