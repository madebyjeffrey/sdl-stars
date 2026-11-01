# M04 — Opening the six race pages

Historical evidence: inspected C11 source at `e3222eb1fbdb01d68067231b5d5ef14a71759b30`.
The retained fixtures run only against the frozen snapshot; current C++ refinement remains open.

Observed 2026-10-27 at `e3222eb1fbdb01d68067231b5d5ef14a71759b30`, with the
working collection. This companion specializes [M04-US01](M04-race-design.md#m04-us01),
[US02](M04-race-design.md#m04-us02) and [US03](M04-race-design.md#m04-us03).
It specifies all six `WM_INITDIALOG` arms, their ordinary state changes, control
requests and habitat layout. [Page commands](M04-page-commands.md) own later
commits/settings; the [controller](M04-wizard-controller.md) owns page dispatch
and completion. M04-AC26–34 remain at the module entry. Runtime C, game behavior,
catalogs, file formats and regression baselines are unchanged.

Canonical model: [M04PageInitialization](dafny/M04-page-initialization.dfy),
reusing native byte/portrait/stat adapters, S01 signed narrowing/division,
M01's signed-32 rectangle vocabulary and existing M04 page numbering.
Source: [raceui.c](https://github.com/madebyjeffrey/sdl-stars/blob/e3222eb1fbdb01d68067231b5d5ef14a71759b30/src/ui/raceui.c) `RaceWizardDlg1`…`RaceWizardDlg6`,
`SetRCWTitle`; [nativeui.c](https://github.com/madebyjeffrey/sdl-stars/blob/e3222eb1fbdb01d68067231b5d5ef14a71759b30/src/ui/nativeui.c) `FSetUtf8WindowText`;
[race.c](https://github.com/madebyjeffrey/sdl-stars/blob/e3222eb1fbdb01d68067231b5d5ef14a71759b30/src/core/race.c) `GetRaceStat`, `GetRaceGrbit`;
[textids](../text/textgen.py) and [race fields](../src/core/structs.h).
Executed checks are in the [collection record](README.md#race-page-initialization-continuation).

## State, interface and ordinary-return scope

`State` embeds the full focused command/native state plus 17 rectangles, their
known initialized slots, raw rectangle count and habitat explanation cache.
`Shape` is an object/array projection, not proof of native pointers or live
windows. Services must return normally without reentrant state mutation. API
calls are modeled as requests; ignored API results do not become successful
widget creation, selection, text assignment or positioning claims.

`Input` supplies tutorial/player state, the default-name control's bounded read,
title base ID and native geometry/text metrics. `Identity` separately requires
the initial name fields to contain a reached NUL. Only a nonzero salt causes
the password buffer to be read as text, so an unused unterminated password is
not rejected when salt is zero. Model `InvalidEvidence` identifies a missing
native-domain/receipt obligation; it is not a new C error or validation branch.

All pages request the title and return callback TRUE. `TitleId` adds the **raw
signed-16** read-only flag to the supplied title base and narrows to StringId;
it does not normalize every nonzero mode to one. The source then formats with
the active panel, which is independent of the callback number. The model keeps
lookup/format/set order. S03/S04 own actual catalog/template/formatter extraction,
the reached 50-byte local and 360-byte work-buffer capacities, and text API
qualification. The later [producer contract](S03-player-labels-and-race-titles.md)
binds current title/grammar/default catalog rows and both actual format helpers;
other raw-mode lookups and native text API effects remain explicit boundaries.
This initialization fixture itself supplies bounded integer-format templates.

## Identity page and preset recognition

Initialization first requests the current singular/plural in the controls.
When singular byte zero is empty, it reads the Humanoid button text with
capacity **16**, writes that prefix and NUL into the 32-byte native name field,
retains later bytes and requests the new text. The plural remains current.
Sticky placement runs next, before preset recognition.

Ordinary recognition tries all seven presets in order. For each comparison it
temporarily assigns that preset's portrait, then compares exactly native bytes
0…127. The first match wins, otherwise Custom is requested. The original
portrait is restored on every ordinary completion of the search. Names at
128…191 do not affect the comparison; credentials, flags, runtime and unused
prefix bytes do. This is not the race library's comparison that clears Learned
and compares names separately. `Preset` and `PresetIgnoresNames` preserve this
specific boundary.

Tutorial + player zero + any nonzero read-only flag bypasses comparison and
requests Humanoid even if the record differs. It does not replace the design or
portrait. `IdentityKeepsPortrait` covers both paths. The three edit limits are
15; actual UTF-8 control enforcement remains S05/S04.

The leftover combo requests exactly **five** catalog additions, Surface Minerals
through Defenses, then requests the current signed-byte stat as its selection.
That stat's existing clamp range is 0…6, so a request for 5/6 is possible even
though those indices have no item after five successful additions. The
[combo mapping](https://github.com/madebyjeffrey/sdl-stars/blob/e3222eb1fbdb01d68067231b5d5ef14a71759b30/compat/win32/src/combo.c) and
[list selection](https://github.com/madebyjeffrey/sdl-stars/blob/e3222eb1fbdb01d68067231b5d5ef14a71759b30/compat/win32/src/list.c) assign -1 to an out-of-range index;
the fixture checks that ordinary five-item projection. Ignored addition results,
missing controls and actual native allocation/event effects remain S05 work.
The model stores the request, not a guaranteed selection. A subsequent commit
uses its independent CB_GETCURSEL receipt; CB_ERR narrows/clamps to zero under
M04-AC19. The specification preserves the five-item request loop and existing stat clamp.

Nonzero salt requests the existing password text; zero salt skips that request.
Read-only mode disables the two identity controls, plural, eight preset/Custom
controls and combo in source order. It does not disable Next here. In particular,
recognizing Random on entry preserves Next's prior/resource state, while a later
Random click explicitly disables it. Initialization does not copy a preset or
apply the Custom Random-bit clearing used by a later commit.

## Other page controls

| Visible page / callback | Initialization requests and state |
| --- | --- |
| 2 / RaceWizardDlg4 | Request primary radio at 271 + raw signed-byte stat; disable ten primary controls in read-only mode; save sticky placement. No clamping, AR resets or race mutation occur here. |
| 3 / RaceWizardDlg5 | Set description index zero, then for all fourteen traits request localized label, current bit check and optional read-only disable. Save sticky placement. |
| 4 / RaceWizardDlg2 | Set habitat cache -1, acquire DC/client/font/metrics, write all 17 habitat rectangles, place/check immunity controls, set count 17, restore font/release DC, optionally disable immunity controls, then save sticky placement. |
| 5 / RaceWizardDlg3 | Request cheap-factory checkbox from bit 31. Disable it for read-only **or AR**; save sticky placement. Do not reset factory/mine values here. |
| 6 / RaceWizardDlg6 | Request six research radio selections from raw stats; optionally disable eighteen radios. Request higher-tech label with 4 for Jack of All Trades, otherwise 3, then bit-29 check and optional disable. Save sticky placement. |

`OtherPagesKeepRace` proves that pages other than identity do not directly modify
the focused native race/password/binding groups during these ordinary arms.
`NonHabitatKeepsRectangles` records that the other five initializers leave the
rectangle array, initialized-slot knowledge and count current. In particular,
the economy page does not establish fresh hit rectangles/count on entry; its
paint routine remains the producer. This does not qualify a mouse event before
paint. GAP-26 retains that lifecycle work.

## Habitat geometry and native widths

`Layout` produces five rectangles per axis plus two growth arrows. It preserves
each source boundary: text extents use unsigned LOWORD, label/unit/middle/top
locals narrow to signed 16 bits, and height is C division of the already narrowed
`3 * font`. Row offsets and growth top use truncation toward zero for `font/2`;
the growth-arrow bottom instead uses the supported native arithmetic right shift,
modeled by floor division for negative fonts. These operations are distinct.

`GeometrySafe` checks the two reached signed-32 client-width subtraction
intermediates before narrowing. Other coordinate expressions use bounded
signed-16/unsigned-16 operands; `LayoutFits` proves all 17 final rectangles fit
signed 32 bits. Zero/negative dimensions and inverted rectangles are not silently
clamped. The model records SetWindowPos requests without claiming successful
positioning or a usable display. Native APIs must have supplied the reached
client rectangle/metrics and live 17-slot storage. Localized Temperature text
must fit the reached 20-byte buffer; S03/S04 own catalog/text extraction.

Checkbox placement follows axis order, then growth text is measured. C leaves
the order of the two GetTextExtent calls within the growth-label addition
unspecified; `growthFirst` preserves either order. Count is assigned only after
all geometry writes. The original font is then restored and the DC released.
This establishes ordinary initialization order, not every native handle/resource
failure, cancellation, callback or paint lifetime.

## Interaction, quality, evidence and remaining work

M04-IR01/02 and QR01–03 remain authoritative. Initialization supplies labels,
selection/check requests and byte limits; it does not establish keyboard focus,
accessibility, successful controls, readable layout or an agreed latency target.
S05 owns those native checks. M02/S01 retain persistence and credential ownership;
the controller retains modal completion and caller snapshot handoffs.

The [source fixture](../tests/spec/check_race_initialization_source.py) compiles
the actual six initialization arms, title function, UTF-8 window-text wrapper
and stat/bit helpers using native PLAYER layout and fresh IDs. An independent
oracle checks **1,126 cases**: all presets/Custom, tutorial/player/raw read-only
branches, name/password conditional reads and tails, five-item selection,
control request order, AR/research labels and every rectangle under positive,
zero, negative and narrowing-boundary metrics. It permits only the source's two
growth-extent evaluation orders. Warnings are errors except a local diagnostic
suppression around SetRCWTitle's existing assigned-but-unused `cch`; its source
body is preserved. Address/undefined-behavior checks are enabled. Catalog/text,
metrics, windows and ordinary UI service replies are scripted; this does not
execute complete native dialogs or qualify all displays.

[Pointer/message contracts](M04-pointer-and-message-interactions.md) now specify
mouse/cursor/tracking and color/erase arms. Remaining D/A/V work is full painting,
complete points/drawing and native widget/state extraction.
[Player-name and title producers](S03-player-labels-and-race-titles.md) now have
checked contracts and actual generated-text/formatter fixtures; complete catalog
and formatting grammar remain with S03/S04. GAP-26/27 and open policy/quality decisions retain their owners. The
initialization contract does not complete the full six-page interaction audit.
