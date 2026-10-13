# Heap handles and timers

The SDL runtime now implements the seven inventoried global/local heap calls
and both timer calls. Coverage is **76/178 inventoried Win32 APIs (43%)**.
This measures tested application subsets, not all behavior of the Windows API.

## Memory

Fixed allocations are pointers; movable allocations are opaque handles with
separate storage, lock counts and discarded states. Resizing preserves data,
`GMEM_ZEROINIT` clears newly added bytes, and last-error storage is local to
each thread. Size, flag and handle queries and the corresponding local-memory
calls are available even where they are outside the original inventory.

Memory is protected by a registry lock and can be allocated from worker
threads. Locked/fixed growth without permission to move can fail rather than
relocate a live pointer. Immutable resource handles remain a separate type.
The runtime does not implement heap compaction or actual memory pressure
discarding; obsolete allocation hints do not schedule either operation.

## Timers

Window timers are keyed by window and ID; independent windows can use the
same ID. Thread timers have generated IDs. Replacing a timer restarts its
deadline. Intervals are clamped to the Windows timer bounds. Expired timers
are materialized by the message pump after posted messages and painting,
with one outstanding message per timer. `GetMessage` and `WaitMessage` wake
at the next applicable deadline. Callbacks run through `DispatchMessage`,
on the UI thread, rather than on an SDL worker thread.

Killing a timer prevents future expiration but does not retract an already
queued message. Destroying a window removes its timers and queued messages.
There is no claim of matching Windows scheduling latency or every interaction
between timer synthesis, message filters and removal.

## Build and verification

No additional dependencies are needed. Both SDL presets build
`stars-compat-memory` and `stars-compat-timers`; CLion can run/debug them.

```sh
cmake --preset sdl-debug
cmake --build --preset sdl-debug
ctest --test-dir dist/sdl-debug -L compat --output-on-failure
```

Verified October 10, 2026:

- All ten compatibility tests pass in native Debug and Release builds.
- The heap and timer tests also pass against the actual MinGW SDK under Wine.
- The Windows unit suite passes all 20 tests; Linux host tests pass all 12.
- Full native regression: 519 matches, 22 with existing warnings, no differences
  or errors. The regression baseline is unchanged.
- The tutorial passes the corrected year-2406 waypoint action but stops in
  year 2408, page 26, instruction 203: `No ship transfer arrow is available`.
  Artifacts are in `dist/scaffold/tutorial/20261010T103228.623152Z`.

The game UI is still a compile-only target on Linux. Child windows, controls,
menus, dialogs, full fonts and the other inventory gaps remain required for
the complete client.
