# File and settings services

The SDL runtime now supplies all six inventoried legacy file calls and all
three profile calls. Coverage is **92/178 inventoried Win32 APIs (51.7%, rounded
to 52%)**, with the six CRT helpers counted separately. This is coverage of
tested application contracts; the Linux game UI is not yet runnable.

## Settings paths

Explicit POSIX paths are used verbatim, including absolute paths and filenames
with spaces. Bare filenames such as the game's `Stars.ini` resolve in this order:

1. `STARS_SETTINGS_DIR`, when set to a nonempty directory path.
2. `$XDG_CONFIG_HOME/stars`, when `XDG_CONFIG_HOME` is absolute.
3. `$HOME/.config/stars`, when `HOME` is absolute.
4. The current directory if neither user configuration location is available.

Reading defaults does not create a directory. Writing a bare settings name
creates missing configuration directories with mode 0700. New INI files are
private (0600); an existing file's permission bits are retained. Explicit paths
require their parent directory to exist. These rules replace the Windows
directory search; filenames are subject to Linux filesystem case sensitivity.
The host's existing `--ini` path handling is unchanged.

## Profile behavior

`GetPrivateProfileString`, `GetPrivateProfileInt` and `WritePrivateProfileString`
support ANSI/byte-oriented files, case-insensitive section/key matching, empty
and missing values, trimmed defaults, single/double-quoted strings, decimal and
hexadecimal integers, signed defaults and 32-bit integer wrap. String reads
respect the caller's size and return count. Null section/key queries enumerate
names in multi-string form, including truncation and exact-size boundary checks.

Writes update/add keys and sections or delete them for null key/value arguments.
They preserve unrelated raw lines, comments and malformed lines. A temporary
file in the same directory is flushed, synced and renamed to replace the
destination; failures retain the original file. Operations serialize within
this process; external writers can still overwrite one another's changes.
There is no settings cache; the legacy flush call returns false.

Profiles are limited to 8 MiB and reject embedded NULs/UTF-16 content rather
than rewriting an unsupported encoding. Registry INI mappings, Unicode profile
conversion and Windows locale collation are not implemented. The native
enumeration path always supplies the documented double terminator; the shared
Wine comparison accommodates Wine's single-terminator behavior for truncated
lists while checking return counts and buffer boundaries.

## Legacy files

| API | Contract |
| --- | --- |
| `OpenFile` | Literal native paths, read/write/create, parse/existence/delete/reopen, populated `OFSTRUCT`, error codes and owned small integer handles. Its legacy 128-byte path field remains the limit. |
| `_lread`, `_lwrite` | Binary I/O, EOF/partial counts and access checks. Zero-length writes truncate or extend at the current position; reads/writes retry interrupted system calls. |
| `_llseek` | Start/current/end origins and signed 32-bit offsets/results. |
| `_lclose` | Closes only registered descriptors; invalid or closed handles fail. |
| `GetFileSize` | 64-bit size split into high/low DWORDs, including the valid low-word `INVALID_FILE_SIZE` case and last-error distinction. |

Sharing modes are enforced between wrapper handles referring to the same
device/inode, including read/write denial and exclusion. A failed create under
a sharing denial cannot truncate the file. An open wrapper handle blocks
wrapper deletion. This does not impose Windows sharing rules on unrelated
POSIX processes. Prompt/verify modes return an unsupported-operation error;
there is no Windows executable/system/PATH search or drive-letter translation.
Shutdown releases remaining owned descriptors.

The game core still uses its existing POSIX implementation in `native.c`; these
adapters do not change save formats, core file I/O or host behavior.

## Build and verification

No additional libraries or packages are needed. Both SDL presets build
`stars-compat-profile` and `stars-compat-file`, which CLion can run/debug.

```sh
cmake --preset sdl-debug
cmake --build --preset sdl-debug
ctest --test-dir dist/sdl-debug -L compat --output-on-failure
python3 compat/win32/audit_free_api.py
```

The file/profile contracts run against both the local runtime and the Windows
SDK under Wine. Native-only tests check configuration-directory selection,
directory creation, preservation of comments/malformed lines, unsupported
encoding and write failures. File tests exercise sparse sizes above 4 GiB,
sharing, reopen, failed-creation data preservation and truncate/extend behavior.

Debug and Release pass all 13 native compatibility tests. Both new tests pass
address, undefined-behavior and leak sanitizers. Linux host tests pass all 12.
The shared timer test now checks timer identity/callback across repeated peeks
without assuming Wine preserves the synthesized timestamp; native tests retain
the stable pending-message timestamp assertion.

Repository checks on October 10, 2026: the MinGW Debug build succeeds without
new warnings, all 23 Windows unit tests pass, and the full native regression
has 519 matches (22 with existing warnings), zero differences/errors and no
skipped reference inputs. Game sources and regression baselines are unchanged.
The tutorial still stops at year 2408, page 26, instruction 203 with
`No ship transfer arrow is available`; artifacts are in
`dist/scaffold/tutorial/20261010T105806.038016Z`.

Reference contracts: Microsoft's [profile strings](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-getprivateprofilestringa),
[profile integers](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-getprivateprofileinta),
[profile writes](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-writeprivateprofilestringa),
[OpenFile](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-openfile)
and [legacy writes](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-_lwrite).
