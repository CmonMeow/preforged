"""Developer preparation tests; this is not server runtime code."""
import copy,hashlib,json,sys,unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools/capture'))
from reviewed_npc_selection import supplements_at,append_supplements


class ReviewedNpcSelection(unittest.TestCase):
    def setUp(self):
        self.npc={'agent':90,'model':2013,'name_encoded_utf16':[11143,33439,41421,21363]}
        self.row={'event':'npc_observations','map':55,'instance_epoch':90,'instance_ms':700,'npcs':[self.npc]}
        self.raw=json.dumps(self.row)+'\n'
        self.item={'snapshot_line':39,'snapshot_instance_ms':700,'snapshot_sha256':hashlib.sha256(self.raw.rstrip('\n').encode()).hexdigest(),'captured_agent':90,'model':2013,'name_encoded_utf16':self.npc['name_encoded_utf16']}
        self.selection={'capture':'town.jsonl','map':55,'instance_epoch':90,'supplemental_observations':[self.item]}

    def observed(self,selection=None,capture='town.jsonl',epoch=90,row=None,raw=None,line=39):
        return supplements_at(selection or self.selection,capture,55,epoch,row or self.row,line,raw or self.raw)

    def test_exact_actor_added_after_unchanged_base(self):
        base=[{'agent':2,'model':3}];found=self.observed()
        result=append_supplements(base,found,[self.item])
        self.assertEqual(result,[base[0],self.npc]);self.assertEqual(base,[{'agent':2,'model':3}])
        self.assertEqual(self.observed(line=40),[])

    def test_cross_instance_capture_or_modified_snapshot_rejected(self):
        for kwargs in ({'epoch':91},{'capture':'other.jsonl'},{'raw':self.raw+' '}):
            with self.assertRaises(RuntimeError):self.observed(**kwargs)
        modified=copy.deepcopy(self.row);modified['instance_ms']=701
        with self.assertRaises(RuntimeError):self.observed(row=modified)

    def test_identity_and_duplicate_actor_rejected_even_when_hash_reviewed(self):
        for mutate in ('model','name','duplicate'):
            row=copy.deepcopy(self.row)
            if mutate=='model':row['npcs'][0]['model']=2011
            elif mutate=='name':row['npcs'][0]['name_encoded_utf16']=[1]
            else:row['npcs'].append(copy.deepcopy(row['npcs'][0]))
            raw=json.dumps(row);selection=copy.deepcopy(self.selection)
            selection['supplemental_observations'][0]['snapshot_sha256']=hashlib.sha256(raw.encode()).hexdigest()
            with self.assertRaises(RuntimeError):self.observed(selection=selection,row=row,raw=raw)

    def test_no_missing_duplicate_or_base_actor_reuse(self):
        found=self.observed()
        for base,observations in (([],[]),([],found+found),([self.npc],found)):
            with self.assertRaises(RuntimeError):append_supplements(base,observations,[self.item])


if __name__=='__main__':unittest.main()
