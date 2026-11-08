import hashlib,json,sys,tempfile,unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools/skills'))
from merge_packet_checkpoints import merge

class MergeTests(unittest.TestCase):
 def snapshot(self,root,content):
  root.mkdir();name='packets-1-1.jsonl';(root/name).write_bytes(content)
  entry=dict(snapshot=name,snapshot_bytes=len(content),sha256=hashlib.sha256(content).hexdigest(),complete_lines=content.count(b'\n'))
  (root/'manifest.json').write_text(json.dumps(dict(created_utc='2026-10-22T00:00:00+00:00',files=[entry])))
  return root/name
 def test_fullest_immutable_prefix(self):
  with tempfile.TemporaryDirectory() as tmp:
   p=Path(tmp);short=self.snapshot(p/'a',b'{}\n');long=self.snapshot(p/'b',b'{}\n{}\n')
   r=merge(p/'out',[p/'a',p/'b']);self.assertEqual(r['files'][0]['snapshot_bytes'],6)
   self.assertTrue((p/'out'/long.name).samefile(long));self.assertEqual(short.read_bytes(),b'{}\n')
 def test_conflict_and_changed_hash_fail_before_output(self):
  with tempfile.TemporaryDirectory() as tmp:
   p=Path(tmp);a=self.snapshot(p/'a',b'{}\n');self.snapshot(p/'b',b'[]\n')
   with self.assertRaises(ValueError):merge(p/'out',[p/'a',p/'b'])
   self.assertFalse((p/'out').exists());a.write_bytes(b'[]\n')
   with self.assertRaises(ValueError):merge(p/'out',[p/'a'])
   self.assertFalse((p/'out').exists())
 def test_existing_output_not_overwritten(self):
  with tempfile.TemporaryDirectory() as tmp:
   p=Path(tmp);self.snapshot(p/'a',b'{}\n');(p/'out').mkdir()
   with self.assertRaises(FileExistsError):merge(p/'out',[p/'a'])
if __name__=='__main__':unittest.main()
