"""Integration checks for the source-extracted precision experiments."""

import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

import precision


class SourceCoverage(unittest.TestCase):
    def test_every_cast_has_typed_operands_and_result(self):
        cases, count, _ = precision.extract()
        self.assertEqual(sum(case["casts"] for case in cases), count)
        self.assertGreaterEqual(count, 178)
        for case in cases:
            with self.subTest(file=case["file"], function=case["function"], lines=case["lines"]):
                self.assertNotIn(case["function"], ("if", "for", "while", "switch"))
                precision.bind(case)
                self.assertIn("long double", case["bound_expression"])
                self.assertTrue(case["inputs"])
                self.assertNotIn("->", case["bound_expression"])
        self.assertIn("IncreaseAIMinefieldSizes", {case["function"] for case in cases})


class DifferentialOracle(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory(prefix="stars-precision-test-")
        cls.path = Path(cls.temp.name) / "report.json"
        cls.command = [sys.executable, str(precision.HERE / "precision.py"), "--sanitize",
                       "--function", "IdGetBestScannerDest", "--function", "PctPlanetDesirability",
                       "--function", "DrawPlanetStats", "--samples", "32", "--race-samples", "32",
                       "--no-axis-sweep", "--habitat-count", "16", "--report", str(cls.path)]
        if "STARS_PRECISION_GENERATED_DIR" in os.environ:
            cls.command += ["--generated-dir", os.environ["STARS_PRECISION_GENERATED_DIR"]]
        subprocess.run(cls.command, check=True, stdout=subprocess.DEVNULL)
        cls.report = json.loads(cls.path.read_text())

    @classmethod
    def tearDownClass(cls):
        cls.temp.cleanup()

    def test_supported_universe_sizes_have_exact_counterexamples(self):
        cases = [c for c in self.report["cases"] if c["function"] == "IdGetBestScannerDest"]
        self.assertEqual(len(cases), 2)
        for case in cases:
            self.assertTrue(case["exhaustive_domain"])
            self.assertEqual(case["evaluated"], 5)
            self.assertEqual(case["different"], 5)
            self.assertEqual(case["max_abs"], 1)
            self.assertEqual(case["first"]["inputs"], [400])
            self.assertEqual(case["first"]["double"] - case["first"]["extended"], 1)

    def test_positive_desirability_root_domain_matches(self):
        case = next(c for c in self.report["cases"] if c["function"] == "PctPlanetDesirability")
        self.assertTrue(case["exhaustive_domain"])
        self.assertEqual(case["evaluated"], 30001)
        self.assertEqual(case["different"], 0)
        self.assertIsNone(case["first"])

    def test_race_witness_propagation_and_input_preservation(self):
        race = self.report["race"]
        self.assertTrue(race["golden_witness_checked"])
        self.assertEqual(race["complete_habitat_domain"], 9517173136)
        metrics = race["metrics"]
        self.assertEqual(metrics[1]["first"]["extended"], 1470286)
        self.assertEqual(metrics[1]["first"]["double"], 1470287)
        self.assertEqual(metrics[4]["different"], 0)
        self.assertEqual(metrics[3]["first"], None)

    def test_ulp_distance_counts_zero_once(self):
        source = Path(self.temp.name) / "ulp.c"
        executable = Path(self.temp.name) / "ulp"
        source.write_text(f'#include "{precision.HERE / "precision_probe.c"}"\n' + r'''
int main(void) {
    double d = nextafter(0.0, 1.0);
    float pct = nextafterf(0.0f, 1.0f);
    if (UlpKey(-0.0, 0) != UlpKey(0.0, 0)) return 1;
    if (UlpKey(0.0, 0) - UlpKey(-d, 0) != 1) return 2;
    if (UlpKey(d, 0) - UlpKey(-d, 0) != 2) return 3;
    if (UlpKey(0.0, 1) - UlpKey(-pct, 1) != 1) return 4;
    if (UlpKey(pct, 1) - UlpKey(-pct, 1) != 2) return 5;
    return 0;
}
''')
        subprocess.run(["gcc", "-std=gnu11", str(source), "-lm", "-o", str(executable)], check=True)
        subprocess.run([str(executable)], check=True)

    def test_counterexample_exit_and_reproducibility(self):
        proc = subprocess.run([*self.command, "--fail-on-difference"], stdout=subprocess.DEVNULL)
        self.assertEqual(proc.returncode, 1)
        self.assertEqual(json.loads(self.path.read_text()), self.report)


if __name__ == "__main__":
    unittest.main()
