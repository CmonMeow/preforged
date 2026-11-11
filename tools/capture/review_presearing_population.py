"""Pin NPC wiki location review and captured trainer/guide walking overlap."""
import gzip
import hashlib
import json
from pathlib import Path
import re
import sys

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/quests'))
from extract_wiki import text
WIKI=Path('C:/Users/CmonMeow/Desktop/gw/Wiki/wiki.guildwars.com/wiki')
TRAINERS=['Van the Warrior','Artemis the Ranger','Ciglo the Monk','Verata the Necromancer','Sebedoh the Mesmer','Howland the Elementalist']

def wiki(name):
    path=WIKI/(name.replace(' ','_')+'.html')
    raw=path.read_bytes()
    html=(gzip.decompress(raw) if raw[:2]==bytes([31,139]) else raw).decode('utf8')
    sections={}
    for heading in re.finditer(r'<h([23])>\s*<span class="mw-headline" id="([^"]+)"',html):
        if heading[2] not in ('Location','Locations','Notes'):continue
        end=re.search(r'<h[23]>',html[heading.end():])
        fragment=html[heading.start():heading.end()+end.start() if end else len(html)]
        sections[heading[2]]=dict(line=html.count('\n',0,heading.start())+1,text=text(fragment).split('Retrieved from')[0][:3000])
    return dict(path=str(path),sha256=hashlib.sha256(raw).hexdigest(),url='https://wiki.guildwars.com/wiki/'+name.replace(' ','_'),sections=sections)

def main():
    staged=json.loads((ROOT/'data/gw/npcs/presearing-review-evidence.json').read_text(encoding='utf8'))
    review=[]
    for item in staged['candidates']:
        name=item['name'].split(' [')[0].strip()
        review.append(dict(map=item['map'],name=item['name'],wiki=wiki(name),
            decision='hold_map_location_conflict' if name=='Brother Mhenlo' else 'baseline_location_supported',
            rationale='Captured stationary friendly placement and published ordinary location; quest-giver eligibility is separate from actor presence.',
            caveat=('Captured map146 placement conflicts with published Ashford Abbey location and A New Monk Trainer walkthrough; investigate before baseline installation.' if name=='Brother Mhenlo' else
                    'Farrah in Lakeside County is Protection Prayers-conditioned; this candidate is Green Hills County.' if name=='Farrah Cappo' else
                    'Service implementation and dialogue/quest eligibility are not enabled by this review.'),
            capture_reference='presearing-review-evidence.json',staging_agent=item['staging_agent']))
    transitions=[]
    for path in sorted((ROOT/'data/captures').rglob('world-*.jsonl')):
        by_epoch={}
        with path.open(encoding='utf8') as stream:
            for line,raw in enumerate(stream,1):
                row=json.loads(raw)
                if row.get('event')!='npc_observations' or row.get('map')!=146:continue
                actors=[n for n in row['npcs'] if n.get('name') in TRAINERS+['Haversdan']]
                if not actors:continue
                epoch=by_epoch.setdefault(row['instance_epoch'],[])
                epoch.append(dict(line=line,instance_ms=row['instance_ms'],actors=actors))
        for epoch,samples in by_epoch.items():
            overlap=[s for s in samples if any(n['name']=='Haversdan' for n in s['actors']) and any(n['name'] in TRAINERS for n in s['actors'])]
            if not overlap:continue
            transitions.append(dict(source=path.relative_to(ROOT).as_posix(),sha256=hashlib.sha256(path.read_bytes()).hexdigest(),
                map=146,epoch=epoch,first_overlap=overlap[0],last_overlap=overlap[-1],overlap_observations=len(overlap),
                samples=[s for s in samples if overlap[0]['instance_ms']-3000<=s['instance_ms']<=overlap[-1]['instance_ms']+15000]))
    report=dict(schema=1,candidate_review=review,starter_references=[wiki(n) for n in TRAINERS+['Haversdan']],
        captured_transitions=transitions,
        policy=['22 shortlisted map placements have published ordinary-location support. Hold Brother Mhenlo: captured map146 conflicts with published Ashford Abbey location.',
                'Six initial profession trainers are separate conditional Lakeside actors and are excluded from this shortlist.',
                'Objective completion must preserve the trainer for turn-in. Only committed reward starts the handoff.',
                'Captures establish simultaneous departing trainer and arriving Haversdan; immediate replacement would contradict observations.',
                'Quest-log disappearance alone does not prove reward, abandonment or a trigger.',
                'Transition movement completion is caller-owned; no timer or walk path inferred from visibility loss.',
                'Solo recordings do not establish party leader precedence. Native resolver requires an explicit multiplayer policy.'])
    output=ROOT/'data/gw/npcs/presearing-eligibility-evidence.json'
    output.write_text(json.dumps(report,indent=2,ensure_ascii=False)+'\n',encoding='utf8')
    print('Reviewed',len(review),'locations;',len(transitions),'captured walking overlaps')

if __name__=='__main__':main()
