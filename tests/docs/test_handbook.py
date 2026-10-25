"""Publication and rendered-URL contracts for the specification handbook."""
import importlib.util
from pathlib import Path
import tempfile
import re
import unittest
from unittest.mock import patch
import zipfile

SPEC = importlib.util.spec_from_file_location("handbook", Path(__file__).resolve().parents[2] / "tools/handbook.py")
handbook = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(handbook)


class LinkTests(unittest.TestCase):
    def test_sequence_index_follows_source_titles_anchors_edits_and_removal(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory).resolve()
            path = root / "spec/M06-session.md"
            text = ('# Session\n\n## Cross-module sequences\n\n'
                    '<a id="m06-seq-save"></a>\n### Save orders\n\n'
                    '```mermaid\n---\nconfig: {sequence: {useMaxWidth: false}}\n---\n'
                    'sequenceDiagram\nA->>B: Save\n```\n')
            handbook.write(path, text)
            with patch.object(handbook, "ROOT", root):
                index = handbook.sequence_index([path])
                self.assertIn("**1 cross-module sequences**", index)
                self.assertIn("[Save orders](../spec/M06-session.md#m06-seq-save)", index)
                self.assertNotIn("A->>B", index)
                handbook.write(path, text.replace("### Save orders", "### Save and submit"))
                self.assertIn("[Save and submit]", handbook.sequence_index([path]))
                groups = [("Play", [("M06", "Session", path.relative_to(root))])]
                atlas = handbook.diagram_index(groups)
                self.assertIn("#cross-module-sequences", atlas)
                self.assertIn("**1 module diagrams**", atlas)
                self.assertNotIn("#workflow)", atlas)
                handbook.write(path, "# Session\n")
                self.assertIn("**0 cross-module sequences**", handbook.sequence_index([path]))

    def test_sequence_without_stable_anchor_fails_instead_of_disappearing(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory).resolve()
            path = root / "spec/M06-session.md"
            handbook.write(path, "# Session\n\n## Cross-module sequences\n\n### Save\n```mermaid\nsequenceDiagram\nA->>B: Save\n```\n")
            with patch.object(handbook, "ROOT", root), self.assertRaisesRegex(ValueError, "stable anchor"):
                handbook.sequence_entries([path])

    def test_player_and_host_sequences_have_evidence_and_canonical_scope(self):
        entries = handbook.sequence_entries(handbook.sources())
        self.assertGreaterEqual(len(entries), 7)
        self.assertEqual(len({anchor for _, anchor, _ in entries}), len(entries))
        self.assertTrue({"M01", "M02", "M03", "M06", "M17", "M18"}.issubset({p.name[:3] for p, _, _ in entries}))
        for path, anchor, _ in entries:
            section = path.read_text().split("## Cross-module sequences", 1)[1]
            self.assertIn("Evidence:", section)
            self.assertRegex(section, r"\[[^]]+\]\([^)]+\.dfy\)")
            self.assertIn(f'id="{anchor}"', section)
            self.assertRegex(section.lower(), r"cancel")

    def test_diagram_presentation_preserves_body_and_explicit_author_config(self):
        source = "```mermaid\nstateDiagram-v2\n[*] --> Draft\n```\n"
        presented = handbook.readable_diagrams(source)
        self.assertIn("useMaxWidth: false", presented)
        self.assertTrue(presented.endswith("stateDiagram-v2\n[*] --> Draft\n```\n"))
        configured = "```mermaid\n---\nconfig: {state: {useMaxWidth: true}}\n---\nstateDiagram-v2\n[*] --> Draft\n```\n"
        self.assertEqual(handbook.readable_diagrams(configured), configured)

    def test_each_module_has_workflow_and_state_diagrams_with_source_evidence(self):
        for _, group in handbook.module_map():
            for identity, _, entry in group:
                with self.subTest(module=identity):
                    text = (handbook.ROOT / entry).read_text()
                    self.assertIn("## Workflow and state diagrams", text)
                    section = text.split("## Workflow and state diagrams", 1)[1]
                    blocks = re.findall(r"^```mermaid\s*\n(.*?)^```\s*$", section, re.M | re.S)
                    self.assertTrue(any(block.startswith("flowchart") for block in blocks))
                    self.assertTrue(any(block.startswith("stateDiagram-v2") for block in blocks))
                    self.assertRegex(section, r"\[canonical contracts\]\([^)]+\.dfy\)")
                    self.assertRegex(section, r"\[[^]]+\]\(#[^)]+\)")
                    self.assertRegex(section.lower(), r"cancel")

    def test_diagram_atlas_counts_source_blocks_and_links_to_owning_specs(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory).resolve()
            entry = Path("spec/M01-library.md")
            text = "# Library\n\n```mermaid\nflowchart TD\nA --> B\n```\n\n```mermaid\nstateDiagram-v2\n[*] --> A\n```\n"
            handbook.write(root / entry, text)
            groups = [("Setup", [("M01", "Library", entry)])]
            with patch.object(handbook, "ROOT", root):
                atlas = handbook.diagram_index(groups)
                self.assertIn("**2 module diagrams**", atlas)
                self.assertIn("../spec/M01-library.md#state-and-recovery", atlas)
                self.assertIn("../spec/M01-library.md#workflow", atlas)
                handbook.write(root / entry, text.split("```mermaid\nstateDiagram-v2")[0])
                atlas = handbook.diagram_index(groups)
                self.assertIn("**1 module diagrams**", atlas)
                self.assertIn("Not yet diagrammed", atlas)

    def test_repository_links_and_formal_views_preserve_fragments(self):
        source = handbook.ROOT / "spec/M01-game-library.md"
        model = handbook.ROOT / "spec/M01-library-specification.dfy"
        mapping = {source: Path("spec/M01-game-library.md"), model: Path("models/M01-library-specification.md")}
        link = handbook.resolve_link("M01-library-specification.dfy#Rule", source, mapping, "abc123")
        self.assertEqual(link, "../models/M01-library-specification.md#Rule")
        link = handbook.resolve_link("../src/core/ui.h", source, mapping, "abc123")
        self.assertEqual(link, handbook.REPO + "/blob/abc123/src/core/ui.h")

    def test_multiline_and_code_labels_are_links_but_examples_are_not(self):
        source = handbook.ROOT / "spec/M01-game-library.md"
        mapping = {source: Path("spec/M01-game-library.md")}
        text = "`[0,n)`\n[`PLAYER`](../src/core/structs.h)\n[two\nlines](../src/core/ui.h)\n"
        text += "`[example](missing.c)`\n```md\n[example](missing.c)\n```\n"
        result = handbook.rewrite_links(text, source, mapping, "abc123")
        self.assertIn("[`PLAYER`](https://", result)
        self.assertIn("[two\nlines](https://", result)
        self.assertIn("`[example](missing.c)`", result)
        self.assertIn("```md\n[example](missing.c)\n```", result)

    def test_root_relative_and_missing_targets_fail(self):
        source = handbook.ROOT / "spec/M01-game-library.md"
        for target in ("/spec/example.md", "../../outside-the-repository.md", "missing.md"):
            with self.subTest(target=target), self.assertRaises(ValueError):
                handbook.resolve_link(target, source, {source: Path("spec/example.md")}, "abc123")

    def test_publication_excludes_agent_instructions_and_build_outputs(self):
        paths = handbook.sources()
        self.assertTrue(any(path.suffix == ".dfy" for path in paths))
        self.assertFalse(any("AGENTS" in path.name or "dist" in path.relative_to(handbook.ROOT).parts for path in paths))


class RegenerationTests(unittest.TestCase):
    def test_source_edits_rename_and_removal_reach_the_next_build(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory).resolve()
            work = root / "dist/handbook"
            files = {
                "zensical.toml": '[project]\nsite_name = "Test"\nsite_url = "https://example.org/game/"\n',
                "GLOSSARY.md": "# Glossary\n",
                "spec/README.md": "# Map\n\n## Proposed first-level module map\n### Setup\n| M01 | [Library](M01-library.md) | Scope |\n## Shared data contracts\n",
                "spec/M01-library.md": '# Library\n\n<a id="m01-us01"></a>First story.\n[Rule](M01-rule.dfy)\n',
                "spec/M01-detail.md": "# Companion\n",
                "spec/M01-rule.dfy": 'include "dafny/M01-rule.dfy"\n',
                "spec/dafny/M01-rule.dfy": "predicate Valid() { true }\n",
                "spec/AGENTS-private.md": "SECRET\n",
                "handbook/index.md": "# Welcome\n[Library](../spec/M01-library.md)\n",
                "handbook/AGENTS.md": "SECRET\n",
            }
            for name, text in files.items():
                handbook.write(root / name, text)
            def fake_git(*args):
                return "abc123" if args[0] == "rev-parse" else "feature" if args[0] == "branch" else ""
            with patch.multiple(handbook, ROOT=root, WORK=work, SOURCE=work / "source", CONFIG=work / "zensical.toml"), patch.object(handbook, "git", side_effect=fake_git):
                handbook.prepare()
                staged = work / "source"
                self.assertIn("M01-US01", (staged / "requirements.md").read_text())
                self.assertEqual((staged / "models/M01-rule.dfy").read_bytes(), (root / "spec/M01-rule.dfy").read_bytes())
                with zipfile.ZipFile(staged / "models.zip") as archive:
                    self.assertIn("dafny/M01-rule.dfy", archive.namelist())
                self.assertFalse((staged / "spec/AGENTS-private.md").exists())
                self.assertFalse((staged / "AGENTS.md").exists())
                handbook.write(root / "spec/M01-library.md", '# Updated\n\n<a id="m01-us02"></a>New story.\n')
                (root / "spec/M01-library.md").rename(root / "spec/M01-renamed.md")
                map_path = root / "spec/README.md"
                map_path.write_text(map_path.read_text().replace("M01-library.md", "M01-renamed.md"))
                handbook.write(root / "handbook/index.md", "# Welcome\n[Library](../spec/M01-renamed.md)\n")
                (root / "spec/M01-detail.md").unlink()
                handbook.prepare()
                self.assertIn("# Updated", (staged / "spec/M01-renamed.md").read_text())
                self.assertFalse((staged / "spec/M01-library.md").exists())
                self.assertIn("M01-US02", (staged / "requirements.md").read_text())
                self.assertNotIn("M01-US01", (staged / "requirements.md").read_text())
                self.assertFalse((staged / "spec/M01-detail.md").exists())
                self.assertNotIn("Companion", (work / "zensical.toml").read_text())


class RenderedTests(unittest.TestCase):
    def check_html(self, body, extra=None):
        with tempfile.TemporaryDirectory() as directory:
            site = Path(directory)
            handbook.write(site / "index.html", body)
            for name, content in (extra or {}).items():
                handbook.write(site / name, content)
            handbook.check_site(site, "https://example.org/game/")

    def test_project_subpath_and_requirement_fragment(self):
        self.check_html('<a href="/game/spec/#m01-us01">Story</a><script src="assets/app.js"></script>',
                        {"spec/index.html": '<a id="m01-us01"></a>', "assets/app.js": ""})

    def test_missing_rendered_anchor_fails(self):
        with self.assertRaisesRegex(ValueError, "missing fragment"):
            self.check_html('<a href="#absent">Story</a>')

    def test_file_directory_collision_fails(self):
        with self.assertRaisesRegex(ValueError, "not a directory URL"):
            self.check_html('<a href="model.dfy/">Model</a>', {"model.dfy": "predicate P() { true }"})

    def test_asset_escaping_project_path_fails(self):
        with self.assertRaisesRegex(ValueError, "escapes project path"):
            self.check_html('<script src="/assets/app.js"></script>')


if __name__ == "__main__":
    unittest.main()
