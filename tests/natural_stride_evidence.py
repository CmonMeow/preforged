"""Offline fixed-capture/original2022 first consumers; never renders or attaches."""
import hashlib
import json
from pathlib import Path
import sys
ROOT=Path(__file__).resolve().parents[1]
sys.path[:0]=[str(ROOT/'tools/protocol')]
from captured_animation_audit_2022 import capture_start, body
from image_2022 import EXPECTED

def main():
    checkpoint=ROOT/'data/captures/skill-checkpoints/20261024-all-completed'
    inventory=json.loads((checkpoint/'skill-audit/captured-skills.json').read_text())
    skill=next(row for row in inventory['skills'] if row['id']==1727)
    assert skill['local_starts']==1 and skill['local_instant']==1
    event=skill['activation_evidence'][0]
    assert (event['file'],event['line'],event['tick_ms'],event['actor'],event['kind'])==('packets-1360-15821281.jsonl',539798,27588484,25,48)
    path=checkpoint/event['file']
    with path.open('rb') as source:
        assert hashlib.file_digest(source,'sha256').hexdigest()=='59e2ae82e4c8187a88b5f3f8cac919a085a14aca314834ada70117039f3287d6'
    with path.open() as source:
        row=next(json.loads(raw) for number,raw in enumerate(source,1) if number==event['line'])
    assert row['hex']==event['hex']=='9f0000003000000019000000bf060000'
    start=capture_start(1727,[event]);caster=body(1727,0x78,True);target=body(1727,0x7c,False)
    assert start['path']=='instant48' and caster['resource']==724 and target['resource']==2077
    result=dict(skill=1727,client_sha256=EXPECTED,activation=start,caster_body=caster,target_body=target,
        limitations=['Local instant receipt only; no captured movement/block/outcome contrast.',
                    'Offline2022 asset services stubbed; no rendered client acceptance.',
                    'Detached policy only; production binding and complete shared consumers pending.'])
    out=ROOT/'build/reports/natural-stride-detached-20261026.json';out.parent.mkdir(parents=True,exist_ok=True)
    out.write_text(json.dumps(result,indent=2)+'\n')
    print('PASS Natural Stride fixed local instant48 and original2022 caster724/target2077 consumers')

if __name__=='__main__':main()
