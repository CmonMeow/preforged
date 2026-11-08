import importlib.util
import unittest
from pathlib import Path

SPEC = importlib.util.spec_from_file_location('pvp_maps',
    Path(__file__).resolve().parents[1] / 'tools/skills/pvp_map_skill_activations.py')
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


class PvpMapSkillActivationsTests(unittest.TestCase):
    def test_known_pvp_windows_only_and_other_actor_is_not_assumed_player(self):
        audit = dict(checkpoint='fixed', cutoff='now', skills=[
            dict(id=10, name='Ordinary Skill', runtime_status='unsupported',
                 known_map_activations={
                     '2': dict(local=1, other=4, sample=dict(file='packets.jsonl', line=7, tick_ms=100)),
                     '55': dict(local=8, other=2, sample=dict(file='packets.jsonl', line=8, tick_ms=200))}),
            dict(id=20, name='PvP Variant', runtime_status='implemented_binding',
                 known_map_activations={
                     '120': dict(local=0, other=3, sample=dict(file='packets.jsonl', line=9, tick_ms=300))}),
        ])
        catalog = [dict(id=2, region_type_name='Arena'),
                   dict(id=55, region_type_name='City'),
                   dict(id=120, region_type_name='HeroesAscent')]
        result = MODULE.report(audit, catalog)
        self.assertEqual(result['summary'], dict(distinct=2, local=1, other=7,
            awaiting_runtime_binding=1))
        self.assertEqual([row['map'] for row in result['skills'][0]['maps']], [2])
        self.assertEqual(result['skills'][1]['maps'][0]['other'], 3)
        self.assertIn('NPC casts', result['limitations'][1])


if __name__ == '__main__':
    unittest.main()
