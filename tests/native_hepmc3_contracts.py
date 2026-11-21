"""Reject incomplete/malformed native runs before publishing HepMC output."""
import copy,json,tempfile,unittest,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from export_native_hepmc3 import scan,collection,export


def fixture():
 return [dict(type='run',schema='jewel-hadron-development-v1',attempts_requested=2),
  dict(type='event',attempt=1,weight=1,hadronized_particles=[dict(id=1,pdg=211,final=True,momentum=[1,0,0,1.01],mass=.14)]),
  dict(type='evolution_failure',attempt=2,weight=1),
  dict(type='summary',attempts=2,accepted=1,evolution_failures=1,generation_failures=0,generator_weight_sum=2,accepted_weight=1,failed_weight=1)]


class ExportContracts(unittest.TestCase):
 def check_run(self,rows):
  with tempfile.TemporaryDirectory() as directory:
   path=Path(directory)/'input.jsonl';path.write_text(''.join(json.dumps(r)+'\n' for r in rows))
   return scan(path)
 def test_complete_run_includes_failure_denominator(self):
  header,summary,digest=self.check_run(fixture());self.assertEqual(summary['attempts'],2);self.assertEqual(len(digest),64)
 def test_malformed_runs(self):
  cases=[fixture()[:-1]]
  for section,key,value in [(2,'attempt',3),(3,'accepted',2),(3,'accepted_weight',0),(3,'generator_weight_sum',1),(1,'weight',float('nan')),(0,'schema','jewel-parton-development-v1')]:
   rows=fixture();rows[section][key]=value;cases.append(rows)
  rows=fixture();rows[1]['hadronized_particles']*=2;cases.append(rows)
  for rows in cases:
   with self.assertRaises(ValueError):self.check_run(rows)
 def test_collection_policy(self):
  self.assertEqual(collection({}),'hadronized_particles')
  self.assertEqual(collection({'subtraction':{'radius':.4}}),'subtracted_particles')
  self.assertEqual(collection({'subtraction':{'put_on_shell':2}}),'projected_particles')
 def test_existing_output_untouched(self):
  with tempfile.TemporaryDirectory() as directory:
   path=Path(directory)/'output';path.write_text('keep me')
   with self.assertRaises(ValueError):export(Path(directory)/'absent',path)
   self.assertEqual(path.read_text(),'keep me')


if __name__=='__main__':unittest.main()
