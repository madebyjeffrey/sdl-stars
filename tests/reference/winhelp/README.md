# Archived WinHelp decoder

These scripts decode the original `stars!.hlp` beside these scripts for historical inspection and reference tests. They are excluded from the client build and contain the original archive encoding rules. The application uses the canonical UTF-8 assets in `res/help-utf8.zip`, staged by `compat/win32/helpgen.py`; no legacy text conversion runs when building or using the client.
