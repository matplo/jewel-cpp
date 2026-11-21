"""Analytic grooming checks through the actual hepyy/cppyy binding."""
import math
from pathlib import Path
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from jet_substructure import soft_drop
from hepyy_utils.fastjet import require_fastjet
fj=require_fastjet()

def check(inputs):
    particles=fj.gbl.std.vector[fj.PseudoJet]()
    for pt,phi in inputs:
        particles.push_back(fj.PseudoJet(pt*math.cos(phi),pt*math.sin(phi),0,pt))
    sequence=fj.ClusterSequence(particles,fj.JetDefinition(fj.antikt_algorithm,.4))
    jets=list(sequence.inclusive_jets(0))
    assert len(jets)==1
    return soft_drop(jets[0],fj),jets[0].m()

r,m=check([(80,0),(20,.2)])
assert r['soft_drop_tagged'] and math.isclose(r['zg'],.2,abs_tol=1e-15)
expected=math.sqrt(2*80*20*(1-math.cos(.2)))
assert math.isclose(r['groomed_mass'],expected,rel_tol=1e-12)
assert r['soft_drop_dropped_branches']==0
r,m=check([(40,-.05),(40,.05),(1,.3)])
assert r['soft_drop_tagged'] and math.isclose(r['zg'],.5,abs_tol=1e-15)
assert r['soft_drop_dropped_branches']==1
assert math.isclose(r['groomed_mass'],math.sqrt(2*40*40*(1-math.cos(.1))),rel_tol=1e-12)
assert r['groomed_mass']<m
for inputs,dropped in [([(20,0)],0), ([(90,0),(10,0)],1)]:
    r,m=check(inputs)
    assert not r['soft_drop_tagged'] and r['zg'] is None and r['groomed_mass'] is None
    assert r['soft_drop_dropped_branches']==dropped
print('hepyy Soft Drop: analytic two-body, soft-branch removal, singleton, strict threshold passed')

if '--contrib' in sys.argv:
    import hepyy,random
    hepyy.load('fjcontrib')
    reference=fj.gbl.fastjet.contrib.SoftDrop(0.,.1,.4)
    rng=random.Random(2601001)
    compared=0
    for n in [1,2,3,8,30]:
        for case in range(40):
            particles=fj.gbl.std.vector[fj.PseudoJet]()
            for i in range(n):
                pt=10**rng.uniform(-1,2)
                eta=rng.uniform(-.12,.12)
                phi=rng.uniform(-.12,.12)+(math.pi if case%2 else 0)
                mass=rng.choice([0.,.13957,.93827])
                px,py,pz=pt*math.cos(phi),pt*math.sin(phi),pt*math.sinh(eta)
                particles.push_back(fj.PseudoJet(px,py,pz,math.sqrt(px*px+py*py+pz*pz+mass*mass)))
            sequence=fj.ClusterSequence(particles,fj.JetDefinition(fj.antikt_algorithm,.4))
            jets=list(sequence.inclusive_jets(0))
            assert len(jets)==1
            actual=soft_drop(jets[0],fj)
            expected=reference(jets[0])
            structure=expected.structure_of[fj.gbl.fastjet.contrib.SoftDrop]()
            assert actual['soft_drop_tagged']==structure.has_substructure()
            if actual['soft_drop_tagged']:
                assert math.isclose(actual['zg'],structure.symmetry(),rel_tol=2e-12,abs_tol=2e-13)
                assert math.isclose(actual['groomed_mass'],expected.m(),rel_tol=2e-12,abs_tol=2e-13)
            compared+=1
    print(f'FastJet-contrib differential: {compared} jets passed')
