#ifndef STARS_DECOMPILED_TUTOR_H
#define STARS_DECOMPILED_TUTOR_H

#include "win.h"
#include <cstdint>
#include <span>
#include <windows.h>

extern ITEMACTION rgiaUnloadAllCol[5];
extern ITEMACTION rgiaQuikDrop[5];
extern ITEMACTION rgiaQuikLoad[5];
extern ITEMACTION rgiaLoadAllCol[5];
extern ZIPPRODQ1  rgzpqTut[2];

INT_PTR  TutorDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR  PanicDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void             ShowTutor(APPSESSION &app, std::int16_t fShow);
void             DrawTutorText(APPSESSION &app, HWND hwnd);
void             StartTutor(APPSESSION &app, std::int16_t fRestart);
void             EndTutor(APPSESSION &app, std::int16_t fClose);
void             SaveGameState(APPSESSION &app);
void             RestoreGameState(APPSESSION &app);
std::int16_t          FAskKillTutor(APPSESSION &app);
std::int16_t          FTutorTaskDone(APPSESSION &app);
std::int16_t          FCheckZip(APPSESSION &app, std::int16_t iZip, std::span<const ITEMACTION, 5> lpiaGoal, StringId ids);
std::int16_t          FCheckTemplate(APPSESSION &app, std::int16_t iTemplate);
void             TutorError(APPSESSION &app, StringId idsError);
std::int16_t          FCheckScanner(APPSESSION &app, std::int16_t md, std::int16_t iZoom);
std::int16_t          FCheckFleetName(APPSESSION &app, std::int16_t id, StringId ids);
std::int16_t          FCheckSummary(APPSESSION &app, GrobjClass grobj, std::int16_t id);
std::int16_t          FCheckSelection(APPSESSION &app, GrobjClass grobj, std::int16_t id);
std::int16_t          FCheckMessages(APPSESSION &app, std::int16_t imsg, MessageId idm, std::int16_t fFilter);
std::int16_t          FCheckResearch(APPSESSION &app, TechFieldType iTech, TechFieldType iTechNext, std::int16_t pct);
std::int16_t          FCheckFleetWP(APPSESSION &app, std::uint16_t ifl, std::int16_t iord, GrobjClass grobj, std::int16_t id, TaskType grTask, std::uint16_t iWarp);
std::int16_t          FCheckPlanetRoute(APPSESSION &app, std::int16_t idpl, std::int16_t idplRoute);
std::int16_t          FCheckLayingWP(APPSESSION &app, std::uint16_t ifl, std::int16_t iord, std::int16_t id, std::int16_t iYears);
std::int16_t          FCheckColonizeWP(APPSESSION &app, std::uint16_t ifl, std::int16_t id, std::uint16_t iWarp);
std::int16_t          FCheckPatrolWP(APPSESSION &app, std::uint16_t ifl, std::int16_t iord, std::int16_t id, std::uint16_t iWarp, std::uint16_t iPlan, std::uint16_t iDist);
std::int16_t          FCheckXferWP(APPSESSION &app, std::uint16_t ifl, std::int16_t iord, std::int16_t id, std::uint16_t iWarp, ITEMACTION *lpiaGoal);
std::int16_t          FCheckQueue(APPSESSION &app, std::int16_t ipl, std::int16_t iprod, GrobjClass grobj, std::uint16_t iItem, std::uint16_t cItem, std::uint16_t fNoResearch);
std::int16_t FCheckBtlPlan(APPSESSION &app, std::int16_t ibp, std::uint16_t imdTarget, std::int16_t fSpread, std::int16_t fBomb, std::int16_t fDump, std::uint16_t mdUnarmed, std::uint16_t mdScout, std::uint16_t mdWar,
                      std::uint16_t mdBomber);
std::int16_t FCheckCargo(APPSESSION &app, FLEET *lpfl, std::int16_t wtMin1, std::int16_t wtMin2, std::int16_t wtMin3, std::int16_t wtColonists);
std::int16_t FCheckBuilderPart(APPSESSION &app, std::int16_t iSlot, HS *phs, std::uint16_t cInit);
std::int16_t FCheckShipBuilder(APPSESSION &app, std::int16_t iCategory, std::int16_t iShip);
std::int16_t FTutorialEnabledShipBuilder(APPSESSION &app, TutorShipBuilderAction itutsbAction);
std::int16_t FOKMergeDialog(APPSESSION &app);

#endif
