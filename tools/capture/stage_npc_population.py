"""Stage named stationary NPCs for review, without altering runtime catalogs."""
import json
from pathlib import Path
from audit_npc_population import ROOT

# Explicit review shortlist. Excludes cinematic participants, unnamed crowds,
# wildlife, known scripted spectres, miniatures and existing quest overlays.
SHORTLIST = {
    146: ['Alison the Tanner', 'Brother Mhenlo', 'Chantalle the Troubadour', 'Devona',
          'Gate Guard Torin', 'Hamish [Merchant]', 'Humphreys [Collector]',
          'Jacobs [Collector]', 'Kaylee  [Collector]', 'Miller Upton', 'Walden [Collector]'],
    148: ['Ascalonian Scribe', 'Cynn', 'Keeth [Festival Hats]', 'King Adelbern', 'Serenity [Costume Maker]'],
    160: ['Farrah Cappo', 'Grazden the Protector'],
    162: ['Ivor Trueshot', 'Jarrel the Tamer', 'Master Ranger Nente', 'Rownan [Collector]', 'Varis [Collector]'],
}


def stage(report):
    chosen = []
    lines = ['gw-npcs 1', '# REVIEW ONLY: observed placements; quest gating and baseline eligibility remain unproven.']
    for map_id, names in SHORTLIST.items():
        for name in names:
            matches = [c for c in report['candidates'] if c['map']==map_id and c['name']==name
                       and c['status']=='stationary_review_candidate']
            if len(matches) != 1:
                raise ValueError(f'Candidate is missing or ambiguous: {map_id} {name}')
            candidate = matches[0]
            item = candidate['representative']
            npc, definition = item['first'], item['definition']['value']
            index = len(chosen)+1
            visual = sum(v << (8*i) for i,v in enumerate(definition['visual_adjustment_bytes']))
            fields = ['npc', map_id, 30000+index, 30000+index, definition['model_file'], definition['skin_file'],
                      visual, definition['appearance'], definition['flags'], definition['primary'], npc['level'],
                      npc['plane'], npc['x'], npc['y'], npc['facing'],
                      ''.join(f'{v:04x}' for v in npc['name_encoded_utf16']),
                      ','.join(map(str,definition.get('model_files', []))) or '-']
            lines.extend(['# '+name, ' '.join(map(str, fields))])
            chosen.append(dict(candidate, staging_agent=30000+index, staging_definition=30000+index))
    return lines, chosen


if __name__ == '__main__':
    report = json.loads((ROOT/'build/reports/npc-population/audit.json').read_text(encoding='utf8'))
    lines, chosen = stage(report)
    output = ROOT/'data/gw/npcs'
    output.mkdir(exist_ok=True)
    (output/'presearing-review-candidates.txt').write_text('\n'.join(lines)+'\n', encoding='utf8')
    (output/'presearing-review-evidence.json').write_text(json.dumps(dict(schema=1, policy=report['policy'],
        sources=report['sources'], candidates=chosen), indent=2, ensure_ascii=False)+'\n', encoding='utf8')
    print(f'Staged {len(chosen)} review candidates; runtime population unchanged.')
