import importlib.util
import unittest
from pathlib import Path

path=Path(__file__).resolve().parents[1]/'tools/skills/audit_overcast_gate.py'
spec=importlib.util.spec_from_file_location('overcast_gate_audit',path)
module=importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class OvercastGateAuditTests(unittest.TestCase):
    def test_gate_not_unqualified_byte_or_capture_count(self):
        captured=[dict(id=i,name=str(i),local_finished=0 if i==2 else 3) for i in (1,2,3)]
        coverage=dict(skills=[dict(id=i,metadata_gate_blocks=i==2,
                                  native=dict(overcast=10 if i==2 else 99),
                                  dispatcher_binding=False,status='unsupported') for i in (1,2,3)])
        result=module.inventory(captured,coverage)
        self.assertEqual([r['id'] for r in result],[2])
        self.assertEqual(result[0]['local_finished'],0)
        self.assertEqual(result[0]['overcast'],10)
        self.assertFalse(result[0]['dispatcher_binding'])

    def test_unknown_native_row_is_not_silently_ignored(self):
        with self.assertRaises(KeyError):
            module.inventory([dict(id=999,name='unknown',local_finished=1)],dict(skills=[]))

    def test_conditional_hook_dependency_remains_in_inventory(self):
        row=dict(id=219,metadata_gate_blocks=False,requires_overcast_hook=True,
                 native=dict(overcast=5),dispatcher_binding=True,status='implemented_binding')
        result=module.inventory([dict(id=219,name='Obsidian Flame',local_finished=1)],dict(skills=[row]))
        self.assertEqual(len(result),1)
        self.assertFalse(result[0]['metadata_gate_blocks'])
        self.assertTrue(result[0]['dispatcher_binding'])


if __name__=='__main__':unittest.main()
