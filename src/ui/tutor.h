#ifndef STARS_DECOMPILED_TUTOR_H
#define STARS_DECOMPILED_TUTOR_H

#include "win.h"
#include <stdint.h>
#include <windows.h>

extern ITEMACTION rgiaUnloadAllCol[5];
extern ITEMACTION rgiaQuikDrop[5];
extern ITEMACTION rgiaQuikLoad[5];
extern ITEMACTION rgiaLoadAllCol[5];
extern ZIPPRODQ1  rgzpqTut[2];

INT_PTR  TutorDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR  PanicDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void             ShowTutor(APPSESSION &app, int16_t fShow);
void             DrawTutorText(APPSESSION &app, HWND hwnd);
void             StartTutor(APPSESSION &app, int16_t fRestart);
void             EndTutor(APPSESSION &app, int16_t fClose);
void             SaveGameState(APPSESSION &app);
void             RestoreGameState(APPSESSION &app);
int16_t          FAskKillTutor(APPSESSION &app);
int16_t          FTutorTaskDone(APPSESSION &app);
int16_t          FCheckZip(APPSESSION &app, int16_t iZip, ITEMACTION *lpiaGoal, StringId ids);
int16_t          FCheckTemplate(APPSESSION &app, int16_t iTemplate);
void             TutorError(APPSESSION &app, StringId idsError);
int16_t          FCheckScanner(APPSESSION &app, int16_t md, int16_t iZoom);
int16_t          FCheckFleetName(APPSESSION &app, int16_t id, StringId ids);
int16_t          FCheckSummary(APPSESSION &app, GrobjClass grobj, int16_t id);
int16_t          FCheckSelection(APPSESSION &app, GrobjClass grobj, int16_t id);
int16_t          FCheckMessages(APPSESSION &app, int16_t imsg, MessageId idm, int16_t fFilter);
int16_t          FCheckResearch(APPSESSION &app, TechFieldType iTech, TechFieldType iTechNext, int16_t pct);
int16_t          FCheckFleetWP(APPSESSION &app, uint16_t ifl, int16_t iord, GrobjClass grobj, int16_t id, TaskType grTask, uint16_t iWarp);
int16_t          FCheckPlanetRoute(APPSESSION &app, int16_t idpl, int16_t idplRoute);
int16_t          FCheckLayingWP(APPSESSION &app, uint16_t ifl, int16_t iord, int16_t id, int16_t iYears);
int16_t          FCheckColonizeWP(APPSESSION &app, uint16_t ifl, int16_t id, uint16_t iWarp);
int16_t          FCheckPatrolWP(APPSESSION &app, uint16_t ifl, int16_t iord, int16_t id, uint16_t iWarp, uint16_t iPlan, uint16_t iDist);
int16_t          FCheckXferWP(APPSESSION &app, uint16_t ifl, int16_t iord, int16_t id, uint16_t iWarp, ITEMACTION *lpiaGoal);
int16_t          FCheckQueue(APPSESSION &app, int16_t ipl, int16_t iprod, GrobjClass grobj, uint16_t iItem, uint16_t cItem, uint16_t fNoResearch);
int16_t FCheckBtlPlan(APPSESSION &app, int16_t ibp, uint16_t imdTarget, int16_t fSpread, int16_t fBomb, int16_t fDump, uint16_t mdUnarmed, uint16_t mdScout, uint16_t mdWar,
                      uint16_t mdBomber);
int16_t FCheckCargo(APPSESSION &app, FLEET *lpfl, int16_t wtMin1, int16_t wtMin2, int16_t wtMin3, int16_t wtColonists);
int16_t FCheckBuilderPart(APPSESSION &app, int16_t iSlot, HS *phs, uint16_t cInit);
int16_t FCheckShipBuilder(APPSESSION &app, int16_t iCategory, int16_t iShip);
int16_t FTutorialEnabledShipBuilder(APPSESSION &app, TutorShipBuilderAction itutsbAction);
int16_t FOKMergeDialog(APPSESSION &app);

#endif
