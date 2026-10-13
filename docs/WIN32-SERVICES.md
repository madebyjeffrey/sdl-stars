# UI string, arithmetic and CRT services

The local runtime implements the UI's narrow Windows string functions,
`MulDiv` and `GetCurrentThreadId`. This brings coverage to **83/178 inventoried
Win32 APIs (46.6%, rounded to 47%)**. All six inventoried Microsoft CRT helpers
are also implemented; they are counted separately from the Win32 percentage.

## Contracts and limits

- `wsprintfA` supports the game's narrow strings, characters, signed/unsigned
  decimal and hexadecimal values, field widths, precision, alignment and
  `h`/`l` numeric modifiers. The translation keeps Win32 `LONG` arguments at
  32 bits even on Linux's LP64 ABI. Output is capped at 1,023 bytes plus its
  terminator; as on Windows, the caller must supply enough destination space.
  Wide strings, pointer/64-bit modifiers, floating point and other format
  extensions are rejected. This is the application's subset, not a complete
  printf or Unicode implementation.
- `lstrcpyA`, `lstrcatA`, `lstrlenA` and `lstrcmpiA` provide narrow byte strings.
  Comparisons follow the current C locale; Windows locale-specific collation
  is not emulated. No UTF-16 conversion is added to these ANSI calls.
- `MulDiv` uses a 64-bit intermediate, nearest rounding with ties away from
  zero, and `-1` for zero denominators or overflow. The SDK comparison also
  checks the legacy rejection of an exact `INT_MIN` result. Core `LMulDiv`
  remains unchanged.
- `GetCurrentThreadId` returns Linux's thread ID, stable for a thread's lifetime.
- `_access`, `_mkdir` and `_getcwd` use native POSIX paths and permissions.
  `_stricmp`, `_strnicmp`, lowercase aliases and `itoa` are supplied locally.
  `_strdate`/`_strtime` use the local clock and their Windows nine-byte buffer
  formats. There is no drive-letter or backslash translation.

The game core continues to use `native.c`. These symbols are linked only into
SDL compatibility consumers. Windows uses its SDK and CRT; the Linux host
does not link this library.

## Build and verification

No additional dependencies are needed. `stars-compat-services` is built by
both SDL presets and can be run/debugged through CLion.

```sh
cmake --preset sdl-debug
cmake --build --preset sdl-debug
ctest --test-dir dist/sdl-debug -L compat --output-on-failure
python3 compat/win32/audit_free_api.py
```

The shared test runs against the actual Windows SDK under Wine as
`test_win32_services_reference`. It checks 1,331 signed arithmetic combinations,
overflow boundaries, mixed 32-bit long formatting, widths/precision, short
arguments, strings, numeric conversion, local time formats and temporary
directory/file access and removal. Native Debug and Release pass all eleven
compatibility tests, and the services test passes address, undefined-behavior
and leak sanitizers.

Repository checks on October 10, 2026: the MinGW Debug build succeeds without
new warnings, all 21 Windows unit tests pass, and the full native regression
has 519 matches (22 with existing warnings), zero differences/errors and no
skipped reference inputs. No baseline or game source changed. The tutorial
again stops at year 2408, page 26, instruction 203 with
`No ship transfer arrow is available`; its latest artifacts are in
`dist/scaffold/tutorial/20261010T104117.347130Z`. The previous Linux host unit
run passed all 12 tests; this service batch does not link into the host.

The inventory script now detects `WINAPIV` definitions and the six ordinary
CRT definitions, and prints separate Win32/CRT coverage totals. Its CSV is a
definition and scope inventory, not an automated proof of semantic completeness.

Reference contracts: Microsoft's [wsprintfA documentation](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-wsprintfa)
and [MulDiv documentation](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-muldiv).
