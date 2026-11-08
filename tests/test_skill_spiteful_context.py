"""Frozen Spiteful action context must not invent a damage source or cause."""
import contextlib,hashlib,io,json,struct,sys,tempfile,unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools/skills'))
from audit_spiteful_checkpoint import audit

class SpitefulContext(unittest.TestCase):
 def row(self,tick,*words,**extra):
  return dict(tick_ms=tick,opcode=words[0],complete=True,direction='StoC',
   hex=struct.pack('<'+'I'*len(words),*words).hex(),**extra)
 def run_capture(self,rows,changed=False):
  with tempfile.TemporaryDirectory() as tmp:
   root=Path(tmp);name='packets-110488-195213578.jsonl'
   raw=('\n'.join(json.dumps(r) for r in rows)+'\n').encode();(root/name).write_bytes(raw)
   digest=hashlib.sha256(raw).hexdigest()
   (root/'manifest.json').write_text(json.dumps(dict(created_utc='fixture',files=[dict(snapshot=name,sha256='0'*64 if changed else digest)])))
   with contextlib.redirect_stdout(io.StringIO()):return audit(root,root/'report.json')
 def test_pair_and_surrounding_skill_finish_are_context(self):
  r=self.run_capture([self.row(100,159,60,10,1239),self.row(1100,159,10,10,121),
   self.row(1100,163,55,10,369,0),self.row(1100,159,58,10,0)])
  self.assertEqual((r['damage_markers'],r['paired_damage']),(1,1));s=r['samples'][0]
  self.assertEqual(s['damage_caster'],369);self.assertEqual(s['previous_actions'][0]['words'][-1],1239)
  self.assertEqual(s['following_terminal']['tick_ms'],1100)
  self.assertNotIn('confirmed_cause',s)
 def test_pair_requires_immediate_matching_recipient_and_tick(self):
  for rows in ([self.row(100,159,10,10,121),self.row(100,163,55,11,369,0)],
   [self.row(100,159,10,10,121),self.row(101,163,55,10,369,0)],
   [self.row(100,159,10,10,121),self.row(100,159,6,10,1),self.row(100,163,55,10,369,0)]):
   r=self.run_capture(rows);self.assertEqual(r['paired_damage'],0);self.assertNotIn('damage_caster',r['samples'][0])
 def test_clock_regression_clears_action_context(self):
  r=self.run_capture([self.row(100,159,60,10,1239),self.row(50,159,10,10,121)])
  self.assertEqual(r['clock_regressions'],1);self.assertEqual(r['samples'][0]['previous_actions'],[])
 def test_old_actions_and_other_actors_do_not_become_context(self):
  r=self.run_capture([self.row(0,159,60,10,1239),self.row(10000,159,60,11,1239),self.row(10001,159,10,10,121)])
  self.assertEqual(r['samples'][0]['previous_actions'],[])
 def test_source_digest_and_metadata_corruption_reject(self):
  with self.assertRaisesRegex(ValueError,'Changed fixed'):self.run_capture([self.row(100,159,10,10,121)],True)
  row=self.row(100,159,10,10,121);row['opcode']=160
  with self.assertRaisesRegex(ValueError,'Header/opcode'):self.run_capture([row])

if __name__=='__main__':unittest.main()
