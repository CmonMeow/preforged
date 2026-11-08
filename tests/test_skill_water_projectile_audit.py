"""Exercise capture attribution boundaries without a live client."""
import contextlib
import hashlib
import importlib.util
import io
import json
from pathlib import Path
import struct
import tempfile
import unittest

spec = importlib.util.spec_from_file_location('water_audit',Path(__file__).resolve().parents[1]/'tools/skills/audit_water_projectile_flights.py')
audit = importlib.util.module_from_spec(spec); spec.loader.exec_module(audit)


class WaterProjectileAuditTests(unittest.TestCase):
    def capture(self, root, *, targeted=True, changed=False, plane=0, epoch=1, seconds=.5):
        def packet(tick, *fields):
            return dict(tick_ms=tick,opcode=fields[0],complete=True,direction='StoC',hex=struct.pack('<'+'I'*len(fields),*fields).hex())
        start = (160,60,7,8,237) if targeted else (159,60,7,237)
        packets = [packet(1100,*start)]
        for tick in (1200,1201,1202):
            raw = struct.pack('<IIffIfIII',164,7,1200.,0.,plane,seconds,417,1,0)
            packets.append(dict(tick_ms=tick,opcode=164,complete=True,direction='StoC',hex=raw.hex()))
        npc = dict(agent=7,x=0.,y=0.,plane=0,model=20,level=20,allegiance=3,name_encoded_utf16='abc')
        after = dict(npc); after['model'] += int(changed)
        world = [dict(event='anchor',tick_ms=1000,instance_ms=0,instance_epoch=1),
                 dict(event='npc_observations',instance_ms=150,instance_epoch=1,map=2,npcs=[npc])]
        if epoch != 1:
            world.append(dict(event='anchor',tick_ms=1250,instance_ms=0,instance_epoch=epoch))
        world.append(dict(event='npc_observations',instance_ms=300 if epoch == 1 else 50,instance_epoch=epoch,map=2,npcs=[after]))
        files = []
        for name, rows in [('packets-1-test.jsonl',packets),('world-1-test.jsonl',world)]:
            p = root/name; p.write_text(''.join(json.dumps(r)+'\n' for r in rows),encoding='utf-8')
            files.append(dict(snapshot=name,sha256=hashlib.sha256(p.read_bytes()).hexdigest()))
        (root/'manifest.json').write_text(json.dumps(dict(created_utc='fixed',files=files)))

    def analyze(self, **kwargs):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder); self.capture(root,**kwargs)
            with contextlib.redirect_stdout(io.StringIO()): return audit.analyze(root)

    def test_targeted_and_untargeted_preserve_three_launches(self):
        for targeted in (False,True):
            report = self.analyze(targeted=targeted)
            self.assertEqual(report['candidate_count'],3)
            self.assertEqual(report['summary'][2]['within_001'],3)
            self.assertEqual(report['summary'][2]['cluster_hypothesis'],2400)

    def test_identity_plane_and_epoch_mismatch_rejected(self):
        for params in (dict(changed=True),dict(plane=1),dict(epoch=2)):
            report = self.analyze(**params)
            self.assertEqual(report['candidate_count'],3)
            self.assertEqual(report['stationary_brackets'],[])

    def test_noncluster_measurements_retained(self):
        report = self.analyze(seconds=.501)
        self.assertEqual(len(report['stationary_brackets']),3)
        self.assertEqual(report['summary'][2]['outliers'],3)

    def test_changed_frozen_source_rejected(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder); self.capture(root)
            with (root/'packets-1-test.jsonl').open('a') as stream: stream.write('\n')
            with self.assertRaisesRegex(ValueError,'Frozen source changed'): audit.analyze(root)


if __name__ == '__main__': unittest.main()
