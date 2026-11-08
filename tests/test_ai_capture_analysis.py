"""Synthetic join/provenance failures, not tests of guessed retail AI constants."""
import json
from pathlib import Path
import struct
import sys
import tempfile
import unittest

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'/'ai'))
from audit_action_sequences import analyze, decode_action, epoch_at, sha256, target_change
from audit_movement_context import excursions, lifetime_at


class CaptureJoins(unittest.TestCase):
    def test_action_forms(self):
        a=decode_action(struct.pack('<4I',159,60,49,1376),159)
        self.assertEqual((a['source'],a['skill'],a['target']),(49,1376,None))
        a=decode_action(struct.pack('<5I',160,60,49,45,186),160)
        self.assertEqual((a['source'],a['target'],a['skill']),(49,45,186))
        self.assertIsNone(decode_action(b'bad',159))

    def test_epoch_gap(self):
        windows=[[100,200,1],[300,400,2]]
        self.assertEqual(epoch_at(200,windows,[100,300]),1)
        self.assertIsNone(epoch_at(250,windows,[100,300]))
        self.assertEqual(epoch_at(300,windows,[100,300]),2)

    def test_same_id_new_generation_is_changed_target(self):
        self.assertEqual(target_change(dict(target=2,target_lifetime='1:2:1'),
            dict(target=2,target_lifetime='1:2:2')),(True,'exact_lifetimes'))
        self.assertIsNone(target_change(dict(target=2,target_lifetime=None),
            dict(target=2,target_lifetime=None))[0])

    def fixture(self,directory):
        packet=directory/'packets-fixture.jsonl'; world=directory/'world-fixture.jsonl'
        data=[]; joins=[]
        def row(tick,op,words,join=False,complete=True):
            data.append(dict(direction='StoC',tick_ms=tick,opcode=op,complete=complete,
                hex=struct.pack('<'+'I'*(len(words)+1),op,*words).hex()))
            if join:
                joins.append(dict(line=len(data),source_lifetime='1:1:1',
                    source_npc=dict(name='Fire Imp',model=513,level=1,allegiance=4),
                    source_world_line=1,source_sample_delta_ms=0,map=146))
        row(100,32,[1]);row(101,32,[2])
        row(110,159,[60,1,1376],True);row(111,159,[58,1,0])
        row(120,160,[4,1,2,0],True);row(121,159,[1,1,0])
        row(122,33,[2]);row(123,32,[2])
        row(130,160,[4,1,2,0],True);row(131,159,[1,1,0])
        row(140,159,[60,1,194],True)
        row(141,159,[58,1,0],complete=False)
        row(142,159,[58,1,0])
        row(150,33,[1]);row(151,32,[1]);row(152,159,[58,1,0])
        row(160,160,[4,99,2,0])
        packet.write_text(''.join(json.dumps(r)+'\n' for r in data))
        world.write_text('{}\n')
        report=dict(sources={str(packet):sha256(packet),str(world):sha256(world)},
            epoch_windows=[[100,200,1]],clock_offsets={'1':{}},npc_events=joins)
        audit=directory/'audit.json';audit.write_text(json.dumps(report))
        return audit,packet

    def test_full_replay_lifetime_and_incomplete_matching(self):
        with tempfile.TemporaryDirectory() as temp:
            audit,_=self.fixture(Path(temp));r=analyze(audit)
            self.assertEqual(r['totals']['ordinary_actions'],4)
            self.assertEqual(r['totals']['ordinary_target_changes'],1)
            change=r['ordinary_target_changes'][0]
            self.assertEqual(change['before']['target_lifetime'],'1:2:1')
            self.assertEqual(change['after']['target_lifetime'],'1:2:2')
            self.assertTrue(change['previous_target_ended_by_new_start'])
            glyph=r['ordinary_actions'][0]
            self.assertEqual(glyph['skill'],1376)
            self.assertEqual(glyph['end']['outcome'],'finished')
            self.assertNotIn('end',r['ordinary_actions'][-1])
            self.assertEqual(r['ordinary_actions'][-1]['unpaired_reason'],'incomplete_action_packet')
            self.assertEqual(r['rejected']['action_unknown_source_lifetime'],1)

    def test_stale_audit_fails_closed(self):
        with tempfile.TemporaryDirectory() as temp:
            audit,packet=self.fixture(Path(temp));packet.write_text('{}\n')
            with self.assertRaisesRegex(ValueError,'SHA256'):analyze(audit)

    def test_same_tick_action_before_despawn_keeps_raw_generation(self):
        with tempfile.TemporaryDirectory() as temp:
            audit,packet=self.fixture(Path(temp));data=[json.loads(s) for s in packet.read_text().splitlines()]
            data[5]['tick_ms']=120;data[6]['tick_ms']=120
            packet.write_text(''.join(json.dumps(r)+'\n' for r in data))
            r=json.loads(audit.read_text());r['sources'][str(packet)]=sha256(packet)
            audit.write_text(json.dumps(r));result=analyze(audit)
            action=result['ordinary_actions'][1]
            self.assertEqual(action['target_lifetime'],'1:2:1')
            life=result['lifetimes'][action['target_lifetime']]
            self.assertEqual(action['tick_ms'],life['end_tick_ms'])
            self.assertLess(action['packet_line'],life['end_packet_line'])

    def test_nearest_identity_sample_after_despawn_is_excluded(self):
        with tempfile.TemporaryDirectory() as temp:
            audit,_=self.fixture(Path(temp));r=json.loads(audit.read_text())
            r['npc_events'][-1]['source_sample_delta_ms']=11
            audit.write_text(json.dumps(r));result=analyze(audit)
            self.assertEqual(result['totals']['ordinary_actions'],3)
            self.assertEqual(result['rejected']['identity_sample_after_source_lifetime'],1)

    def test_identity_join_lifetime_disagreement_fails_closed(self):
        with tempfile.TemporaryDirectory() as temp:
            audit,_=self.fixture(Path(temp));r=json.loads(audit.read_text())
            r['npc_events'][0]['source_lifetime']='1:1:2'
            audit.write_text(json.dumps(r))
            with self.assertRaisesRegex(ValueError,'lifetime disagreement'):analyze(audit)

    def test_snapshot_lifetime_halfopen(self):
        lives=[('1:2:1',dict(spawn_tick_ms=100,end_tick_ms=200)),
               ('1:2:2',dict(spawn_tick_ms=300,end_tick_ms=None))]
        self.assertEqual(lifetime_at(199,lives),'1:2:1')
        self.assertIsNone(lifetime_at(200,lives))
        self.assertIsNone(lifetime_at(250,lives))
        self.assertEqual(lifetime_at(300,lives),'1:2:2')

    def test_sampled_excursion_requires_same_endpoint_plane(self):
        def sample(tick,x,plane=0):return dict(tick_ms=tick,position=(x,0,plane),world_line=tick)
        self.assertEqual(excursions([sample(0,0),sample(6000,0)],100,5000),[])
        self.assertEqual(excursions([sample(0,0),sample(3000,200),sample(6000,0,1)],100,5000),[])
        r=excursions([sample(0,0),sample(3000,200),sample(6000,0),sample(9000,0)],100,5000)
        self.assertEqual(len(r),1);self.assertEqual(r[0]['sampled_excursion'],200)


if __name__=='__main__':unittest.main()
