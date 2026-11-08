import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

module_path = Path(__file__).resolve().parents[2] / 'tools/quests/index_npc_interactions.py'
spec = importlib.util.spec_from_file_location('npc_index', module_path)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class InteractionIndexTest(unittest.TestCase):
    def test_reused_ids_are_not_joined_across_zones(self):
        with tempfile.TemporaryDirectory() as directory:
            world = Path(directory) / 'world.jsonl'
            packets = Path(directory) / 'packets.jsonl'
            rows = [
                {'event': 'quest_snapshot_changed', 'instance_epoch': 1, 'tick_ms': 1000, 'instance_ms': 50},
                {'event': 'npc_observations', 'instance_epoch': 1, 'map': 146, 'instance_ms': 60,
                 'npcs': [{'agent': 7, 'name': 'Collector A', 'model': 2}]},
                {'event': 'quest_snapshot_changed', 'instance_epoch': 2, 'tick_ms': 1100, 'instance_ms': 10},
                {'event': 'npc_observations', 'instance_epoch': 2, 'map': 148, 'instance_ms': 100,
                 'npcs': [{'agent': 7, 'name': 'Collector B', 'model': 3}]},
            ]
            world.write_text(''.join(json.dumps(r)+'\n' for r in rows))
            packets.write_text(''.join(json.dumps({'event': 'packet', 'opcode': opcode,
                'tick_ms': tick, 'hex': opcode.to_bytes(4, 'little').hex()+'07000000'})+'\n'
                for tick, opcode in [(1050, 129), (1150, 196), (1200, 196), (4000, 129)]))
            entries = module.index(world, packets)['interactions']
            self.assertEqual(entries[0]['name'], 'Collector A')
            self.assertEqual(entries[1]['status'], 'unresolved')
            self.assertEqual(entries[2]['name'], 'Collector B')
            self.assertEqual(entries[2]['event'], 'window_owner')
            self.assertEqual(entries[3]['status'], 'unresolved')


if __name__ == '__main__':
    unittest.main()
