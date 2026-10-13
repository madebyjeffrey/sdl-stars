# Desktop icon

`stars-app.png` is the approved 512×512 desktop artwork, embedded by
`compat/win32/imagegen.py` and passed to SDL at runtime on every desktop platform.
`stars-app.ico` contains PNG representations at 16, 32, 64, 128 and 256 pixels
for the Windows executable. `stars-app.icns` includes the macOS sizes through
1024 pixels and identifies the signed macOS app bundle built by `make mac-bundle`.

The approved full-resolution image is `stars-macos-v3.png`; v1 and v2 retain the
earlier design iterations. The original `starsico.ico` remains the source for
small in-game icons and resource compatibility tests. Update the desktop PNG,
ICO, ICNS and Linux artwork together when replacing the approved design.
