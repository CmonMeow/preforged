import importlib.util
from pathlib import Path
import struct
import unittest

spec = importlib.util.spec_from_file_location('agony_audit', Path(__file__).resolve().parents[1]/'tools/skills/audit_signet_agony_checkpoint.py')
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)


class AgonyCapture(unittest.TestCase):
    def rows(self):
        words = [(159,60,25,145), (159,58,25,0), (159,21,25,259),
                 (229,25,145,0,8), (163,55,25,25,0xbdcccccd),
                 (66,25,478,0,111,0x41c80000), (160,20,29,25,260)]
        return [(i+1, dict(event='packet', direction='StoC', complete=True, tick_ms=100,
                          opcode=w[0], hex=struct.pack('<'+'I'*len(w),*w).hex())) for i,w in enumerate(words)]

    def test_exact_sequence(self):
        self.assertEqual(len(audit.sequence(self.rows(),dict(actor=25,tick_ms=100,line=1))),7)

    def test_missing_or_incomplete_effect(self):
        for rows in (self.rows()[:-1], self.rows()):
            rows[-1][1]['complete'] = False
            with self.assertRaises(ValueError): audit.sequence(rows,dict(actor=25,tick_ms=100,line=1))

    def test_wrong_owner_or_activation_line(self):
        for start in (dict(actor=26,tick_ms=100,line=1),dict(actor=25,tick_ms=100,line=2)):
            with self.assertRaises(ValueError): audit.sequence(self.rows(),start)

    def test_wrong_header(self):
        rows=self.rows(); rows[-1][1]['opcode']=999
        with self.assertRaises(ValueError): audit.sequence(rows,dict(actor=25,tick_ms=100,line=1))
