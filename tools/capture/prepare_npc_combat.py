"""Prepare relation-only combat metadata from exact reviewed NPC observations.

Offline developer tool; no client/process/network access. It never estimates
health, armor, packed allegiance, condition eligibility or foe placement.
"""
import argparse
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def digest(raw):
    return hashlib.sha256(raw).hexdigest()


def line_hash(line):
    return digest(line.rstrip('\r\n').encode('utf-8'))


def prepare(population_path, provenance_path, output_directory):
    population_raw = population_path.read_bytes()
    population_sha = digest(population_raw)
    population = {}
    for line in population_raw.decode('utf-8').splitlines():
        fields = line.split()
        if not fields or fields[0] != 'npc':
            continue
        if len(fields) != 17:
            raise ValueError('Population row does not have17 fields')
        key = int(fields[1]), int(fields[2])
        if key in population:
            raise ValueError('Duplicate population actor')
        population[key] = fields
    provenance_raw = provenance_path.read_bytes()
    reviewed = json.loads(provenance_raw)['maps']
    contexts = {}
    needed_files = {}
    for selection in reviewed:
        key = selection['capture'], selection['map'], selection['epoch']
        if key in contexts:
            raise ValueError('Ambiguous reviewed population context')
        wanted = {selection['snapshot_line']}
        wanted.update(row['snapshot_line'] for row in selection.get('supplemental_observations', []))
        contexts[key] = {'selection': selection, 'wanted': wanted, 'snapshots': {}, 'definitions': {}}
        path = ROOT / 'data/captures' / selection['capture']
        if path.parent.resolve() != (ROOT / 'data/captures').resolve():
            raise ValueError('Capture reference escapes project capture directory')
        needed_files[path.name] = path
    file_hashes = {}
    for filename, path in sorted(needed_files.items()):
        file_hashes[filename] = digest(path.read_bytes())
        for number, line in enumerate(path.open(encoding='utf-8'), 1):
            record = json.loads(line)
            key = filename, record.get('map'), record.get('instance_epoch')
            context = contexts.get(key)
            if not context:
                continue
            if record.get('event') == 'npc_definition_observed':
                context['definitions'][record['npc_table_id']] = (record['definition'], number, line_hash(line))
            if number in context['wanted']:
                if record.get('event') != 'npc_observations':
                    raise ValueError('Pinned capture line is not an NPC snapshot')
                context['snapshots'][number] = (record, line_hash(line))
    capture_sources = {filename: 'capture_' + str(index) for index, filename in enumerate(sorted(needed_files), 1)}
    enum_path = ROOT / 'third_party/gwca-current/include/GWCA/Constants/Constants.h'
    enum_hash = digest(enum_path.read_bytes())
    enum_text = enum_path.read_text(encoding='utf-8')
    if 'Ally_NonAttackable = 0x1' not in enum_text or 'Npc_Minipet = 0x6' not in enum_text:
        raise ValueError('Reviewed GWCA allegiance definitions changed')
    policy_path = ROOT / 'docs/npc-combat.txt'
    policy_hash = digest(policy_path.read_bytes())
    output = [
        'gw-npc-combat 1 ' + population_sha,
        '# Relation-only reviewed population: no health, armor, packed allegiance or condition defaults.',
        '# Captured1 -> protected; captured6 -> conservative neutral/nonhostile private-world policy.',
    ]
    for filename, token in capture_sources.items():
        output.append('source ' + token + ' capture ' + file_hashes[filename] + ' ' +
                      json.dumps('data/captures/' + filename, ensure_ascii=False))
    output.extend([
        'source relation_policy authored ' + policy_hash + ' docs/npc-combat.txt',
    ])
    evidence = []
    covered = set()
    counts = {'protected': 0, 'neutral': 0, 'unknown': 0}
    for key, context in sorted(contexts.items(), key=lambda entry: entry[0][1]):
        filename, map_id, epoch = key
        selection = context['selection']
        if context['wanted'] != set(context['snapshots']):
            raise ValueError('Reviewed snapshot or supplement is missing')
        base, base_hash = context['snapshots'][selection['snapshot_line']]
        if base.get('instance_ms') != selection['snapshot_instance_ms']:
            raise ValueError('Reviewed base snapshot time changed')
        supplements = {row['captured_agent']: row for row in selection.get('supplemental_observations', [])}
        snapshot_agents = {row['agent']: row for row in base['npcs']}
        if len(snapshot_agents) != len(base['npcs']):
            raise ValueError('Reviewed base snapshot has duplicate actors')
        for actor in selection['population']:
            server_actor = actor['server_agent']
            population_key = map_id, server_actor
            if population_key not in population or population_key in covered:
                raise ValueError('Reviewed actor is missing, duplicated or has been renumbered')
            fields = population[population_key]
            captured_agent = actor['captured_agent']
            supplemental = supplements.get(captured_agent)
            if supplemental:
                snapshot_line = supplemental['snapshot_line']
                snapshot, snapshot_hash = context['snapshots'][snapshot_line]
                if snapshot_hash != supplemental['snapshot_sha256'] or \
                        snapshot.get('instance_ms') != supplemental['snapshot_instance_ms']:
                    raise ValueError('Reviewed supplemental snapshot changed')
                matches = [row for row in snapshot['npcs'] if row['agent'] == captured_agent]
                if len(matches) != 1:
                    raise ValueError('Reviewed supplemental actor is ambiguous')
                observed = matches[0]
                if observed['model'] != supplemental['model'] or \
                        observed['name_encoded_utf16'] != supplemental['name_encoded_utf16']:
                    raise ValueError('Reviewed supplemental actor changed')
            else:
                snapshot_line, snapshot_hash = selection['snapshot_line'], base_hash
                if captured_agent not in snapshot_agents:
                    raise ValueError('Reviewed actor is absent from the exact selected snapshot')
                observed = snapshot_agents[captured_agent]
            if observed['model'] != actor['captured_model']:
                raise ValueError('Captured NPC model identity changed')
            definition, definition_line, definition_hash = context['definitions'][observed['model']]
            visual = sum(value << (8 * index) for index, value in enumerate(definition['visual_adjustment_bytes']))
            wanted_definition = [definition['model_file'], definition['skin_file'], visual,
                                 definition['appearance'], definition['flags'], definition['primary'], observed['level']]
            if list(map(int, fields[4:11])) != wanted_definition or \
                    int(fields[3]) != actor['server_definition']:
                raise ValueError('Installed NPC appearance/level does not match reviewed capture')
            encoded_name = ''.join(format(unit, '04x') for unit in observed['name_encoded_utf16'])
            if fields[15] != encoded_name or int(fields[11]) != observed['plane'] or \
                    any(float(fields[index]) != observed[name] for index, name in [(12, 'x'), (13, 'y'), (14, 'facing')]):
                raise ValueError('Installed NPC name/placement does not match reviewed capture')
            model_files = ','.join(map(str, definition.get('model_files', []))) or '-'
            if fields[16] != model_files:
                raise ValueError('Installed NPC model-file list does not match reviewed capture')
            allegiance = observed['allegiance']
            # 6 is NPC/minipet, not proof of invulnerability or a packed faction.
            # Persist a conservative nonhostile relation; never create a foe.
            if allegiance == 1:
                relation = 'protected'
            elif allegiance == 6:
                relation = 'neutral'
            else:
                # Preserve observed map populations without inferring hostile
                # player relation from an allegiance byte. Unknown actors are
                # visible but receive no health, armor or combat permissions.
                relation = 'unknown'
            binding = [map_id, server_actor, int(fields[3]), int(fields[4]), int(fields[5]),
                       int(fields[7]), int(fields[8]), int(fields[10])]
            relation_source = capture_sources[filename] if relation != 'unknown' else '-'
            metadata = ['npc', *binding, relation, '-', '-', '-', '-', 0, 0,
                        relation_source, '-', '-', '-', '-']
            if len(metadata) != 21:
                raise AssertionError('NPC combat schema field count changed')
            output.append(' '.join(map(str, metadata)))
            counts[relation] += 1
            covered.add(population_key)
            evidence.append({
                'map': map_id, 'server_actor': server_actor, 'server_definition': int(fields[3]),
                'capture': 'data/captures/' + filename, 'capture_sha256': file_hashes[filename],
                'instance_epoch': epoch, 'snapshot_line': snapshot_line, 'snapshot_sha256': snapshot_hash,
                'definition_line': definition_line, 'definition_sha256': definition_hash,
                'captured_agent': captured_agent, 'captured_model': observed['model'],
                'captured_allegiance': allegiance, 'private_relation': relation,
                'health': 'unknown', 'earth_armor': 'unknown', 'packed_allegiance': 'unknown',
                'conditions_allowed': False, 'damageable': False,
            })
    if covered != set(population):
        raise ValueError('Population has actors missing from reviewed capture provenance')
    if digest(population_path.read_bytes()) != population_sha or \
            digest(provenance_path.read_bytes()) != digest(provenance_raw) or \
            digest(enum_path.read_bytes()) != enum_hash or digest(policy_path.read_bytes()) != policy_hash:
        raise ValueError('Reviewed population, provenance or policy changed during preparation')
    for filename, path in needed_files.items():
        if digest(path.read_bytes()) != file_hashes[filename]:
            raise ValueError('Capture changed during preparation')
    output_raw = ('\n'.join(output) + '\n').encode('utf-8')
    report = {
        'schema': 1, 'population': population_path.relative_to(ROOT).as_posix(),
        'population_sha256': population_sha, 'population_provenance': provenance_path.relative_to(ROOT).as_posix(),
        'population_provenance_sha256': digest(provenance_raw),
        'combat_catalog_sha256': digest(output_raw), 'actors': len(covered), 'relation_counts': counts,
        'policy': 'Captured1 Ally_NonAttackable becomes protected. Captured6 Npc_Minipet becomes conservative neutral/nonhostile private relation. Other captured allegiances remain unknown and non-damageable; no allegiance is treated as proof of player hostility. No relation creates combat health, armor, immunity or packed faction.',
        'gwca_enum': {'source': enum_path.relative_to(ROOT).as_posix(), 'sha256': enum_hash},
        'review_policy': {'source': policy_path.relative_to(ROOT).as_posix(), 'sha256': policy_hash},
        'rows': evidence,
    }
    # Finish all validation before replacing either prepared output. Persistent
    # runtime states and live NPC population are never touched by this tool.
    output_directory.mkdir(parents=True, exist_ok=True)
    (output_directory / 'npc-combat.txt').write_bytes(output_raw)
    (output_directory / 'npc-combat-provenance.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print('Prepared', len(covered), 'relation-only NPC definitions;', counts,
          '; health/armor/packed allegiance absent; population SHA256', population_sha)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--population', type=Path, default=ROOT / 'data/gw/npcs.txt')
    parser.add_argument('--provenance', type=Path, default=ROOT / 'data/gw/npc-population-provenance.json')
    parser.add_argument('--output-directory', type=Path, default=ROOT / 'data/gw')
    args = parser.parse_args()
    prepare(args.population, args.provenance, args.output_directory)
