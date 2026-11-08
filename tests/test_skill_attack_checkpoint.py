import importlib.util
from pathlib import Path
import struct
import unittest
spec=importlib.util.spec_from_file_location('attack_audit',Path(__file__).resolve().parents[1]/'tools/skills/audit_attack_checkpoint.py')
audit=importlib.util.module_from_spec(spec);spec.loader.exec_module(audit)


class AttackCapture(unittest.TestCase):
    def rows(self):
        words=[(160,50,25,30,332),(229,25,332,0,10),(159,46,25,0),(163,16,30,25,0xbb088889),(227,25,332,0)]
        return [(i+1,dict(event='packet',direction='StoC',complete=True,tick_ms=100,opcode=w[0],hex=struct.pack('<'+'I'*len(w),*w).hex())) for i,w in enumerate(words)]
    def run_sequence(self,rows):
        return audit.sequence(rows,dict(actor=25,tick_ms=100,line=1,hex=self.rows()[0][1]['hex']),332,10)
    def test_exact(self):
        self.assertEqual(len(self.run_sequence(self.rows())),5)
    def test_no_recharge_requires_skill_release_frame(self):
        rows=[(line,row) for line,row in self.rows() if row['opcode']!=229]
        start=dict(actor=25,tick_ms=100,line=1,hex=rows[0][1]['hex'])
        self.assertEqual(len(audit.sequence(rows,start,332,None)),4)
        rows[-1][1]['tick_ms']=101
        with self.assertRaises(ValueError):audit.sequence(rows,start,332,None)
    def test_thrill_recharge_is_skill_specific(self):
        rows=self.rows()
        for _,row in rows:
            raw=bytes.fromhex(row['hex']);words=list(struct.unpack('<'+'I'*(len(raw)//4),raw))
            words=[324 if word==332 else word for word in words]
            if words[0]==229:words[-1]=8
            row['hex']=struct.pack('<'+'I'*len(words),*words).hex()
        start=dict(actor=25,tick_ms=100,line=1,hex=rows[0][1]['hex'])
        self.assertEqual(len(audit.sequence(rows,start,324,8)),5)
        with self.assertRaises(ValueError):audit.sequence(rows,start,324,10)
    def test_wrong_target(self):
        rows=self.rows();rows[3][1]['hex']=struct.pack('<5I',163,16,31,25,0xbb088889).hex()
        with self.assertRaises(ValueError): self.run_sequence(rows)
    def test_wrong_frame_or_incomplete(self):
        for incomplete in (False,True):
            rows=self.rows();rows[3][1]['tick_ms']=101 if not incomplete else 100;rows[3][1]['complete']=not incomplete
            with self.assertRaises(ValueError): self.run_sequence(rows)
    def test_wrong_header(self):
        rows=self.rows();rows[0][1]['opcode']=999
        with self.assertRaises(ValueError): self.run_sequence(rows)
    def test_previous_attack_completion_does_not_capture_the_skill(self):
        rows=self.rows();tail=[]
        for line,row in rows[1:]: row['tick_ms']=200;tail.append((line+1,row))
        old=dict(rows[2][1]);old['tick_ms']=150
        evidence=self.run_sequence([rows[0],(2,old)]+tail)
        self.assertEqual(evidence['completion']['tick_ms'],200)
    def test_activation_tick_mismatch(self):
        rows=self.rows();rows[0][1]['tick_ms']=101
        with self.assertRaises(ValueError): self.run_sequence(rows)
