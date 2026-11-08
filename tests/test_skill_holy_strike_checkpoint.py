import importlib.util
from pathlib import Path
import struct
import unittest

spec=importlib.util.spec_from_file_location('holy_audit',Path(__file__).resolve().parents[1]/'tools/skills/audit_holy_strike_checkpoint.py')
audit=importlib.util.module_from_spec(spec);spec.loader.exec_module(audit)


class HolyCapture(unittest.TestCase):
    def rows(self):
        words=[(160,60,25,30,312),(229,25,312,0,8),(159,58,25,0),(160,20,30,25,556),(163,55,30,25,0xbcaaaaab)]
        return [(i+1,dict(event='packet',direction='StoC',complete=True,tick_ms=100,opcode=w[0],hex=struct.pack('<'+'I'*len(w),*w).hex())) for i,w in enumerate(words)]

    def start(self):
        return dict(actor=25,line=1,tick_ms=100,hex=self.rows()[0][1]['hex'])

    def test_exact(self):
        self.assertEqual(len(audit.sequence(self.rows(),self.start())),5)

    def test_incomplete(self):
        rows=self.rows();rows[3][1]['complete']=False
        with self.assertRaises(ValueError): audit.sequence(rows,self.start())

    def test_wrong_owner(self):
        start=self.start();start['actor']=26
        with self.assertRaises(ValueError): audit.sequence(self.rows(),start)

    def test_wrong_header(self):
        rows=self.rows();rows[3][1]['opcode']=999
        with self.assertRaises(ValueError): audit.sequence(rows,self.start())
