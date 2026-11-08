"""Identity boundaries and packet-evidence regressions for the offline audit."""
import importlib.util
import json
import struct
import tempfile
import unittest
from pathlib import Path

SPEC=importlib.util.spec_from_file_location('checkpoint',Path(__file__).resolve().parents[1]/'tools/skills/audit_packet_checkpoint.py')
MODULE=importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)

def write_coverage(root,skills):
    import hashlib
    source=root/'runtime.cpp';source.write_text('reviewed source\n')
    report=dict(skills=skills,source_sha256={'runtime.cpp':hashlib.sha256(source.read_bytes()).hexdigest()})
    (root/'build/reports/skill-runtime-coverage.json').write_text(json.dumps(report))
    return report


class CheckpointTests(unittest.TestCase):
    def test_upkeep_and_effect_skill_ids_are_not_casts(self):
        raw=bytes.fromhex('410000002000000000000000c80900000000000096000000')
        row=MODULE.effect_skill_record(65,raw)
        self.assertEqual(row['id'],2504)
        self.assertEqual(row['kind'],'upkeep_applied')
        self.assertEqual(row['caster_actor'],32)
        self.assertEqual(row['receiver_actor'],0)
        effect=struct.pack('<6I',66,32,2504,0,123,0x41200000)
        self.assertEqual(MODULE.effect_skill_record(66,effect)['affected_actor'],32)
        self.assertIsNone(MODULE.effect_skill_record(66,raw))
        self.assertIsNone(MODULE.effect_skill_record(65,raw[:-1]))
        self.assertIsNone(MODULE.effect_skill_record(68,effect))
    def test_inline_runtime_omission_and_edits_reject_stale_coverage(self):
        import hashlib
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);(root/'build/reports').mkdir(parents=True)
            report=write_coverage(root,[])
            source=root/'src/runtime/gw_world_spell.inl'
            source.parent.mkdir(parents=True);source.write_text('original spell')
            with self.assertRaisesRegex(ValueError,'omits source'):
                MODULE.verify_coverage_sources(report,root)
            report['source_sha256'][source.relative_to(root).as_posix()]=hashlib.sha256(source.read_bytes()).hexdigest()
            MODULE.verify_coverage_sources(report,root)
            source.write_text('changed spell')
            with self.assertRaisesRegex(ValueError,'source changed'):
                MODULE.verify_coverage_sources(report,root)
    def test_packet_only_has_no_local_identity_and_ambiguous_pair_rejects(self):
        self.assertEqual(MODULE.capture_windows({'packets-1-1.jsonl':Path('packet')},'1'),([],[]))
        with self.assertRaisesRegex(ValueError,'Ambiguous'):
            MODULE.capture_windows({'world-1-1.jsonl':Path('one'),'world-1-2.jsonl':Path('two')},'1')
    def test_changed_source_rejects_stale_coverage(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);(root/'build/reports').mkdir(parents=True)
            report=write_coverage(root,[]);MODULE.verify_coverage_sources(report,root)
            (root/'runtime.cpp').write_text('changed source\n')
            with self.assertRaisesRegex(ValueError,'Runtime coverage source changed'):
                MODULE.verify_coverage_sources(report,root)
            with self.assertRaisesRegex(ValueError,'lacks source hashes'):
                MODULE.verify_coverage_sources(dict(skills=[]),root)
    def test_freeze_uses_complete_lines_and_leaves_live_source_untouched(self):
        freeze_spec=importlib.util.spec_from_file_location('freeze',Path(__file__).resolve().parents[1]/'tools/skills/freeze_packet_checkpoint.py')
        freeze_module=importlib.util.module_from_spec(freeze_spec);freeze_spec.loader.exec_module(freeze_module)
        with tempfile.TemporaryDirectory() as directory:
            source=Path(directory)/'source';target=Path(directory)/'snapshot'
            original=b'{"event":"complete"}\n{"event":"unfinished'
            source.write_bytes(original)
            receipt=freeze_module.freeze(source,target)
            self.assertEqual(source.read_bytes(),original)
            self.assertEqual(target.read_bytes(),b'{"event":"complete"}\n')
            self.assertEqual(receipt['source_read_limit'],len(original))
            self.assertEqual(receipt['complete_lines'],1)
            with source.open('ab') as stream:stream.write(b'"}\n')
            self.assertEqual(target.read_bytes(),b'{"event":"complete"}\n')

    def test_zoning_invalidates_identity_until_new_snapshot(self):
        with tempfile.TemporaryDirectory() as directory:
            p=Path(directory)/'world.jsonl'
            p.write_text('\n'.join(json.dumps(r) for r in [
                dict(event='quest_snapshot_changed',tick_ms=100,instance_epoch=1,local_actor=25,map=248),
                dict(event='local_map_transition_observed',**{'from':dict(observed_ms=200)}),
                dict(event='quest_snapshot_changed',tick_ms=300,instance_epoch=3,local_actor=26,map=280)]))
            ticks,identities=MODULE.local_windows(p)
            import bisect
            def at(t):
                i=bisect.bisect_right(ticks,t)-1
                return identities[i] if i>=0 else None
            self.assertIsNone(at(99))
            self.assertEqual(at(199),(1,25,248))
            self.assertIsNone(at(200))
            self.assertIsNone(at(299))
            self.assertEqual(at(300),(3,26,280))

    def test_manifest_tamper_rejected_before_packet_admission(self):
        # Audit must never reinterpret changed checkpoint data as the original
        # capture. Patch only metadata roots, leaving the actual hash guard live.
        import hashlib
        from unittest.mock import patch
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);checkpoint=root/'checkpoint';checkpoint.mkdir()
            (root/'data/reference/buildwars').mkdir(parents=True)
            (root/'build/reports').mkdir(parents=True)
            (root/'data/reference/buildwars/skilldesc-en.json').write_text('{"skilldesc":{}}')
            write_coverage(root,[])
            (root/'build/reports/skill-coverage.json').write_text('{"classification":{"obtainable_base_skill_ids":[]}}')
            (checkpoint/'packets-1-1.jsonl').write_text('changed\n')
            (checkpoint/'manifest.json').write_text(json.dumps(dict(files=[dict(snapshot='packets-1-1.jsonl',sha256=hashlib.sha256(b'original\n').hexdigest())])))
            with patch.object(MODULE,'ROOT',root):
                with self.assertRaisesRegex(ValueError,'Checkpoint changed'):
                    MODULE.analyze(checkpoint)

    def test_local_and_other_actors_and_instant_completion(self):
        import hashlib
        from unittest.mock import patch
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);checkpoint=root/'checkpoint';checkpoint.mkdir()
            (root/'data/reference/buildwars').mkdir(parents=True);(root/'build/reports').mkdir(parents=True)
            (root/'data/reference/buildwars/skilldesc-en.json').write_text('{"skilldesc":{"174":{"name":"Aftershock"},"6":{"name":"Mantra of Earth"}}}')
            write_coverage(root,[dict(id=174,status='implemented_binding')])
            (root/'build/reports/skill-coverage.json').write_text('{"classification":{"obtainable_base_skill_ids":[6,174,999]}}')
            world=checkpoint/'world-1-1.jsonl';world.write_text(json.dumps(dict(event='quest_snapshot_changed',tick_ms=100,instance_epoch=1,local_actor=25,map=280))+'\n')
            packet=checkpoint/'packets-1-1.jsonl'
            def row(t,kind,actor,value):
                return json.dumps(dict(event='packet',complete=True,direction='StoC',opcode=159,tick_ms=t,hex=struct.pack('<4I',159,kind,actor,value).hex()))
            packet.write_text('\n'.join([row(99,60,25,999),row(101,60,26,999),row(102,60,25,174),row(103,48,25,6),row(104,58,25,0),row(104,21,25,209),row(104,20,26,210),row(105,10,25,1176),row(106,10,26,1176)])+'\n')
            manifest=dict(created_utc='test-cutoff',files=[dict(snapshot=p.name,sha256=hashlib.sha256(p.read_bytes()).hexdigest()) for p in [world,packet]])
            (checkpoint/'manifest.json').write_text(json.dumps(manifest))
            with patch.object(MODULE,'ROOT',root):r=MODULE.analyze(checkpoint)
            skills={s['id']:s for s in r['skills']}
            candidates=json.loads((checkpoint/'skill-audit/effect-candidates.json').read_text())['events']
            self.assertEqual([(e['kind'],e['resource'],e['affected_local']) for e in candidates],[(21,209,True),(20,210,False)])
            self.assertTrue(all(e['attribution']=='unassigned_resource_event' for e in candidates))
            self.assertEqual(skills[174]['local_finished'],1)
            self.assertEqual(skills[6]['local_instant'],1)
            self.assertEqual(skills[6]['local_finished'],0)
            self.assertEqual(skills[999]['local_starts'],0)
            self.assertEqual(r['summary']['local_obtainable'],2)
            self.assertNotIn(1176,skills) # Damage recipient is not a caster.
            damage=r['damage_skill_sightings'][0]
            self.assertEqual((damage['id'],damage['count']),(1176,2))
            self.assertEqual([e['affected_local'] for e in damage['evidence']],[True,False])
            self.assertTrue(all(e['attribution']=='damage_skill_id;caster_unassigned' for e in damage['evidence']))
            self.assertEqual(json.loads((checkpoint/'skill-audit/not-yet-captured-ids.json').read_text()),[999])


if __name__=='__main__':unittest.main()
