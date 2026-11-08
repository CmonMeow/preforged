"""Frozen evidence integrity and missing-channel reporting regressions."""
import hashlib
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location('completed_session', Path(__file__).resolve().parents[1] / 'tools/skills/review_completed_session.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class CompletedSessionReview(unittest.TestCase):
    def fixture(self, root):
        raw = (json.dumps({'event': 'npc_observations', 'map': 146,
                           'npcs': [{'name': 'Haversdan'}]}) + '\n').encode()
        path = root / 'world-1-2.jsonl'
        path.write_bytes(raw)
        manifest = {'created_utc': 'fixed-cutoff', 'files': [dict(snapshot=path.name,
                    snapshot_bytes=len(raw), sha256=hashlib.sha256(raw).hexdigest())]}
        (root / 'manifest.json').write_text(json.dumps(manifest))
        return path, manifest

    def test_partial_channels_are_explicit_without_discarding_npcs(self):
        with tempfile.TemporaryDirectory() as name:
            root = Path(name)
            self.fixture(root)
            report = module.review(root)
            self.assertEqual(report['decoded_npc_names'], ['Haversdan'])
            self.assertIn('quest_snapshot_changed', report['missing_world_channels'])
            self.assertEqual(report['quest_ids'], [])

    def test_same_size_capture_mutation_is_rejected(self):
        with tempfile.TemporaryDirectory() as name:
            root = Path(name)
            path, _ = self.fixture(root)
            path.write_bytes(path.read_bytes().replace(b'Haversdan', b'Wrongname'))
            with self.assertRaisesRegex(ValueError, 'snapshot changed'):
                module.review(root)

    def test_capture_append_is_rejected(self):
        with tempfile.TemporaryDirectory() as name:
            root = Path(name)
            path, _ = self.fixture(root)
            with path.open('ab') as stream:
                stream.write(b'{}\n')
            with self.assertRaisesRegex(ValueError, 'snapshot changed'):
                module.review(root)

    def test_manifest_cannot_escape_checkpoint(self):
        with tempfile.TemporaryDirectory() as name:
            root = Path(name)
            _, manifest = self.fixture(root)
            manifest['files'][0]['snapshot'] = '../outside.jsonl'
            (root / 'manifest.json').write_text(json.dumps(manifest))
            with self.assertRaisesRegex(ValueError, 'Invalid snapshot filename'):
                module.review(root)


if __name__ == '__main__':
    unittest.main()
