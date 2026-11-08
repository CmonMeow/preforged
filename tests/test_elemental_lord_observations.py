"""Detached receipt validation tests; no client or final-heal formula."""
import copy
import importlib.util
from pathlib import Path
import struct
import unittest

source=Path(__file__).resolve().parents[1]/'tools/skills/audit_elemental_lord_checkpoint.py'
spec=importlib.util.spec_from_file_location('lord_audit',source)
audit=importlib.util.module_from_spec(spec);spec.loader.exec_module(audit)

def row(tick,*words):
    return dict(tick_ms=tick,complete=True,direction='StoC',hex=struct.pack('<'+'I'*len(words),*words).hex())

class Observations(unittest.TestCase):
    def setUp(self):
        fraction=struct.unpack('<I',struct.pack('<f',15/430))[0]
        self.rows=[row(100,160,60,25,30,2193),row(200,163,55,25,25,fraction),
                   row(200,159,58,25,0),row(150,159,42,25,430)]
    def test_observation_is_not_base_cost_formula(self):
        result=audit.spell_heal_observation(2193,*self.rows)
        self.assertAlmostEqual(result['approximate_health_modifier'],15,places=5)
        self.assertNotIn('final_heal',result)
    def test_wrong_completion_or_lifetime_facts_reject(self):
        for index,key,value in [(0,'direction','CtoS'),(1,'complete',False),
                                (2,'tick_ms',201),(3,'tick_ms',201)]:
            changed=copy.deepcopy(self.rows);changed[index][key]=value
            with self.assertRaises(ValueError):audit.spell_heal_observation(2193,*changed)
    def test_wrong_self_source_reject(self):
        changed=copy.deepcopy(self.rows);changed[1]=row(200,163,55,25,26,0x3d0ee23c)
        with self.assertRaises(ValueError):audit.spell_heal_observation(2193,*changed)

if __name__=='__main__':unittest.main()
