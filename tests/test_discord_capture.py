import copy,json,sys,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/protocol'))
from discord_2022 import positive_sequence
class DiscordOrdering(unittest.TestCase):
 @classmethod
 def setUpClass(cls):
  cls.rows={};wanted={68862,68988,68989,68990}
  path=ROOT/'data/captures/skill-checkpoints/20261022-cumulative-skills/packets-110488-195213578.jsonl'
  with path.open(encoding='utf-8') as stream:
   for line,text in enumerate(stream,1):
    if line in wanted:cls.rows[line]=dict(json.loads(text),line=line)
    if line>=max(wanted):break
 def test_valid_batch(self):self.assertTrue(positive_sequence(self.rows,68862,68988,68989,68990)['finish_before_body_before_damage'])
 def test_early_damage_rejected(self):
  r=copy.deepcopy(self.rows);r[68990]['line']=68987
  with self.assertRaises(ValueError):positive_sequence(r,68862,68988,68989,68990)
 def test_wrong_target_rejected(self):
  r=copy.deepcopy(self.rows);r[68990]['hex']=r[68990]['hex'].replace('16000000','17000000',1)
  with self.assertRaises(ValueError):positive_sequence(r,68862,68988,68989,68990)
 def test_different_batch_rejected(self):
  r=copy.deepcopy(self.rows);r[68990]['tick_ms']+=1
  with self.assertRaises(ValueError):positive_sequence(r,68862,68988,68989,68990)
if __name__=='__main__':unittest.main()
