"""Source provenance, exact-name resolution, and regeneration of trace views."""
import importlib.util
from pathlib import Path
import tempfile
import unittest

SPEC = importlib.util.spec_from_file_location("traces", Path(__file__).resolve().parents[2] / "tools/traceability.py")
traces = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(traces)


class TraceTests(unittest.TestCase):
    def fixture(self, root):
        sources = {
            "spec/M01-library.md": '''# Library
[Model](M01-spec.dfy)
[Broader harness context](../tests/context.py)
<a id="m01-us01"></a>
**M01-US01 — Save (Observed).** As a player, save work.

<a id="m01-us02"></a>
**M01-US02 — Leave (Proposed).** As a player, leave safely.

| ID / story / status | Acceptance | Rule | Test / gap |
| --- | --- | --- | --- |
| <a id="m01-ac01"></a>M01-AC01 / US01, US02 / Observed | Keep `a\\|b` and [workflow](#workflow). | `M01Model.Checked`, `Ambiguous`; [rule](dafny/M01-rules.dfy). | [Focused harness](../tests/focused.py), `unique.py`, `duplicate.py`; prior run passed for bounded inputs, full crash coverage remains open. |

## Workflow
Source behavior.

```md
<a id="m01-ac99"></a>M01-AC99 / US99
```
''',
            "spec/M01-detail.md": '''# Companion
[Model](M01-spec.dfy)
<a id="m01-detail-ac02"></a>
**M01-DETAIL-AC02 — Source boundary.** Maps to M01-US01; partial effects remain.

Unrelated discussion mentions M01-US02 without assigning this criterion to it.
''',
            "spec/M01-spec.dfy": 'include "dafny/M01-rules.dfy"\ninclude "dafny/M01-more.dfy"\n',
            "spec/dafny/M01-rules.dfy": 'module M01Model {\n  predicate Checked() { true }\n  predicate Ambiguous() { true }\n}\n',
            "spec/dafny/M01-more.dfy": 'module Other {\n  predicate Ambiguous() { true }\n}\n',
            "tests/focused.py": "",
            "tests/context.py": "",
            "tests/unique.py": "",
            "tests/a/duplicate.py": "",
            "tests/b/duplicate.py": "",
        }
        for name, text in sources.items():
            path = root / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(text)
        return [root / name for name in sources]

    def test_filename_discovery_uses_only_the_supplied_test_corpus(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory).resolve()
            paths = self.fixture(root)
            public_tests = [root / "tests/focused.py", root / "tests/context.py"]
            catalog = traces.Catalog(root, paths, public_tests)
            self.assertNotIn(root / "tests/unique.py", catalog.criteria["m01-ac01"]["tests"])
            # An authored path is explicit evidence, unlike basename discovery.
            source = root / "spec/M01-library.md"
            source.write_text(source.read_text().replace("`unique.py`", "`../tests/unique.py`"))
            catalog = traces.Catalog(root, paths, public_tests)
            self.assertIn(root / "tests/unique.py", catalog.criteria["m01-ac01"]["tests"])

    def test_parent_edges_preserve_multi_story_and_companion_scope(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory).resolve()
            catalog = traces.Catalog(root, self.fixture(root))
            self.assertEqual(catalog.criteria["m01-ac01"]["stories"], ["m01-us01", "m01-us02"])
            self.assertEqual(catalog.criteria["m01-detail-ac02"]["stories"], ["m01-us01"])
            self.assertNotIn("m01-ac99", catalog.requirements)
            self.assertEqual(catalog.stories["m01-us01"]["criteria"], ["m01-detail-ac02", "m01-ac01"])
            self.assertIn("a|b", catalog.criteria["m01-ac01"]["cells"][1])

    def test_direct_and_exact_links_do_not_inherit_context_as_criterion_coverage(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory).resolve()
            catalog = traces.Catalog(root, self.fixture(root))
            c = catalog.criteria["m01-ac01"]
            self.assertEqual({e["kind"] for e in c["models"]}, {"criterion file link", "exact declaration name"})
            named = [e for e in c["models"] if e["symbol"]]
            self.assertEqual([e["symbol"]["qualified"] for e in named], ["M01Model.Checked"])
            self.assertEqual(c["ambiguous"], ["Ambiguous"])
            self.assertEqual(set(c["tests"]), {root / "tests/focused.py", root / "tests/unique.py"})
            self.assertNotIn(root / "tests/context.py", c["tests"])
            self.assertIn(root / "tests/context.py", catalog.documents[c["source"]]["tests"])
            data = catalog.export("revision")
            self.assertIn("prior run passed", data["criteria"][1]["source_text"])
            self.assertNotIn("test_results", data)
            self.assertNotIn("passed", data["counts"])
            self.assertIn("not proof/test coverage", data["scope"])

    def test_edits_removal_and_include_changes_refresh_navigation(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory).resolve()
            paths = self.fixture(root)
            catalog = traces.Catalog(root, paths)
            self.assertTrue(catalog.criteria["m01-ac01"]["models"])
            model = root / "spec/dafny/M01-rules.dfy"
            model.write_text(model.read_text().replace("Checked", "Renamed"))
            entry = root / "spec/M01-library.md"
            entry.write_text(entry.read_text().replace("US01, US02", "US02").replace("full crash coverage remains open", "revised evidence gap"))
            paths.remove(root / "spec/M01-detail.md")
            (root / "spec/M01-detail.md").unlink()
            updated = traces.Catalog(root, paths)
            self.assertEqual(updated.criteria["m01-ac01"]["stories"], ["m01-us02"])
            self.assertFalse(any(e["symbol"] for e in updated.criteria["m01-ac01"]["models"]))
            self.assertNotIn("m01-detail-ac02", updated.criteria)
            self.assertIn("revised evidence gap", updated.criteria["m01-ac01"]["block"])
            # Removing an include removes its declarations from implicit context.
            (root / "spec/M01-spec.dfy").write_text('include "dafny/M01-rules.dfy"\n')
            updated = traces.Catalog(root, paths)
            self.assertNotIn("Ambiguous", updated.criteria["m01-ac01"]["ambiguous"])

    def test_unknown_parent_and_unsupported_layout_fail(self):
        for before, after in (("US01, US02", "US99"), (" | Keep", " | Unexpected | Keep")):
            with self.subTest(change=after), tempfile.TemporaryDirectory() as directory:
                root = Path(directory).resolve()
                paths = self.fixture(root)
                path = root / "spec/M01-library.md"
                path.write_text(path.read_text().replace(before, after))
                with self.assertRaises(ValueError):
                    traces.Catalog(root, paths)

    def test_declaration_navigation_ignores_nested_comments_and_strings_and_keeps_lines(self):
        text = '''// predicate Fake() {}
module Real {
  /* module Wrong { /* nested */ predicate Hidden() {} */
  const s := "module String { predicate Nope() {}"
  const v := @"quoted ""predicate AlsoNope()"""
  predicate Checked() { true }
  lemma Witness() ensures Checked() {}
}
'''
        symbols = traces.declarations(text)
        self.assertEqual([s["qualified"] for s in symbols], ["Real.s", "Real.v", "Real.Checked", "Real.Witness"])
        for symbol in symbols:
            self.assertIn(symbol["name"], text.splitlines()[symbol["line"] - 1])
        self.assertEqual(symbols[-1]["anchor"], "def-real-witness")

    def test_source_quotes_and_reverse_views_keep_gaps_and_scope(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory).resolve()
            catalog = traces.Catalog(root, self.fixture(root))
            link = lambda p, a="": p.relative_to(root).as_posix() + ("#" + a if a else "")
            body = traces.module_page(catalog, "M01", "Library", link, lambda text, _: text)
            self.assertEqual(body.count('id="trace-m01-ac01"'), 1)
            self.assertNotIn('<a id="m01-ac01"', body)
            self.assertIn("full crash coverage remains open", body)
            self.assertIn("not a criterion-level mapping", body)
            reverse = traces.reverse_page(catalog, "evidence", link)
            self.assertIn("tests/context.py", reverse)
            self.assertIn("No criterion-level reference resolved", reverse)

    def test_cross_module_edges_link_to_the_owning_trace_page(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory).resolve()
            paths = self.fixture(root)
            source = root / "spec/M01-library.md"
            source.write_text(source.read_text().replace("US01, US02 / Observed", "US01, US02, M02-US01 / Observed"))
            other = root / "spec/M02-races.md"
            other.write_text('''# Races
<a id="m02-us01"></a>
**M02-US01 — Select.** As a player, select a race.

| ID / story | Acceptance | Rule | Evidence |
| --- | --- | --- | --- |
| <a id="m02-ac01"></a>M02-AC01 / M01-US01 | Keep the library handoff. | Native boundary. | Open. |
''')
            catalog = traces.Catalog(root, paths + [other])
            link = lambda p, a="": p.relative_to(root).as_posix() + ("#" + a if a else "")
            first = traces.module_page(catalog, "M01", "Library", link, lambda text, _: text)
            second = traces.module_page(catalog, "M02", "Races", link, lambda text, _: text)
            self.assertIn("[M02-AC01](M02.md#trace-m02-ac01)", first)
            self.assertIn("[M02-US01](M02.md#trace-m02-us01)", first)
            self.assertIn("[M01-AC01](M01.md#trace-m01-ac01)", second)
            self.assertIn("[M01-US01](M01.md#trace-m01-us01)", second)
            self.assertIn("[M01-AC01](#trace-m01-ac01)", first)
            self.assertNotIn("](#trace-m02-", first)
            self.assertNotIn("](#trace-m01-", second)


if __name__ == "__main__":
    unittest.main()
