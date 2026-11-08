"""Explicit-boundary tests; fixtures contain no proposed retail durations."""
import sys
from pathlib import Path
import struct
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'/'ai'))
from audit_action_intervals import Ledger,decode_projectile,snapshot


def action(line,tick,source=10,token='1:10:1',target=20,target_token='1:20:1',kind='attack',skill=None):
    return dict(epoch=1,map=146,source=source,source_lifetime=token,target=target,
        target_lifetime=target_token,action=kind,skill=skill,packet_line=line,tick_ms=tick)


def status(line,tick,source=10,token='1:10:1',kind=1,value=0):
    return dict(epoch=1,map=146,source=source,source_lifetime=token,
        packet_line=line,tick_ms=tick,kind=kind,value=value)


class ExplicitIntervals(unittest.TestCase):
    def test_ranged_launch_closes_pending_attack_not_projectile_lifetime(self):
        ledger=Ledger();a=action(1,100);ledger.begin(a,(1,20,'1:20:1'))
        ledger.projectile(dict(status(2,200),is_attack=True,destination_xy=[300,0]))
        self.assertEqual(a['endpoint']['boundary'],'weapon_projectile_release')
        self.assertEqual(a['endpoint']['elapsed_ms'],100)
        self.assertFalse(ledger.pending)
        ledger.barrier(dict(epoch=1,packet_line=3,tick_ms=300),'source_lifetime_end',(1,10,'1:10:1'))
        self.assertEqual(a['projectile_releases'][0]['packet_line'],2)
        ledger.begin(action(4,1000),(1,20,'1:20:1'))
        self.assertEqual(a['endpoint_to_next_start_ms'],800)
        self.assertNotIn('recovery_end',a)

    def test_flag0_ordinary_launch_is_recorded_without_invented_completion(self):
        ledger=Ledger();a=action(1,100);ledger.begin(a,(1,20,'1:20:1'))
        ledger.projectile(dict(status(2,200),is_attack=False))
        self.assertNotIn('endpoint',a)
        self.assertEqual(a['projectile_releases'][0]['start_to_release_ms'],100)
        before=snapshot(ledger.pending,ledger.closed,(1,10,'1:10:1'))
        ledger.projectile(dict(status(3,250),is_attack=False))
        self.assertEqual(len(before['pending']['observed_projectile_releases']),1)
        ledger.begin(action(4,1000),(1,20,'1:20:1'))
        self.assertEqual(a['endpoint']['outcome'],'unresolved')

    def test_spell_finish_and_projectile_release_are_separate(self):
        ledger=Ledger();a=action(1,100,kind='skill',skill=186);ledger.begin(a,(1,20,'1:20:1'))
        ledger.end(status(2,200,kind=58),58,lambda agent:(1,agent,'1:20:1'))
        ledger.projectile(dict(status(3,200),is_attack=False))
        self.assertEqual(a['endpoint']['boundary'],'skill_finished')
        self.assertEqual(a['projectile_releases'][0]['packet_line'],3)
        self.assertNotIn('impact',a)

    def test_mismatched_end_and_reused_source_do_not_pair(self):
        ledger=Ledger();a=action(1,100,kind='skill',skill=186);ledger.begin(a,(1,20,'1:20:1'))
        ledger.end(status(2,200),1,lambda agent:(1,agent,'1:20:1'))
        self.assertEqual(a['endpoint']['boundary'],'nonmatching_end_kind')
        b=action(3,300);ledger.begin(b,(1,20,'1:20:1'))
        ledger.barrier(dict(epoch=1,tick_ms=350,packet_line=4),'source_lifetime_end',(1,10,'1:10:1'))
        ledger.end(status(5,360,token='1:10:2'),1,lambda agent:(1,agent,'1:20:1'))
        self.assertEqual(b['endpoint']['outcome'],'unresolved')

    def test_bane_snapshot_precedes_attack_stop_kd_and_damage(self):
        ledger=Ledger();target=(1,20,'1:20:1')
        ledger.begin(action(1,100,source=20,token=target[2],target=10,target_token='1:10:1'),(1,10,'1:10:1'))
        b=action(2,110,kind='skill',skill=296);ledger.begin(b,target)
        ledger.end(status(3,200,kind=58),58,lambda agent:target)
        ledger.end(status(4,200,source=20,token=target[2],kind=3),3,lambda agent:(1,10,'1:10:1'))
        ledger.knockdown(dict(epoch=1,tick_ms=200,packet_line=5,target=20,target_lifetime=target[2],value=2),target)
        ledger.modifier(dict(status(6,200),target=20,target_lifetime=target[2],value=-0.5),target)
        probe=ledger.bane[0]
        self.assertEqual(probe['target_before_caster_finish']['state'],'weapon_or_attack_skill_pending')
        self.assertEqual(probe['same_tick_cause_modifiers'][0]['target_state_before_modifier']['state'],'no_open_action_in_observed_ledger')
        self.assertEqual(probe['same_tick_target_knockdowns'][0]['packet_line'],5)

    def test_bane_after_melee_finish_preserves_unknown_recovery_boundary(self):
        ledger=Ledger();target=(1,20,'1:20:1')
        ledger.begin(action(1,100,source=20,token=target[2]),target)
        b=action(2,110,kind='skill',skill=296);ledger.begin(b,target)
        ledger.end(status(3,200,source=20,token=target[2]),1,lambda agent:target)
        ledger.end(status(4,250,kind=58),58,lambda agent:target)
        ledger.knockdown(dict(epoch=1,tick_ms=250,packet_line=5,target=20,target_lifetime=target[2],value=2),target)
        phase=ledger.bane[0]['target_before_caster_finish']
        self.assertEqual(phase['state'],'no_open_action_in_observed_ledger')
        self.assertEqual(phase['prior_endpoint']['endpoint']['boundary'],'melee_attack_finished')
        self.assertNotIn('recovery_end',phase)

    def test_projectile_form_and_flag_validation(self):
        raw=struct.pack('<IIff5I',164,10,100.,200.,0,123,143,1,1)
        result=decode_projectile(raw,164)
        self.assertTrue(result['is_attack']);self.assertEqual(result['raw_words_16_to_32'],[0,123,143,1,1])
        self.assertIsNone(decode_projectile(raw[:-1],164))
        self.assertIsNone(decode_projectile(struct.pack('<IIff5I',164,10,100.,200.,0,123,143,1,2),164))


if __name__=='__main__':unittest.main()
