# Desktop-managed game and race libraries

The desktop client will store each game in its own folder within persistent per-user application data and maintain a separate race-design library. Imports copy files into this storage, leaving the originals untouched; existing game file formats and relationships remain intact. This replaces user-selected save locations with a consistent cross-platform workflow, while the command-line host continues to accept explicit file paths.

A managed game may contain turns for multiple players; the desktop will offer explicit player selection and remember the last opened player for each game. Continue resumes the most recently played game and identifies the game and player. Additional multiplayer password controls are deferred.

Imports must provide a playable set for at least one player when adding a game, and may include other players. An existing game can receive a player turn file alone, using its stored universe file. A valid later turn for the same game and player is an expected update and does not require a replacement warning, including when intermediate years are skipped. Once that update succeeds, remove that player's previous orders file; preserve existing history and backups. Other conflicting replacements require a warning.

Desktop hosting remains supported through a separate option rather than through opening a player's turn. A host-only import containing the universe and host files is allowed.

Export Game copies the complete managed game. Export Orders copies the selected player's saved orders for delivery to a host. Both retain the managed originals.

Game exports offer a folder or ZIP destination; imports accept ZIPs, folders, or game files. Orders export produces an ordinary player orders file, without ZIP packaging.

The main screen has Games, Host Games, and Race Designs views. Games offers player selection, while Host Games enters host management directly. A game with both player and host files can appear in both views, backed by the same managed folder.

Each managed game must have a unique name that is valid as a filename across supported platforms, with case-insensitive uniqueness. Tying names to files simplifies management. When an import's name collides, suggest a unique suffix and require a unique name before proceeding. Internal game identity, not filename, is used to match imports.

Users may duplicate games or import a separate copy under a different unique name. Offer updating an existing game or importing a separate copy; when several copies match an incoming turn, require selection of the destination. Renaming a library game renames its folder and matching file family, including backups, while retaining its stored game title.

For changed same-year or older turns, offer Cancel, Replace, and Import as Separate Copy. Replacement preserves a backup before changing existing files.

Race Designs offers New, Edit, Duplicate, Import, Export, and Delete. Finishing an edit in the existing race wizard saves to the managed destination; duplicating creates an independent design. Race export produces an ordinary race file, preserving existing password behavior.

A race design's library filename may differ from its in-game race names. Race filenames are unique case-insensitively, while multiple designs may share the same in-game names. Numbered race extensions such as .r1, .r2, and .r3 may distinguish designs.

The full race filename determines uniqueness: Humans.r1 and Humans.r2 are separate designs. Suggest the next available numbered race extension when importing or duplicating a design; the number does not assign a game player slot.

Startup opens the library directly, without offering to import recent-file entries or migrating files automatically. Users import through the explicit library actions; opening an external game or race also goes through Import. Imported originals remain untouched. This replaces the original one-time migration offer at the user's request.

Host Games offers Import Orders for the selected game, validating game, player, and turn and warning before replacing different existing orders. Export resolves unsaved work through the existing save behavior and includes saved orders; exporting does not automatically mark a turn submitted.

Orders export remembers and suggests the game's original imported basename even after the managed game is renamed, retaining the player's numbered orders extension. Users may change the export filename; new games initially use their library name as the exchange basename.

Deleting a managed game removes its player files, host state, and backups together after a confirmation naming the game. The confirmation explains the scope and the option to export first. Imported originals remain untouched.

The game list shows the library name, available players and their years, and last played time. If Continue's remembered player is unavailable, open that game's player selector rather than silently choosing another player. Switching players or entering host management preserves the existing Save / Don't Save / Save & Submit prompt for unsaved player orders.

Players in one managed game may have different latest imported years. Show each player's year independently; a later turn import retires only that player's previous orders.
