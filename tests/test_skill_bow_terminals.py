import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import tempfile
import unittest
spec=importlib.util.spec_from_file_location('bow_audit',Path(__file__).resolve().parents[1]/'tools/skills/audit_bow_terminals.py')
audit=importlib.util.module_from_spec(spec);spec.loader.exec_module(audit)


class BowReceipts(unittest.TestCase):
 def capture(self,root,melee=False,release=True,bad_hash=False):
  words=[(160,50,25,30,399)]
  if melee:words.append((159,46,25,0))
  if release:words.append((227,25,399,0))
  rows=[dict(event='packet',direction='StoC',complete=True,tick_ms=100+n,opcode=w[0],hex=struct.pack('<'+'I'*len(w),*w).hex()) for n,w in enumerate(words)]
  path=root/'packets-test.jsonl';path.write_text(''.join(json.dumps(row)+'\n' for row in rows),encoding='utf-8')
  digest=hashlib.sha256(path.read_bytes()).hexdigest()
  (root/'manifest.json').write_text(json.dumps(dict(created_utc='fixed',files=[dict(snapshot=path.name,sha256='bad' if bad_hash else digest)])))
  (root/'skill-audit').mkdir()
  event=dict(file=path.name,line=1,tick_ms=100,actor=25,hex=rows[0]['hex'])
  (root/'skill-audit/captured-skills.json').write_text(json.dumps(dict(skills=[dict(id=399,activation_evidence=[event])])) )
 def test_verified_release(self):
  with tempfile.TemporaryDirectory() as temp:
   root=Path(temp);self.capture(root);self.assertEqual(audit.audit(root)['reviewed_windows'],1)
 def test_melee_terminal_contradicts_absence(self):
  with tempfile.TemporaryDirectory() as temp:
   root=Path(temp);self.capture(root,melee=True)
   with self.assertRaises(ValueError):audit.audit(root)
 def test_missing_release_is_not_verified(self):
  with tempfile.TemporaryDirectory() as temp:
   root=Path(temp);self.capture(root,release=False)
   with self.assertRaises(ValueError):audit.audit(root)
 def test_changed_capture_rejects(self):
  with tempfile.TemporaryDirectory() as temp:
   root=Path(temp);self.capture(root,bad_hash=True)
   with self.assertRaises(ValueError):audit.audit(root)
