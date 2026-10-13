"""Validate exported UTF-8 JSON, including IDs that must not lose precision."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile

with tempfile.TemporaryDirectory(prefix="stars-inspect-") as directory:
    path = Path(directory) / "hierarchy.json"
    subprocess.run([sys.argv[1], str(path)], check=True)
    snapshot = json.loads(path.read_text(encoding="utf-8"))
    assert snapshot["schema"] == 1
    windows = snapshot["windows"]
    assert len(windows) == 5
    by_id = {row["id"]: row for row in windows}
    assert len(by_id) == len(windows)
    for row in windows:
        assert isinstance(row["id"], str) and int(row["id"]) > 0
        assert row["parent"] == "0" or row["parent"] in by_id
        assert row["owner"] == "0" or row["owner"] in by_id
    frame = next(row for row in windows if row["text"].startswith("Frame"))
    assert frame["text"] == 'Frame "quote" \\ €\r\n'
    assert by_id["9007199254740993"]["text"] == "Owned"
    action = next(row for row in windows if row["controlId"] == "7")
    assert action["parent"] == frame["id"] and action["focused"] and action["captured"]
    assert action["styleNames"] == "WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON"
    assert action["controlKindName"] == "button" and action["exStyleNames"] == "0"
    assert "WS_CAPTION" in frame["styleNames"] and "WS_MINIMIZEBOX" in frame["styleNames"]
    assert "WS_GROUP" not in frame["styleNames"]
    password = next(row for row in windows if row["controlId"] == "8")
    assert password["text"] == "[password]" and password["parent"] == action["id"]
    assert "ES_PASSWORD" in password["styleNames"] and password["controlKindName"] == "edit"
