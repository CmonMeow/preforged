"""Reject ambiguous particle/cast attribution in immutable overlap discovery."""
import struct,sys,unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools/skills'))
from audit_reversal_life_steal_overlap import scan

def packet(words,tick=100):
 raw=struct.pack('<'+'I'*len(words),*words)
 return dict(direction='StoC',complete=True,opcode=words[0],tick_ms=tick,hex=raw.hex())
def applied(duration=8.):return packet([66,22,307,3,7,struct.unpack('<I',struct.pack('<f',duration))[0]])
class Overlap(unittest.TestCase):
 def test_targeted_and_damage_label_candidates(self):
  result=scan(enumerate([applied(),packet([160,60,44,22,153],200),packet([159,10,22,153],300)],1))
  self.assertEqual(result['counts']['candidate_records'],2)
  self.assertEqual(result['candidates'][0]['caster'],44);self.assertIsNone(result['candidates'][1]['caster'])
 def test_particle_and_untargeted_cast_do_not_infer_target(self):
  result=scan(enumerate([applied(),packet([160,20,44,22,153],200),packet([159,60,44,153],300)],1))
  self.assertEqual(result['counts'].get('candidate_records',0),0)
  self.assertEqual(result['counts']['life_steal_skill_records'],1)
 def test_retirement_grace_and_expiry_are_bounded(self):
  rows=[applied(),packet([68,22,7],200),packet([159,10,22,153],300),packet([159,10,22,153],301),packet([159,10,22,153],9000)]
  result=scan(enumerate(rows,1));self.assertEqual(len(result['candidates']),1)
  self.assertEqual(result['candidates'][0]['reversal']['removed_tick'],200)
 def test_bad_duration_direction_and_partial_are_ignored(self):
  for bad in [applied(float('nan')),applied(-1),applied(9),dict(applied(),complete=False),dict(applied(),direction='CtoS')]:
   result=scan(enumerate([bad,packet([159,10,22,153],200)],1));self.assertFalse(result['candidates'])
 def test_cast_sighting_does_not_establish_effect_ownership(self):
  result=scan(enumerate([packet([160,60,23,22,307]),packet([159,10,22,153],200)],1));self.assertFalse(result['candidates'])
 def test_other_receiver_and_old_effect_removal_do_not_match(self):
  result=scan(enumerate([applied(),packet([68,22,99],150),packet([159,10,23,153],200),packet([159,10,22,153],300)],1))
  self.assertEqual(len(result['candidates']),1);self.assertNotIn('removed_tick',result['candidates'][0]['reversal'])
if __name__=='__main__':unittest.main()
