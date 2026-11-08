"""Offline candidate evidence check; never promotes a runtime gate or edits logs."""
import hashlib
import importlib.util
import json
import math
from pathlib import Path

root = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('portal_import', root/'tools/portals/import_captured_routes.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
candidate = root/'data/gw/portal-candidates/lakeside-regent.txt'
fields = next(row.split() for row in candidate.read_text().splitlines()
              if row.startswith('capture_lakeside_regent_'))
assert len(fields) == 18 and list(map(int, fields[1:6])) == [146, 162, 113021, 113437, 0]
box = tuple(map(float, fields[6:10]))
arrival = tuple(map(float, fields[11:14]))
outward = tuple(map(float, fields[14:16]))
source = module.read_mesh(root/'data/gw/navigation/113021.fwnav')
destination = module.read_mesh(root/'data/gw/navigation/113437.fwnav')
assert module.rect_in_one_cell(box, source, 0), 'full trigger must fit actual source geometry'
assert any(module.contains(cell, *arrival[:2]) for cell in destination['planes'][0])
assert abs(math.hypot(*outward)-1) < 1e-7
capture = root/'data/captures/pre-searing-four-professions-2026-10-11/world-30352-81344734.jsonl'
hashes = {1574: '137e38818e27e427ae1c5cfd0bc01856740caed07eb0340c41a6bc9f136afd09',
          7180: 'd6cc36db4e8a9f2890a1fb4b59a1ae31d06d50a0af402d007c5eb186e7e20d5e'}
verified = 0
for number, line in enumerate(capture.open(encoding='utf8'), 1):
    if number not in hashes:
        continue
    assert hashlib.sha256(line.rstrip('\r\n').encode()).hexdigest() == hashes[number]
    record = json.loads(line)
    a, b = record['from'], record['to']
    assert record['event'] == 'local_map_transition_observed' and record['loading_observed']
    assert record['departure_sample_age_ms'] <= 16
    assert (a['map'], b['map'], a['plane'], b['plane']) == (146, 162, 0, 0)
    assert box[0] <= a['x'] <= box[2] and box[1] <= a['y'] <= box[3]
    assert (b['x'], b['y'], b['facing']) == arrival
    verified += 1
assert verified == 2
pages = {module.norm(path.stem): path for path in module.DEFAULT_WIKI.glob('*.html')}
exits, _ = module.wiki_exits(pages, 'Lakeside County')
assert module.norm('Regent Valley') in exits
print('PASS two exact captured departures, identical directed arrival, Wiki exit and pinned geometry')
