"""Identity-boundary checks for offline population evidence."""
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location('audit', Path(__file__).resolve().parents[1]/'tools/capture/audit_npc_population.py')
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)


class PopulationAuditTest(unittest.TestCase):
    def test_visibility_identity_epoch_movement_and_overlay_boundaries(self):
        old_root = audit.ROOT
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            try:
                audit.ROOT = root
                (root/'data/gw/quests/actors').mkdir(parents=True)
                (root/'data/gw/map-catalog.json').write_text('[{"id":146,"region_name":"Presearing"}]')
                (root/'data/gw/npcs.txt').write_text('gw-npcs 1\n')
                (root/'data/gw/quests/actors/test.txt').write_text('gw-npcs 1\n# Quest actor\nnpc 146 20001 1 1 0 0 0 0 1 2 0 0 0 0 0002 -\n')
                header = root/'third_party/gwca-current/include/GWCA/Constants/AgentIDs.h'
                header.parent.mkdir(parents=True)
                header.write_text('namespace Minipet { constexpr int Toy = 99; } // namespace Minipet')
                rows = []
                def observation(epoch, tick, model=1, encoded=1, x=0, present=True):
                    rows.append(dict(event='npc_definition_observed', map=146, instance_epoch=epoch,
                                     npc_table_id=model, definition=dict(flags=524)))
                    rows.append(dict(event='npc_observations', map=146, instance_epoch=epoch, instance_ms=tick,
                        npcs=[dict(agent=7, model=model, x=x, y=0, plane=0, allegiance=6,
                                   name_encoded_utf16=[encoded], name=None)] if present else []))
                observation(1, 0)
                observation(1, 1000, x=20)
                observation(1, 2000, present=False)
                observation(1, 3000)
                observation(1, 4000, model=2, encoded=2)
                observation(2, 0)
                rows.append(dict(event='name_decoded', name_encoded_utf16=[1], name='Decoded actor'))
                rows.append(dict(event='name_decoded', name_encoded_utf16=[2], name='Quest actor'))
                path = root/'capture.jsonl'
                path.write_text('\n'.join(json.dumps(r) for r in rows))
                report = audit.audit([path])
                self.assertEqual(len(report['segments']), 4)
                self.assertEqual(report['segments'][0]['position_class'], 'moving_observed')
                self.assertEqual(report['segments'][1]['visibility_segment'], 2)
                self.assertEqual(report['segments'][2]['visibility_segment'], 3)
                self.assertEqual(report['segments'][3]['visibility_segment'], 1)
                self.assertEqual(report['segments'][0]['first']['name'], 'Decoded actor')
                self.assertEqual(next(c for c in report['candidates'] if c['name']=='Quest actor')['status'], 'quest_overlay_only')
            finally:
                audit.ROOT = old_root


if __name__ == '__main__':
    unittest.main()
