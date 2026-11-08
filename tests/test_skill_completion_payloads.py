import hashlib
import importlib.util
import json
import struct
import tempfile
import unittest
from pathlib import Path

spec=importlib.util.spec_from_file_location('completion_audit',Path(__file__).resolve().parents[1]/'tools/protocol/audit_completion_payloads.py')
module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)


class CompletionPayloads(unittest.TestCase):
    def test_preserves_values_and_rejects_untrusted_packet_shapes(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);packet=root/'packets-1-1.jsonl'
            def row(kind,value,**extra):
                return dict(event='packet',complete=True,direction='StoC',tick_ms=100,
                            hex=struct.pack('<4I',159,kind,25,value).hex(),**extra)
            rows=[row(58,0),row(58,0),row(46,777),row(59,123)]
            incoming=row(58,987);incoming['direction']='CtoS';rows.append(incoming)
            partial=row(58,988);partial['complete']=False;rows.append(partial)
            targeted=row(58,989);targeted['hex']=struct.pack('<5I',160,58,25,26,989).hex();rows.append(targeted)
            packet.write_text('\n'.join(json.dumps(r) for r in rows)+'\n')
            manifest=dict(created_utc='fixed',files=[dict(snapshot=packet.name,sha256=hashlib.sha256(packet.read_bytes()).hexdigest())])
            (root/'manifest.json').write_text(json.dumps(manifest))
            result=module.audit(root)
            self.assertEqual([(r['kind'],r['value'],r['count']) for r in result['events']],[(46,777,1),(58,0,2),(59,123,1)])
            self.assertEqual(result['events'][1]['first']['line'],1)
            packet.write_text('{}\n')
            with self.assertRaisesRegex(ValueError,'Checkpoint changed'):module.audit(root)


if __name__=='__main__':unittest.main()
