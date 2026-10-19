# Desktop icon

`stars-app.png` is the approved 512×512 desktop artwork, embedded by
`compat/win32/imagegen.py` and passed to SDL at runtime on Windows and Linux.
macOS uses `stars-app-macos.png`, with the artwork scaled to 87.5% within the same
512×512 transparent canvas so the Dock/application-switcher highlight stays visible.
`stars-app.ico` contains PNG representations at 16, 32, 64, 128 and 256 pixels
for the Windows executable. `stars-app.icns` includes the macOS sizes through
1024 pixels with the same inset and identifies the signed macOS app bundle built
by `make mac-bundle`.

Regenerate both macOS assets from the approved full-resolution artwork on a Mac:

```sh
swift packaging/macos/icon.swift .
```

The approved full-resolution image is `stars-macos-v3.png`; v1 and v2 retain the
earlier design iterations. The original `starsico.ico` remains the source for
small in-game icons and resource compatibility tests. Update the desktop PNG,
ICO, ICNS and Linux artwork together when replacing the approved design.
