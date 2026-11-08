"""Offline receipt check for the fixed Quickening Zephyr appearance only."""
from pathlib import Path
import hashlib
import json
import struct

ROOT = Path(__file__).resolve().parents[1]
CHECKPOINT = ROOT / 'data/captures/skill-checkpoints/20261024-all-completed'


def pinned_rows(filename, expected_sha256, wanted):
    path = CHECKPOINT / filename
    with path.open('rb') as source:
        assert hashlib.file_digest(source, 'sha256').hexdigest() == expected_sha256
    with path.open(encoding='utf-8') as source:
        return {number: json.loads(raw) for number, raw in enumerate(source, 1)
                if number in wanted}


def main():
    world = pinned_rows('world-1360-15811984.jsonl',
        'de6f061799d3db9f7406f55e5e0da20fff0c5e46daaa634e1e4d952d0fd4882f',
        {34101, 34102})
    definition = world[34101]
    assert (definition['map'], definition['instance_epoch'], definition['npc_table_id']) == (280, 310, 2937)
    assert definition['definition'] == dict(model_file=141876, skin_file=0,
        appearance=0, flags=16384, primary=2, secondary=0, default_level=0,
        visual_adjustment_bytes=[0, 0, 0, 80], model_files=[],
        name_encoded_utf16=[3992, 34034, 51222, 10638],
        name='Spirit of Quickening Zephyr', name_status='decoded')
    actor = next(actor for actor in world[34102]['npcs'] if actor['agent'] == 97)
    assert (actor['model'], actor['level'], actor['allegiance'], actor['name']) == (
        2937, 1, 4, 'Spirit of Quickening Zephyr')
    packet = pinned_rows('packets-1360-15821281.jsonl',
        '59e2ae82e4c8187a88b5f3f8cac919a085a14aca314834ada70117039f3287d6',
        {499268})[499268]
    assert packet['tick_ms'] == 26811078 and packet['complete']
    words = struct.unpack('<13I', bytes.fromhex(packet['hex']))
    assert words[:9] == (86, 2937, 141876, 0, 0x50000000, 0, 16384, 2, 0)
    print('PASS fixed Zephyr observed appearance/name and actor97 level; ownership/2022 rendering unproven')


if __name__ == '__main__':
    main()
