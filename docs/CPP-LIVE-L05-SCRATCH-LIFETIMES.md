# L05 scratch and borrowed-view lifetime audit

**Source reviewed:** `develop` at `d9ce84ab` (2026-11-03), plus the lifetime comments in the current working tree.

This audit follows production borrowed text and byte views to their consuming operation. A view is safe across a modal pump only when its backing owner remains alive and unchanged for the entire pump, or when the consumer copies it before entering the pump. App/session-owned storage is not valid after its session is destroyed. No reviewed view is retained beyond its stated owner.

## Returned views and spans

| Producer | Backing owner | Validity boundary and final-consumer behavior |
|---|---|---|
| `PszGetLine` (`src/core/utilgen.cpp`) | The caller's input buffer; production parser input is the local `std::vector<std::byte>` in `CreateUniverse`. | Each line is consumed synchronously during parsing. Parsing copies fields into records/locals before advancing; no line view is saved across callbacks or modal UI. The backing vector remains alive for the parse. |
| `PszCurrentLibraryRaceName` (`src/ui/libraryui.cpp`) | `APPUISTATE::szRaceLibraryPath` in the supplied `APPSESSION`. | Callers either format/copy the basename immediately or copy it into the caller's name span after `RaceCreationWizard` returns. The view is not kept across a modal loop. It expires if the path changes or the session is torn down. |
| `FNativeEnumerateDirectory` callback entry (`src/platform/native.cpp`) | The enumerator's current `std::optional<std::string> szEntry`. | The name is valid through the synchronous callback only. Current callbacks compare it, construct an operation-local path, or copy it into UI/menu/list storage. None retains it. A callback may run a modal loop while the view remains valid, but must copy before returning if it needs the name afterward. |
| `SzTrimProfile` / `SzvProfileValue` (`compat/win32/src/profile.cpp`) | `COMPATPROFILE::szParsed`, owned by the profile node. The node list owns it until profile release. | The profile-string API reads the view and copies to the caller buffer while holding the profile lock; profile nodes are released after that copy. No view escapes the call or profile teardown. |
| `NATIVEBYTEWINDOW::Rgb` and typed log/record views (`src/platform/byteview.h`, `src/core/*views.h`) | The constructor-supplied byte span, generally an app heap block or record buffer. The adapter itself is non-owning. | Reads/writes stay within synchronous operation scopes. Views must not outlive or overlap relocation/free of the source block. They are not stored over callbacks, modal loops, or session teardown. |
| `NATIVEPARTCOMMONVIEW::szName` / `SzvPartName` (`src/core/partviews.h`) | Fixed name field in the selected hull/part catalog record. The short-lived wrapper snapshots the selected record pointer and kind. | Consumers compare, format, append, or synchronously pass the bounded field to `SendMessage(CB_ADDSTRING)`, which copies it. No name view is retained across catalog replacement, callbacks, modal UI, or teardown. |
| `RgMemStreamRecord` (`src/core/file.cpp`) | The current `app.state.vlpMemStream` span, itself backed by a memory stream/resource. | Consumers query its size or synchronously copy/advance the stream. Log replay resets the app-held stream span on completion/error. No record view escapes the replay operation or session. |
| `RgbOrderStorage` (`src/core/memory.cpp`) | The order's heap block, addressed through `PLORD`. | Its two callers read or write immediately within the operation and do not invoke UI callbacks or retain the span. It expires if the underlying heap block is freed or relocated. |
| `LpbLoadTutorLog` (`src/ui/logui.cpp`) | Program resource storage; the host implementation returns an empty span. | Log replay stores the span in app state only for replay and resets it at completion/error. The resource itself lasts for the process; the app-held reference does not survive session teardown. |
| `SzvControlKind` (`compat/win32/src/inspect.cpp`) | Static literals / `static constexpr` table entries. | Process-lifetime storage; consumed synchronously by inspector output. |
| `REPORTTEXTRESULT::view` and other owned text result views (`src/core/report.h`, `util.h`, `planet.h`, and related result headers) | The fixed `std::array` inside the result value. | The view lasts only while that result object lives. Stored UI values keep the result object locally; direct temporary `.view()` calls consume it in the same full expression. Report sorting copies the result arrays before the next lookup. |
| Formatter `string_view` arguments (`src/platform/nativeformat.*`) | Caller-owned strings, catalog storage, or local scratch passed as a formatter argument. | The formatter consumes all fields synchronously and retains no views. No argument crosses a callback or modal pump inside the formatter. |

## Scratch owners and modal boundaries

| Scratch owner | Lifetime / handoff |
|---|---|
| `APPSESSION::state.szWork` and `szBase` | Shared per-session scratch, valid only until the next use that writes the same field or session teardown. Reviewed view uses are synchronous. Values needed by later sorting/drawing are copied into owned result arrays; cached paths/names use dedicated `APPUISTATE` arrays. |
| `APPUISTATE` race/game library paths, roots, prompted names, pending path, exchange basename, and new-race filename array | Session owner. The existing eleven path/name records are itemized in `ui_scratch_storage` in `CPP-LIVE-CLEANUP.json`; they are reset at wizard entry or session shutdown as appropriate. Path/name borrows are consumed before mutation or copied to operation-local storage. The pending path intentionally remains through game generation within the session. |
| `FILESELECTIONREQUEST` / `FILESELECTIONRESULT` (`compat/win32/include/file_selection_types.h`) | Request owns title, paths, extensions, and filter strings; the synchronous chooser keeps the request alive on its stack through the modal pump. A selected result owns its path. No borrowed view is used to represent chooser state across that pump. |
| Popup label `optional<string_view>` arrays (`src/ui/popup.cpp` and its callers in report, ship, mine, production, toolbar, and VCR UI) | Caller owns the backing local arrays, result objects, or session fields until `PopupMenu` returns. `PopupMenu` escapes/copies every label into menu-owned text before calling `TrackPopupMenu`; it does not read the input views during the modal tracking loop. |
| New-game race list and library list callbacks | Enumeration callback uses the entry view only while the enumerator owns its `std::string`; UI menu/list APIs copy labels during insertion. Later modal selection reads menu/list-owned copies. |
| Operation-local paths and staging arrays in library flows | The stack owner remains alive through each synchronous filesystem/chooser call. When a path is needed after a modal chooser returns, it is held in an owned request/session/local `std::string` or fixed array, not a view into destroyed callback scratch. |

## Report-label copy check

`SzReportText` in `src/core/report.cpp` copies the input view with `CchCopyUtf8Prefix` into `REPORTTEXTRESULT::sz`, storing the resulting byte count. `SzReportLabel` delegates to that copy. `PszGetDestName`, `PszGetETA`, and `PszGetTaskName` route catalog text, app scratch (`szWork` / `szDblDash`), game fields, and local location buffers through these helpers before returning. Therefore callers receive owned arrays rather than views into scratch, catalog storage, or a temporary. The result's `.view()` is a borrow of its own array and is used only while the result object is alive.

## Findings

- No dangling production `string_view` or `span` return was found in the reviewed interfaces and call paths.
- No reviewed view is retained across a callback return, modal loop, or app/session teardown. Where a view can be supplied to callback/UI code, the source owner remains alive through the synchronous operation or the receiver copies the bytes before returning/entering modal tracking.
- Report-label helpers already copy at the producer boundary, including complete UTF-8 prefixes. No source behavior fix was indicated.
- Added explicit lifetime contracts to `FNativeEnumerateDirectory`, `PszGetLine`, `PszCurrentLibraryRaceName`, and `REPORTTEXTRESULT::view` declarations.

This is a static ownership audit; no build, unit, regression, or tutorial suite was run for this documentation/comment-only batch.
