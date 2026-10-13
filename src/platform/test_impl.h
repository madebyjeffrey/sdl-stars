/* Test builds may retain real implementations under separate C names.
 * Production builds expand every definition to its original symbol. */
#ifndef STARS_TEST_IMPL_H
#define STARS_TEST_IMPL_H
#define STARS_TEST_IMPL(name) STARS_TEST_IMPL_##name
#ifndef STARS_TEST_IMPL_AlertSz
#define STARS_TEST_IMPL_AlertSz AlertSz
#endif
#ifndef STARS_TEST_IMPL_Random
#define STARS_TEST_IMPL_Random Random
#endif
#ifndef STARS_TEST_IMPL_PctPlanetCapacity
#define STARS_TEST_IMPL_PctPlanetCapacity PctPlanetCapacity
#endif
#ifndef STARS_TEST_IMPL_WriteRt
#define STARS_TEST_IMPL_WriteRt WriteRt
#endif
#ifndef STARS_TEST_IMPL_FCreateFile
#define STARS_TEST_IMPL_FCreateFile FCreateFile
#endif
#ifndef STARS_TEST_IMPL_ReadRt
#define STARS_TEST_IMPL_ReadRt ReadRt
#endif
#ifndef STARS_TEST_IMPL_StreamOpen
#define STARS_TEST_IMPL_StreamOpen StreamOpen
#endif
#ifndef STARS_TEST_IMPL_StreamClose
#define STARS_TEST_IMPL_StreamClose StreamClose
#endif
#ifndef STARS_TEST_IMPL_DirtyGame
#define STARS_TEST_IMPL_DirtyGame DirtyGame
#endif
#ifndef STARS_TEST_IMPL_InitInstance
#define STARS_TEST_IMPL_InitInstance InitInstance
#endif
#ifndef STARS_TEST_IMPL_PromptPassword
#define STARS_TEST_IMPL_PromptPassword PromptPassword
#endif
#ifndef STARS_TEST_IMPL_NewGameWizard
#define STARS_TEST_IMPL_NewGameWizard NewGameWizard
#endif
#ifndef STARS_TEST_IMPL_FNativeSaveFileName
#define STARS_TEST_IMPL_FNativeSaveFileName FNativeSaveFileName
#endif
#ifndef STARS_TEST_IMPL_TrackPopupMenu
#define STARS_TEST_IMPL_TrackPopupMenu TrackPopupMenu
#endif
#ifndef STARS_TEST_IMPL_MessageBoxA
#define STARS_TEST_IMPL_MessageBoxA MessageBoxA
#endif
#ifndef STARS_TEST_IMPL_IChooseWindowsFile
#define STARS_TEST_IMPL_IChooseWindowsFile IChooseWindowsFile
#endif
#endif
