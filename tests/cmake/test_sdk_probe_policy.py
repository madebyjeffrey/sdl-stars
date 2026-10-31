"""The native SDK diagnostic list must never hide a boundary failure or missing run."""
from pathlib import Path
import tempfile
import unittest
import xml.etree.ElementTree as ET

from configure_matrix import sdk_outcome


class SdkProbePolicy(unittest.TestCase):
    names = {'test_win32_abi_reference', 'test_win32_controls_reference', 'test_sdk_type_isolation'}
    diagnostics = {'test_win32_controls_reference'}

    def receipt(self, failures=(), skipped=(), omitted=()):
        suite = ET.Element('testsuite')
        for name in sorted(self.names - set(omitted)):
            case = ET.SubElement(suite, 'testcase', name=name)
            if name in failures:
                ET.SubElement(case, 'failure', message='native probe assertion')
            if name in skipped:
                ET.SubElement(case, 'skipped')
        temporary = tempfile.TemporaryDirectory(prefix='stars-sdk-policy-')
        self.addCleanup(temporary.cleanup)
        path = Path(temporary.name) / 'tests.xml'
        ET.ElementTree(suite).write(path)
        return path

    def test_known_probe_failure_is_visible(self):
        result = sdk_outcome(self.receipt(failures=self.diagnostics), self.diagnostics, self.names)
        self.assertEqual(result['failed_diagnostics'], ['test_win32_controls_reference'])
        self.assertEqual(result['failed_contracts'], [])
        self.assertEqual(result['passed'], 2)

    def test_abi_failure_remains_a_gate(self):
        result = sdk_outcome(self.receipt(failures={'test_win32_abi_reference'}), self.diagnostics, self.names)
        self.assertEqual(result['failed_contracts'], ['test_win32_abi_reference'])

    def test_fixed_probe_can_pass_without_changing_expected_values(self):
        result = sdk_outcome(self.receipt(), self.diagnostics, self.names)
        self.assertEqual(result['passed'], 3)
        self.assertEqual(result['failed_diagnostics'], [])

    def test_diagnostics_must_execute(self):
        for report in (self.receipt(skipped=self.diagnostics), self.receipt(omitted=self.diagnostics)):
            with self.assertRaises(AssertionError):
                sdk_outcome(report, self.diagnostics, self.names)


if __name__ == '__main__':
    unittest.main()
