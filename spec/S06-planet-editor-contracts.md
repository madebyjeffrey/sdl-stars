# S06 — Planet editor copying and write-back

Status: observed native contracts, reviewed 2026-10-25 at
`e3222eb1fbdb01d68067231b5d5ef14a71759b30`. This companion supports
[S06-US01](S06-shared-game-state.md#s06-us01),
[S06-US03](S06-shared-game-state.md#s06-us03) and
[S06-US04](S06-shared-game-state.md#s06-us04); acceptance criteria remain in the
owning entry. [Canonical definitions](dafny/S06-planet-editor.dfy) cover queue
copying, allocation outcomes, native entry dispatch and write-call ordering.
[Collection verification](README.md#planet-editor-copy-and-write-back-continuation)
records actual proof, source-fixture and repository checks.

Sources: [util.c](../src/core/util.c) `FDupPlanet`, `FLookupPlanet`, `LpplFromId`;
[memory.c](../src/core/memory.c) `LpplAlloc`, `LpplReAlloc`, `FreePl`;
[structs.h](../src/core/structs.h) `PLANET`, `PL`, `PLPROD`, `PROD`;
[log.c](../src/core/log.c) `LogChangePlanet`; [produce.c](../src/core/produce.c)
`FinishProduction`. This specifies caller/queue effects, not the full log,
heap allocator, tutorial or session-lifetime implementation.

## Interface and ownership

| Value | Native representation / model |
| --- | --- |
| Planet prefix | Packed `PLANET` bytes 0..51, including ID, owner, detail/flags, environment/mineral/improvement state, cargo, base/fling/routing fields and observation turn. `Planet.prefix` contains exactly 52 bytes; `Id` decodes the signed little-endian ID from bytes 0/1. |
| Queue pointer | `PLANET.lpplprod` starts at offset 52 under the source's pack(1), including 64-bit builds. The native struct size is 52 plus pointer size; it is not a fixed-size disk record. `Planet.queue` is a separate nullable handle. |
| Queue allocation | Four-byte `PLPROD` header followed by physical `PROD` storage, four bytes per row. `Block` separates eight-bit maximum/used headers from the measured usable row sequence. Rows are complete unsigned 32-bit words, retaining count, kind/item, progress and unused bits. This does not introduce a second semantic production-row schema; S01/S06 retain field meanings. |
| Allocator assumptions | These callers expect `cbItem=4` and a valid native heap tag. `handleNew` denotes either the retained destination handle or a new unoccupied handle. `extra` supplies rows beyond the requested extent from alignment or free-block reuse; reallocation retains an existing larger usable extent. New bytes are arbitrary supplied words, not assumed zero. Heap metadata, free-block selection and actual allocation failure remain S04/native obligations. |
| Native fit | Requests, IDs and counts retain their C widths. Active/capacity headers fit 0..255, but physical capacity can exceed 255 after a write-growth request. Normal rows must be readable and destination storage writable. The model marks invalid handles, insufficient storage and overlapping nonzero row copies as boundaries, not native error-return values. |

Destination/source `PLANET` storage must be distinct for the model's ordinary
copying guarantees. A direct write to the array object itself reaches overlapping
`memcpy(...,52)`; a selected read with aliased nonempty queues reaches overlapping
row copying. No memory-safety or recovery result is asserted past those guards.
`InvalidHandle` identifies the model's ownership/liveness boundary; it does not
assert that pointer comparison alone is a native dereference or always undefined.

## Selected read: preserve ownership and copy rows

`FDupPlanet(source, destination)` first saves the destination queue pointer,
assigns the entire planet struct from source, and restores that saved pointer.
Thus all prefix fields have changed **before** any queue allocation can escape.
It then behaves as follows (`DuplicatePlanet`, `Transfer`, `Request`):

| Source queue | Destination queue | Observed result if calls return |
| --- | --- | --- |
| Null | Null | Return TRUE with copied prefix and null queue. |
| Null | Present | Free destination queue, set null, return TRUE. |
| Present | Null | Allocate source **capacity header**, with element size 4 and ordinary heap, even for an allocated empty queue. |
| Present | Capacity header less than source used count | Reallocate using source **capacity header**, retaining the destination used header until the copy completes. |
| Present | Sufficient capacity header | Reuse existing destination allocation. |

After allocation/reuse, copy exactly source used-count × four bytes and set the
destination used header to that count. Source storage remains unchanged in the
ordinary distinct-allocation case. Reused inactive destination rows remain
unchanged (`CopiesKeepTail`); allocated/reallocated suffix bytes retain allocator
contents. Source and destination pointer identities do not become aliases merely
because their active queues are equal. Failure of the allocation leaves the
already copied prefix plus the old destination queue pointer; this is an escape,
not a FALSE return from `FDupPlanet`.

## Write-back and ordinary reads

`Enter` preserves the `FLookupPlanet` guards and ordering:

1. `cPlanet <= 0` returns FALSE before reading the destination, even if it is null.
2. Every negative request, not only `idWriteBack=-1`, reads `ppl->id`. A null
   destination here is outside the defined domain. ID -1 calls
   `LogChangePlanet(NULL,ppl)` and returns TRUE if that call returns; it does not
   invalidate reports or copy a planet. Other IDs go to `LpplFromId` in write mode.
3. Nonnegative requests go to `LpplFromId` in read mode. `Resolve` reuses the
   [canonical sparse/full lookup](dafny/S06-lookups.dfy). No found pointer returns
   FALSE; a found pointer with null destination returns TRUE without copying.
4. A read into the actual `&sel.pl` uses `FDupPlanet`. Any other nonnull
   destination receives a shallow whole-struct assignment, including the queue
   pointer, without freeing a previous destination queue. The caller must track
   that borrowed alias and any previously owned allocation separately.

For writes to a found planet, invalidate the planet report, then call
`LogChangePlanet(arrayPlanet,ppl)` **before** transferring queues or copying the
prefix (`WriteStep`). Each continuation uses the actual post-callback state and
live pointer returned by lookup. After both calls return, `WritePlanet` performs:

- Equal queue pointers: skip queue allocation/copy entirely, including null/null.
- Different pointers, incoming queue null: free stored queue, null its pointer.
- Different pointers, stored queue null: allocate incoming **used count**.
- Different pointers, insufficient stored capacity header: reallocate incoming
  **used count plus two**, not the incoming capacity.
- For a nonnull incoming queue, copy its active rows and set the stored used
  header. The caller's incoming queue remains separately owned in the ordinary
  distinct-allocation case. Copy only the first 52 planet bytes; retain the stored
  queue pointer. If the current tutorial flag is set and player is zero, call
  `AdvanceTutor`, then return TRUE if that call returns.

`WritePlanet` begins after invalidation/log callbacks and ends before tutorial.
`QueueFailureAborts` keeps failed queue completion out of the tutorial phase.
If queue allocation escapes, the old prefix is retained; report/log side effects
may already have occurred. After prefix copy, a tutorial escape or mutation can
leave the updated stored planet and additional effects. TRUE means a pointer was
found and control returned, or the special log-only path returned; it is not a
file-save, host-receipt or all-effects-success result. Full `LogChangePlanet`
record generation/coalescing and callback reentrancy remain S01/M06/M16/S05 work.

## Header wrap and defined-domain boundaries

`LpplAlloc`/`LpplReAlloc` request real storage for the full count, then assign an
eight-bit maximum header. Growth from active count 254 requests 256 rows with
header 0; active count 255 requests 257 rows with header 1. The used header is
then set to 254/255. Physical storage is adequate for this write even though the
maximum header is smaller than the used count. The model retains both facts.

A subsequent selected duplication allocates/reallocates from the **wrapped
source maximum**. With an exactly sized heap-service double, physical capacity
256 / maximum 0 / used 2 reaches an eight-byte copy into a zero-row allocation.
Native `LpAlloc` can provide more usable rows through alignment and free-block
reuse, so that small case does not itself prove native heap corruption.
`Transfer` accepts measured spare rows; its witnesses show the same header/count
case failing with zero spare rows and succeeding with two. `OutsideAllocation`
marks a copy exceeding the supplied actual usable extent, not a test of the
maximum header alone.

The fixtures connect actual write-back of 254/255 rows to a fresh selected read,
trapping the undersized copy with their exact-size heap service. They also
execute actual `CbHeapAlign`, `CbHbHeader` and `CbHeapItem`: on this platform,
fresh-block alignment of a 0/1-row request still gives fewer than 254 usable
rows. Reusing a much larger free block can differ. Full allocator state and UI
reachability remain separate obligations; the request/header mismatch and copy
length are confirmed without executing the invalid copy. The
[known-bug entry](../docs/KNOWN-BUGS.md#planet-queue-capacity-wrap--located-on-main)
and [roadmap](../docs/ROADMAP.md) track UI reachability, original comparison and
a separate repair. Native fixtures also stop before self/aliased memcpy; they
do not fix or execute undefined post-copy behavior. Broader caller invariants,
reachability and a separately scoped runtime repair remain open.

## M09 handoff and evidence

[M09 lifecycle](M09-production-management.md#314-completion-recovery-and-navigation)
installs its working pointer into `sel.pl`, nulls the working global, writes back,
then reloads using the current selected ID. The native fixture now executes actual
`FinishProduction` → `FLookupPlanet` → `FDupPlanet` with actual packed structs and
list allocation wrappers. It covers ordinary separate stored/editor queues,
research propagation and a tutorial callback that redirects reload to another
planet. A successful finish can call tutorial once inside write-back and again
in the common finish tail. Subsequent selection, log/persistence effects and
arbitrary reentrancy remain explicitly owned, not proved pure by these contracts.

[Source fixtures](../tests/spec/check_planet_editor_source.py) cover all
256 source used counts × 256 destination capacity headers × read/write modes,
checking exact active words, prefix bytes, allocation sizes, retained tails and
source ownership. Native enum constants and packed declarations are extracted,
and actual heap-alignment helpers qualify requested versus usable storage. They
also cover null/empty queues, borrowed reads, all-negative
write dispatch, no-planets/null destinations, allocation/callback escapes,
wrapped headers, alias/bounds guards and the integrated M09 call chain. Native
struct sizes/offsets are compile-time assertions; C11 warnings and address/
undefined-behavior instrumentation are fatal. Logging, heap service, report and
tutorial results remain controlled doubles. Formal verification establishes these
model properties; the finite source matrix does not prove every C execution.


## Remaining adapters and acceptance work

| Owner / boundary | Concrete next evidence |
| --- | --- |
| S04 — native heap service | Bind `extra` and retained physical extent to `LpAlloc`/`LpReAlloc` item headers across fresh allocation, aligned padding, oversized free-block reuse and no-shrink realloc. Exercise heap-growth and allocation-escape state; the present exact-size service plus alignment helpers does not refine the free list. |
| S01/M06 — change logging and persistence | Model `LogChangePlanet` mineral deltas, pending-transfer flush, queue/routing record generation and preceding-record coalescing, with before/after log cursor/buffer state and failure effects. Tie successful memory write-back separately to later order-file publication. |
| S06 — caller identity and ownership | Audit lookup callers for distinct planet storage, borrowed versus owned queues, source/header/usable-extent consistency and lifetime through array movement/destruction. Add actual allocator/UI reachability evidence for GAP-15 before claiming a complete player-facing reproduction. S01 must include `FLoadGame` count+2 queue allocation; M09 must include `EstimateItemProdSched` capacity-based initial copying, alongside the checked selected-read path. |
| M16/S05/M07 — callbacks and reentrancy | Inventory report/log/tutorial callbacks that can change selection, arrays, heaps, tutorial flags or session lifetime; bind each continuation to actual post-call state. Verify modal failure/teardown separately from ordinary returning callbacks. |
| S06/S01 — semantic byte projection | Complete native-prefix ↔ logical-field ↔ record mapping beyond the existing field inventory; raw byte equality here does not validate ownership, visibility or gameplay legality of those fields. |
