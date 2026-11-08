import sys,unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools/skills'))
from publish_skill_backlog import partition,verification_header,evidence_tiers,refresh_bindings
class Backlog(unittest.TestCase):
 def test_only_local_obtainable_and_actual_binding(self):
  rows=[dict(id=i,local_starts=local,obtainable_baseline=obtainable,runtime_status=status) for i,local,obtainable,status in [(1,1,True,'implemented_binding'),(2,1,True,'unsupported'),(3,0,True,'implemented_binding'),(4,1,False,'implemented_binding')]]
  captured,pending,bound,missing=partition(dict(skills=rows,summary=dict(local_obtainable=2)),[1,2,3])
  self.assertEqual(set(captured),{1,2});self.assertEqual(pending,{2});self.assertEqual(bound,{1});self.assertEqual(missing,{3})
 def test_nonbaseline_and_other_actor_evidence_are_not_lost_or_merged(self):
  rows=[dict(id=i,local_starts=local,obtainable_baseline=baseline,other_or_unassigned_starts=other,runtime_status=status) for i,local,baseline,other,status in [(1,1,True,2,'implemented_binding'),(2,1,False,0,'unsupported'),(3,0,False,3,'unsupported'),(4,1,False,1,'implemented_binding'),(5,0,False,0,'unsupported')]]
  tiers=evidence_tiers(dict(skills=rows))
  self.assertEqual(set(tiers['nonbaseline_local']['rows']),{2,4})
  self.assertEqual(tiers['nonbaseline_local']['source_bound'],{4})
  self.assertEqual(tiers['nonbaseline_local']['awaiting_binding'],{2})
  self.assertEqual(set(tiers['other_or_unassigned']['rows']),{1,3,4})
  self.assertEqual(tiers['other_or_unassigned']['awaiting_binding'],{3})
 def test_duplicate_tier_ids_are_rejected(self):
  with self.assertRaises(ValueError):evidence_tiers(dict(skills=[dict(id=1),dict(id=1)]))
 def test_count_inconsistency_rejected(self):
  with self.assertRaises(ValueError):partition(dict(skills=[],summary=dict(local_obtainable=1)),[1])
 def test_current_bindings_join_frozen_capture_without_mutation(self):
  frozen=dict(skills=[dict(id=1227,local_starts=38,runtime_status='unsupported')],summary=dict(local_obtainable=1))
  coverage=dict(skills=[dict(id=1227,status='implemented_binding')],summary=dict(total=1))
  current=refresh_bindings(frozen,coverage)
  self.assertEqual(current['skills'][0]['local_starts'],38)
  self.assertEqual(current['skills'][0]['runtime_status'],'implemented_binding')
  self.assertEqual(frozen['skills'][0]['runtime_status'],'unsupported')
  with self.assertRaises(ValueError):refresh_bindings(frozen,dict(skills=[],summary=dict(total=0)))
 def test_verification_header_preserves_history_and_refreshes_once(self):
  text='Current capture inventory: index\nCurrent fixed checkpoint: old\nCurrent cutoff UTC: yesterday\nHistorical skill remains unbound at old checkpoint.\n'
  updated=verification_header(text,'new','today')
  self.assertIn('Current fixed checkpoint: new\n',updated)
  self.assertIn('Current cutoff UTC: today\n',updated)
  self.assertTrue(updated.endswith('Historical skill remains unbound at old checkpoint.\n'))
  self.assertEqual(verification_header(updated,'new','today'),updated)
  self.assertEqual(updated.count('Current fixed checkpoint:'),1)
  self.assertEqual(updated.count('Current cutoff UTC:'),1)
if __name__=='__main__':unittest.main()
