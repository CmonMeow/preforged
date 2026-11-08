"""Guard native packet shape and property/effect separation using pinned code."""
import ast
import unittest
from pathlib import Path
from skill_animation_2022 import Native
from signet_sorrow_2022 import capacity_case


class DispatchShape(unittest.TestCase):
    def test_wrong_shape_rejected_before_write(self):
        native = Native()
        before = bytes(native.uc.mem_read(native.packet, 24))
        for opcode, fields in ((158, [21, 42]), (159, [21, 42, 1619]),
                               (159, [21, 42, 43, 1619, 0]), (161, [34, 43])):
            with self.subTest(opcode=opcode, fields=fields):
                with self.assertRaises(ValueError):
                    native.dispatch(opcode, fields)
                self.assertEqual(bytes(native.uc.mem_read(native.packet, 24)), before)
                self.assertFalse(native.events or native.effects)

    def test_generic_effect_keeps_resource(self):
        native = Native()
        native.dispatch(158, [21, 42, 1619])
        self.assertEqual(native.effects[0]['payload'], [42, 1619])
        self.assertEqual(native.particle_consumer(native.effects[0])['encoded_file'], '9b0f04010000')

    def test_capacity_is_not_particle(self):
        for value in (100, 480, 1000):
            with self.subTest(value=value):
                result = capacity_case(value)
                self.assertEqual(result['native_actor_slot'], 1)
                self.assertEqual(result['event']['code'], 14)

    def test_targeted_packet_keeps_caster_and_resource(self):
        native = Native()
        native.dispatch(159, [20, 43, 42, 489])
        self.assertEqual(native.effects[0]['actor'], 43)
        self.assertEqual(native.particle_consumer(native.effects[0])['actor'], 43)

    def test_literal_dispatch_shapes(self):
        directory = Path(__file__).resolve().parents[1] / 'tools/protocol'
        for path in directory.glob('*.py'):
            tree = ast.parse(path.read_text(encoding='utf-8'))
            for node in ast.walk(tree):
                if not isinstance(node, ast.Call) or not isinstance(node.func, ast.Attribute) or node.func.attr != 'dispatch' or len(node.args) < 2:
                    continue
                opcode, fields = node.args[:2]
                if isinstance(opcode, ast.Constant) and opcode.value in (158, 159, 161) and isinstance(fields, (ast.List, ast.Tuple)):
                    with self.subTest(file=path.name, line=node.lineno):
                        self.assertEqual(len(fields.elts), {158: 3, 159: 4, 161: 3}[opcode.value])


if __name__ == '__main__':
    unittest.main()
