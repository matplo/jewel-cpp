"""Fixed Soft Drop definition for the JEWEL comparison, using hepyy FastJet.

Recluster every constituent with Cambridge/Aachen (E scheme), then follow the
harder pT branch until z > 0.1 (beta=0, reference R0=0.4). Untagged jets have
null zg and groomed_mass, rather than contributing an artificial zero entry.
"""
import math

SOFT_DROP = dict(algorithm="Soft Drop", reclustering="Cambridge/Aachen",
                recombination="E scheme", zcut=0.1, beta=0., R0=0.4,
                reclustering_R=1000., condition="z > zcut (delta_R/R0)^beta",
                untagged="null zg and groomed_mass; counted separately")


def soft_drop(jet, fj):
    constituents = fj.gbl.std.vector[fj.PseudoJet]()
    for particle in jet.constituents():
        constituents.push_back(particle)
    sequence = fj.ClusterSequence(constituents, fj.JetDefinition(
        fj.cambridge_algorithm, SOFT_DROP['reclustering_R']))
    roots = list(sequence.inclusive_jets(0))
    if len(roots) != 1:
        raise ValueError("C/A reclustering did not produce one complete jet")
    node = roots[0]
    left, right = fj.PseudoJet(), fj.PseudoJet()
    dropped = 0
    while node.has_parents(left, right):
        total = left.pt() + right.pt()
        if not math.isfinite(total) or total <= 0:
            raise ValueError("invalid Soft Drop branch transverse momentum")
        z = min(left.pt(), right.pt()) / total
        # beta=0: no division by angle, including exactly collinear branches.
        if z > SOFT_DROP['zcut']:
            return dict(zg=z, groomed_mass=node.m(), soft_drop_tagged=True,
                        soft_drop_dropped_branches=dropped)
        node = fj.PseudoJet(left if left.pt() >= right.pt() else right)
        dropped += 1
    return dict(zg=None, groomed_mass=None, soft_drop_tagged=False,
                soft_drop_dropped_branches=dropped)
