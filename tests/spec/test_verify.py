"""Exercise the specification inventory checker with broken and valid documents."""
import unittest

from verify import requirement_errors


def story(owner="m01", number="01"):
    return f'<a id="{owner}-us{number}"></a>\nStory.\n'


def criterion(reference="US01", owner="m01", number="01"):
    return f'<a id="{owner}-ac{number}"></a>\nGiven / when / then; {reference}.\n'


class RequirementsTest(unittest.TestCase):
    def errors(self, documents):
        return requirement_errors(documents)[1]

    def test_local_and_companion_references(self):
        counts, errors = requirement_errors({
            "M01-entry.md": story() + story(number="02") + criterion("US01, US02"),
            "M01-companion.md": criterion("M01-US01", number="02"),
        })
        self.assertEqual(errors, [])
        self.assertEqual(counts, {"us": 2, "ac": 2, "ir": 0, "qr": 0})

    def test_duplicate_in_one_document(self):
        errors = self.errors({"M01-entry.md": story() + story() + criterion()})
        self.assertTrue(any("duplicate requirement m01-us01" in e for e in errors))

    def test_duplicate_across_documents(self):
        errors = self.errors({"M01-entry.md": story() + criterion(),
                              "M01-other.md": criterion()})
        self.assertTrue(any("duplicate requirement m01-ac01" in e for e in errors))

    def test_undefined_story(self):
        errors = self.errors({"M01-entry.md": story() + criterion("US01, M02-US03")})
        self.assertTrue(any("undefined story m02-us03" in e for e in errors))

    def test_story_without_criterion(self):
        self.assertIn("M01-entry.md: m01-us01 has no acceptance criterion",
                      self.errors({"M01-entry.md": story()}))

    def test_criterion_without_story(self):
        self.assertIn("M01-entry.md: m01-ac01 has no story reference",
                      self.errors({"M01-entry.md": criterion("A rule only")}))

    def test_wrong_module(self):
        errors = self.errors({"M01-entry.md": story("m02") + criterion(owner="m02")})
        self.assertTrue(any("belongs to another module" in e for e in errors))

    def test_fenced_example_is_not_a_definition(self):
        text = story() + criterion() + "```html\n" + story() + "```\n"
        self.assertEqual(self.errors({"M01-entry.md": text}), [])

    def test_later_requirement_does_not_supply_acceptance(self):
        text = story() + criterion("No story") + '<a id="m01-ir01"></a> US01\n'
        errors = self.errors({"M01-entry.md": text})
        self.assertTrue(any("has no story reference" in e for e in errors))
        self.assertTrue(any("has no acceptance criterion" in e for e in errors))


if __name__ == "__main__":
    unittest.main()
