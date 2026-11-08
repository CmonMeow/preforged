"""Meaningful stale-capacity and malformed-capture regression; offline only."""
import importlib.util
import struct
from pathlib import Path

path = Path(__file__).resolve().parents[1] / 'tools/quests/decode_collector_windows.py'
spec = importlib.util.spec_from_file_location('collector_windows', path)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)

prefix = [0x2b33, 0xd6e2, 0xd8c7, 0x0a6a, 0x010a, 0x2906, 1, 0x0101, 0x0105]
raw = struct.pack('<II122H', 197, 2, *(prefix + [0] + [0xdead] * (121 - len(prefix))))
row = {'complete': True, 'hex': raw.hex()}
assert module.heading(row, [197, 260, 31255]) == {'type': 2, 'encoded_utf16': prefix}
for corrupt, schema in [
    ({'complete': False, 'hex': raw.hex()}, [197, 260, 31255]),
    ({'complete': True, 'hex': raw[:-2].hex()}, [197, 260, 31255]),
    ({'complete': True, 'hex': struct.pack('<II122H', 197, 256, *([1] * 122)).hex()}, [197, 260, 31255]),
    ({'complete': True, 'hex': struct.pack('<II122H', 197, 2, *([1] * 122)).hex()}, [197, 260, 31255]),
    (row, [197, 260, 31511]),
]:
    try:
        module.heading(corrupt, schema)
    except ValueError:
        pass
    else:
        raise AssertionError('invalid collector capture accepted')
print('PASS bounded native heading, stale-tail omission, schema and malformed captures')
