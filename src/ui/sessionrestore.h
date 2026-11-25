#ifndef STARS_SESSION_RESTORE_H
#define STARS_SESSION_RESTORE_H

class APPSESSION;
#include <cstdint>
void    RestoreGameWindows(APPSESSION &app);
std::int16_t FinishOpenGame(APPSESSION &app);
#endif
