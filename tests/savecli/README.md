# Stars! save CLI

Standalone Go module used by the regression harness. Build from the
repository root with `make save-cli`, then use:

```sh
dist/stars-save save compare LEFT RIGHT
dist/stars-save save compare LEFT RIGHT --show-bytes
dist/stars-save save update game.m1 --ai maid
dist/stars-save save update game.hst --ai maid --player 1
```

Comparison decrypts records and preserves the original comparison rules,
including clock-derived identity exclusions and unused-storage warnings.
Matches and `MATCH with warnings` exit 0; differences and invalid inputs exit 1.
Updates preserve encrypted saves and select a one-based player for host files.

Run tests with `cd tests/savecli && go test ./...`.

The offline `go run ./cmd/utf8-tutorial SOURCE DESTINATION` tool converts only
the count-prefixed bundled tutorial streams. It retags BOFs as 2.85.0 and
upgrades pre-2.80 battle event headers using encrypted-byte XOR deltas, without
changing record framing. It is reference asset tooling, not a user-game importer.
