"""Synthetic parser tests only; these do not prove retail Scythe mechanics."""
import gzip,importlib.util,tempfile,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
def load(name,path):
 spec=importlib.util.spec_from_file_location(name,ROOT/path);module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);return module
AUDIT=load('scythe_audit','tools/observer/audit_scythe_geometry_cache_draft.py')
BOUNDS=load('scythe_bounds','tools/observer/summarize_scythe_geometry_cache_draft.py')
class ObserverScytheSchema(unittest.TestCase):
 def test_actual_snapshot_indices_and_replay_time(self):
  fields=['0']*51
  for index,value in {0:'2',1:'3',2:'-8',3:'.75',7:'1',9:'.5',25:'46',29:'12',30:'24',32:'1',33:'5',34:'1.5',48:'1'}.items():fields[index]=value
  with tempfile.TemporaryDirectory()as d:
   path=Path(d)/'agent.gz'
   with gzip.open(path,'wt')as stream:stream.write('[01:00.005] '+';'.join(fields)+'\n')
   frame=AUDIT.frames(path)[0]
  self.assertEqual(frame['time_ms'],60005);self.assertEqual(frame['weapon_type'],5);self.assertEqual(frame['weapon_item_type'],46);self.assertTrue(frame['moving']);self.assertEqual(frame['move_x'],12)
 def test_bracket_keeps_exact_sample_not_future_and_reports_age(self):
  values=[{'time_ms':10,'x':1,'y':0,'rotation':0},{'time_ms':20,'x':1,'y':0,'rotation':0}]
  exact=AUDIT.pose((values,[10,20]),10);self.assertEqual(exact['before'],exact['after']);self.assertEqual(exact['max_sample_gap_ms'],0)
  middle=AUDIT.pose((values,[10,20]),15);self.assertEqual(middle['max_sample_gap_ms'],5);self.assertTrue(middle['same_position_endpoints']);self.assertIsNone(AUDIT.pose((values,[10,20]),21))
 def test_conditional_box_bounds_do_not_claim_exact_trajectory(self):
  a={'before':{'x':0,'y':0},'after':{'x':2,'y':0}};b={'before':{'x':5,'y':0},'after':{'x':8,'y':0}}
  bound=BOUNDS.box_interval(a,b);self.assertEqual(bound['conditional_lower'],3);self.assertEqual(bound['conditional_upper'],8);self.assertIn('not certified',bound['condition'])
if __name__=='__main__':unittest.main()
