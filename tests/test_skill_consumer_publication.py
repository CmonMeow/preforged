"""Exercise the real coverage writer without publishing its output."""
import io
import json
import sys
import unittest
from contextlib import redirect_stdout
from pathlib import Path
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools/protocol"))
from skill_runtime_coverage import generate


class ConsumerPublication(unittest.TestCase):
    def test_unfinished_consumers_do_not_become_published_bindings(self):
        captured = {}

        def collect(path, text, *args, **kwargs):
            if path.name == "skill-runtime-coverage.json":
                captured["report"] = json.loads(text)
            return len(text)

        with patch.object(Path, "write_text", collect), redirect_stdout(io.StringIO()):
            generate()
        rows = {row["id"]: row for row in captured["report"]["skills"]}
        # Paid/staged producers are not proof that all required consumers work.
        # In particular274was incorrectly promoted when its producer appeared.
        for skill in (97, 181, 274, 307, 786, 1399, 1727, 2860):
            with self.subTest(skill=skill):
                self.assertEqual(rows[skill]["status"], "unsupported")
        self.assertEqual(rows[2222]["status"], "implemented_binding")
        self.assertFalse(rows[2222]["effect_consumers_unfinished"])
        self.assertTrue(rows[227]["dispatcher_binding"])
        self.assertFalse(rows[227]["effect_consumers_unfinished"])
        self.assertEqual(rows[227]["status"], "implemented_binding")
        self.assertTrue(rows[274]["dispatcher_binding"])
        self.assertTrue(rows[274]["effect_consumers_unfinished"])
        # The publication gate must not erase the reviewed life-steal routes.
        for skill in (153, 156, 902, 1076, 1077, 2189):
            with self.subTest(bound_life_steal=skill):
                self.assertEqual(rows[skill]["status"], "implemented_binding")


if __name__ == "__main__":
    unittest.main()
