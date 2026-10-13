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

The planet/fleet pane now has a viewport around its existing content window.
Expanded tiles determine the virtual size; proportional scrollbars appear only
when necessary. Wheel input and live thumb tracking move the content while
retaining its original drawing and control coordinates. Changing the selected
object resets the origin. The saved tile expansion/layout settings are unchanged.

The minimum-size check verifies range/page values, thumb tracking, bottom
clamping, wheel navigation and visibility of the actual Repeat Orders control.
Fleet workflow clicks scroll addressed controls into view before injecting real
SDL input. Pane state is destroyed before game shutdown, including WM_QUIT paths.
Focused fatal sanitizer fleet and polish tests pass on dummy/private X11.
Real desktop feedback remains queued under desktop qualification.

Final batch verification covers all 122 non-tutorial sanitizer cases. The first
full run passed 121/122: the file-workflow executable predated the WinMain pane
cleanup relink and reported its 56-byte state allocation. Rebuilding every
sanitizer target and rerunning file workflow plus installed-help checks passes
2/2 with fatal ASan/UBSan/LSan. No suppression or disabled leak check is used.
Tutorial execution remains paused by the product-first scope.

Windows units pass 55/55. The full native regression matches all 519
comparisons (87 existing warnings), with no differences/errors or skipped
reference inputs. The MinGW and Linux Release product builds pass without new
warnings. Turn-generation baselines remain unchanged.
