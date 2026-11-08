"""Offline regression checks for capture planning; no game process access."""
import importlib.util
import hashlib
import json
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

ROOT=Path(__file__).resolve().parents[1]


def module(name):
    spec=importlib.util.spec_from_file_location(name,ROOT/'tools/skills'/f'{name}.py')
    result=importlib.util.module_from_spec(spec);spec.loader.exec_module(result);return result


queue=module('rebuild_capture_queue')
hunt=module('nonplayer_capture_plan')


class CapturePlans(unittest.TestCase):
    def test_independent_published_template_vector(self):
        # Published skill-template example, also independently used by native tests.
        expected=(9,0,[1600,1546,1553,1557,1584,0,0,2],{37:12,40:12})
        self.assertEqual(queue.decode('OQCiUyo8AkVwR4KMMGAAAEAA'),expected)
        self.assertEqual(queue.decode(queue.encode(*[expected[0],expected[1],expected[2],expected[3]])),expected)
        self.assertEqual(queue.encode(2,0,[],{}),'OgAAAAAAAAAAAAAA')

    def test_pet_discord_and_attack_chain_support(self):
        names=json.loads((ROOT/'data/reference/buildwars/skilldesc-en.json').read_text(encoding='utf-8'))['skilldesc']
        meta=queue.constants()
        supports,steps,_=queue.setup(817,names,meta)
        self.assertIn(99,supports);self.assertIn(117,supports)
        self.assertTrue(any('SAME living foe' in s for s in steps))
        supports,_,_=queue.setup(775,names,meta)
        self.assertEqual(supports[:2],[782,780])
        supports,_,_=queue.setup(781,names,meta)
        self.assertEqual(supports[:3],[782,780,775])
        supports,_,_=queue.setup(415,names,meta)
        self.assertIn(436,supports)

    def test_sections_reject_unrelated_backlinks_and_keep_child_context(self):
        page='<h2><span class="mw-headline" id="Locations">Locations</span></h2>A<h3><span class="mw-headline" id="Hard_mode">Hard mode</span></h3>B<h2><span class="mw-headline" id="Skills">Skills</span></h2>C'
        result=hunt.sections(page,('Locations',))
        self.assertIn('A',result);self.assertIn('B',result);self.assertNotIn('>C',result)

    def test_verified_owner_and_unresolved_environment(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);wiki=root/'wiki';wiki.mkdir()
            (root/'build/reports').mkdir(parents=True);(root/'data/reference/buildwars').mkdir(parents=True)
            (root/'build/reports/skill-coverage.json').write_text(json.dumps({'classification':{'obtainable_base_skill_ids':[1],'separate_pvp_variant_ids':[2]}}))
            (root/'build/reports/skill-runtime-coverage.json').write_text(json.dumps({'skills':[dict(id=10,gwca_name='Special_Hit')]}))
            (root/'data/reference/buildwars/skilldesc-en.json').write_text(json.dumps({'skilldesc':{'10':{'name':'Special Hit'}}}))
            checkpoint=root/'checkpoint';(checkpoint/'skill-audit').mkdir(parents=True)
            (checkpoint/'skill-audit/captured-skills.json').write_text(json.dumps({'cutoff':'fixed','skills':[dict(id=10,local_starts=0,other_or_unassigned_starts=2)]}))
            (wiki/'Monster_skill.html').write_text('<div class="mw-parser-output"><a href="Special_Hit.html">hit</a><a href="Fog.html">fog</a>')
            (wiki/'Special_Hit.html').write_text('<h1>Special Hit</h1><div class="mw-parser-output"><h2><span class="mw-headline" id="Notes">Notes</span></h2><a href="Beast.html">beast</a><a href="Impostor.html">impostor</a><a href="../outside.html">outside</a><a href="Category_Monster_skills.html">category</a>')
            (wiki/'Beast.html').write_text('<h1>Beast</h1>Template:NPC_infobox<div class="mw-parser-output"><h2><span class="mw-headline" id="Locations">Locations</span></h2><ul><li>Test Cave</li></ul><h2><span class="mw-headline" id="Skills">Skills</span></h2><a href="Special_Hit.html">hit</a>')
            # A non-NPC comparison list is not an owner even with a Skills heading.
            (wiki/'Impostor.html').write_text('<h1>Comparison</h1><div class="mw-parser-output"><h2><span class="mw-headline" id="Skills">Skills</span></h2><a href="Special_Hit.html">hit</a>')
            (wiki/'Fog.html').write_text('<h1>Fog</h1><div class="mw-parser-output"><a href="Category_Environment_effects.html">category</a><h2><span class="mw-headline" id="Notes">Notes</span></h2>Appears in Test Cave.')
            with patch.object(hunt,'ROOT',root):report=hunt.generate(wiki,checkpoint,root/'out')
            hit=next(r for r in report['skills'] if r['id']==10)
            self.assertEqual([o['name'] for o in hit['owners']],['Beast'])
            self.assertIn('Test Cave',hit['owners'][0]['location'])
            self.assertEqual(report['entries_with_packet_sightings'],1)
            checklist=(root/'out/MONSTER-NPC-ENVIRONMENT-CAPTURES.txt').read_text(encoding='utf-8')
            self.assertNotIn('Special Hit [10]',checklist)
            self.assertIn('Fog [ID unresolved]',checklist)
            self.assertNotIn('[seen]',checklist)
            fog=next(r for r in report['skills'] if r['name']=='Fog')
            self.assertIsNone(fog['id']);self.assertEqual(fog['kind'],'environment')
            self.assertIsNone(hunt.local(wiki,'../outside.html'))
            (checkpoint/'skill-audit/captured-skills.json').write_text(json.dumps(dict(cutoff='fixed',skills=[],
                damage_skill_sightings=[dict(id=10,count=2,evidence=[dict(file='packet',line=9)])])))
            with patch.object(hunt,'ROOT',root):damage=hunt.generate(wiki,checkpoint,root/'damage')
            self.assertEqual(next(r for r in damage['skills'] if r['id']==10)['damage_attributions'],2)
            self.assertNotIn('Special Hit [10]',(root/'damage/MONSTER-NPC-ENVIRONMENT-CAPTURES.txt').read_text(encoding='utf-8'))
            (checkpoint/'skill-audit/captured-skills.json').write_text(json.dumps(dict(cutoff='fixed',skills=[],
                effect_skill_sightings=[dict(id=10,count=2,evidence=[dict(file='packet',line=12,kind='upkeep_applied')])])))
            with patch.object(hunt,'ROOT',root):effects=hunt.generate(wiki,checkpoint,root/'effects')
            row=next(r for r in effects['skills'] if r['id']==10)
            self.assertEqual(row['effect_applications'],2)
            self.assertEqual(row['local_starts'],0)
            self.assertEqual(row['effect_evidence'][0]['kind'],'upkeep_applied')
            self.assertNotIn('Special Hit [10]',(root/'effects/MONSTER-NPC-ENVIRONMENT-CAPTURES.txt').read_text(encoding='utf-8'))
            # A shared player/special article does not attribute its special ID
            # to every NPC that links the ordinary skill page.
            page=wiki/'Special_Hit.html';original=page.read_text()
            page.write_text(original+'<div class="skill-image"><a title="1, 10">shared</a></div>')
            (checkpoint/'skill-audit/captured-skills.json').write_text(json.dumps(dict(cutoff='fixed',skills=[])))
            with patch.object(hunt,'ROOT',root):variants=hunt.generate(wiki,checkpoint,root/'variants')
            special=next(r for r in variants['skills'] if r['id']==10)
            self.assertEqual(special['owners'],[])
            self.assertFalse(special['npc_variant_attribution_verified'])
            self.assertIn('Shared article IDs 1, 10',special['context'])
            self.assertIn('Special Hit [10]',(root/'variants/MONSTER-NPC-ENVIRONMENT-CAPTURES.txt').read_text(encoding='utf-8'))
            page.write_text(original)
            # An observed NPC must disappear even when its conditional skill
            # has no activation packet. Keep the skill/reference in JSON.
            world=checkpoint/'world-1-1.jsonl'
            world.write_text(json.dumps(dict(event='npc_observations',map=34,instance_epoch=2,
                npcs=[dict(name='Beast',name_status='decoded',agent=9,model=10)]))+'\n')
            (checkpoint/'manifest.json').write_text(json.dumps(dict(files=[dict(snapshot=world.name,sha256=hashlib.sha256(world.read_bytes()).hexdigest())])))
            (checkpoint/'skill-audit/captured-skills.json').write_text(json.dumps(dict(cutoff='fixed',skills=[])))
            with patch.object(hunt,'ROOT',root):observed=hunt.generate(wiki,checkpoint,root/'observed')
            self.assertIn('Beast',observed['encountered_npc_sources'])
            remaining=(root/'observed/MONSTER-NPC-ENVIRONMENT-CAPTURES.txt').read_text(encoding='utf-8')
            self.assertNotIn('Special Hit [10]',remaining)
            self.assertNotIn('\nBeast\n',remaining)
            self.assertTrue(any(r['id']==10 for r in observed['skills']))
            receipts=(root/'observed/MONSTER-RECENT-CAPTURES.txt').read_text(encoding='utf-8')
            self.assertIn('cutoff UTC: fixed',receipts)
            self.assertIn('Beast | map 34 | world-1-1.jsonl:1',receipts)
            world.write_text('{}\n')
            with self.assertRaisesRegex(ValueError,'World checkpoint changed'):hunt.npc_encounters(checkpoint)


if __name__=='__main__':unittest.main()
