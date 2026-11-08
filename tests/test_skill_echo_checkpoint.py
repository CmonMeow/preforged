import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import tempfile
import unittest

SPEC = importlib.util.spec_from_file_location('echo_audit', Path(__file__).resolve().parents[1] / 'tools/skills/audit_echo_checkpoint.py')
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


class EchoCheckpointTests(unittest.TestCase):
    def fixture(self, root, mismatch=False):
        (root / 'skill-audit').mkdir()
        rows = []
        for tick, actor, complete in [(1000, 25, True), (1010, 26, True), (1020, 25, False), (91001, 25, True)]:
            rows.append(dict(event='packet', direction='StoC', tick_ms=tick, complete=complete, opcode=217,
                             hex=struct.pack('<5I', 218 if mismatch else 217, actor, 0, 1341, 0xffffffff).hex()))
        source = root / 'packets-1-1.jsonl'
        source.write_text('\n'.join(json.dumps(row) for row in rows) + '\n')
        (root / 'manifest.json').write_text(json.dumps(dict(created_utc='fixed', files=[dict(snapshot=source.name, sha256=hashlib.sha256(source.read_bytes()).hexdigest())])))
        (root / 'skill-audit/captured-skills.json').write_text(json.dumps(dict(skills=[dict(id=74, name='Echo', activation_evidence=[dict(file=source.name, actor=25, tick_ms=1000, line=1,hex=rows[0]['hex'])])])))
        return source

    def test_scoped_complete_window_and_unchanged_capture(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory); source = self.fixture(root); original = source.read_bytes()
            result = MODULE.audit(root)
            self.assertEqual([event['line'] for event in result['windows'][0]['events']], [1])
            self.assertEqual(result['windows'][0]['events'][0]['words'][-1], 0xffffffff)
            self.assertEqual(source.read_bytes(), original)

    def test_changed_source_and_inconsistent_headers_reject(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory); source = self.fixture(root)
            source.write_text('tampered\n')
            with self.assertRaisesRegex(ValueError, 'Checkpoint changed'):
                MODULE.audit(root)
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory); self.fixture(root, mismatch=True)
            with self.assertRaisesRegex(ValueError, 'Opcode/header mismatch'):
                MODULE.audit(root)

    def test_mimicry_filter_and_target_receipts(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);source=self.fixture(root)
            rows=[json.loads(line) for line in source.read_text().splitlines()]
            rows[0].update(opcode=160,hex=struct.pack('<5I',160,60,25,26,65).hex())
            rows[2].update(opcode=160,complete=True,hex=struct.pack('<5I',160,20,26,25,118).hex())
            source.write_text('\n'.join(map(json.dumps,rows))+'\n')
            (root/'manifest.json').write_text(json.dumps(dict(created_utc='fixed',files=[dict(snapshot=source.name,sha256=hashlib.sha256(source.read_bytes()).hexdigest())])))
            evidence=dict(file=source.name,actor=25,tick_ms=1000,line=1,hex=rows[0]['hex'])
            catalog=dict(skills=[dict(id=65,name='Arcane Mimicry',activation_evidence=[evidence])])
            (root/'skill-audit/captured-skills.json').write_text(json.dumps(catalog))
            self.assertEqual(MODULE.audit(root)['windows'],[])
            result=MODULE.audit(root,[65],True)
            self.assertEqual(result['windows'][0]['subjects'],[25,26])
            self.assertEqual([e['line'] for e in result['windows'][0]['events']],[1,2,3])
            self.assertEqual(result['windows'][0]['events'][2]['words'],(160,20,26,25,118))
            self.assertTrue(result['windows'][0]['activation_verified'])
            evidence['tick_ms']=999
            (root/'skill-audit/captured-skills.json').write_text(json.dumps(catalog))
            with self.assertRaisesRegex(ValueError,'Activation receipt mismatch'):
                MODULE.audit(root,[65],True)
if __name__ == '__main__':
    unittest.main()
