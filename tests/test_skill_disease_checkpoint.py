import hashlib
import json
from pathlib import Path
import struct
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools/skills'))
from audit_disease_checkpoint import audit


class DiseaseCheckpointTests(unittest.TestCase):
    def fixture(self, root, mismatch=False, invalid_duration=False):
        (root / 'skill-audit').mkdir()
        duration = float('nan') if invalid_duration else 14.
        bits = struct.unpack('<I', struct.pack('<f', duration))[0]
        packets = []
        for opcode, words, tick, complete in [
            (66, [65 if mismatch else 66, 22, 483, 14, 1, bits], 1000, True),
            (65, [65, 1, 22, 483, 1, 2], 1001, True),  # upkeep is not EffectApplied
            (66, [66, 22, 483, 14, 3, bits], 1002, False),
            (160, [160, 60, 1, 22, 1353], 999, True),
            (160, [160, 60, 1, 22, 97], 1004, True),
            (160, [160, 60, 1, 22, 1353], 20000, True),
        ]:
            packets.append(dict(event='packet', direction='StoC', complete=complete,
                                tick_ms=tick, opcode=opcode,
                                hex=struct.pack('<' + 'I' * len(words), *words).hex()))
        source = root / 'packets-1-1.jsonl'
        source.write_text(''.join(json.dumps(r) + '\n' for r in packets))
        (root / 'manifest.json').write_text(json.dumps(dict(created_utc='fixed', files=[dict(
            snapshot=source.name, sha256=hashlib.sha256(source.read_bytes()).hexdigest())])))
        (root / 'skill-audit/captured-skills.json').write_text(json.dumps(dict(effect_skill_sightings=[
            dict(id=483, evidence=[dict(file=source.name)])])))
        return source

    def test_complete_effect_not_upkeep_and_context_not_causality(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            source = self.fixture(root)
            before = source.read_bytes()
            result = audit(root)
            self.assertEqual(len(result['applications']), 1)
            effect = result['applications'][0]
            self.assertEqual(effect['duration_seconds'], 14)
            self.assertEqual(effect['caster_attribution'], 'not present in effect receipt')
            self.assertIsNone(effect['instance_epoch'])
            self.assertEqual(len(result['nearby_context']), 2)
            self.assertEqual({r['role'] for r in result['nearby_context']}, {'generator', 'corpse_movement'})
            self.assertEqual(result['nearby_context'][0]['skill'], 1353)
            self.assertEqual(source.read_bytes(), before)

    def test_corruption_header_mismatch_and_bad_duration_reject(self):
        for variant in ('changed', 'header', 'duration'):
            with tempfile.TemporaryDirectory() as folder:
                root = Path(folder)
                source = self.fixture(root, mismatch=variant == 'header', invalid_duration=variant == 'duration')
                if variant == 'changed':
                    source.write_text(source.read_text() + '\n')
                with self.assertRaises(ValueError):
                    audit(root)


if __name__ == '__main__':
    unittest.main()
