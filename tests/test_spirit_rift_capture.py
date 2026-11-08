import copy,importlib.util,json,sys,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
# The pure receipt check lives in the native verifier; importing it also
# authenticates the pinned client, so run with protocol dependency PYTHONPATH.
sys.path.insert(0,str(ROOT/'tools/protocol'))
from spirit_rift_2022 import impact_sequence
class OrderTests(unittest.TestCase):
 @classmethod
 def setUpClass(cls):
  cls.rows={};wanted={351063,351087,351088,351154,351155,351156}
  p=ROOT/'data/captures/skill-checkpoints/20261022-cumulative-skills/packets-19472-19186421.jsonl'
  with p.open(encoding='utf-8') as stream:
   for line,text in enumerate(stream,1):
    if line in wanted:cls.rows[line]=dict(json.loads(text),line=line)
    if line>=max(wanted):break
 def test_observed_sequence(self):
  self.assertEqual(impact_sequence(self.rows)['delay_ms'],3000)
 def test_condition_before_damage_rejected(self):
  rows=copy.deepcopy(self.rows);rows[351156]['line']=351154
  with self.assertRaises(ValueError):impact_sequence(rows)
 def test_wrong_timing_rejected(self):
  rows=copy.deepcopy(self.rows);rows[351155]['tick_ms']+=1
  with self.assertRaises(ValueError):impact_sequence(rows)
 def test_wrong_target_rejected(self):
  rows=copy.deepcopy(self.rows);rows[351155]['hex']=rows[351155]['hex'].replace('0e000000','0f000000',1)
  with self.assertRaises(ValueError):impact_sequence(rows)
if __name__=='__main__':unittest.main()
