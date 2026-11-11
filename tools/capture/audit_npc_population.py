"""Audit captured pre-Searing NPC visibility lifetimes; never install populations.

Snapshot disappearance ends a visibility segment, not a proven agent lifetime.
Numeric actor/model IDs are scoped to capture + map + epoch. Changed identity or
disappearance starts a new segment. Stationarity describes only observations.
"""
import argparse
from collections import Counter, defaultdict
import hashlib
import json
import math
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def capture_lines(path):
    with path.open(encoding='utf8') as stream:
        yield from enumerate(stream, 1)


def catalog(path):
    result = []
    name = ''
    for line in path.read_text(encoding='utf8').splitlines():
        if line.startswith('# '):
            name = line[2:]
        if line.startswith('npc '):
            fields = line.split()
            result.append(dict(map=int(fields[1]), name=name, x=float(fields[12]),
                               y=float(fields[13]), encoded=fields[15], path=path.relative_to(ROOT).as_posix()))
    return result


def audit(paths):
    maps = {r['id'] for r in json.loads((ROOT/'data/gw/map-catalog.json').read_text())
            if r['region_name'] == 'Presearing'}
    baseline = catalog(ROOT/'data/gw/npcs.txt')
    overlays = [r for p in sorted((ROOT/'data/gw/quests/actors').glob('*.txt')) for r in catalog(p)]
    decoded = {}
    header = (ROOT/'third_party/gwca-current/include/GWCA/Constants/AgentIDs.h').read_text(encoding='utf8')
    block = header.split('namespace Minipet {', 1)[1].split('} // namespace Minipet', 1)[0]
    miniatures = {int(v) for v in re.findall(r'constexpr int\s+\w+\s*=\s*(\d+)', block)}
    segments = []
    sources = []
    for path in paths:
        source = path.relative_to(ROOT).as_posix()
        sources.append(dict(path=source, sha256=hashlib.sha256(path.read_bytes()).hexdigest()))
        active = {}
        definitions = {}
        quests = {}
        serials = Counter()
        for line, raw in capture_lines(path):
            row = json.loads(raw)
            event = row.get('event')
            if event == 'name_decoded' and row.get('name'):
                decoded[tuple(row['name_encoded_utf16'])] = row['name']
            if row.get('map') not in maps or 'instance_epoch' not in row:
                continue
            context = (row['map'], row['instance_epoch'])
            if event == 'npc_definition_observed':
                definitions[(*context, row['npc_table_id'])] = dict(line=line, value=row['definition'])
            if event == 'quest_snapshot_changed':
                quests[context] = dict(line=line, active_quest_id=row.get('active_quest_id'), quests=row.get('quests', []))
            if event != 'npc_observations':
                continue
            present = set()
            for npc in row.get('npcs', []):
                key = (*context, npc['agent'])
                present.add(key)
                identity = (npc['model'], tuple(npc.get('name_encoded_utf16', [])))
                current = active.get(key)
                if current is None or current['_identity'] != identity:
                    serials[key] += 1
                    current = dict(source=source, map=context[0], epoch=context[1], agent=npc['agent'],
                        visibility_segment=serials[key], first_line=line, last_line=line,
                        first_instance_ms=row['instance_ms'], last_instance_ms=row['instance_ms'],
                        first=npc.copy(), definition=definitions.get((*context, npc['model'])),
                        quest_context=quests.get(context), observations=0, allegiance=set(), planes=set(),
                        bounds=[npc['x'], npc['y'], npc['x'], npc['y']], _identity=identity)
                    segments.append(current)
                    active[key] = current
                current['observations'] += 1
                current['last_line'] = line
                current['last_instance_ms'] = row['instance_ms']
                current['allegiance'].add(npc['allegiance'])
                current['planes'].add(npc['plane'])
                if npc.get('name'):
                    current['first']['name'] = npc['name']
                bounds = current['bounds']
                bounds[:] = [min(bounds[0], npc['x']), min(bounds[1], npc['y']),
                             max(bounds[2], npc['x']), max(bounds[3], npc['y'])]
            for key in list(active):
                if key[:2] != context or key not in present:
                    del active[key]
    groups = defaultdict(list)
    for item in segments:
        item.pop('_identity')
        npc = item['first']
        npc['name'] = npc.get('name') or decoded.get(tuple(npc.get('name_encoded_utf16', [])))
        item['allegiance'] = sorted(item['allegiance'])
        item['planes'] = sorted(item['planes'])
        b = item['bounds']
        item['observed_displacement_bound'] = math.hypot(b[2]-b[0], b[3]-b[1])
        item['position_class'] = ('moving_observed' if item['observed_displacement_bound'] > 1 else
            'stationary_observed' if item['observations'] >= 2 and item['last_instance_ms']-item['first_instance_ms'] >= 1000
            else 'insufficient_observation')
        definition = item.get('definition')
        item['known_miniature'] = bool(npc['model'] in miniatures and definition and
            definition['value']['flags'] & 0x18000 == 0x18000)
        encoded = ''.join(f'{u:04x}' for u in npc.get('name_encoded_utf16', []))
        same = lambda r: r['map'] == item['map'] and (r['encoded'] == encoded or bool(npc['name']) and r['name'] == npc['name'])
        existing = [r for r in baseline if same(r)]
        item['baseline_name_matches'] = len(existing)
        item['nearest_baseline_distance'] = min((math.hypot(r['x']-npc['x'], r['y']-npc['y']) for r in existing), default=None)
        item['quest_overlay_matches'] = sorted({r['path'] for r in overlays if same(r)})
        groups[(item['map'], npc['name'], encoded)].append(item)
    candidates = []
    for (map_id, name, encoded), items in sorted(groups.items(), key=lambda x: str(x[0])):
        if items[0]['baseline_name_matches']:
            continue
        stationary = [x for x in items if x['position_class'] == 'stationary_observed' and x['definition']]
        representative = max(stationary or items, key=lambda x: x['observations'])
        overlay = sorted({p for x in items for p in x['quest_overlay_matches']})
        status = ('quest_overlay_only' if overlay else 'transient_miniature' if any(x['known_miniature'] for x in items)
                  else 'missing_name' if not name else
                  'stationary_review_candidate' if stationary and all(x['allegiance'] == [6] and x['position_class'] != 'moving_observed' for x in items)
                  else 'moving_hostile_or_unresolved_context')
        candidates.append(dict(map=map_id, name=name, name_encoded_utf16=representative['first'].get('name_encoded_utf16'),
            status=status, contexts=len({(x['source'], x['epoch']) for x in items}),
            visibility_segments=len(items), quest_overlay_matches=overlay,
            representative=representative))
    return dict(schema=1, policy=[
        'Read-only evidence audit. No candidate is automatically a baseline spawn.',
        'Capture/map/epoch/agent/visibility segment isolates numeric-ID reuse; invisibility is not death.',
        'Names resolve by encoded text; model IDs never supply names across epochs.',
        'Name matches identify review groups, not proof of one spawn. Positions remain per segment.',
        'Quest snapshots are context, not proof of causal gating; existing overlays stay separate.',
        'No whole-map completeness claim: missing visibility and moving actors require further capture.'],
        sources=sources, candidates=candidates, segments=segments)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT/'build/reports/npc-population/audit.json')
    parser.add_argument('captures', nargs='*', type=Path)
    args = parser.parse_args()
    report = audit(args.captures or sorted((ROOT/'data/captures').rglob('world-*.jsonl')))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2, ensure_ascii=False)+'\n', encoding='utf8')
    print(json.dumps(dict(sources=len(report['sources']), segments=len(report['segments']),
                         candidates=dict(Counter(x['status'] for x in report['candidates'])))))
