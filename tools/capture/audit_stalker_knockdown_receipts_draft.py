"""Offline exact-name identity + instance-epoch bounded KD receipt audit. Not gameplay proof."""
import argparse,collections,hashlib,json,struct
from pathlib import Path

def audit(archive):
    worlds=sorted(archive.glob('world*.jsonl'));packets=sorted(archive.glob('packets*.jsonl'))
    if len(worlds)!=1:raise ValueError('require exactly one explicitly selected world archive')
    ids=collections.defaultdict(set);bounds={};identity_receipts=[];sources=[]
    p=worlds[0]
    for line,text in enumerate(p.open(encoding='utf-8'),1):
        try:d=json.loads(text)
        except (ValueError,TypeError):continue
        epoch=d.get('instance_epoch');tick=d.get('tick_ms')
        if epoch is not None and tick is not None:
            bounds.setdefault(epoch,[tick,tick]);bounds[epoch][1]=max(bounds[epoch][1],tick)
        for actor in d.get('npcs',[]):
            if actor.get('name')=="Melandru's Stalker":
                if actor['agent'] not in ids[epoch]:identity_receipts.append(dict(file=str(p),line=line,epoch=epoch,actor=actor['agent'],model=actor.get('model')))
                ids[epoch].add(actor['agent'])
    total=0;matches=[]
    for p in packets:
        for line,text in enumerate(p.open(encoding='utf-8'),1):
            try:d=json.loads(text)
            except (ValueError,TypeError):continue
            if d.get('opcode')!=162 or not d.get('complete') or d.get('direction')!='StoC':continue
            raw=bytes.fromhex(d.get('hex',''))
            if len(raw)!=16:continue
            _,kind,actor=struct.unpack_from('<III',raw)
            if kind!=63:continue
            total+=1
            for epoch,agents in ids.items():
                if actor in agents and epoch in bounds and bounds[epoch][0]<=d.get('tick_ms',0)<=bounds[epoch][1]:
                    matches.append(dict(file=str(p),line=line,epoch=epoch,actor=actor,tick=d['tick_ms'],seconds=struct.unpack_from('<f',raw,12)[0]))
    for p in worlds+packets:sources.append(dict(path=str(p),sha256=hashlib.sha256(p.read_bytes()).hexdigest()))
    return dict(sources=sources,identity_receipts=identity_receipts,epoch_bounds=bounds,total_complete_kd_records=total,verified_stalker_kd_receipts=matches,limitations=['No matching receipt is not immunity evidence.','Only this selected archive scanned; no complete corpus claim.','Wild actor identity uses decoded exact name plus epoch and model; this is not a durable owned-species provider.','Bounds use observed epoch tick extrema, not proof of full actor lifetime.','No rendered state corroboration is inferred from duration packet alone.'])
if __name__=='__main__':
    ap=argparse.ArgumentParser();ap.add_argument('archive',type=Path);ap.add_argument('--out',required=True,type=Path);a=ap.parse_args();a.out.write_text(json.dumps(audit(a.archive),indent=2)+'\n')
