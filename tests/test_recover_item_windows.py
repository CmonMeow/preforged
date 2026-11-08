"""Protect window order and definition provenance during offline recovery."""
import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import sys
import tempfile
import unittest

directory = Path(__file__).resolve().parents[1] / 'tools/quests'
sys.path.insert(0, str(directory))
spec = importlib.util.spec_from_file_location('item_windows', directory / 'recover_item_windows.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


def packet(opcode, raw):
    return dict(event='packet', direction='StoC', opcode=opcode, tick_ms=100,
                complete=True, bytes=len(raw), hex=raw.hex())


def window(item_ids=(4, 5), definitions=True):
    rows = [packet(196, struct.pack('<II', 196, 17))]
    if definitions:
        for identity in item_ids:
            raw = bytearray(436)
            struct.pack_into('<12I', raw, 0, 353, identity, 800, 30, 0, 0, 0, 0, 0, 0, 7, 1)
            struct.pack_into('<I', raw, 176, 1)
            struct.pack_into('<I', raw, 180, 0x12345678)
            rows.append(packet(353, raw))
    raw = bytearray(72)
    struct.pack_into('<II', raw, 0, 132, len(item_ids))
    struct.pack_into('<' + 'I' * len(item_ids), raw, 8, *item_ids)
    rows.append(packet(132, raw))
    raw = bytearray(252)
    struct.pack_into('<II', raw, 0, 197, 2)
    rows.append(packet(197, raw))
    return rows


class RecoverItemWindows(unittest.TestCase):
    def recover(self, rows):
        schemas = [dict(event='schema', opcode=353, fields=module.GENERAL),
                   dict(event='schema', opcode=132, fields=[132, 4139, 6]),
                   dict(event='schema', opcode=197, fields=[197, 260, 31255])]
        raw = ''.join(json.dumps(row) + '\n' for row in schemas + rows).encode()
        with tempfile.TemporaryDirectory() as name:
            path = Path(name) / 'packets-1-2.jsonl'
            path.write_bytes(raw)
            return module.recover(path, hashlib.sha256(raw).hexdigest())

    def test_order_is_packet_order_not_definition_order(self):
        rows = window()
        rows[1], rows[2] = rows[2], rows[1]
        result = self.recover(rows)['windows'][0]
        self.assertEqual([item['id'] for item in result['ordered_items']], [4, 5])
        self.assertTrue(result['collector_cost_first'])
        self.assertEqual(result['ordered_items'][0]['evidence']['definition']['modifiers'], [0x12345678])
        self.assertIsNone(result['npc_name'])

    def test_old_window_definitions_do_not_leak(self):
        result = self.recover(window() + window(definitions=False))['windows']
        self.assertTrue(all(item['evidence'] for item in result[0]['ordered_items']))
        self.assertTrue(all(item['evidence'] is None for item in result[1]['ordered_items']))

    def test_incomplete_list_does_not_claim_order(self):
        rows = window()
        rows[-2].update(complete=False, bytes=4, hex=struct.pack('<I', 132).hex())
        result = self.recover(rows)['windows'][0]
        self.assertEqual(result['status'], 'incomplete_window_evidence')
        self.assertFalse(result['collector_cost_first'])

    def test_out_of_bounds_count_is_rejected(self):
        rows = window()
        raw = bytearray.fromhex(rows[-2]['hex'])
        struct.pack_into('<I', raw, 4, 17)
        rows[-2]['hex'] = raw.hex()
        with self.assertRaisesRegex(ValueError, 'count exceeds'):
            self.recover(rows)

    def test_full_inline_name_does_not_read_past_capacity(self):
        rows = window()
        raw = bytearray.fromhex(rows[1]['hex'])
        struct.pack_into('<64H', raw, 48, *([0x8101] * 64))
        rows[1]['hex'] = raw.hex()
        item = self.recover(rows)['windows'][0]['ordered_items'][0]['evidence']['definition']
        self.assertEqual(len(item['encoded_name']), 64)
        self.assertFalse(item['name_terminated'])


if __name__ == '__main__':
    unittest.main()
