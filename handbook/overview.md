# How the game fits together

These diagrams summarize the existing specification collection. The logical
module ownership is proposed; the implementation layers describe the existing
organization. Follow the linked specifications for conditions and exceptions.

## From race design to the next year

```mermaid
flowchart TD
    Race["Race design and library · M02 / M04"] --> Setup["New Game Wizard · M03"]
    Setup --> Creation["Universe Creation · M05"]
    Creation --> Library["Game Library · M01"]
    Library --> Session["Player Session · M06"]
    Session --> Plans["Inspect and plan · M07–M16"]
    Plans --> Orders["Save and exchange orders · M06 / S01"]
    Orders --> Host["Host management and AI orders · M17 / M20"]
    Host --> Generation["Ordered year generation · M18"]
    Generation --> Rules["Gameplay resolution · M08–M14 / M19"]
    Rules --> Files["Publish and validate next-year files · S01"]
    Files --> Session
```

This is a successful-path orientation, not the exact phase schedule or a claim
of atomic saving. Start with [setup](../spec/M03-new-game-wizard.md),
[player sessions](../spec/M06-player-session-and-turn-orders.md), and
[host management](../spec/M17-host-game-management.md).
[M18](../spec/M18-turn-generation.md#ordered-successful-year) owns sequencing;
gameplay modules own their effects and [S01](../spec/S01-game-file-formats.md)
owns persistence. Tutorial/help supports player activity rather than owning
gameplay rules.

## Existing implementation layers

```mermaid
flowchart TD
    Desktop["Desktop UI · src/ui"] --> Core["Shared game logic · src/core"]
    Host["CLI host · src/host"] --> Core
    Core --> Contract["Player-facing calls · ui.h"]
    Contract -. "desktop implementation" .-> Desktop
    Contract -. "host implementation" .-> Host
    Desktop --> Wrapper["SDL Win32 wrapper · compat/win32"]
    Wrapper --> SDL["SDL3"]
    Core --> Platform["Native services · src/platform"]
    Catalogs["Generated data and text catalogs"] --> Core
```

Arrows show calls or supplied data; dotted arrows identify implementations of
the UI contract. The command-line host is independent of SDL. The source map is
the [existing technical organization](../spec/README.md#existing-technical-organization),
with [CMakeLists.txt](../CMakeLists.txt) defining build groups and
[ui.h](../src/core/ui.h) defining the game-to-UI contract.

## Orders exchange crosses several boundaries

```mermaid
sequenceDiagram
    actor Player
    participant Session as Player session · M06
    participant Files as Persistence · S01
    actor Operator as Host operator
    participant Host as Host management · M17
    Player->>Session: Choose orders export destination
    Session->>Files: Save locally, then copy externally
    alt Local save and external copy succeed
        Files-->>Player: Exported orders file
        Player->>Operator: Deliver exported orders
        Operator->>Host: Import orders and check readiness
        Host->>Files: Validate game, player, and year
    else Save or copy fails
        Files-->>Session: Failure at the documented boundary
        Note over Session,Files: A completed local save may survive a later copy failure
    end
```

Delivery is an external action. Export does not imply submission or host receipt.
The diagram summarizes [M06 export](../spec/M06-player-session-and-turn-orders.md#41-export-orders)
and [M17 readiness](../spec/M17-host-game-management.md). Their entry-specific
failure behavior remains authoritative.

## Generation success needs output validation

```mermaid
flowchart TD
    Dispatch["M17 permits dispatch"] --> Prepare["M18 loads, shuffles, and replays orders"]
    Prepare --> Ready{"Prerequisites succeed?"}
    Ready -->|No| Cleanup["Failure cleanup"]
    Ready -->|Yes| Phases["Canonical ordered phase schedule"]
    Phases --> Year["Increment year and update scores"]
    Year --> Write["Rotate prior files and attempt player / host writes"]
    Write --> Validate["Validate output family before distribution"]
    Validate --> Complete{"Files are coherent?"}
    Complete -->|Yes| Distribute["Distribute next-year files"]
    Complete -->|No| Recover["Inspect partial outputs and backups"]
    Cleanup --> Recover
```

Validation before distribution is an operator obligation highlighted by
[M18's publication boundary](../spec/M18-turn-generation.md#publication-failure-and-cancellation),
not an assertion of a new automatic engine check. The top-level success flag
does not establish that every output write succeeded. Cleanup does not provide
general whole-year rollback. The canonical schedule and exact failure boundaries
remain in M18, M17, and S01.
