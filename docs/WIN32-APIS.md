# Win32 API inventory

Source revision: `e7245dd3599e3a5dcf5e6ba74e90896321a251a5`. Scope: all root application C sources and headers, including the `_WIN32` branch of `native.c` and inline adapters in `nativeui.h`. Test harnesses are listed separately below.

The compatibility layer primarily needs a window and dialog system, a message dispatcher, GDI drawing, and resource loading. The game core already builds without Windows headers; its remaining Windows operations are behind `native.c`. Preserve that boundary when introducing a UI backend.

## Coverage and interpretation

This is a source inventory, including calls in conditional branches. Counts refer to source spellings, not DLL exports: generic names can select ANSI exports or expand into other functions. Legacy macros and Microsoft CRT calls are separated from Win32 function calls. Comments, string literals and application-defined functions are excluded. [WIN32-API-CALLS.csv](WIN32-API-CALLS.csv) records every matched application call with its source line.

The MinGW Debug build and its executable imports were verified on October 10, 2026. [WIN32-IMPORTS.csv](WIN32-IMPORTS.csv) separates imports matching application spellings from runtime and compiler support imports. No `LoadLibrary` or `GetProcAddress` usage occurs in the scanned application sources.

| Category | Unique source symbols | Call occurrences |
| --- | ---: | ---: |
| Windows and message dispatch | 39 | 1110 |
| Dialogs and controls | 11 | 478 |
| Menus and accelerators | 14 | 83 |
| Input coordinates and scrolling | 15 | 193 |
| Painting and device contexts | 7 | 217 |
| GDI objects and palettes | 14 | 737 |
| GDI drawing text and state | 29 | 1213 |
| Rectangle operations | 7 | 164 |
| Resources and movable memory | 17 | 133 |
| File dialogs and printing | 3 | 4 |
| Files configuration and time | 12 | 145 |
| System settings help and sound | 5 | 88 |
| Windows string functions | 5 | 354 |
| Legacy compatibility macros | 5 | 96 |
| Microsoft CRT dependencies | 6 | 12 |

**178 Win32 function spellings**, plus 5 legacy macros and 6 Microsoft CRT functions.

## API locations

Each row lists all files containing calls, with the first call in each file linked. The CSV includes subsequent calls. Categories indicate implementation responsibilities; they do not imply DLL ownership. CMake explicitly links `user32`, `gdi32` and `comdlg32`; kernel functions use the normal Windows runtime linkage.

### Windows and message dispatch

| Source symbol | Calls | Source locations |
| --- | ---: | --- |
| `CallWindowProc` | 6 | [buildui.c:1713](../buildui.c#L1713), [shipui.c:2418](../shipui.c#L2418), [tb.c:747](../tb.c#L747) |
| `CreateWindow` | 32 | [fileui.c:47](../fileui.c#L47), [init.c:225](../init.c#L225), [mdi.c:133](../mdi.c#L133), [mineui.c:19](../mineui.c#L19), [msgui.c:37](../msgui.c#L37), [planetui.c:26](../planetui.c#L26), [popup.c:360](../popup.c#L360), [reportui.c:38](../reportui.c#L38), [researchui.c:596](../researchui.c#L596), [tb.c:60](../tb.c#L60) |
| `DefWindowProc` | 12 | [mdi.c:614](../mdi.c#L614), [mineui.c:88](../mineui.c#L88), [msgui.c:405](../msgui.c#L405), [planetui.c:226](../planetui.c#L226), [popup.c:47](../popup.c#L47), [reportui.c:274](../reportui.c#L274), [researchui.c:782](../researchui.c#L782), [scan.c:416](../scan.c#L416), [tb.c:200](../tb.c#L200) |
| `DestroyWindow` | 22 | [createui.c:259](../createui.c#L259), [fileui.c:96](../fileui.c#L96), [mdi.c:156](../mdi.c#L156), [popup.c:35](../popup.c#L35), [reportui.c:269](../reportui.c#L269), [stars.c:392](../stars.c#L392), [tb.c:620](../tb.c#L620), [tutor.c:205](../tutor.c#L205), [utilgenui.c:1120](../utilgenui.c#L1120) |
| `DispatchMessage` | 2 | [stars.c:42](../stars.c#L42) |
| `EnableWindow` | 80 | [battleui.c:159](../battleui.c#L159), [buildui.c:291](../buildui.c#L291), [createui.c:494](../createui.c#L494), [mdi.c:1964](../mdi.c#L1964), [msgui.c:621](../msgui.c#L621), [planetui.c:815](../planetui.c#L815), [produceui.c:83](../produceui.c#L83), [raceui.c:167](../raceui.c#L167), [ship2ui.c:172](../ship2ui.c#L172), [shipui.c:367](../shipui.c#L367), [vcr.c:1186](../vcr.c#L1186) |
| `FlashWindow` | 2 | [mdi.c:2197](../mdi.c#L2197) |
| `GetActiveWindow` | 1 | [stars.c:391](../stars.c#L391) |
| `GetClassNameA` | 1 | [nativeui.c:60](../nativeui.c#L60) |
| `GetClientRect` | 143 | [battleui.c:24](../battleui.c#L24), [buildui.c:102](../buildui.c#L102), [createui.c:342](../createui.c#L342), [mdi.c:474](../mdi.c#L474), [mineui.c:32](../mineui.c#L32), [msgui.c:73](../msgui.c#L73), [planetui.c:44](../planetui.c#L44), [popup.c:19](../popup.c#L19), [produceui.c:94](../produceui.c#L94), [raceui.c:207](../raceui.c#L207), [reportui.c:45](../reportui.c#L45), [researchui.c:79](../researchui.c#L79), [scan.c:100](../scan.c#L100), [ship2ui.c:50](../ship2ui.c#L50), [shipui.c:42](../shipui.c#L42), [stars.c:306](../stars.c#L306), [tb.c:132](../tb.c#L132), [tutor.c:36](../tutor.c#L36), [utilgenui.c:15](../utilgenui.c#L15), [vcr.c:279](../vcr.c#L279) |
| `GetFocus` | 8 | [buildui.c:60](../buildui.c#L60), [mdi.c:1213](../mdi.c#L1213), [stars.c:371](../stars.c#L371), [utilgenui.c:1257](../utilgenui.c#L1257) |
| `GetMessage` | 1 | [stars.c:36](../stars.c#L36) |
| `GetParent` | 6 | [nativeui.c:78](../nativeui.c#L78), [stars.c:404](../stars.c#L404) |
| `GetUpdateRect` | 1 | [scan.c:2060](../scan.c#L2060) |
| `GetWindow` | 3 | [mdi.c:2221](../mdi.c#L2221), [tb.c:78](../tb.c#L78) |
| `GetWindowLongPtr` | 4 | [buildui.c:111](../buildui.c#L111), [planetui.c:37](../planetui.c#L37), [tb.c:76](../tb.c#L76) |
| `GetWindowPlacement` | 1 | [mdi.c:2252](../mdi.c#L2252) |
| `GetWindowRect` | 58 | [battleui.c:29](../battleui.c#L29), [buildui.c:101](../buildui.c#L101), [createui.c:373](../createui.c#L373), [mdi.c:2805](../mdi.c#L2805), [produceui.c:118](../produceui.c#L118), [raceui.c:236](../raceui.c#L236), [reportui.c:242](../reportui.c#L242), [researchui.c:78](../researchui.c#L78), [ship2ui.c:70](../ship2ui.c#L70), [shipui.c:1371](../shipui.c#L1371), [tutor.c:144](../tutor.c#L144), [utilgenui.c:308](../utilgenui.c#L308), [vcr.c:278](../vcr.c#L278) |
| `GetWindowText` | 11 | [buildui.c:259](../buildui.c#L259), [createui.c:596](../createui.c#L596), [msgui.c:534](../msgui.c#L534), [reportui.c:2651](../reportui.c#L2651), [ship2ui.c:270](../ship2ui.c#L270), [shipui.c:1023](../shipui.c#L1023), [tb.c:578](../tb.c#L578), [utilgenui.c:1077](../utilgenui.c#L1077) |
| `GetWindowThreadProcessId` | 1 | [nativeui.c:73](../nativeui.c#L73) |
| `InvalidateRect` | 89 | [battleui.c:89](../battleui.c#L89), [buildui.c:67](../buildui.c#L67), [createui.c:830](../createui.c#L830), [mdi.c:135](../mdi.c#L135), [mineui.c:121](../mineui.c#L121), [msgui.c:100](../msgui.c#L100), [planetui.c:1827](../planetui.c#L1827), [produceui.c:414](../produceui.c#L414), [raceui.c:336](../raceui.c#L336), [reportui.c:229](../reportui.c#L229), [researchui.c:252](../researchui.c#L252), [scan.c:2239](../scan.c#L2239), [ship2ui.c:107](../ship2ui.c#L107), [shipui.c:777](../shipui.c#L777), [stars.c:462](../stars.c#L462), [tb.c:446](../tb.c#L446), [tutor.c:275](../tutor.c#L275) |
| `IsIconic` | 8 | [mdi.c:473](../mdi.c#L473), [nativeui.c:39](../nativeui.c#L39), [stars.c:44](../stars.c#L44) |
| `IsWindowVisible` | 7 | [scan.c:2052](../scan.c#L2052), [shipui.c:236](../shipui.c#L236), [tb.c:617](../tb.c#L617) |
| `IsZoomed` | 2 | [mdi.c:1676](../mdi.c#L1676) |
| `MoveWindow` | 12 | [mdi.c:2579](../mdi.c#L2579) |
| `PeekMessage` | 5 | [createui.c:747](../createui.c#L747), [mdi.c:2031](../mdi.c#L2031), [popup.c:463](../popup.c#L463), [utilgenui.c:648](../utilgenui.c#L648) |
| `PostMessage` | 25 | [mdi.c:256](../mdi.c#L256), [msgui.c:367](../msgui.c#L367), [nativeui.c:34](../nativeui.c#L34), [scan.c:374](../scan.c#L374), [stars.c:35](../stars.c#L35), [tb.c:183](../tb.c#L183), [tutor.c:43](../tutor.c#L43) |
| `PostQuitMessage` | 6 | [mdi.c:244](../mdi.c#L244) |
| `RegisterClass` | 11 | [mdi.c:18](../mdi.c#L18) |
| `SendMessage` | 266 | [battleui.c:17](../battleui.c#L17), [buildui.c:229](../buildui.c#L229), [createui.c:334](../createui.c#L334), [mdi.c:167](../mdi.c#L167), [mineui.c:21](../mineui.c#L21), [msgui.c:39](../msgui.c#L39), [planetui.c:28](../planetui.c#L28), [popup.c:361](../popup.c#L361), [produceui.c:241](../produceui.c#L241), [raceui.c:158](../raceui.c#L158), [reportui.c:130](../reportui.c#L130), [researchui.c:40](../researchui.c#L40), [scan.c:94](../scan.c#L94), [ship2ui.c:271](../ship2ui.c#L271), [shipui.c:158](../shipui.c#L158), [stars.c:373](../stars.c#L373), [tb.c:62](../tb.c#L62), [tutor.c:1071](../tutor.c#L1071), [utilui.c:47](../utilui.c#L47) |
| `SetFocus` | 23 | [battleui.c:288](../battleui.c#L288), [buildui.c:61](../buildui.c#L61), [createui.c:457](../createui.c#L457), [mdi.c:2103](../mdi.c#L2103), [msgui.c:137](../msgui.c#L137), [planetui.c:90](../planetui.c#L90), [produceui.c:777](../produceui.c#L777), [reportui.c:2654](../reportui.c#L2654), [scan.c:132](../scan.c#L132), [ship2ui.c:149](../ship2ui.c#L149), [shipui.c:759](../shipui.c#L759), [tb.c:606](../tb.c#L606), [utilgenui.c:1024](../utilgenui.c#L1024), [vcr.c:1192](../vcr.c#L1192) |
| `SetWindowLongPtr` | 4 | [buildui.c:112](../buildui.c#L112), [planetui.c:38](../planetui.c#L38), [tb.c:77](../tb.c#L77) |
| `SetWindowPos` | 82 | [battleui.c:100](../battleui.c#L100), [buildui.c:103](../buildui.c#L103), [createui.c:410](../createui.c#L410), [mdi.c:1383](../mdi.c#L1383), [mineui.c:1183](../mineui.c#L1183), [msgui.c:60](../msgui.c#L60), [planetui.c:765](../planetui.c#L765), [produceui.c:93](../produceui.c#L93), [raceui.c:382](../raceui.c#L382), [reportui.c:36](../reportui.c#L36), [researchui.c:46](../researchui.c#L46), [ship2ui.c:189](../ship2ui.c#L189), [shipui.c:51](../shipui.c#L51), [tb.c:639](../tb.c#L639), [tutor.c:32](../tutor.c#L32), [utilgenui.c:11](../utilgenui.c#L11), [vcr.c:282](../vcr.c#L282) |
| `SetWindowText` | 52 | [buildui.c:262](../buildui.c#L262), [createui.c:483](../createui.c#L483), [mdi.c:130](../mdi.c#L130), [msgui.c:223](../msgui.c#L223), [planetui.c:1695](../planetui.c#L1695), [produceui.c:488](../produceui.c#L488), [raceui.c:1103](../raceui.c#L1103), [reportui.c:549](../reportui.c#L549), [researchui.c:39](../researchui.c#L39), [ship2ui.c:19](../ship2ui.c#L19), [shipui.c:520](../shipui.c#L520), [stars.c:302](../stars.c#L302), [tb.c:75](../tb.c#L75), [tutor.c:322](../tutor.c#L322), [utilgenui.c:990](../utilgenui.c#L990) |
| `ShowWindow` | 104 | [buildui.c:23](../buildui.c#L23), [createui.c:261](../createui.c#L261), [fileui.c:49](../fileui.c#L49), [init.c:242](../init.c#L242), [mdi.c:266](../mdi.c#L266), [msgui.c:220](../msgui.c#L220), [nativeui.c:40](../nativeui.c#L40), [planetui.c:740](../planetui.c#L740), [ship2ui.c:20](../ship2ui.c#L20), [shipui.c:29](../shipui.c#L29), [tutor.c:123](../tutor.c#L123) |
| `TranslateMessage` | 2 | [stars.c:41](../stars.c#L41) |
| `UpdateWindow` | 12 | [mdi.c:1611](../mdi.c#L1611), [msgui.c:328](../msgui.c#L328), [reportui.c:183](../reportui.c#L183), [scan.c:2070](../scan.c#L2070), [shipui.c:2481](../shipui.c#L2481), [tb.c:699](../tb.c#L699), [utilui.c:46](../utilui.c#L46) |
| `ValidateRect` | 3 | [scan.c:2065](../scan.c#L2065) |
| `WindowFromPoint` | 2 | [nativeui.c:72](../nativeui.c#L72), [stars.c:411](../stars.c#L411) |

### Dialogs and controls

| Source symbol | Calls | Source locations |
| --- | ---: | --- |
| `CheckDlgButton` | 1 | [raceui.c:383](../raceui.c#L383) |
| `CheckRadioButton` | 16 | [battleui.c:14](../battleui.c#L14), [buildui.c:113](../buildui.c#L113), [createui.c:331](../createui.c#L331), [produceui.c:648](../produceui.c#L648), [raceui.c:151](../raceui.c#L151), [researchui.c:26](../researchui.c#L26), [ship2ui.c:22](../ship2ui.c#L22) |
| `CreateDialog` | 3 | [mdi.c:1308](../mdi.c#L1308), [tutor.c:264](../tutor.c#L264), [utilgenui.c:1111](../utilgenui.c#L1111) |
| `DialogBox` | 36 | [battleui.c:286](../battleui.c#L286), [buildui.c:14](../buildui.c#L14), [createui.c:42](../createui.c#L42), [fileui.c:30](../fileui.c#L30), [mdi.c:927](../mdi.c#L927), [produceui.c:33](../produceui.c#L33), [raceui.c:15](../raceui.c#L15), [ship2ui.c:127](../ship2ui.c#L127), [shipui.c:769](../shipui.c#L769), [tutor.c:49](../tutor.c#L49), [utilgenui.c:1266](../utilgenui.c#L1266), [vcr.c:49](../vcr.c#L49) |
| `EndDialog` | 39 | [battleui.c:63](../battleui.c#L63), [buildui.c:590](../buildui.c#L590), [createui.c:431](../createui.c#L431), [fileui.c:12](../fileui.c#L12), [mdi.c:2090](../mdi.c#L2090), [produceui.c:18](../produceui.c#L18), [raceui.c:300](../raceui.c#L300), [reportui.c:484](../reportui.c#L484), [researchui.c:166](../researchui.c#L166), [scan.c:2747](../scan.c#L2747), [ship2ui.c:114](../ship2ui.c#L114), [shipui.c:1419](../shipui.c#L1419), [stars.c:359](../stars.c#L359), [tutor.c:75](../tutor.c#L75), [utilgenui.c:44](../utilgenui.c#L44), [vcr.c:480](../vcr.c#L480) |
| `GetDlgItem` | 323 | [battleui.c:17](../battleui.c#L17), [buildui.c:23](../buildui.c#L23), [createui.c:333](../createui.c#L333), [mdi.c:1913](../mdi.c#L1913), [produceui.c:83](../produceui.c#L83), [raceui.c:155](../raceui.c#L155), [reportui.c:542](../reportui.c#L542), [researchui.c:38](../researchui.c#L38), [scan.c:2741](../scan.c#L2741), [ship2ui.c:20](../ship2ui.c#L20), [shipui.c:1370](../shipui.c#L1370), [stars.c:302](../stars.c#L302), [tutor.c:145](../tutor.c#L145), [utilgenui.c:990](../utilgenui.c#L990), [vcr.c:281](../vcr.c#L281) |
| `GetDlgItemText` | 14 | [battleui.c:125](../battleui.c#L125), [raceui.c:137](../raceui.c#L137), [researchui.c:580](../researchui.c#L580), [scan.c:2738](../scan.c#L2738), [ship2ui.c:215](../ship2ui.c#L215), [utilgenui.c:33](../utilgenui.c#L33) |
| `IsDlgButtonChecked` | 17 | [createui.c:423](../createui.c#L423), [raceui.c:226](../raceui.c#L226), [researchui.c:139](../researchui.c#L139) |
| `MessageBox` | 4 | [buildui.c:60](../buildui.c#L60), [mdi.c:1213](../mdi.c#L1213), [utilgenui.c:1257](../utilgenui.c#L1257) |
| `SendDlgItemMessage` | 16 | [battleui.c:101](../battleui.c#L101), [createui.c:484](../createui.c#L484), [raceui.c:152](../raceui.c#L152), [scan.c:2715](../scan.c#L2715), [ship2ui.c:190](../ship2ui.c#L190), [utilgenui.c:12](../utilgenui.c#L12) |
| `SetDlgItemText` | 9 | [battleui.c:102](../battleui.c#L102), [buildui.c:33](../buildui.c#L33), [raceui.c:134](../raceui.c#L134) |

### Menus and accelerators

| Source symbol | Calls | Source locations |
| --- | ---: | --- |
| `AppendMenu` | 23 | [createui.c:704](../createui.c#L704), [mdi.c:2024](../mdi.c#L2024), [popup.c:389](../popup.c#L389) |
| `CheckMenuItem` | 18 | [mdi.c:997](../mdi.c#L997), [reportui.c:261](../reportui.c#L261), [researchui.c:633](../researchui.c#L633), [tutor.c:74](../tutor.c#L74) |
| `CreatePopupMenu` | 8 | [createui.c:700](../createui.c#L700), [mdi.c:2017](../mdi.c#L2017), [popup.c:384](../popup.c#L384) |
| `DeleteMenu` | 1 | [mdi.c:1636](../mdi.c#L1636) |
| `DestroyMenu` | 5 | [createui.c:743](../createui.c#L743), [mdi.c:2029](../mdi.c#L2029), [popup.c:457](../popup.c#L457) |
| `DrawMenuBar` | 2 | [mdi.c:1592](../mdi.c#L1592) |
| `EnableMenuItem` | 8 | [mdi.c:1645](../mdi.c#L1645) |
| `GetMenu` | 4 | [mdi.c:1632](../mdi.c#L1632) |
| `GetMenuItemCount` | 1 | [mdi.c:1660](../mdi.c#L1660) |
| `GetSubMenu` | 5 | [mdi.c:1583](../mdi.c#L1583) |
| `InsertMenu` | 1 | [mdi.c:1643](../mdi.c#L1643) |
| `LoadAccelerators` | 2 | [init.c:229](../init.c#L229) |
| `TrackPopupMenu` | 3 | [createui.c:742](../createui.c#L742), [mdi.c:2028](../mdi.c#L2028), [popup.c:456](../popup.c#L456) |
| `TranslateAccelerator` | 2 | [stars.c:40](../stars.c#L40) |

### Input coordinates and scrolling

| Source symbol | Calls | Source locations |
| --- | ---: | --- |
| `ClientToScreen` | 3 | [buildui.c:872](../buildui.c#L872), [nativeui.h:40](../nativeui.h#L40) |
| `GetAsyncKeyState` | 24 | [buildui.c:1454](../buildui.c#L1454), [mdi.c:1277](../mdi.c#L1277), [msgui.c:279](../msgui.c#L279), [produceui.c:247](../produceui.c#L247), [scan.c:116](../scan.c#L116), [vcr.c:432](../vcr.c#L432) |
| `GetCursorPos` | 1 | [nativeui.h:24](../nativeui.h#L24) |
| `GetKeyState` | 5 | [planetui.c:171](../planetui.c#L171), [produceui.c:439](../produceui.c#L439), [scan.c:21](../scan.c#L21), [stars.c:477](../stars.c#L477) |
| `GetScrollPos` | 7 | [reportui.c:129](../reportui.c#L129), [scan.c:405](../scan.c#L405) |
| `KillTimer` | 11 | [mdi.c:355](../mdi.c#L355), [stars.c:357](../stars.c#L357), [tb.c:661](../tb.c#L661), [vcr.c:356](../vcr.c#L356) |
| `MapWindowPoints` | 10 | [createui.c:398](../createui.c#L398), [mdi.c:481](../mdi.c#L481), [nativeui.h:51](../nativeui.h#L51), [produceui.c:644](../produceui.c#L644), [reportui.c:543](../reportui.c#L543), [researchui.c:380](../researchui.c#L380), [tb.c:157](../tb.c#L157), [vcr.c:292](../vcr.c#L292) |
| `ReleaseCapture` | 13 | [buildui.c:996](../buildui.c#L996), [mdi.c:565](../mdi.c#L565), [planetui.c:1119](../planetui.c#L1119), [popup.c:38](../popup.c#L38), [raceui.c:687](../raceui.c#L687), [scan.c:328](../scan.c#L328), [shipui.c:1488](../shipui.c#L1488), [tb.c:126](../tb.c#L126), [utilgenui.c:464](../utilgenui.c#L464) |
| `ScreenToClient` | 41 | [battleui.c:30](../battleui.c#L30), [buildui.c:180](../buildui.c#L180), [createui.c:374](../createui.c#L374), [nativeui.h:32](../nativeui.h#L32), [produceui.c:120](../produceui.c#L120), [raceui.c:237](../raceui.c#L237), [ship2ui.c:71](../ship2ui.c#L71), [tutor.c:147](../tutor.c#L147) |
| `ScrollWindow` | 2 | [reportui.c:181](../reportui.c#L181), [scan.c:2071](../scan.c#L2071) |
| `SetCapture` | 12 | [buildui.c:931](../buildui.c#L931), [mdi.c:550](../mdi.c#L550), [planetui.c:1108](../planetui.c#L1108), [popup.c:362](../popup.c#L362), [raceui.c:664](../raceui.c#L664), [scan.c:311](../scan.c#L311), [shipui.c:1467](../shipui.c#L1467), [tb.c:117](../tb.c#L117), [utilgenui.c:439](../utilgenui.c#L439) |
| `SetCursor` | 40 | [buildui.c:170](../buildui.c#L170), [createui.c:685](../createui.c#L685), [mdi.c:536](../mdi.c#L536), [mineui.c:53](../mineui.c#L53), [msgui.c:82](../msgui.c#L82), [planetui.c:111](../planetui.c#L111), [produceui.c:208](../produceui.c#L208), [raceui.c:431](../raceui.c#L431), [reportui.c:455](../reportui.c#L455), [researchui.c:129](../researchui.c#L129), [scan.c:115](../scan.c#L115), [stars.c:190](../stars.c#L190), [tb.c:178](../tb.c#L178), [vcr.c:315](../vcr.c#L315) |
| `SetScrollPos` | 10 | [reportui.c:51](../reportui.c#L51), [scan.c:404](../scan.c#L404) |
| `SetScrollRange` | 5 | [reportui.c:61](../reportui.c#L61), [scan.c:2033](../scan.c#L2033) |
| `SetTimer` | 9 | [mdi.c:1438](../mdi.c#L1438), [stars.c:303](../stars.c#L303), [tb.c:676](../tb.c#L676), [vcr.c:357](../vcr.c#L357) |

### Painting and device contexts

| Source symbol | Calls | Source locations |
| --- | ---: | --- |
| `BeginPaint` | 37 | [battleui.c:28](../battleui.c#L28), [buildui.c:177](../buildui.c#L177), [createui.c:372](../createui.c#L372), [mdi.c:488](../mdi.c#L488), [mineui.c:56](../mineui.c#L56), [msgui.c:156](../msgui.c#L156), [planetui.c:94](../planetui.c#L94), [popup.c:26](../popup.c#L26), [produceui.c:211](../produceui.c#L211), [raceui.c:225](../raceui.c#L225), [reportui.c:235](../reportui.c#L235), [researchui.c:120](../researchui.c#L120), [scan.c:98](../scan.c#L98), [ship2ui.c:68](../ship2ui.c#L68), [shipui.c:1406](../shipui.c#L1406), [tb.c:189](../tb.c#L189), [tutor.c:141](../tutor.c#L141), [utilgenui.c:1162](../utilgenui.c#L1162), [vcr.c:409](../vcr.c#L409) |
| `CreateCompatibleDC` | 11 | [buildui.c:643](../buildui.c#L643), [mineui.c:199](../mineui.c#L199), [msgui.c:426](../msgui.c#L426), [reportui.c:600](../reportui.c#L600), [scan.c:512](../scan.c#L512), [vcr.c:1070](../vcr.c#L1070) |
| `DeleteDC` | 14 | [buildui.c:766](../buildui.c#L766), [mdi.c:1152](../mdi.c#L1152), [mineui.c:527](../mineui.c#L527), [msgui.c:491](../msgui.c#L491), [reportui.c:630](../reportui.c#L630), [scan.c:516](../scan.c#L516), [vcr.c:1075](../vcr.c#L1075) |
| `EndPaint` | 37 | [battleui.c:42](../battleui.c#L42), [buildui.c:210](../buildui.c#L210), [createui.c:416](../createui.c#L416), [mdi.c:490](../mdi.c#L490), [mineui.c:75](../mineui.c#L75), [msgui.c:237](../msgui.c#L237), [planetui.c:96](../planetui.c#L96), [popup.c:28](../popup.c#L28), [produceui.c:214](../produceui.c#L214), [raceui.c:267](../raceui.c#L267), [reportui.c:237](../reportui.c#L237), [researchui.c:123](../researchui.c#L123), [scan.c:105](../scan.c#L105), [ship2ui.c:101](../ship2ui.c#L101), [shipui.c:1409](../shipui.c#L1409), [tb.c:197](../tb.c#L197), [tutor.c:184](../tutor.c#L184), [utilgenui.c:1164](../utilgenui.c#L1164), [vcr.c:411](../vcr.c#L411) |
| `GetDC` | 52 | [buildui.c:633](../buildui.c#L633), [createui.c:875](../createui.c#L875), [mdi.c:201](../mdi.c#L201), [mineui.c:103](../mineui.c#L103), [planetui.c:1106](../planetui.c#L1106), [popup.c:251](../popup.c#L251), [produceui.c:523](../produceui.c#L523), [raceui.c:195](../raceui.c#L195), [reportui.c:28](../reportui.c#L28), [researchui.c:27](../researchui.c#L27), [scan.c:60](../scan.c#L60), [shipui.c:1569](../shipui.c#L1569), [stars.c:115](../stars.c#L115), [tb.c:114](../tb.c#L114), [utilgenui.c:419](../utilgenui.c#L419), [vcr.c:339](../vcr.c#L339) |
| `GetDeviceCaps` | 11 | [init.c:173](../init.c#L173), [mdi.c:1014](../mdi.c#L1014), [stars.c:116](../stars.c#L116), [utilgenui.c:1242](../utilgenui.c#L1242) |
| `ReleaseDC` | 55 | [buildui.c:768](../buildui.c#L768), [createui.c:937](../createui.c#L937), [mdi.c:206](../mdi.c#L206), [mineui.c:115](../mineui.c#L115), [planetui.c:1120](../planetui.c#L1120), [popup.c:355](../popup.c#L355), [produceui.c:589](../produceui.c#L589), [raceui.c:204](../raceui.c#L204), [reportui.c:34](../reportui.c#L34), [researchui.c:84](../researchui.c#L84), [scan.c:79](../scan.c#L79), [shipui.c:1602](../shipui.c#L1602), [stars.c:117](../stars.c#L117), [tb.c:137](../tb.c#L137), [utilgenui.c:466](../utilgenui.c#L466), [vcr.c:354](../vcr.c#L354) |

### GDI objects and palettes

| Source symbol | Calls | Source locations |
| --- | ---: | --- |
| `CreateCompatibleBitmap` | 4 | [buildui.c:927](../buildui.c#L927), [scan.c:514](../scan.c#L514), [vcr.c:1073](../vcr.c#L1073) |
| `CreateFontIndirect` | 10 | [init.c:176](../init.c#L176), [mdi.c:2790](../mdi.c#L2790), [raceui.c:1283](../raceui.c#L1283), [utilgenui.c:249](../utilgenui.c#L249) |
| `CreatePalette` | 2 | [utilgenui.c:821](../utilgenui.c#L821) |
| `CreatePatternBrush` | 4 | [init.c:73](../init.c#L73) |
| `CreatePen` | 12 | [init.c:61](../init.c#L61), [reportui.c:954](../reportui.c#L954), [scan.c:634](../scan.c#L634) |
| `CreateRectRgn` | 2 | [init.c:24](../init.c#L24) |
| `CreateSolidBrush` | 1 | [utilgenui.c:733](../utilgenui.c#L733) |
| `DeleteObject` | 48 | [buildui.c:999](../buildui.c#L999), [init.c:74](../init.c#L74), [mdi.c:1147](../mdi.c#L1147), [raceui.c:1292](../raceui.c#L1292), [reportui.c:975](../reportui.c#L975), [scan.c:1276](../scan.c#L1276), [stars.c:188](../stars.c#L188), [utilgenui.c:260](../utilgenui.c#L260), [vcr.c:1096](../vcr.c#L1096) |
| `GetObject` | 1 | [utilgenui.c:892](../utilgenui.c#L892) |
| `GetStockObject` | 21 | [buildui.c:672](../buildui.c#L672), [mdi.c:25](../mdi.c#L25), [planetui.c:1517](../planetui.c#L1517), [scan.c:532](../scan.c#L532), [utilgenui.c:890](../utilgenui.c#L890) |
| `RealizePalette` | 17 | [buildui.c:752](../buildui.c#L752), [mdi.c:219](../mdi.c#L219), [mineui.c:243](../mineui.c#L243), [planetui.c:874](../planetui.c#L874), [raceui.c:197](../raceui.c#L197), [researchui.c:856](../researchui.c#L856), [shipui.c:1161](../shipui.c#L1161), [tb.c:116](../tb.c#L116), [utilgenui.c:910](../utilgenui.c#L910) |
| `SelectObject` | 591 | [battleui.c:38](../battleui.c#L38), [buildui.c:188](../buildui.c#L188), [createui.c:381](../createui.c#L381), [init.c:191](../init.c#L191), [mdi.c:494](../mdi.c#L494), [mineui.c:104](../mineui.c#L104), [msgui.c:169](../msgui.c#L169), [planetui.c:249](../planetui.c#L249), [popup.c:81](../popup.c#L81), [produceui.c:525](../produceui.c#L525), [raceui.c:244](../raceui.c#L244), [reportui.c:29](../reportui.c#L29), [researchui.c:28](../researchui.c#L28), [scan.c:64](../scan.c#L64), [ship2ui.c:78](../ship2ui.c#L78), [shipui.c:56](../shipui.c#L56), [stars.c:321](../stars.c#L321), [tb.c:240](../tb.c#L240), [thingui.c:15](../thingui.c#L15), [tutor.c:143](../tutor.c#L143), [utilgenui.c:250](../utilgenui.c#L250), [vcr.c:342](../vcr.c#L342) |
| `SelectPalette` | 23 | [buildui.c:751](../buildui.c#L751), [mdi.c:218](../mdi.c#L218), [mineui.c:242](../mineui.c#L242), [planetui.c:873](../planetui.c#L873), [raceui.c:196](../raceui.c#L196), [researchui.c:855](../researchui.c#L855), [shipui.c:1160](../shipui.c#L1160), [tb.c:115](../tb.c#L115), [utilgenui.c:909](../utilgenui.c#L909) |
| `UnrealizeObject` | 1 | [scan.c:743](../scan.c#L743) |

### GDI drawing text and state

| Source symbol | Calls | Source locations |
| --- | ---: | --- |
| `BitBlt` | 52 | [buildui.c:719](../buildui.c#L719), [mineui.c:520](../mineui.c#L520), [msgui.c:451](../msgui.c#L451), [reportui.c:611](../reportui.c#L611), [scan.c:718](../scan.c#L718), [vcr.c:1085](../vcr.c#L1085) |
| `DrawIcon` | 6 | [mdi.c:489](../mdi.c#L489), [vcr.c:1066](../vcr.c#L1066) |
| `DrawText` | 26 | [createui.c:409](../createui.c#L409), [mineui.c:439](../mineui.c#L439), [msgui.c:218](../msgui.c#L218), [raceui.c:617](../raceui.c#L617), [researchui.c:1107](../researchui.c#L1107), [shipui.c:262](../shipui.c#L262) |
| `Ellipse` | 7 | [buildui.c:673](../buildui.c#L673), [scan.c:1023](../scan.c#L1023) |
| `Escape` | 3 | [mdi.c:1064](../mdi.c#L1064) |
| `ExcludeClipRect` | 14 | [mdi.c:482](../mdi.c#L482), [scan.c:508](../scan.c#L508) |
| `ExtTextOut` | 23 | [reportui.c:1165](../reportui.c#L1165), [scan.c:792](../scan.c#L792), [tb.c:730](../tb.c#L730), [utilgenui.c:199](../utilgenui.c#L199), [utilui.c:78](../utilui.c#L78) |
| `FillRect` | 84 | [battleui.c:25](../battleui.c#L25), [buildui.c:174](../buildui.c#L174), [createui.c:343](../createui.c#L343), [mdi.c:475](../mdi.c#L475), [mineui.c:33](../mineui.c#L33), [msgui.c:74](../msgui.c#L74), [planetui.c:68](../planetui.c#L68), [popup.c:20](../popup.c#L20), [produceui.c:159](../produceui.c#L159), [raceui.c:208](../raceui.c#L208), [reportui.c:76](../reportui.c#L76), [researchui.c:100](../researchui.c#L100), [scan.c:532](../scan.c#L532), [ship2ui.c:51](../ship2ui.c#L51), [shipui.c:62](../shipui.c#L62), [stars.c:307](../stars.c#L307), [tb.c:166](../tb.c#L166), [tutor.c:37](../tutor.c#L37), [utilgenui.c:16](../utilgenui.c#L16), [vcr.c:309](../vcr.c#L309) |
| `FrameRect` | 6 | [buildui.c:742](../buildui.c#L742), [planetui.c:1425](../planetui.c#L1425), [tb.c:725](../tb.c#L725), [utilgenui.c:347](../utilgenui.c#L347) |
| `GetBkColor` | 1 | [scan.c:1261](../scan.c#L1261) |
| `GetDIBits` | 2 | [utilgenui.c:919](../utilgenui.c#L919) |
| `GetROP2` | 1 | [scan.c:749](../scan.c#L749) |
| `GetTextExtentPoint32A` | 1 | [nativeui.c:6](../nativeui.c#L6) |
| `GetTextMetrics` | 8 | [init.c:192](../init.c#L192), [mdi.c:203](../mdi.c#L203), [raceui.c:1285](../raceui.c#L1285), [utilgenui.c:1247](../utilgenui.c#L1247) |
| `IntersectClipRect` | 5 | [scan.c:586](../scan.c#L586), [stars.c:325](../stars.c#L325) |
| `LineTo` | 35 | [reportui.c:966](../reportui.c#L966), [researchui.c:1066](../researchui.c#L1066), [scan.c:67](../scan.c#L67), [vcr.c:1063](../vcr.c#L1063) |
| `MoveToEx` | 27 | [reportui.c:964](../reportui.c#L964), [researchui.c:1064](../researchui.c#L1064), [scan.c:66](../scan.c#L66), [vcr.c:1062](../vcr.c#L1062) |
| `PatBlt` | 289 | [mdi.c:498](../mdi.c#L498), [mineui.c:224](../mineui.c#L224), [msgui.c:170](../msgui.c#L170), [planetui.c:264](../planetui.c#L264), [popup.c:133](../popup.c#L133), [raceui.c:504](../raceui.c#L504), [reportui.c:380](../reportui.c#L380), [researchui.c:239](../researchui.c#L239), [scan.c:873](../scan.c#L873), [shipui.c:1144](../shipui.c#L1144), [tb.c:191](../tb.c#L191), [tutor.c:154](../tutor.c#L154), [utilgenui.c:392](../utilgenui.c#L392), [utilui.c:103](../utilui.c#L103), [vcr.c:591](../vcr.c#L591) |
| `Rectangle` | 1 | [mdi.c:1065](../mdi.c#L1065) |
| `SelectClipRgn` | 5 | [scan.c:910](../scan.c#L910), [stars.c:338](../stars.c#L338) |
| `SetBkColor` | 126 | [battleui.c:37](../battleui.c#L37), [buildui.c:161](../buildui.c#L161), [createui.c:367](../createui.c#L367), [mdi.c:1887](../mdi.c#L1887), [mineui.c:42](../mineui.c#L42), [msgui.c:153](../msgui.c#L153), [planetui.c:78](../planetui.c#L78), [popup.c:78](../popup.c#L78), [produceui.c:153](../produceui.c#L153), [raceui.c:220](../raceui.c#L220), [reportui.c:603](../reportui.c#L603), [researchui.c:112](../researchui.c#L112), [scan.c:705](../scan.c#L705), [ship2ui.c:63](../ship2ui.c#L63), [shipui.c:1724](../shipui.c#L1724), [stars.c:323](../stars.c#L323), [tb.c:175](../tb.c#L175), [tutor.c:99](../tutor.c#L99), [utilgenui.c:26](../utilgenui.c#L26), [utilui.c:77](../utilui.c#L77), [vcr.c:341](../vcr.c#L341) |
| `SetBkMode` | 62 | [buildui.c:660](../buildui.c#L660), [createui.c:878](../createui.c#L878), [mdi.c:1021](../mdi.c#L1021), [mineui.c:202](../mineui.c#L202), [msgui.c:209](../msgui.c#L209), [planetui.c:712](../planetui.c#L712), [popup.c:80](../popup.c#L80), [raceui.c:497](../raceui.c#L497), [reportui.c:345](../reportui.c#L345), [researchui.c:843](../researchui.c#L843), [scan.c:931](../scan.c#L931), [shipui.c:1099](../shipui.c#L1099), [stars.c:322](../stars.c#L322), [tb.c:728](../tb.c#L728), [thingui.c:35](../thingui.c#L35), [utilgenui.c:626](../utilgenui.c#L626), [vcr.c:340](../vcr.c#L340) |
| `SetBrushOrgEx` | 3 | [scan.c:745](../scan.c#L745) |
| `SetPixel` | 1 | [reportui.c:972](../reportui.c#L972) |
| `SetROP2` | 17 | [scan.c:63](../scan.c#L63) |
| `SetTextColor` | 193 | [buildui.c:1041](../buildui.c#L1041), [createui.c:896](../createui.c#L896), [mdi.c:1896](../mdi.c#L1896), [mineui.c:60](../mineui.c#L60), [msgui.c:158](../msgui.c#L158), [planetui.c:79](../planetui.c#L79), [popup.c:79](../popup.c#L79), [produceui.c:556](../produceui.c#L556), [raceui.c:863](../raceui.c#L863), [reportui.c:591](../reportui.c#L591), [researchui.c:216](../researchui.c#L216), [scan.c:704](../scan.c#L704), [ship2ui.c:86](../ship2ui.c#L86), [shipui.c:139](../shipui.c#L139), [stars.c:324](../stars.c#L324), [tb.c:729](../tb.c#L729), [tutor.c:162](../tutor.c#L162), [utilgenui.c:627](../utilgenui.c#L627), [vcr.c:631](../vcr.c#L631) |
| `SetWindowOrgEx` | 2 | [scan.c:523](../scan.c#L523) |
| `StretchDIBits` | 1 | [utilgenui.c:872](../utilgenui.c#L872) |
| `TextOut` | 212 | [battleui.c:40](../battleui.c#L40), [buildui.c:190](../buildui.c#L190), [createui.c:384](../createui.c#L384), [mdi.c:1073](../mdi.c#L1073), [mineui.c:254](../mineui.c#L254), [planetui.c:310](../planetui.c#L310), [popup.c:96](../popup.c#L96), [produceui.c:549](../produceui.c#L549), [raceui.c:247](../raceui.c#L247), [reportui.c:359](../reportui.c#L359), [researchui.c:229](../researchui.c#L229), [scan.c:1885](../scan.c#L1885), [ship2ui.c:81](../ship2ui.c#L81), [shipui.c:74](../shipui.c#L74), [utilgenui.c:59](../utilgenui.c#L59), [vcr.c:348](../vcr.c#L348) |

### Rectangle operations

| Source symbol | Calls | Source locations |
| --- | ---: | --- |
| `CopyRect` | 1 | [buildui.c:1534](../buildui.c#L1534) |
| `EqualRect` | 1 | [tb.c:625](../tb.c#L625) |
| `InflateRect` | 10 | [buildui.c:1536](../buildui.c#L1536), [planetui.c:1551](../planetui.c#L1551), [scan.c:3005](../scan.c#L3005), [shipui.c:1332](../shipui.c#L1332), [utilgenui.c:1189](../utilgenui.c#L1189), [utilui.c:66](../utilui.c#L66) |
| `IntersectRect` | 2 | [scan.c:1426](../scan.c#L1426) |
| `OffsetRect` | 17 | [buildui.c:654](../buildui.c#L654), [createui.c:646](../createui.c#L646), [mdi.c:1909](../mdi.c#L1909), [planetui.c:1014](../planetui.c#L1014), [raceui.c:262](../raceui.c#L262), [reportui.c:544](../reportui.c#L544), [researchui.c:436](../researchui.c#L436), [stars.c:334](../stars.c#L334), [utilgenui.c:319](../utilgenui.c#L319), [vcr.c:300](../vcr.c#L300) |
| `PtInRect` | 51 | [buildui.c:168](../buildui.c#L168), [mdi.c:531](../mdi.c#L531), [mineui.c:48](../mineui.c#L48), [msgui.c:498](../msgui.c#L498), [planetui.c:103](../planetui.c#L103), [produceui.c:165](../produceui.c#L165), [raceui.c:180](../raceui.c#L180), [researchui.c:496](../researchui.c#L496), [scan.c:111](../scan.c#L111), [shipui.c:1450](../shipui.c#L1450), [tb.c:120](../tb.c#L120), [utilgenui.c:448](../utilgenui.c#L448), [vcr.c:323](../vcr.c#L323) |
| `SetRect` | 82 | [buildui.c:652](../buildui.c#L652), [createui.c:882](../createui.c#L882), [mdi.c:1891](../mdi.c#L1891), [mineui.c:58](../mineui.c#L58), [msgui.c:63](../msgui.c#L63), [planetui.c:256](../planetui.c#L256), [raceui.c:260](../raceui.c#L260), [reportui.c:354](../reportui.c#L354), [researchui.c:223](../researchui.c#L223), [scan.c:790](../scan.c#L790), [shipui.c:96](../shipui.c#L96), [utilgenui.c:195](../utilgenui.c#L195), [vcr.c:619](../vcr.c#L619) |

### Resources and movable memory

| Source symbol | Calls | Source locations |
| --- | ---: | --- |
| `DestroyCursor` | 11 | [stars.c:191](../stars.c#L191) |
| `DestroyIcon` | 4 | [stars.c:206](../stars.c#L206) |
| `FindResource` | 4 | [logui.c:29](../logui.c#L29), [utilgenui.c:960](../utilgenui.c#L960) |
| `FreeResource` | 14 | [logui.c:19](../logui.c#L19), [mdi.c:2703](../mdi.c#L2703), [stars.c:213](../stars.c#L213) |
| `GlobalAlloc` | 2 | [utilgenui.c:911](../utilgenui.c#L911) |
| `GlobalFree` | 3 | [mdi.c:1154](../mdi.c#L1154), [utilgenui.c:946](../utilgenui.c#L946) |
| `GlobalLock` | 5 | [stars.c:102](../stars.c#L102), [utilgenui.c:867](../utilgenui.c#L867) |
| `GlobalReAlloc` | 1 | [utilgenui.c:929](../utilgenui.c#L929) |
| `GlobalUnlock` | 21 | [logui.c:18](../logui.c#L18), [mdi.c:2702](../mdi.c#L2702), [stars.c:106](../stars.c#L106), [utilgenui.c:809](../utilgenui.c#L809) |
| `LoadBitmap` | 11 | [init.c:72](../init.c#L72) |
| `LoadCursor` | 22 | [buildui.c:984](../buildui.c#L984), [init.c:86](../init.c#L86), [mdi.c:14](../mdi.c#L14), [scan.c:115](../scan.c#L115), [stars.c:190](../stars.c#L190), [tb.c:178](../tb.c#L178) |
| `LoadIcon` | 11 | [init.c:131](../init.c#L131), [mdi.c:13](../mdi.c#L13) |
| `LoadResource` | 3 | [logui.c:34](../logui.c#L34), [utilgenui.c:964](../utilgenui.c#L964) |
| `LocalAlloc` | 8 | [init.c:167](../init.c#L167), [mdi.c:2770](../mdi.c#L2770), [raceui.c:1279](../raceui.c#L1279), [utilgenui.c:233](../utilgenui.c#L233) |
| `LocalFree` | 8 | [init.c:206](../init.c#L206), [mdi.c:2810](../mdi.c#L2810), [raceui.c:1293](../raceui.c#L1293), [utilgenui.c:283](../utilgenui.c#L283) |
| `LockResource` | 4 | [logui.c:40](../logui.c#L40), [utilgenui.c:802](../utilgenui.c#L802) |
| `SizeofResource` | 1 | [utilgenui.c:973](../utilgenui.c#L973) |

### File dialogs and printing

| Source symbol | Calls | Source locations |
| --- | ---: | --- |
| `GetOpenFileName` | 1 | [mdi.c:1724](../mdi.c#L1724) |
| `GetSaveFileName` | 2 | [createui.c:306](../createui.c#L306), [raceui.c:1389](../raceui.c#L1389) |
| `PrintDlg` | 1 | [mdi.c:1013](../mdi.c#L1013) |

### Files configuration and time

| Source symbol | Calls | Source locations |
| --- | ---: | --- |
| `GetCurrentThreadId` | 1 | [nativeui.c:73](../nativeui.c#L73) |
| `GetFileSize` | 1 | [native.c:188](../native.c#L188) |
| `GetPrivateProfileInt` | 35 | [init.c:387](../init.c#L387) |
| `GetPrivateProfileString` | 11 | [init.c:296](../init.c#L296) |
| `GetTickCount` | 16 | [mdi.c:1241](../mdi.c#L1241), [native.c:198](../native.c#L198), [scan.c:71](../scan.c#L71), [stars.c:34](../stars.c#L34), [tb.c:623](../tb.c#L623), [tutor.c:345](../tutor.c#L345), [vcr.c:913](../vcr.c#L913) |
| `MulDiv` | 34 | [init.c:173](../init.c#L173), [mdi.c:2564](../mdi.c#L2564), [mineui.c:621](../mineui.c#L621), [planetui.c:427](../planetui.c#L427), [raceui.c:552](../raceui.c#L552), [reportui.c:1313](../reportui.c#L1313), [researchui.c:1056](../researchui.c#L1056), [scan.c:598](../scan.c#L598), [utilgenui.c:241](../utilgenui.c#L241) |
| `OpenFile` | 1 | [native.c:177](../native.c#L177) |
| `WritePrivateProfileString` | 42 | [fileui.c:84](../fileui.c#L84), [mdi.c:2302](../mdi.c#L2302) |
| `_lclose` | 1 | [native.c:190](../native.c#L190) |
| `_llseek` | 1 | [native.c:186](../native.c#L186) |
| `_lread` | 1 | [native.c:182](../native.c#L182) |
| `_lwrite` | 1 | [native.c:184](../native.c#L184) |

### System settings help and sound

| Source symbol | Calls | Source locations |
| --- | ---: | --- |
| `GetSysColor` | 8 | [stars.c:84](../stars.c#L84) |
| `GetSystemMetrics` | 33 | [fileui.c:45](../fileui.c#L45), [init.c:13](../init.c#L13), [mdi.c:278](../mdi.c#L278), [popup.c:358](../popup.c#L358), [reportui.c:63](../reportui.c#L63), [researchui.c:573](../researchui.c#L573), [scan.c:306](../scan.c#L306), [shipui.c:1368](../shipui.c#L1368), [stars.c:98](../stars.c#L98), [tutor.c:210](../tutor.c#L210), [utilgenui.c:313](../utilgenui.c#L313) |
| `MessageBeep` | 11 | [buildui.c:1502](../buildui.c#L1502), [mdi.c:2199](../mdi.c#L2199), [produceui.c:292](../produceui.c#L292), [reportui.c:2432](../reportui.c#L2432), [scan.c:2300](../scan.c#L2300), [shipui.c:2246](../shipui.c#L2246), [tb.c:98](../tb.c#L98) |
| `SystemParametersInfo` | 1 | [nativeui.c:108](../nativeui.c#L108) |
| `WinHelp` | 35 | [battleui.c:80](../battleui.c#L80), [buildui.c:597](../buildui.c#L597), [createui.c:460](../createui.c#L460), [fileui.c:15](../fileui.c#L15), [mdi.c:1402](../mdi.c#L1402), [produceui.c:476](../produceui.c#L476), [raceui.c:271](../raceui.c#L271), [reportui.c:492](../reportui.c#L492), [researchui.c:174](../researchui.c#L174), [scan.c:2750](../scan.c#L2750), [ship2ui.c:160](../ship2ui.c#L160), [shipui.c:1425](../shipui.c#L1425), [tutor.c:65](../tutor.c#L65), [utilgenui.c:1032](../utilgenui.c#L1032), [vcr.c:483](../vcr.c#L483) |

### Windows string functions

| Source symbol | Calls | Source locations |
| --- | ---: | --- |
| `lstrcat` | 1 | [planetui.c:1688](../planetui.c#L1688) |
| `lstrcmpiA` | 1 | [nativeui.c:60](../nativeui.c#L60) |
| `lstrcpy` | 1 | [buildui.c:265](../buildui.c#L265) |
| `lstrlen` | 12 | [buildui.c:1136](../buildui.c#L1136), [planetui.c:310](../planetui.c#L310), [produceui.c:557](../produceui.c#L557), [researchui.c:917](../researchui.c#L917), [scan.c:1949](../scan.c#L1949), [shipui.c:596](../shipui.c#L596), [utilgenui.c:56](../utilgenui.c#L56) |
| `wsprintf` | 339 | [buildui.c:53](../buildui.c#L53), [createui.c:171](../createui.c#L171), [mdi.c:124](../mdi.c#L124), [mineui.c:253](../mineui.c#L253), [msgui.c:185](../msgui.c#L185), [planetui.c:314](../planetui.c#L314), [popup.c:91](../popup.c#L91), [produceui.c:487](../produceui.c#L487), [raceui.c:82](../raceui.c#L82), [reportui.c:670](../reportui.c#L670), [researchui.c:242](../researchui.c#L242), [scan.c:1883](../scan.c#L1883), [ship2ui.c:38](../ship2ui.c#L38), [shipui.c:124](../shipui.c#L124), [tb.c:69](../tb.c#L69), [thingui.c:40](../thingui.c#L40), [tutor.c:321](../tutor.c#L321), [utilgenui.c:1141](../utilgenui.c#L1141), [vcr.c:347](../vcr.c#L347) |

### Legacy compatibility macros

| Source symbol | Calls | Source locations |
| --- | ---: | --- |
| `ExitWindows` | 4 | [mdi.c:241](../mdi.c#L241) |
| `FreeProcInstance` | 44 | [battleui.c:287](../battleui.c#L287), [buildui.c:15](../buildui.c#L15), [createui.c:43](../createui.c#L43), [fileui.c:31](../fileui.c#L31), [mdi.c:928](../mdi.c#L928), [produceui.c:34](../produceui.c#L34), [raceui.c:16](../raceui.c#L16), [ship2ui.c:148](../ship2ui.c#L148), [shipui.c:770](../shipui.c#L770), [stars.c:179](../stars.c#L179), [tutor.c:50](../tutor.c#L50), [utilgenui.c:1267](../utilgenui.c#L1267), [vcr.c:53](../vcr.c#L53) |
| `GetCurrentTime` | 2 | [utilgenui.c:441](../utilgenui.c#L441) |
| `MakeProcInstance` | 45 | [battleui.c:285](../battleui.c#L285), [buildui.c:13](../buildui.c#L13), [createui.c:41](../createui.c#L41), [fileui.c:29](../fileui.c#L29), [init.c:141](../init.c#L141), [mdi.c:926](../mdi.c#L926), [produceui.c:32](../produceui.c#L32), [raceui.c:14](../raceui.c#L14), [ship2ui.c:126](../ship2ui.c#L126), [shipui.c:768](../shipui.c#L768), [tutor.c:48](../tutor.c#L48), [utilgenui.c:1265](../utilgenui.c#L1265), [vcr.c:48](../vcr.c#L48) |
| `Yield` | 1 | [scan.c:73](../scan.c#L73) |

### Microsoft CRT dependencies

| Source symbol | Calls | Source locations |
| --- | ---: | --- |
| `_access` | 2 | [native.c:192](../native.c#L192) |
| `_mkdir` | 1 | [native.c:196](../native.c#L196) |
| `_strdate` | 2 | [init.c:588](../init.c#L588), [native.c:201](../native.c#L201) |
| `_stricmp` | 5 | [fileui.c:70](../fileui.c#L70), [tutor.c:2363](../tutor.c#L2363) |
| `_strnicmp` | 1 | [native.c:205](../native.c#L205) |
| `_strtime` | 1 | [native.c:202](../native.c#L202) |

## Compatibility contracts

### Function aliases and local adapters

The generic string and UI calls use narrow strings throughout this application. Preserve ANSI behavior initially and verify the preprocessor mapping against the MinGW headers when building. `CreateWindow`, `CreateDialog` and `DialogBox` are convenience macros over the corresponding `Ex` or `Param` APIs. Pointer-sized `GetWindowLongPtr` and `SetWindowLongPtr` hold subclass procedures and window data. Keep pointer widths in the UI while preserving all disk record widths.

`MakeProcInstance` and `FreeProcInstance` are inherited Win16 callback spellings; a native compatibility layer needs callback identity and lifetime rather than Win16 procedure thunks. `MakeProcInstance` and `FreeProcInstance` expand to their procedure argument in the installed MinGW headers; `Yield()` expands to nothing. `GetCurrentTime()` expands to `GetTickCount()`. `ExitWindows(dwReserved, Code)` expands to `ExitWindowsEx(EWX_LOGOFF, 0xFFFFFFFF)`, ignoring both arguments. These expansions were checked with `gcc -dM -E -include windows.h`. `ExitWindows` occurs in the host completion and shutdown paths in `mdi.c`.

`GetTextExtent` is an application helper in `nativeui.c`, not a Win32 function. It calls `GetTextExtentPoint32A` and packs width and height into 16-bit words. `GetCursorPos16`, `ScreenToClient16`, `ClientToScreen16` and `MapWindowPoints16` are inline application adapters in `nativeui.h`. Their underlying APIs are included above. Preserve signed 16-bit game coordinates at these boundaries.

`HfOpenFile`, `CbReadFile`, `CbWriteFile`, `LSeekFile`, `CbFileSize`, `CloseFile` and `DwTickCount` already provide the core's file and clock abstraction. Their POSIX implementations exist. `CchSprintf` and `LMulDiv` are portable application implementations, not Win32 calls; UI code still calls `MulDiv` and `wsprintf` directly; the latter imports `wsprintfA` and is included in the source inventory. `POINT16`, `qsort16` and these arithmetic and formatting rules must keep their current behavior.

### Message and control protocols

A message-compatible backend must implement more than `SendMessage`: dialog initialization, command notifications, focus, edit selection, list and combo contents, owner drawing, painting, timers, capture, menu state, keyboard accelerators and scroll ranges all depend on the existing protocols. Window procedures and dialog procedures return pointer-sized results; subclass procedures are saved and forwarded with `CallWindowProc`.

[WIN32-UI-SURFACE.csv](WIN32-UI-SURFACE.csv) lists message and notification constants, styles, expression macros and selected Windows types, with source references, including resources. This supporting list uses explicit type names and Windows constant prefixes; it is not an exhaustive catalog of every SDK constant. Resource identifiers matching those prefixes can be application-defined. `WM_STARS_*` and `WM_STARS_SYSCOMMAND` are application messages, not operating system APIs.

**Window messages:** `WM_ACTIVATE`, `WM_APP`, `WM_CAPTURECHANGED`, `WM_CHAR`, `WM_CLOSE`, `WM_COMMAND`, `WM_CREATE`, `WM_CTLCOLORBTN`, `WM_CTLCOLORDLG`, `WM_CTLCOLOREDIT`, `WM_CTLCOLORLISTBOX`, `WM_CTLCOLORMSGBOX`, `WM_CTLCOLORSCROLLBAR`, `WM_CTLCOLORSTATIC`, `WM_DESTROY`, `WM_DRAWITEM`, `WM_ENTERIDLE`, `WM_ERASEBKGND`, `WM_GETMINMAXINFO`, `WM_HSCROLL`, `WM_INITDIALOG`, `WM_INITMENU`, `WM_KEYDOWN`, `WM_KEYUP`, `WM_LBUTTONDBLCLK`, `WM_LBUTTONDOWN`, `WM_LBUTTONUP`, `WM_MBUTTONDOWN`, `WM_MDIACTIVATE`, `WM_MEASUREITEM`, `WM_MOUSEHWHEEL`, `WM_MOUSEMOVE`, `WM_MOUSEWHEEL`, `WM_NULL`, `WM_PAINT`, `WM_PALETTECHANGED`, `WM_QUERYDRAGICON`, `WM_QUERYNEWPALETTE`, `WM_RBUTTONDOWN`, `WM_RBUTTONUP`, `WM_SETCURSOR`, `WM_SETFONT`, `WM_SIZE`, `WM_STARS_CONTINUE`, `WM_STARS_HOST`, `WM_STARS_STARTUP`, `WM_STARS_SYSCOMMAND`, `WM_SYSCOLORCHANGE`, `WM_SYSCOMMAND`, `WM_TIMER`, `WM_VSCROLL`, `WM_WININICHANGE`.

**Combo box messages:** `CB_ADDSTRING`, `CB_DELETESTRING`, `CB_GETCOUNT`, `CB_GETCURSEL`, `CB_GETLBTEXT`, `CB_RESETCONTENT`, `CB_SETCURSEL`, `CB_SETEXTENDEDUI`.

**List box messages:** `LB_ADDSTRING`, `LB_GETCURSEL`, `LB_GETSEL`, `LB_GETTEXT`, `LB_RESETCONTENT`, `LB_SETCURSEL`, `LB_SETSEL`, `LB_SETTOPINDEX`.

**Edit messages:** `EM_GETSEL`, `EM_LIMITTEXT`, `EM_SETSEL`.

**Button messages:** `BM_GETCHECK`, `BM_SETCHECK`.

### Rendering resources and printing

GDI compatibility needs persistent DC state, selected objects, clip regions, raster operations, text measurements and backgrounds, palette selection and realization, DIB color tables and memory-backed bitmaps. A simple drawing-function translation will miss state and ownership dependencies. `scan.c` is the main stress case for clipping, scrolling, coordinate origins and XOR drawing; `utilgenui.c` contains the DIB and palette helpers.

`res/stars.rc` defines dialogs, menus, accelerators, bitmaps, icons, cursors and embedded tutorial game files. It uses code page 1252. A replacement resource loader must preserve IDs, control IDs, dialog styles, accelerator bindings and access to embedded binary data. Standard controls are mixed with application windows and subclasses; retaining their message behavior is a separate requirement from parsing dialog templates.

Printing uses `PrintDlg`, a printer DC and `Escape` in `mdi.c`, including numeric escape values 10, 1 and 11. Treat the printing path as a separate backend feature and validate its document and page lifecycle. `WinHelp` is called throughout dialogs; help navigation also needs an explicit backend decision.

### Existing platform boundary

`STARS_CORE_SOURCES` and `STARS_UI_SOURCES` in `CMakeLists.txt` identify the separation. Core code includes `common.h`; Windows UI code includes `win.h`. `ui.h` is implemented by both the Windows UI and `hostui.c`. A replacement UI should reuse the core rather than add Windows headers to it. `native.h` already supplies a small set of Windows-compatible constants, handle declarations and `RECT` for the portable core.

Keep `structs.h` disk layouts, `POINT16`, record sizes, saved hull and part numbers and the native regression baseline unchanged during compatibility work. Handle and pointer sizes must never enter saved records. The tutorial observer depends on existing procedures and globals and must be updated with any affected UI restructuring.

## Test infrastructure

[WIN32-TEST-APIS.csv](WIN32-TEST-APIS.csv) separately lists matched Windows and Microsoft CRT calls in test sources and headers, including the vendored acutest header. Conditional test code is included even when disabled for a normal build. These dependencies are not part of the 178 application function spellings.

Additional spellings used only in tests or the tutorial observer:

- `CloseHandle`: [tests/unit/acutest.h:1259](../tests/unit/acutest.h#L1259)
- `CreateDialogParamA`: [tests/unit/test_battle.c:17](../tests/unit/test_battle.c#L17), [tests/unit/test_race.c:103](../tests/unit/test_race.c#L103)
- `CreateProcessA`: [tests/unit/acutest.h:1256](../tests/unit/acutest.h#L1256)
- `CreateWindowExA`: [tests/scaffold/tutorial/observer.c:257](../tests/scaffold/tutorial/observer.c#L257)
- `DefWindowProcA`: [tests/scaffold/tutorial/observer.c:241](../tests/scaffold/tutorial/observer.c#L241)
- `DeleteFileA`: [tests/unit/stars_test.c:57](../tests/unit/stars_test.c#L57)
- `ExitProcess`: [tests/scaffold/tutorial/observer.c:33](../tests/scaffold/tutorial/observer.c#L33)
- `FindClose`: [tests/unit/stars_test.c:59](../tests/unit/stars_test.c#L59)
- `FindFirstFileA`: [tests/unit/stars_test.c:53](../tests/unit/stars_test.c#L53)
- `FindNextFileA`: [tests/unit/stars_test.c:58](../tests/unit/stars_test.c#L58)
- `FindWindowA`: [tests/scaffold/tutorial/observer.c:123](../tests/scaffold/tutorial/observer.c#L123)
- `GetConsoleScreenBufferInfo`: [tests/unit/acutest.h:573](../tests/unit/acutest.h#L573)
- `GetCurrentProcessId`: [tests/scaffold/tutorial/observer.c:54](../tests/scaffold/tutorial/observer.c#L54)
- `GetExitCodeProcess`: [tests/unit/acutest.h:1258](../tests/unit/acutest.h#L1258)
- `GetLastError`: [tests/unit/acutest.h:1282](../tests/unit/acutest.h#L1282)
- `GetMenuItemRect`: [tests/scaffold/tutorial/observer.c:130](../tests/scaffold/tutorial/observer.c#L130)
- `GetMenuStringA`: [tests/scaffold/tutorial/observer.c:129](../tests/scaffold/tutorial/observer.c#L129)
- `GetModuleHandle`: [tests/scaffold/regression_trace.c:44](../tests/scaffold/regression_trace.c#L44), [tests/unit/stars_test.c:27](../tests/unit/stars_test.c#L27)
- `GetStdHandle`: [tests/unit/acutest.h:572](../tests/unit/acutest.h#L572)
- `GetTopWindow`: [tests/scaffold/tutorial/observer.c:52](../tests/scaffold/tutorial/observer.c#L52)
- `GetWindowTextA`: [tests/scaffold/tutorial/observer.c:57](../tests/scaffold/tutorial/observer.c#L57)
- `IsDebuggerPresent`: [tests/unit/acutest.h:1744](../tests/unit/acutest.h#L1744)
- `QueryPerformanceCounter`: [tests/unit/acutest.h:458](../tests/unit/acutest.h#L458)
- `QueryPerformanceFrequency`: [tests/unit/acutest.h:456](../tests/unit/acutest.h#L456)
- `RegisterClassA`: [tests/scaffold/tutorial/observer.c:253](../tests/scaffold/tutorial/observer.c#L253)
- `SetConsoleTextAttribute`: [tests/unit/acutest.h:602](../tests/unit/acutest.h#L602)
- `SetUnhandledExceptionFilter`: [tests/unit/acutest.h:1794](../tests/unit/acutest.h#L1794)
- `SetWindowTextA`: [tests/scaffold/tutorial/observer.c:231](../tests/scaffold/tutorial/observer.c#L231)
- `WaitForSingleObject`: [tests/unit/acutest.h:1257](../tests/unit/acutest.h#L1257)

## Verification and follow-up

The source scan strips comments and literals, identifies call expressions, groups Windows and CRT spellings, and checks remaining uppercase calls against repository declarations. The unmatched application expressions are packing and input macros, `StarsLongJump`, and the application message definition. The location CSVs can be checked directly against the corresponding source lines.

The MinGW Debug configure and build passed with no compiler warnings in the build output. No runtime code, baselines or original fixtures were changed. `make regression` passed all nine scenarios through turn 150: 519 matches, including 22 warnings for differences confined to unused storage, and zero differences/errors or skipped reference inputs. The comparison report is `dist/scaffold/regression/comparison.json`. Go 1.27.2 was loaded from the updated `.profile` to build `tests/savecli`.

The import table was extracted with `x86_64-w64-mingw32-objdump -p dist/mingw-debug/bin/stars.exe`. It contains 255 entries: 106 from USER32, 42 from GDI32, 43 from KERNEL32, 3 from comdlg32 and 61 from msvcrt. All application function spellings map to imports, either directly or through ANSI aliases; `ExitWindows` and `GetCurrentTime` also map to imports. The three procedure and yield macros have no imports. This cross-check corrected the initial omission of `wsprintf` from the source scan.

The classification of additional imports as runtime or compiler support means they have no matching direct application call spelling. Some CRT imports also support ordinary C functions used by the application. A native source compatibility layer does not need to reproduce the MinGW executable startup machinery.

The tutorial-instrumented Debug build also passed. Its additional imports are: `ExitProcess`, `FindWindowA`, `GetCurrentProcessId`, `GetMenuItemRect`, `GetMenuStringA`, `GetTopWindow`. These correspond to the separately inventoried observer dependencies.

`make test-unit` passed all 13 Windows unit tests under Wine outside the sandbox. The sandboxed attempt terminated with SIGSYS before tests could execute. The tutorial walkthrough failed at year 2406, page 21, instruction 166 ("Now shift click back on Stove Top"): the instruction did not advance after two attempts. It completed 107 actions through 21 pages. Artifacts are in `dist/scaffold/tutorial/20261010T061104.602896Z/`; no source fix was attempted as part of this inventory.

## Compatibility extensions after the inventory

The toolbar migration adds `InitCommonControlsEx` (`ICC_BAR_CLASSES`) and a
scoped `commctrl.h` interface. The original 178-function inventory remains the
baseline for progress reporting: all 178 have basic implementations. Toolbar
messages, structures, notifications and this additional initializer are covered
by [standard toolbar tests](WIN32-TOOLBARS.md), including an independent MinGW /
Windows reference build. This baseline percentage does not measure complete
Win32 or common-control behavioral conformance.
