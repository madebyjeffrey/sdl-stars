#ifndef STARS_SESSION_RESTORE_H
#define STARS_SESSION_RESTORE_H

class APPSESSION;
#include <stdint.h>
void    RestoreGameWindows(APPSESSION &app);
int16_t FinishOpenGame(APPSESSION &app);
#endif
