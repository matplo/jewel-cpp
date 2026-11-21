"""Regression: acceptance cuts must never change the hard-leg assignment."""
import copy
import math
from pathlib import Path
import sys
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from native_geometry_analysis import associate_geometry


def event():
    return dict(initial_hard=[dict(id=3,momentum=[10,0,10*math.sinh(2.05),10*math.cosh(2.05)]),
                              dict(id=4,momentum=[-10,0,0,10])],
        geometry=[dict(leg_id=i,shower_start_id=i+2,production_fm=[0,0,0,0],
                       L_geom_T_fm=5,L_hot_fm=3,L_hot_T_fm=2,int_T3_dl=.1) for i in [1,2]])


class GeometryMatching(unittest.TestCase):
    def test_match_outside_acceptance_is_not_reassigned(self):
        jets=[dict(jet_id=0,jet_rank=1,pt=50,eta=2.04,phi=0),
              dict(jet_id=1,jet_rank=2,pt=40,eta=1.95,phi=0),
              dict(jet_id=2,jet_rank=3,pt=30,eta=0,phi=math.pi)]
        matches,by_jet=associate_geometry(event(),jets)
        self.assertEqual(matches[0]['jet_id'],0)
        self.assertFalse(matches[0]['jet_passes_analysis'])
        self.assertTrue(matches[0]['ambiguous'])
        self.assertNotIn(1,by_jet)
        self.assertTrue(matches[1]['jet_passes_analysis'])
        self.assertEqual(by_jet[2]['matched_leg_id'],2)

    def test_low_pt_match_retained_and_unmatched_has_no_jet(self):
        jets=[dict(jet_id=0,pt=5,eta=2.05,phi=0)]
        matches,by_jet=associate_geometry(event(),jets)
        self.assertTrue(matches[0]['matched']);self.assertFalse(matches[0]['jet_passes_analysis'])
        self.assertFalse(matches[1]['matched']);self.assertIsNone(matches[1]['jet_id'])
        self.assertEqual(set(by_jet),{0})

    def test_empty_jets_and_invalid_geometry(self):
        data=event();before=copy.deepcopy(data)
        matches,by_jet=associate_geometry(data,[])
        self.assertEqual(by_jet,{})
        self.assertTrue(all(not m['matched'] for m in matches))
        self.assertEqual(data,before)
        data['geometry'][0]['L_hot_fm']=float('nan')
        with self.assertRaises(ValueError):associate_geometry(data,[])
        data=event();data['geometry'][1]['shower_start_id']=3
        with self.assertRaises(ValueError):associate_geometry(data,[])


if __name__=='__main__':unittest.main()
