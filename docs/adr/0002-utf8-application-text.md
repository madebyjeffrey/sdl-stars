---
status: accepted
---

# UTF-8 throughout the new client

The application will use UTF-8 for decoded text on every platform, including source-generated text, resources, controls, settings and newly written game text. CP1252 handling will be removed rather than retained behind conversion boundaries. The user explicitly waived compatibility with existing game files for this migration: games will be newly created with this client. This supersedes the earlier proposal to retain CP1252 file and password boundaries, and deliberately revises the repository's legacy compatibility requirement for this work.

Shipped non-ASCII symbols, including terraformer names containing `±` and the About dialog's `©`, retain their characters and become UTF-8. This is an encoding migration, without translation, Unicode normalization, broader case folding, grapheme editing or complex-script layout work. Editing and truncation must respect complete UTF-8 code points.

Passwords use UTF-8 bytes directly; historical non-ASCII password hash compatibility is outside scope. CP1252 input restrictions are removed. Compressed payloads remain binary representations; text codecs must support UTF-8 bytes without interpreting them as CP1252. Native UTF-16 platform APIs remain legitimate boundaries with lossless UTF-8 conversion.

Native paths are supported only when they round-trip losslessly through UTF-8. Unsupported paths produce an explicit error, without substitution or filesystem normalization. This revises the arbitrary POSIX filename-byte preservation policy in `docs/UNICODE-BOUNDARIES.md`.

Settings start fresh as explicitly identified UTF-8 and retain all recognized preferences: paths, passwords, font names, preset names and numeric preferences. Encoded ASCII order prefixes retain their meaning. Old CP1252 settings are not migrated.

New game and race files use format version 2.85; older formats are rejected rather than guessed or reinterpreted. Bundled tutorial game assets must be converted to the new format as part of the migration. Fixed record layouts, field widths and record sizes remain unchanged; saved text limits are UTF-8 byte budgets, so multibyte characters consume more capacity than ASCII. Oversize input must be bounded explicitly without splitting a code point.

Implementation proceeds in small, independently verified stages, covering generators, resources, rendering, controls, game text, settings and platform boundaries. Each batch meets repository build, unit, tutorial and regression requirements. Intentional behavior changes require focused commits and changelog entries; frozen original fixtures remain unchanged. The final documentation must replace superseded CP1252 contracts.

## Delivery and verification

1. Establish UTF-8 validation, capacity and platform conversion contracts; replace CP1252-specific test expectations with the new contracts.
2. Emit UTF-8 from text/data/resource generators and use UTF-8 consistently in rendering, controls and text operations. Preserve shipped characters, text identifiers, part numbering and encoding-independent gameplay behavior.
3. Switch game text and password inputs to UTF-8 bytes, write and require format 2.85, and convert bundled tutorial assets. Retain the byte-based compression codec where it already supports arbitrary UTF-8 bytes.
4. Use fresh UTF-8 settings, enforce lossless native paths, remove remaining CP1252 paths and flags, and update superseded encoding documentation and repository compatibility guidance.

Keep batches buildable and follow the platform SDL Debug build, full native regression, unit and SDL tutorial requirements. Core changes also require host regression and host unit tests. Regression harness inputs need explicit conversion to the new format in generated working copies, preserving frozen original fixtures and keeping conversion separate from gameplay execution. Report expected file-version/text-encoding changes separately from unexpected gameplay differences, and update native baselines only for intentional changes in their focused commits.

The user confirmed this complete design before implementation.
