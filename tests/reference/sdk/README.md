# Independent Windows SDK entries

These 38 ordinary C files consume the unchanged shared `tests/compat/*.inc`
vectors through native Windows headers. Their C++ counterparts consume the same
vectors through the SDL wrapper. `APPMSG` names the appropriate native message
type without importing either platform's headers into the other build.

The entry inventory and exact include/type preambles are checked by
`tests/cmake/check_cpp_manifest.py`; CMake never generates these entries or
changes their language. See `docs/CPP-POLICY.md` for the configuration matrix and
precise reference exceptions. Shared case bodies and expected values stay in the
`.inc` inputs.

`diagnostics.json` pins the 19 native behavior/automation probes tracked
in issue #78. Their original assertions still execute and report their actual
results; other SDK checks, missing/skipped execution and changed diagnostic inputs
remain errors. See `docs/CPP-C09-SDK-FINDINGS.md` for the exact observed failures.

The missing-control `CheckDlgButton` expectation is intentionally corrected after
Milestone C. Its input hash is updated explicitly; the full native controls probe
remains diagnostic until requalified.
