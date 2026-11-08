import importlib.util
from pathlib import Path
import struct
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('captured_body_forms', ROOT/'tools/protocol/captured_body_forms.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class BodyFormTests(unittest.TestCase):
    def test_guardian_simple_body(self):
        self.assertEqual(module.decode_body(struct.pack('<4I', 159, 21, 25, 472)),
                         dict(form='generic21', actor=25, resource=472))

    def test_targeted_and_simple_twenty_remain_distinct(self):
        self.assertEqual(module.decode_body(struct.pack('<5I', 160, 20, 30, 25, 248))['cause'], 25)
        self.assertEqual(module.decode_body(struct.pack('<4I', 159, 20, 30, 248))['form'], 'generic20')

    def test_no_status_or_wrong_shape_matches(self):
        for words in [(159, 60, 25, 472), (159, 21, 25, 0, 472), (160, 21, 30, 25, 472)]:
            self.assertIsNone(module.decode_body(struct.pack('<'+'I'*len(words), *words)))

    def test_native_catalogue_roles_are_not_wire_forms(self):
        rows = module.catalog(ROOT/'src/runtime/gw_skill_catalog_generated.h')
        self.assertEqual(rows[258], dict(caster=2077, target=472))
        self.assertEqual(rows[137], dict(caster=2077, target=248))


if __name__ == '__main__':
    unittest.main()
