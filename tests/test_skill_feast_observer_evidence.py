import gzip
import importlib.util
from pathlib import Path
import sys
import tempfile
import unittest

TOOLS = Path(__file__).resolve().parents[1]/'tools/protocol'
sys.path.insert(0, str(TOOLS))
spec = importlib.util.spec_from_file_location('feast_observer', TOOLS/'feast_of_corruption_observer_evidence.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class FeastObserverEvidenceTests(unittest.TestCase):
    def read(self, lines):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)/'skills.txt.gz'
            with gzip.open(path, 'wt', encoding='utf-8') as stream:
                stream.write('\n'.join(lines))
            return module.collect(path)

    def test_terminal_belongs_to_actor_not_nearest_start(self):
        casts = self.read([
            '[00:01.000] SKILL_ACTIVATED;151;25;30',
            '[00:01.500] SKILL_FINISHED;99',
            '[00:03.000] SKILL_FINISHED;25',
        ])
        self.assertEqual(casts[0]['next_terminal']['at'], 3000)

    def test_new_cast_prevents_false_terminal_association(self):
        casts = self.read([
            '[00:01.000] SKILL_ACTIVATED;151;25;30',
            '[00:02.000] SKILL_ACTIVATED;123;25;31',
            '[00:03.000] SKILL_FINISHED;25',
        ])
        self.assertNotIn('next_terminal', casts[0])
        self.assertEqual(casts[0]['superseded_by_start']['skill'], 123)

    def test_interruption_is_not_completion(self):
        casts = self.read([
            '[00:01.000] SKILL_ACTIVATED;151;25;30',
            '[00:01.500] SKILL_INTERRUPTED;25',
        ])
        self.assertEqual(casts[0]['next_terminal']['fields'][0], 'SKILL_INTERRUPTED')


if __name__ == '__main__':
    unittest.main()
