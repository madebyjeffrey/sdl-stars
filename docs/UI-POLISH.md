# Product UI polish

Message-pane margins and headers now navigate previous/next messages with the
wheel, using the same filtering rules as the existing Up/Down keys. Partial
notches accumulate; reversing direction starts a new accumulation. Ctrl/Shift
and compose mode preserve the current message. Long text in the read-only edit
continues to scroll through that control's existing wheel behavior.

A copied year-10 fixture supplies actual messages. Four ASCII/UTF-8 path
variants verify partial notches, both directions and Ctrl preservation. Debug
checks and two focused fatal ASan/UBSan/LSan cases pass on dummy/private X11.
The Windows build passes. This UI change leaves turn generation, saved layouts
and the native regression baseline unchanged.

Left-pane reachability at small window sizes is the next polish item. Real
desktop feedback remains queued under desktop qualification.
