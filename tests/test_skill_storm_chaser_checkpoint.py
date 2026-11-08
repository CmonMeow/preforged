"""Reject corrupted or misattributed Storm Chaser packet evidence."""
import hashlib
import json
import struct
import sys
import tempfile
import unittest
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools/skills'))
from audit_storm_chaser_checkpoint import audit

class StormCapture(unittest.TestCase):
    def fixture(self, root, partial=False, wrong_header=False):
        filename = 'packets-fixture.jsonl'
        rows = []
        for words in [(159,48,25,455),(159,21,25,767),(66,25,455,0,106,0x41000000),
                      (39,25,0x43B40000),(229,25,455,0,20)]:
            rows.append(dict(event='packet',direction='StoC',tick_ms=100,opcode=words[0],complete=True,
                             hex=struct.pack('<'+'I'*len(words),*words).hex()))
        if partial: rows[1]['complete'] = False
        if wrong_header: rows[1]['opcode'] = 160
        content = ''.join(json.dumps(row)+'\n' for row in rows).encode()
        (root/filename).write_bytes(content)
        (root/'manifest.json').write_text(json.dumps(dict(created_utc='fixed',files=[dict(snapshot=filename,sha256=hashlib.sha256(content).hexdigest())])))
        (root/'skill-audit').mkdir()
        (root/'skill-audit/captured-skills.json').write_text(json.dumps(dict(skills=[dict(id=455,activation_evidence=[dict(file=filename,line=1,tick_ms=100,actor=25)])])))
        return filename

    def test_complete_owned_sequence(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp);self.fixture(root)
            result=audit(root)
            self.assertEqual(set(result['windows'][0]['evidence']),{'instant_action','caster_body','eight_second_buff','speed_360','twenty_second_recharge'})

    def test_source_tampering_rejects(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp);filename=self.fixture(root)
            with (root/filename).open('ab') as stream:stream.write(b'\n')
            with self.assertRaisesRegex(ValueError,'source changed'):audit(root)

    def test_partial_and_header_mismatch_reject(self):
        for partial in (True,False):
            with self.subTest(partial=partial),tempfile.TemporaryDirectory() as tmp:
                root=Path(tmp);self.fixture(root,partial=partial,wrong_header=not partial)
                with self.assertRaises(ValueError):audit(root)

    def test_wrong_owner_rejects(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp);self.fixture(root)
            path=root/'skill-audit/captured-skills.json';data=json.loads(path.read_text());data['skills'][0]['activation_evidence'][0]['actor']=26
            path.write_text(json.dumps(data))
            with self.assertRaisesRegex(ValueError,'Incomplete owner-matched'):audit(root)

if __name__ == '__main__':unittest.main()
