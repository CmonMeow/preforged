import importlib.util,unittest
from pathlib import Path
p=Path(__file__).resolve().parents[1]/'tools/skills/review_effect_candidates.py'
s=importlib.util.spec_from_file_location('review',p);m=importlib.util.module_from_spec(s);s.loader.exec_module(m)
class Review(unittest.TestCase):
 def test_matches_do_not_claim_attribution_or_change_coverage(self):
  end=dict(file='capture',map=280,tick_ms=100,actor=25,kind=58)
  skill=dict(id=110,name='Unholy Feast',runtime_status='unsupported',local_starts=1,endpoint_evidence=[end],activation_evidence=[])
  event=dict(file='capture',map=280,tick_ms=101,actor=30,kind=20,resource=210,source_actor=None)
  rows=m.correlate([skill],[event],{110:dict(caster_body_resource=209,target_body_resource=210)})
  self.assertEqual(len(rows),1);self.assertFalse(rows[0]['source_matches']);self.assertEqual(rows[0]['runtime_status'],'unsupported')
  self.assertEqual(skill['runtime_status'],'unsupported')
  for changed in [dict(event,file='other'),dict(event,map=281),dict(event,tick_ms=99),dict(event,tick_ms=201),dict(event,resource=999)]:
   self.assertEqual(m.correlate([skill],[changed],{110:dict(caster_body_resource=209,target_body_resource=210)}),[])
 def test_stopped_cast_has_no_completion_candidate(self):
  skill=dict(id=1,name='x',local_starts=1,endpoint_evidence=[dict(kind=59)],activation_evidence=[])
  self.assertEqual(m.correlate([skill],[],{1:{}}),[])
