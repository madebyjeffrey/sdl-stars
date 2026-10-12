# Mesa executable-heap cleanup

The Wayland leak comes from Mesa 26.1.6's POSIX runtime assembler, rather than
Stars' handle/window allocator or SDL's window destruction. A local dependency
patch releases the executable heap when its last live allocation is freed.
System graphics libraries are not modified or replaced by the project.

## Cause and repair

Recording module mappings before `dlclose` maps both 56-byte allocation frames
to the installed Pop!_OS `libgallium-26.1.6` library. Its allocation instructions
match `u_mmInit(0, 10*1024*1024)` called by `rtasm_exec_malloc`: one heap sentinel
and one free-list block. `rtasm_exec_free` frees and coalesces individual blocks,
but the original code never calls `u_mmDestroy` or `munmap` on the resulting
empty heap. Unloading the library loses the owning globals. The executable
mapping is 10 MiB; LSan reports only the two heap allocations, totaling 112 bytes.

The patch checks for one coalesced free block under the existing allocator
mutex, destroys the allocator, unmaps its executable memory and clears both
globals. Other live allocations retain their addresses. The next allocation
recreates the heap. This also avoids accumulating executable mappings across
driver unload/reload. The patch is scoped to Mesa's POSIX assembler; Windows
uses its existing `VirtualAlloc`/`VirtualFree` implementation.

Source: the checksum-pinned [Mesa 26.1.6 archive](https://archive.mesa3d.org/mesa-26.1.6.tar.xz),
`src/gallium/auxiliary/rtasm/rtasm_execmem.c` and `src/util/u_mm.c`.
The local patch is [mesa-26.1.6-rtasm-release.patch](../cmake/dependencies/patches/mesa-26.1.6-rtasm-release.patch).
No sanitizer suppression, driver-library retention, allocation interceptor or
disabled leak check is used in the qualification runs.

## Private dependency build

This optional build supplies llvmpipe and softpipe for private software-GPU
qualification. It does not replace the desktop driver or establish physical
GPU performance. Stars continues to use the platform's installed driver by
default. Mesa fixes shipped by the distribution/Flatpak driver extension must
be independently checked before claiming this defect is repaired there.

On Pop!_OS/Ubuntu install:

```bash
sudo apt install meson python3-venv python3-mako bison flex llvm-20-dev \
  libwayland-dev libwayland-egl-backend-dev wayland-protocols libdrm-dev \
  libglvnd-dev libexpat1-dev zlib1g-dev libzstd-dev ninja-build
```

Ubuntu 24.04's packaged Meson 1.3.2 is older than Mesa's required 1.4. Use a
local tool environment, leaving system Python alone:

```bash
python3 -m venv dist/mesa-tools
dist/mesa-tools/bin/pip install meson==1.9.1 Mako==1.3.10
PYTHONPATH= LLVM_CONFIG=/usr/bin/llvm-config-20 python3 tools/build_mesa.py \
  --meson "$PWD/dist/mesa-tools/bin/meson"
```

The helper downloads and checks the archive, applies the patch and installs
only under `dist/mesa/install`. `--archive /path/to/mesa-26.1.6.tar.xz` supports
an offline archive. A changed patch requires a fresh `--root`. It never invokes
sudo or writes to `/usr`.

Use the dependency explicitly in CTest/CLion's CMake profile:

```bash
cmake --preset sdl-sanitize -DSTARS_TEST_MESA_PREFIX="$PWD/dist/mesa/install"
cmake --build dist/sdl-sanitize
ctest --test-dir dist/sdl-sanitize -R wayland --output-on-failure -j 1
python3 tests/compat/test_mesa_rtasm.py dist/mesa/mesa-26.1.6
```

`STARS_TEST_MESA_PREFIX` affects the Wayland test clients only. Private Weston
continues using the system Mesa driver. Without that explicit choice, tests
exercise the system driver and still report its leak if it remains affected.
The test runner selects the prefix's GLVND vendor JSON, libraries and DRI path.

## Evidence

The lifetime test compiles Mesa's real assembler and allocator source with
small libc/pthread header adapters. Four threads perform partial frees and
check surviving allocations, then unload/reload the shared library ten times.
The original retains 100 MiB of anonymous executable mappings across ten
unloads and leaks 1,120 bytes in 20 heap allocations. The patched version passes
fatal ASan/UBSan/LSan, returns executable mappings to their starting size and
repeats allocations after empty-heap cleanup.

A full private Mesa llvmpipe build reproduces exactly 112 bytes in the SDL-only
two-window test when only this patch is reverted. Restoring the patch passes
that test and all six existing native Stars Wayland cases, including 100% and
200% scale, software/OpenGL, title startup and the habitat-page race wizard.
This negative control rules out the private build's other configuration
differences as the reason the leak disappears.

Final qualification passes 115/115 fatal ASan/UBSan/LSan non-tutorial tests,
including all seven registered Wayland checks with the patched dependency,
55/55 Windows unit/reference tests and the full native regression: 519 matches,
87 existing unused-storage warnings, no differences/errors or skipped inputs.
Debug, sanitizer and MinGW Debug builds pass without new warnings. The offline
archive build helper and patched/original lifetime controls also pass their
respective expected success/failure checks. No baseline or saved layout changed.
Tutorial verification stays paused under the product-first scope.
