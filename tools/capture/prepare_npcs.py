"""Prepare observed NPC populations, with explicitly reviewed hall selections."""
import argparse,hashlib,json,re,struct
from pathlib import Path
from reviewed_npc_selection import supplements_at,append_supplements
ROOT=Path(__file__).resolve().parents[2]
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('maps',type=int,nargs='+')
parser.add_argument('--output-directory',type=Path,default=ROOT/'data/gw')
args=parser.parse_args();requested=set(args.maps)
output=args.output_directory;output.mkdir(parents=True,exist_ok=True)
catalog={r['id']:r for r in json.loads((ROOT/'data/gw/map-catalog.json').read_text())}
support={r['map']:r for r in json.loads((ROOT/'data/gw/map-support.json').read_text())['maps'] if r['status'] in ('prepared','prepared_development_shared_asset')}
variants=json.loads((ROOT/'data/gw/npc-capture-variants.json').read_text(encoding='utf-8'))['contexts']
# The collector's 0x2000 living-agent class includes player miniatures. Model
# identities in GWCA's Minipet namespace establish their class; matching the
# observed 0x18200/0x18204 definition flags corroborates that classification.
# The mask alone is not asserted to mean "miniature" and names are not filters.
minipet_source=ROOT/'third_party/gwca-current/include/GWCA/Constants/AgentIDs.h'
minipet_header=minipet_source.read_text(encoding='utf-8')
minipet_block=minipet_header.split('namespace Minipet {',1)[1].split('} // namespace Minipet',1)[0]
minipets={int(value):name for name,value in re.findall(r'^\s*constexpr int\s+(\w+)\s*=\s*(\d+)',minipet_block,re.MULTILINE)}
if not minipets:raise RuntimeError('GWCA miniature model identities are missing')
transient_mask=0x18000
def miniature_class(model,definition):
    if model in minipets and definition['flags']&transient_mask==transient_mask:
        return 'GW::Constants::ModelID::Minipet::'+minipets[model]
    return None
selected={}
for variant in variants:
    selection=variant.get('population_selection')
    if not selection:continue
    map_id=variant['map']
    if map_id in selected:raise RuntimeError('Multiple reviewed populations for map: '+str(map_id))
    if selection['instance_epoch'] not in variant['instance_epochs']:
        raise RuntimeError('Population epoch is outside reviewed variant: '+str(map_id))
    selected[map_id]=variant
contexts={};decoded={};miniature_observations={}
for path in sorted((ROOT/'data/captures').rglob('*.jsonl')):
    if 'interactions' in path.relative_to(ROOT/'data/captures').parts:continue
    capture_name=path.relative_to(ROOT/'data/captures').as_posix()
    for line_number,line in enumerate(path.open(encoding='utf-8'),1):
        r=json.loads(line);event=r.get('event')
        if event=='name_decoded':decoded[tuple(r['name_encoded_utf16'])]=r.get('name','')
        if event=='npc_definition_observed':
            model=r['npc_table_id'];definition=r['definition'];symbol=miniature_class(model,definition)
            if symbol:
                observed=miniature_observations.setdefault((capture_name,r['map'],model),
                    {'capture':capture_name,'map':r['map'],'model':model,'gwca_class':symbol,
                     'flags':set(),'instance_epochs':set(),'name_encoded_utf16':definition.get('name_encoded_utf16',[])})
                observed['flags'].add(definition['flags']);observed['instance_epochs'].add(r['instance_epoch'])
        if r.get('map') not in requested or event not in ('npc_observations','npc_definition_observed'):continue
        key=(capture_name,r['map'],r['instance_epoch'])
        c=contexts.setdefault(key,{'definitions':{},'snapshot':[],'snapshot_line':None,'snapshot_instance_ms':None,'supplements':[]})
        if event=='npc_definition_observed':c['definitions'][r['npc_table_id']]=r['definition']
        else:
            variant=selected.get(r['map']);selection=variant.get('population_selection') if variant else None
            if selection:
                if capture_name==variant['capture'] and r['instance_epoch']==selection['instance_epoch']:
                    pinned=dict(selection,capture=variant['capture'],map=r['map'])
                    c['supplements'].extend(supplements_at(pinned,capture_name,r['map'],r['instance_epoch'],r,line_number,line))
                # A reviewed hall uses this exact snapshot. Never fall back to
                # a larger snapshot/instance that might contain a player's pet.
                if capture_name!=variant['capture'] or r['instance_epoch']!=selection['instance_epoch'] or \
                   r.get('instance_ms')!=selection['snapshot_instance_ms']:continue
                if c['snapshot_line'] is not None:raise RuntimeError('Ambiguous reviewed NPC snapshot: '+str(r['map']))
                if len(r.get('npcs',[]))!=selection['expected_npc_count']:
                    raise RuntimeError('Reviewed NPC snapshot count changed: '+str(r['map']))
                if selection.get('snapshot_line',line_number)!=line_number or \
                   selection.get('snapshot_sha256',hashlib.sha256(line.rstrip('\r\n').encode('utf-8')).hexdigest())!=hashlib.sha256(line.rstrip('\r\n').encode('utf-8')).hexdigest():
                    raise RuntimeError('Reviewed NPC base snapshot changed: '+str(r['map']))
            elif len(r.get('npcs',[]))<=len(c['snapshot']):continue
            c['snapshot']=r['npcs'];c['snapshot_line']=line_number;c['snapshot_instance_ms']=r.get('instance_ms')
def cells(asset):
    raw=(ROOT/f'data/gw/navigation/{asset}.fwnav').read_bytes()
    count=struct.unpack_from('<I',raw,48)[0];at=52;planes=[]
    for plane in range(count):
        count=struct.unpack_from('<I',raw,at)[0];at+=4
        planes.append([struct.unpack_from('<5i6f',raw,at+44*i) for i in range(count)]);at+=44*count
    return planes
def walkable(cell,x,y):
    yt,yb,xtl,xtr,xbl,xbr=cell[5:]
    if not min(yt,yb)-.01<=y<=max(yt,yb)+.01:return False
    t=max(0,min(1,(y-yt)/(yb-yt))) if abs(yb-yt)>1e-6 else 0
    left=xtl+(xbl-xtl)*t;right=xtr+(xbr-xtr)*t
    return min(left,right)-.01<=x<=max(left,right)+.01
lines=['gw-npcs 1','# Observed static placements, not verified official spawn records.']
provenance=[]
for map_id in sorted(requested):
    if map_id not in support:raise RuntimeError('Map has no prepared geometry: '+str(map_id))
    variant=selected.get(map_id)
    if catalog[map_id]['region_type'] in (3,4) and not variant:
        raise RuntimeError('Guild halls require an explicitly selected ownership/upgrade variant')
    if variant:
        selection=variant['population_selection'];key=(variant['capture'],map_id,selection['instance_epoch'])
        context=contexts.get(key)
        if not context or not context['definitions'] or context['snapshot_line'] is None:
            raise RuntimeError('Reviewed NPC snapshot or definitions are missing: '+str(map_id))
    else:
        choices=[(key,c) for key,c in contexts.items() if key[1]==map_id and c['definitions']]
        if not choices:continue
        key,context=max(choices,key=lambda item:len(item[1]['snapshot']))
    planes=cells(support[map_id]['asset']);population=[];skipped=[]
    snapshot=context['snapshot']
    if variant:
        snapshot=append_supplements(snapshot,context['supplements'],variant['population_selection'].get('supplemental_observations',[]))
    for n in snapshot:
        definition=context['definitions'].get(n['model']);encoded=n.get('name_encoded_utf16',[])
        name=n.get('name') or decoded.get(tuple(encoded),'')
        symbol=miniature_class(n['model'],definition) if definition else None
        if symbol:
            skipped.append({'model':n['model'],'captured_agent':n['agent'],'name':name,
                'reason':'transient_minipet_model_class','gwca_class':symbol,
                'captured_definition_flags':definition['flags'],'flags_mask':transient_mask,
                'captured_level':n['level'],'captured_allegiance':n['allegiance']});continue
        if not definition or not name or not 0<len(encoded)<=7 or any(not 0<unit<=65535 for unit in encoded):
            skipped.append({'model':n['model'],'reason':'missing_definition_or_name'});continue
        if not 0<=n['plane']<len(planes) or not any(walkable(cell,n['x'],n['y']) for cell in planes[n['plane']]):
            skipped.append({'model':n['model'],'reason':'outside_2022_navigation'});continue
        if len(definition.get('model_files',[]))>8:continue
        index=len(population)+1;visual=sum(v<<(8*i) for i,v in enumerate(definition['visual_adjustment_bytes']))
        # Remap definitions per agent so instance-specific name overrides do
        # not collide when two actors share the same captured body model.
        lines.append('# '+name.replace('\n',' '))
        fields=['npc',map_id,10000+index,index,definition['model_file'],definition['skin_file'],visual,
            definition['appearance'],definition['flags'],definition['primary'],n['level'],n['plane'],
            n['x'],n['y'],n['facing'],''.join(f'{unit:04x}' for unit in encoded),
            ','.join(map(str,definition.get('model_files',[]))) or '-']
        lines.append(' '.join(map(str,fields)))
        population.append({'name':name,'captured_agent':n['agent'],'captured_model':n['model'],
            'server_agent':10000+index,'server_definition':index})
    report={'map':map_id,'capture':key[0],'epoch':key[2],'population':population,'skipped':skipped,
        'snapshot_line':context['snapshot_line'],'snapshot_instance_ms':context['snapshot_instance_ms'],
        'selection_policy':('explicit_reviewed_snapshot_with_pinned_same_instance_supplements' if context['supplements'] else 'explicit_reviewed_snapshot') if variant else 'largest_single_context_snapshot',
        'placement':('reviewed base snapshot plus exact same-instance visibility supplements; no cross-instance population union' if context['supplements'] else 'one observed snapshot; no cross-instance population union'),'live_validation':'pending'}
    if variant:
        report.update({'variant':variant['variant'],'variant_evidence':variant['evidence'],
            'selection_evidence':variant['population_selection']['evidence'],
            'excluded_context':variant['population_selection'].get('excluded_context')})
        if context['supplements']:report['supplemental_observations']=variant['population_selection']['supplemental_observations']
    provenance.append(report)
    print('Map',map_id,'prepared NPCs',len(population),'skipped',len(skipped))
(output/'npcs-candidates.txt').write_text('\n'.join(lines)+'\n',encoding='utf-8')
observations=[]
for key,observed in sorted(miniature_observations.items()):
    observed['flags']=sorted(observed['flags']);observed['instance_epochs']=sorted(observed['instance_epochs'])
    observed['name']=decoded.get(tuple(observed['name_encoded_utf16']),'')
    observations.append(observed)
policy={'classification':'Known GWCA Minipet model identity AND captured NPC definition flags matching mask; never display-name filtering.',
    'source':minipet_source.relative_to(ROOT).as_posix(),'source_sha256':hashlib.sha256(minipet_source.read_bytes()).hexdigest(),
    'flags_mask':transient_mask,'flags_expected':transient_mask,
    'flags_scope':'0x18200/0x18204 observed on these known miniature classes; the mask alone is not a universal miniature type declaration.',
    'rationale':'Player-summoned companions are temporary player content, not a permanent town or guild-hall population.',
    'wiki_evidence':'https://wiki.guildwars.com/wiki/Miniature',
    'captured_class_observations':observations}
(output/'npc-population-provenance.json').write_text(json.dumps({'schema':1,'transient_exclusion_policy':policy,'maps':provenance},indent=2,ensure_ascii=False)+'\n',encoding='utf-8')
