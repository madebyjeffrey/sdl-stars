# C05 linked witness accounting

Parent: `b12e172` (PR #70), compared with the unchanged C03 witness receipts.
This is the per-program qualification inventory for [Milestone C](CPP-MILESTONE-C.md).
The [machine-readable receipt](CPP-C05-WITNESSES.json) records every original
case count, assertion count, named exclusion/count, independent-oracle AST hash,
linked executable, fixture and specification evidence owner.

All 27 commands retain their `tests/spec/check_*_source.py` names for existing
links. They run normal CMake targets; the scripts contain no production-source
slicing, substitute record declarations or rewritten implementation bodies.
The 67 retained independent Python functions/classes have identical ASTs to the
parent. Existing C++ test-side vectors/assertions use actual application headers,
owned state and linked functions. No baseline, canonical Dafny rule or expected
result was regenerated. Assertion macros remain active in Release fixtures.

| Original program | Linked fixture / CTest | Retained receipt |
| --- | --- | --- |
| [colony_capabilities](../tests/spec/check_colony_capabilities_source.py) | [witness_colony-capabilities](../tests/witnesses/colony_capabilities.cpp) | Colony capabilities: 1756689 cases (count 1253964, gate 405480, scanner 15982, generic 81263) plus four Produce clipping/application cases; 497240245 assertions passed |
| [construction_buildability](../tests/spec/check_construction_buildability_source.py) | [witness_construction-buildability](../tests/witnesses/construction_buildability.cpp) | Construction actual eligibility integration: 179072 cases plus post-charge Produce/fleet-limit case; 43707827 assertions passed |
| [construction_effects](../tests/spec/check_construction_effects_source.py) | [witness_construction-effects](../tests/witnesses/construction_effects.cpp) | Construction effects linked fixtures: 1897437 assertions passed |
| [design_buildability](../tests/spec/check_design_buildability_source.py) | [witness_design-buildability](../tests/witnesses/design_buildability.cpp) | Native design buildability: 299402 cases, 78741602 assertions passed; actual lookup/race/technology/catalog composition, order, restoration and SHDEF/PLAYER/GAME frames checked |
| [host_production](../tests/spec/check_host_production_source.py) | [witness_host-production](../tests/witnesses/host_production.cpp) | M09 host production source: 1362708 assertions passed |
| [host_research](../tests/spec/check_host_research_source.py) | [witness_host-research](../tests/witnesses/host_research.cpp) | Native host research: 11045 cases, 2772295 assertions passed; Native actual catalog terminators: 991759 assertions passed, all 16 counts below byte wrap |
| [library_publication](../tests/spec/check_library_publication_source.py) | [witness_library-publication](../tests/witnesses/library_publication.cpp) |  |
| [mass_driver](../tests/spec/check_mass_driver_source.py) | [witness_mass-driver](../tests/witnesses/mass_driver.cpp) | Mass driver source: 488905 defined standalone cases; 55494 construction/packet cases; 4392987 assertions passed |
| [mining](../tests/spec/check_mining_source.py) | [witness_mining](../tests/witnesses/mining.cpp) | Native mining/depletion: 221240 cases, 4686320 assertions passed |
| [native_colony](../tests/spec/check_native_colony_source.py) | [witness_native-colony](../tests/witnesses/native_colony.cpp) | Native colony source differential: 239344 cases, 1331470 assertions passed |
| [part_availability](../tests/spec/check_part_availability_source.py) | [witness_part-availability](../tests/witnesses/part_availability.cpp) | Prepared 2047760 cases using 239 actual catalog rows; 116 model identities and native enum/bit protocols bound; Native part availability: 2047760 cases, 32854889 assertions passed; pointers, query order and PLAYER/GAME/global frames checked |
| [planet_editor](../tests/spec/check_planet_editor_source.py) | [witness_planet-editor](../tests/witnesses/planet_editor.cpp) | S06 planet copy / M09 completion linked witnesses: 17432153 passed |
| [production_builder](../tests/spec/check_production_builder_source.py) | [witness_production-builder](../tests/witnesses/production_builder.cpp) | M09 builder source differential: 68307 cases, 751377 assertions passed |
| [production_cost](../tests/spec/check_production_cost_source.py) | [witness_production-cost](../tests/witnesses/production_cost.cpp) | M09 production cost linked witnesses: 1495512 passed |
| [production_editor](../tests/spec/check_production_editor_source.py) | [witness_production-editor](../tests/witnesses/production_editor.cpp) | M09 linked editor witnesses: 19415 assertions passed |
| [production_forecast](../tests/spec/check_production_forecast_source.py) | [witness_production-forecast](../tests/witnesses/production_forecast.cpp) | M09 forecast/build/mining linked witnesses: 272841 passed |
| [production_lifecycle](../tests/spec/check_production_lifecycle_source.py) | [witness_production-lifecycle](../tests/witnesses/production_lifecycle.cpp) | M09 lifecycle/M07 starbase linked witnesses: 16243 passed |
| [production_presets](../tests/spec/check_production_presets_source.py) | [witness_production-presets](../tests/witnesses/production_presets.cpp) | M09 linked preset witnesses: 13140 assertions passed |
| [research](../tests/spec/check_research_source.py) | [witness_research](../tests/witnesses/research.cpp) | Native research quotes/preview: 120992 cases, 2654102 assertions passed |
| [restoration](../tests/spec/check_restoration_source.py) | [witness_restoration](../tests/witnesses/restoration.cpp) | Restoration, lookup and message source witnesses: 6512 assertions passed |
| [s02_io](../tests/spec/check_s02_io_source.py) | [witness_s02-io](../tests/witnesses/s02_io.cpp) | S02 profile I/O and host source witnesses: 25 passed |
| [s02_presets](../tests/spec/check_s02_presets_source.py) | [witness_s02-presets](../tests/witnesses/s02_presets.cpp) | S02 linked preset witnesses: 131086 assertions passed (including all 65536 packed production words) |
| [s02_save](../tests/spec/check_s02_save_source.py) | [win32_profile_save](../tests/compat/profile_save.cpp) | S02 linked save-control witnesses: 42 passed |
| [s02](../tests/spec/check_s02_source.py) | [witness_s02-layout](../tests/witnesses/s02_layout.cpp) | S02 linked C witnesses: 21 passed |
| [s02_state](../tests/spec/check_s02_state_source.py) | [witness_s02-state](../tests/witnesses/s02_state.cpp) | S02 linked state witnesses: 42 passed |
| [terraform](../tests/spec/check_terraform_source.py) | [witness_terraform](../tests/witnesses/terraform.cpp) | Terraform selection: 1423925 cases, 303296026 assertions passed |
| [transport_presets](../tests/spec/check_transport_presets_source.py) | [witness_transport-presets](../tests/witnesses/transport_presets.cpp) | M10 linked transport witnesses: 131593 assertions passed |

## Production seams and continuing limits

- Preference grammar blocks live once in [preferencesui.cpp](../src/ui/preferencesui.cpp)
  and [host_profile.cpp](../src/core/host_profile.cpp). The fixture supplies profile
  input/localization and uses real `INI`, `TILE`, preset and MRU state/storage.
- Numeric/economy tests retain real catalog/race/planet/design/queue layouts and
  query order. Observers delegate to original functions; declared quotes,
  resource forecasts, allocations and object/message effects remain scripted.
  [Planet conversion receipts](../src/core/planet_query_io.h) observe the existing
  binary64-to-int32 boundary without replacing its input or result. The original
  defined domains and every overflow, pointer, division and table exclusion remain.
- Queue fixtures link actual list/lifetime, builder, forecast and construction
  code. [Heap services](../src/core/memory.h) supply allocation/extent and byte-copy
  effects; there is no copied heap/list algorithm. Failure receipts use ordinary
  exceptions rather than jumping across C++ scopes, with the same partial state.
- UI fixtures invoke actual production/transport handlers. Private helpers in
  [productionpresets.h](../src/ui/productionpresets.h),
  [transportpresets.h](../src/ui/transportpresets.h) and
  [sessionrestore.h](../src/ui/sessionrestore.h) hold blocks moved once from their
  callers. [UI fixture adapters](../tests/witnesses/ui_fixture.h) convert only
  scripted scalar control IDs/signatures; they do not reconstruct game records.
- [Library publication](../src/ui/librarypublication.h) accepts a borrowed view
  of the real 256-byte path buffers and the actual admission record. Controllers
  run with the original scripted filesystem/helper receipts. Its companion copy
  test uses real `NATIVEFILE` handles and an independent stdio byte oracle;
  reads/writes/close/cleanup faults bind typed services, without stdio-rewriting
  the production body. Admission, recursive helpers, crash durability, full GUI
  rendering and whole-program refinement retain their previous coverage limits.

CMake registers 26 witness CTests plus the existing `win32_profile_save`.
`stars-linked-witnesses` builds them and the library-copy companion from normal
libraries. The automatic linked-witness CI job runs the original 27 commands
under ASan/UBSan/LSan. The independent models remain bounded evidence rather
than a proof that the whole application refines the Dafny collection.
