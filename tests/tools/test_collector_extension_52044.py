import hashlib
import json
from pathlib import Path
import sys
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/quests'))
import extend_collectors_52044 as importer


class ExtensionTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.directory=tempfile.TemporaryDirectory()
        cls.wiki=Path('C:/Users/CmonMeow/Desktop/gw/Wiki/wiki.guildwars.com/wiki')
        cls.base,cls.original_header=importer.original_base(cls.wiki)
        cls.installed_header=(ROOT/'src/runtime/gw_presearing_collectors.h').read_bytes()
        cls.merged,cls.header=importer.stage(Path(cls.directory.name),cls.wiki)

    @classmethod
    def tearDownClass(cls):
        cls.directory.cleanup()

    def test_original_offers_and_source_preserved(self):
        self.assertEqual(self.merged['offers'][:22],self.base['offers'])
        self.assertEqual(self.merged['packet_source'],self.base['packet_source'])
        self.assertEqual(self.merged['packet_sha256'],self.base['packet_sha256'])
        self.assertEqual(self.merged['wiki_sources'][:-2],self.base['wiki_sources'])
        marker=self.original_header.rindex(b'return out;}')
        self.assertTrue(self.header.startswith(self.original_header[:marker]))
        self.assertTrue(self.header.endswith(self.original_header[marker:]))
        self.assertEqual((ROOT/'src/runtime/gw_presearing_collectors.h').read_bytes(),self.installed_header)

    def test_new_capture_distinct_and_exact(self):
        self.assertEqual([o['key'] for o in self.merged['offers']],list(range(1,25)))
        hatcher,varis=self.merged['offers'][-2:]
        self.assertEqual([(o['collector'],o['map'],o['profession'],o['trophy']['definition']['quantity']) for o in [hatcher,varis]],
                         [('Hatcher',166,6,5),('Varis',166,6,3)])
        for offer in [hatcher,varis]:
            self.assertEqual(offer['packet_sha256'],importer.PACKETS)
            self.assertNotEqual(offer['packet_sha256'],self.merged['packet_sha256'])
            self.assertEqual(offer['reviewed_armor'],dict(base=7,physical=0,elemental=0))
            self.assertEqual(offer['ordered_item_ids'],[offer['trophy']['definition']['id'],offer['reward']['definition']['id']])
            self.assertEqual(offer['non_menu_trophy_definition']['definition']['file'],offer['trophy']['definition']['file']&0x7fffffff)
        self.assertEqual(hatcher['reward']['definition']['model'],651)
        self.assertEqual(varis['reward']['definition']['model'],2268)
        self.assertEqual([o['heading']['encoded_utf16'][-1] for o in [hatcher,varis]],[261,259])

    def test_changed_immutable_input_rejected(self):
        path=Path(self.directory.name)/'changed'
        path.write_bytes(b'changed')
        with self.assertRaises(ValueError):importer.checked(path,importer.BASE_HEADER)

    def test_changed_wiki_armor_cannot_grant_stats(self):
        wiki=Path(self.directory.name)/'wiki';wiki.mkdir()
        for who in ['Brownlow','Jacobs','Kaylee','Walden','Savich','Humphreys','Hatcher','Varis']:
            if who!='Hatcher':
                (wiki/(who+'.html')).write_bytes((self.wiki/(who+'.html')).read_bytes())
                continue
            text=(self.wiki/(who+'.html')).read_text(encoding='utf8')
            if who=='Hatcher':
                at=text.index('Elementalist_Krytan_armor.html')
                following=text[at:];self.assertIn('<td align="center">7</td>',following)
                text=text[:at]+following.replace('<td align="center">7</td>','<td align="center">9</td>',1)
            (wiki/(who+'.html')).write_text(text,encoding='utf8')
        with self.assertRaises(AssertionError):importer.stage(Path(self.directory.name)/'bad-output',wiki)


if __name__=='__main__':unittest.main()
