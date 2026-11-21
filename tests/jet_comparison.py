"""Comparison gates must not turn missing evidence into physics validation."""
import copy
import math
from pathlib import Path
import sys
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from compare_jet_samples import compare


def fixture():
    samples=[]
    for schema in ['jewel-hadron-development-v1','jewel-fortran-hepmc-analysis-v1']:
        samples.append(dict(run=dict(schema=schema,mode='vacuum'),selected_jets=1000,
            denominator_scope='written-events',distributions={'pt':[
                dict(low=10,high=20,jets_per_attempt=2.,event_standard_error=.2)]}))
    return dict(samples=samples)


class ComparisonTests(unittest.TestCase):
    def test_equal_yields_do_not_prove_physics_agreement(self):
        result=compare(fixture(),0,1)
        self.assertEqual(result['observables']['pt']['numerical_gate'],'pass')
        self.assertEqual(result['physics_gate'],'not-established')
        self.assertAlmostEqual(result['observables']['pt']['bins'][0]['ratio_standard_error'],math.sqrt(.02))

    def test_ratio_thresholds_and_sample_count(self):
        for ratio,expected in [(.9,'pass'),(1.1,'pass'),(.899,'not-passed'),(1.101,'not-passed')]:
            data=fixture();data['samples'][0]['distributions']['pt'][0]['jets_per_attempt']=2*ratio
            self.assertEqual(compare(data,0,1)['observables']['pt']['numerical_gate'],expected)
        data=fixture();data['samples'][1]['selected_jets']=999
        self.assertFalse(compare(data,0,1)['sample_size_gate'])
        self.assertEqual(compare(data,0,1)['observables']['pt']['numerical_gate'],'not-passed')

    def test_zero_reference_is_not_silently_dropped(self):
        for numerator in [0.,2.]:
            data=fixture()
            data['samples'][0]['distributions']['pt'][0]['jets_per_attempt']=numerator
            data['samples'][1]['distributions']['pt'][0]['jets_per_attempt']=0.
            result=compare(data,0,1)['observables']['pt']
            self.assertEqual(result['undefined_bins'],1)
            self.assertIsNone(result['bins'][0]['ratio'])
            self.assertEqual(result['numerical_gate'],'not-passed')

    def test_zero_numerator_keeps_uncertainty(self):
        data=fixture();data['samples'][0]['distributions']['pt'][0]['jets_per_attempt']=0.
        row=compare(data,0,1)['observables']['pt']['bins'][0]
        self.assertEqual(row['ratio'],0.)
        self.assertEqual(row['ratio_standard_error'],.1)

    def test_incompatible_bins_and_nonfinite_inputs_rejected(self):
        for field,value in [('low',11),('jets_per_attempt',float('nan')),('event_standard_error',-1)]:
            data=fixture();data['samples'][0]['distributions']['pt'][0][field]=value
            with self.assertRaises(ValueError):compare(data,0,1)
        data=fixture();data['samples'][1]['distributions']={}
        with self.assertRaises(ValueError):compare(data,0,1)

    def test_denominator_mismatch_visible_and_input_unchanged(self):
        data=fixture();data['samples'][0]['denominator_scope']='attempted-events'
        before=copy.deepcopy(data);result=compare(data,0,1)
        self.assertTrue(any('denominators differ' in reason for reason in result['blockers']))
        self.assertEqual(data,before)


if __name__=='__main__':unittest.main()
