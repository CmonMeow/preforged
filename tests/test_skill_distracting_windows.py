import importlib.util,struct,unittest
from pathlib import Path
spec=importlib.util.spec_from_file_location('blow_windows',Path(__file__).resolve().parents[1]/'tools/skills/analyze_distracting_blow.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
def row(t,*words):return dict(direction='StoC',complete=True,tick_ms=t,opcode=words[0],hex=struct.pack('<'+'I'*len(words),*words).hex())
def scan(*rows):return m.analyze(enumerate(rows,1))
class Windows(unittest.TestCase):
 def test_primary_secondary_are_candidates_only(self):
  w=scan(row(90,160,60,3,1,281),row(100,160,50,1,2,325),row(600,159,46,1,0),row(600,159,59,2,0),row(601,159,59,3,0))[0]
  self.assertEqual(len(w['primary_stop_candidates']),1);self.assertEqual(len(w['secondary_stop_candidates']),1)
  self.assertEqual(w['secondary_stop_candidates'][0]['last_observed_action_not_proven_active']['skill'],281)
  self.assertFalse(w['causality_proven'])
 def test_interrupted_marker_pairs_only_latest_actor_stop(self):
  w=scan(row(100,160,50,1,2,325),row(600,159,46,1,0),row(600,159,59,3,0),row(601,159,59,3,0),row(601,159,35,3,0))[0]
  self.assertEqual(w['secondary_stop_candidates'][0]['interrupted_marker_lines'],[])
  self.assertEqual(w['secondary_stop_candidates'][1]['interrupted_marker_lines'],[5])
  self.assertFalse(w['causality_proven'])
 def test_replacement_does_not_match_old_finish(self):
  w=scan(row(100,160,50,1,2,325),row(200,160,50,1,2,390),row(700,159,46,1,0),row(701,159,59,3,0))[0]
  self.assertNotIn('source_finish_tick',w);self.assertFalse(w['secondary_stop_candidates'])
 def test_unrelated_start_is_retained(self):
  w=scan(row(100,160,50,1,2,325),row(150,160,60,4,3,23),row(600,159,46,1,0),row(600,159,59,3,0))[0]
  self.assertEqual(w['other_skill_starts'][0]['skill'],23);self.assertFalse(w['causality_proven'])
 def test_wrong_incomplete_and_direction_reject(self):
  a=row(100,160,50,1,2,325);a['complete']=False
  b=row(100,160,50,1,2,325);b['opcode']=159
  c=row(100,160,50,1,2,325);c['direction']='CtoS'
  self.assertFalse(scan(a,b,c))
 def test_clock_reset_does_not_carry_actions(self):
  w=scan(row(100,160,50,1,2,325),row(99,159,46,1,0))[0]
  self.assertTrue(w['clock_reset']);self.assertNotIn('source_finish_tick',w)
 def test_large_gap_cannot_be_secondary_contact(self):
  w=scan(row(100,160,50,1,2,325),row(600,159,46,1,0),row(1000,159,59,3,0))[0]
  self.assertFalse(w['secondary_stop_candidates'])
if __name__=='__main__':unittest.main()
