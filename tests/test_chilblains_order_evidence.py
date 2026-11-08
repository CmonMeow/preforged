import copy,json,sys,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/protocol'))
from chilblains_order_evidence import analyze,verify_reviewed_rows

class OrderingEvidence(unittest.TestCase):
    def setUp(self):
        fixture=json.loads((ROOT/'tests/fixtures/chilblains-order-reviewed.json').read_text())
        self.rows={r['line']:r for r in fixture['receipts']}
        self.types={180:6,1392:6,245:6}

    def test_reviewed_receipts(self):
        self.assertEqual(verify_reviewed_rows(self.rows,self.types)['selected_private_policy'],'damage_then_remove')

    def test_changed_effect_identity_rejected(self):
        self.rows[245064]['words'][2]+=1
        with self.assertRaises(ValueError):verify_reviewed_rows(self.rows,self.types)

    def test_possible_natural_expiry_rejected(self):
        self.rows[244727]['tick_ms']=self.rows[245064]['tick_ms']-60000
        with self.assertRaisesRegex(ValueError,'natural expiry'):verify_reviewed_rows(self.rows,self.types)

    def test_competing_completion_rejected(self):
        extra=copy.deepcopy(self.rows[245056]);extra['line']=245055;extra['words'][2]=11
        self.rows[245055]=extra
        with self.assertRaisesRegex(ValueError,'Competing'):verify_reviewed_rows(self.rows,self.types)

    def test_non_enchantment_metadata_rejected(self):
        self.types[180]=4
        with self.assertRaisesRegex(ValueError,'not an enchantment'):verify_reviewed_rows(self.rows,self.types)

    def test_actor_despawn_rejected(self):
        self.rows[245055]=dict(line=245055,tick_ms=self.rows[245056]['tick_ms'],words=[33,17])
        with self.assertRaisesRegex(ValueError,'lifetime changed'):verify_reviewed_rows(self.rows,self.types)

    def test_missing_addition_does_not_invent_skill_identity(self):
        rows=[r for line,r in sorted(self.rows.items()) if 244741<=line<=245064]
        result=analyze(rows,self.rows[244741],self.types)
        self.assertEqual(len(result['removals']),1)
        self.assertIsNone(result['removals'][0]['skill'])
        self.assertFalse(result['removals'][0]['enchantment'])

    def test_positive_healing_is_not_counted_as_damage(self):
        rows=[r for line,r in sorted(self.rows.items()) if line<=245064]
        rows.append(dict(line=245061,tick_ms=self.rows[245062]['tick_ms'],words=[163,55,17,90,0x3f000000]))
        result=analyze(sorted(rows,key=lambda r:r['line']),self.rows[244741],self.types)
        self.assertEqual(len(result['damage']),1)

if __name__=='__main__':unittest.main()
