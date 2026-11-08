import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools/protocol'))
from skill_variant_audit import audit

def row(identity, counterpart, flag=0, binding=False):
    fields = dict.fromkeys(('encoded_energy', 'health_sacrifice', 'overcast', 'activation', 'aftercast', 'recharge', 'scale0', 'scale15', 'bonus0', 'bonus15'), 0)
    return {'id': identity, 'gwca_name': 'Fixture', 'dispatcher_binding': binding,
            'native': {**fields, 'pvp_skill': counterpart, 'special': flag}}

base, variant = row(865, 3396, binding=True), row(3396, 865, 0x400000)
result = audit([base, variant])
assert len(result['pairs']) == 1 and len(result['implemented_base_missing_variant']) == 1
assert result['pairs'][0]['metadata_differences'] == {}  # Semantic split can have identical numeric metadata.
variant['dispatcher_binding'] = True
assert audit([base, variant])['implemented_base_missing_variant'] == []
variant['native']['pvp_skill'] = 864
assert audit([base, variant])['pairs'] == []
assert len(audit([base, variant])['nonreciprocal_metadata_links']) == 1
assert len(audit([base])['nonreciprocal_metadata_links']) == 1
assert audit([row(281, 3433, binding=True)])['pairs'] == []
try:
    audit([base, base])
except ValueError:
    pass
else:
    raise AssertionError('duplicate identity accepted')
print('PASS reciprocal pairs, semantic split, unsupported variant, missing links and duplicate identities')
