"""Validate explicitly reviewed, same-instance NPC visibility supplements."""
import hashlib


def supplements_at(selection, capture, map_id, epoch, row, line_number, raw_line):
    """Return only exact pinned actors; never merge an unreviewed snapshot."""
    result = []
    for item in selection.get('supplemental_observations', []):
        if item['snapshot_line'] != line_number:
            continue
        if capture != selection['capture'] or map_id != selection['map'] or \
                epoch != selection['instance_epoch'] or row.get('map') != map_id or \
                row.get('instance_epoch') != epoch or row.get('event') != 'npc_observations':
            raise RuntimeError('Reviewed supplement came from a different capture or instance')
        if row.get('instance_ms') != item['snapshot_instance_ms'] or \
                hashlib.sha256(raw_line.rstrip('\r\n').encode('utf-8')).hexdigest() != item['snapshot_sha256']:
            raise RuntimeError('Reviewed supplemental snapshot changed')
        matches = [npc for npc in row.get('npcs', []) if npc.get('agent') == item['captured_agent']]
        if len(matches) != 1:
            raise RuntimeError('Reviewed supplemental actor is missing or duplicated')
        npc = matches[0]
        if npc.get('model') != item['model'] or npc.get('name_encoded_utf16') != item['name_encoded_utf16']:
            raise RuntimeError('Reviewed supplemental actor identity changed')
        result.append((item, npc))
    return result


def append_supplements(snapshot, found, expected):
    if len(found) != len(expected):
        raise RuntimeError('A reviewed NPC visibility supplement was not captured')
    agents = {npc['agent'] for npc in snapshot}
    if len(agents) != len(snapshot):
        raise RuntimeError('Selected NPC snapshot contains duplicate actor IDs')
    result = list(snapshot)
    for expected_item in expected:
        matches = [(item, npc) for item, npc in found if item == expected_item]
        if len(matches) != 1:
            raise RuntimeError('Reviewed NPC visibility supplement is ambiguous')
        npc = matches[0][1]
        if npc['agent'] in agents:
            raise RuntimeError('NPC supplement duplicates an actor in the base population')
        agents.add(npc['agent'])
        result.append(npc)
    return result
