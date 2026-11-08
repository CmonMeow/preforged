import importlib.util
from pathlib import Path

spec = importlib.util.spec_from_file_location('quest_audit',
    Path(__file__).resolve().parents[1] / 'tools/quests/audit_requests.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
context = dict(process_id=7, instance_epoch=1, map=55, local_actor=9)
request = dict(context, event='quest_info_requested', quest_id=101, tick_ms=100)
quest = dict(quest_id=101, log_state=1, description={'encoded_utf16': []},
             objectives={'encoded_utf16': [810, 101]})
snapshot = dict(context, event='quest_snapshot_changed', tick_ms=150, quests=[quest])
def observed(rows):
    return module.audit(list(enumerate(rows, 1)))['details_observed']
assert observed([request, snapshot]) == 1
assert observed([request, snapshot, snapshot]) == 1
for key in context:
    assert observed([request, dict(snapshot, **{key: 999})]) == 0
assert observed([dict(request, local_actor=None), snapshot]) == 0
assert observed([request, dict(snapshot, tick_ms=99)]) == 0
assert observed([snapshot, request]) == 0
for change in ({'log_state': 0}, {'description': {'encoded_utf16': None}},
               {'objectives': {'encoded_utf16': [0]}}, {'quest_id': 102}):
    assert observed([request, dict(snapshot, quests=[dict(quest, **change)])]) == 0
assert observed([request]) == 0
print('Quest request audit: identity, ordering, loaded flags and readable details verified.')
