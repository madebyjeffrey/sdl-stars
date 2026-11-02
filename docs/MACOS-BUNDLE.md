# Signed macOS app bundle

On an Apple-silicon Mac with the existing SDL build dependencies installed:

```sh
make mac-bundle
```

The equivalent CMake commands are:

```sh
cmake --preset macos-bundle
cmake --build --preset macos-bundle
```

The output is `dist/macos-bundle/package/Stars!.app`. Open it from Finder,
copy it to Applications, or run its executable from Terminal. The regular
`macos-sdl-debug` executable remains at its existing location. The host remains
an independent command-line program.

The app contains the approved ICNS artwork, original help archive, validated
offline HTML guide and its integrity stamp, embedded game fonts, font/wrapper
license notices, SDL's runtime framework/resources and the complete non-system
dylib dependency closure. Dependency paths are rewritten inside the bundle;
absolute build-machine search paths are removed. All shipped Mach-O code is
checked for arm64, and universal runtime libraries are thinned to arm64.

SDL normally returns `Contents/Resources` from `SDL_GetBasePath` for an app
bundle. The bundle preset selects that resource layout through the generated
`szStarsInstalledData` C++ constant. Windows, Linux and ordinary Mac builds keep
their existing layouts. See [SDL's filesystem documentation](https://wiki.libsdl.org/SDL3/SDL_GetBasePath).

## Automatic identity selection

`STARS_MACOS_SIGN_IDENTITY` defaults to `AUTO`. Each packaging run queries
`security find-identity -v -p codesigning` again; a certificate fingerprint is
not saved in the build configuration. The private key stays in the keychain.
Selection prefers a unique **Developer ID Application** identity, then a unique
**Apple Development** identity, then a unique legacy **Mac Developer** identity.
Expired/unusable identities are not selected. With multiple identities in the
preferred category, packaging stops and lists the choices.

An explicit public fingerprint or exact identity name can disambiguate a build:

```sh
cmake --preset macos-bundle -DSTARS_MACOS_SIGN_IDENTITY="Apple Development: Your Name (ID)"
cmake --build --preset macos-bundle
```

To restore automatic discovery, configure with
`-DSTARS_MACOS_SIGN_IDENTITY=AUTO`. For an explicitly local, ad-hoc test build,
use `-DSTARS_MACOS_SIGN_IDENTITY=ADHOC`. The build never silently falls back to
ad-hoc signing when an eligible key is missing. Keychain access may require a
normal macOS prompt or an unlocked keychain.

Nested libraries are signed first, then their containing frameworks, then the
app. Hardened Runtime is enabled. Developer ID signing requests a secure Apple
timestamp; development/ad-hoc signing does not need the timestamp service.
`codesign --verify --deep --strict` must succeed before the output is published.
Signing itself does not use `--deep`. Assembly runs in a staging directory;
a failed dependency fixup or signature preserves the previous successful app.
This follows [Apple's signing guidance](https://developer.apple.com/documentation/xcode/creating-distribution-signed-code-for-the-mac/).

The target does not submit the app for notarization or staple a ticket. Public
distribution/notarization and document associations remain separate packaging
work. Signing with the available Apple Development identity produces a
local development build.

## Deployment and verification

The preset requests macOS 14.0 for the client. The app's actual minimum system
version is derived from **all** embedded Mach-O deployment requirements. Runtime
libraries built for a newer OS raise that minimum; they must be rebuilt or
replaced with older-targeted libraries to lower it. The current machine's
Homebrew libraries require macOS **27.0**, so its generated app declares 27.0.

On October 12, 2026:

- Automatic discovery selected the existing Apple Development identity; all
  nested code and the app passed strict signature verification with Hardened
  Runtime enabled.
- A relocated signed copy launched through Launch Services and from Terminal,
  rendered the game, and opened its bundled offline player's guide.
- All 29 registered Mac tests passed, including 17 packaging unit cases covering
  identity selection, ambiguity, failure preservation, signing order,
  architecture/dependency validation and version metadata.
- The signed app executable matched all **519** full native regression
  comparisons across nine scenarios through turn 150: 67 accepted unused-storage
  warnings, zero differences/errors, zero skipped inputs. No baseline changed.
- The first regression staging path hit the existing 80-byte command-line limit;
  the full run used the shorter `dist/mb-rg` working directory.
- The Mac tutorial target still registers no tests. Older macOS execution and
  Developer ID/notarization have not been qualified on this machine.
