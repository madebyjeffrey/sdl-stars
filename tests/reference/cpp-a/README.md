# Frozen C storage reference for milestone A

Captured from `develop` commit `5654a47` before removing the application C
interface. Generated IDs were captured before the A generator conversion;
IDs and table bytes were independently checked unchanged at A02.

These declarations are independent test inputs, not a supported application
interface. Keep them frozen during C++ header changes. `record_layout.c`
compares these C layouts with the actual C++ application declarations using
the shared list of 259 storage/prefix checks. The 145 enum storage contracts
also live in `tests/compat/enum_storage.inc`; the C++ test checks six boundary
bit patterns with the exact underlying types. SDK reference tests separately
exercise real foreign interfaces.
