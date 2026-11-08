import sys,tempfile,unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools/skills'))
from nonplayer_capture_plan import polymock_forms

class PolymockForms(unittest.TestCase):
    def test_acquisition_only_and_observed_omitted(self):
        with tempfile.TemporaryDirectory() as folder:
            wiki=Path(folder)
            (wiki/'skill.html').write_text('<div class="mw-parser-output"><h2><span class="mw-headline" id="Acquisition">Acquisition</span></h2><a href="Real_Form.html">Real Form</a><h2><span class="mw-headline" id="Notes">Notes</span></h2><a href="Wrong_Form.html">Wrong Form</a>')
            rows=[dict(id=i,name='Polymock Test',article='skill.html',**evidence) for i,evidence in
                [(1,{}),(2,dict(local_starts=1)),(3,dict(other_or_unassigned_starts=1)),
                 (4,dict(effect_applications=1)),(5,dict(damage_attributions=1))]]
            forms,unknown=polymock_forms(wiki,rows)
            self.assertEqual([f['name'] for f in forms],['Real Form'])
            self.assertEqual([s['id'] for s in forms[0]['skills']],[1]);self.assertEqual(unknown,[])

    def test_missing_acquisition_stays_unresolved(self):
        with tempfile.TemporaryDirectory() as folder:
            wiki=Path(folder);(wiki/'skill.html').write_text('<a href="Wrong_Form.html">Wrong Form</a>')
            row=dict(id=1,name='Polymock Test',article='skill.html')
            forms,unknown=polymock_forms(wiki,[row]);self.assertEqual(forms,[]);self.assertEqual(unknown,[row])

if __name__=='__main__':unittest.main()
