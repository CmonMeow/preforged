"""Index NPC captures without merging hall upgrade/ownership populations."""
import collections,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
variants=json.loads((ROOT/'data/gw/npc-capture-variants.json').read_text(encoding='utf-8'))['contexts']
reports=[]
for path in sorted((ROOT/'data/captures').glob('*.jsonl')):
    decoded={};contexts={};definitions=0
    for line in path.open(encoding='utf-8'):
        row=json.loads(line);event=row.get('event')
        if event=='name_decoded':decoded[tuple(row.get('name_encoded_utf16',[]))]=row.get('name','')
        if event not in ('npc_definition_observed','npc_observations'):continue
        key=(row['map'],row['instance_epoch'])
        context=contexts.setdefault(key,{'map':key[0],'instance_epoch':key[1],
            'guild_hall_claim_and_upgrade_state':'not_recorded',
            'snapshots':0,'minimum_visible_npcs':None,'maximum_visible_npcs':0,
            'observed_positions_are_not_verified_spawns':True,'models':{},'definitions':{}})
        if event=='npc_definition_observed':
            context['definitions'][str(row['npc_table_id'])]=row['definition'];definitions+=1
            continue
        npcs=row.get('npcs',[]);count=len(npcs);context['snapshots']+=1
        context['minimum_visible_npcs']=count if context['minimum_visible_npcs'] is None else min(count,context['minimum_visible_npcs'])
        context['maximum_visible_npcs']=max(count,context['maximum_visible_npcs'])
        for npc in npcs:
            encoded=tuple(npc.get('name_encoded_utf16',[]))
            identity=str(npc['model'])+':'+','.join(map(str,encoded))
            model=context['models'].setdefault(identity,{'model':npc['model'],'name_encoded_utf16':list(encoded),
                'name':npc.get('name',''),'sample_position':{k:npc.get(k) for k in ('x','y','plane','facing')},
                'level':npc.get('level'),'allegiance':npc.get('allegiance')})
            if npc.get('name'):model['name']=npc['name']
    for context in contexts.values():
        for variant in variants:
            if variant['capture']==path.name and variant['map']==context['map'] and context['instance_epoch'] in variant['instance_epochs']:
                if variant.get('context_kind')=='town_population':
                    context['reviewed_population_variant']=variant['variant']
                else:context['guild_hall_claim_and_upgrade_state']=variant['variant']
                context['variant_evidence']=variant['evidence']
        for model in context['models'].values():
            if not model['name']:model['name']=decoded.get(tuple(model['name_encoded_utf16']),'')
        context['models']=list(context['models'].values())
        context['models_with_names']=sum(bool(model['name']) for model in context['models'])
    reports.append({'capture':path.name,'filename_is_not_population_authority':True,
        'definition_events':definitions,'decoded_names':len(decoded),'contexts':list(contexts.values())})
    print(path.name,'contexts',len(contexts),'definition events',definitions,'decoded names',len(decoded))
(ROOT/'data/gw/npc-capture-index.json').write_text(json.dumps({'schema':1,
    'population_policy':'Keep map/instance captures separate. Claimed and upgraded guild halls must be modelled as variants; filenames alone do not identify that state.',
    'captures':reports},indent=2,ensure_ascii=False)+'\n',encoding='utf-8')
