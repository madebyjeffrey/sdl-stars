# Linked independent witnesses

These executables link normal production libraries and include the actual
application types. Python entry points retain the independent interpreters,
vectors and defined-domain exclusions; see [the complete C05 inventory](../../docs/CPP-C05-WITNESSES.md).

Run `cmake --build <build> --target stars-linked-witnesses`, then
`ctest --test-dir <build> -L witness --output-on-failure`; the original 27 Python
commands also locate/build these targets through `linked_witness.py`.
Fresh standalone runs enable sanitizers. CMake explicitly keeps assertion macros
active in Release fixtures.

Fixtures bind module-owned typed service snapshots. Observers delegate directly
to original functions, and scripted effects retain the parent's declared domains.
`ui_fixture.h` adapts external scalar control IDs and SDK aliases to exact service
pointer signatures; it contains no game types or algorithms. Record storage,
state, numeric operations and list lifetimes always come from production headers
and linked implementations. Narrow internal helpers are shared with real callers,
never extracted or compiled as alternate production bodies.
