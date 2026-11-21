# Effective coupling audit of the comparison samples

The existing sample settings are not coupling-equivalent. This is measured from
the initialized reference state and the installed Pythia8 coupling implementation,
not inferred from similarly named card entries.

The reference uses the same pp/CT14nlo cards as the current vacuum comparisons.
A temporary copy of its core calls a diagnostic at the end of INITPYTHIA and
then stops before event generation. Existing physics initialization and Pythia6
objects are unchanged. An initial misplaced diagnostic was corrected before
recording these results; only the successful post-initialization probe is evidence.
The [Fortran diagnostic](../reference/coupling_probe.f90) evaluates PYALPS using
the initialized hard Lambda and then the initialized ISR Lambda.

The initialized source has MSTP(2)=1, MSTP(3)=2, MSTU(111)=1, reference NF=4,
minimum NF=3 and maximum NF=5. Both PARP(1) and PARP(61) are 0.118 GeV.
PDFALPHAS=0.118 was used by the documented LHAPDF fallback as Lambda, despite its
alpha_s(M_Z) diagnostic label. Threshold matching gives Lambda5=0.0854944113493875
GeV and alpha_s(91.1876 GeV)=0.11754441689724156.

| Q (GeV) | Pythia6 hard/ISR | Native hard | Native ISR |
|---|---:|---:|---:|
| 2 | 0.266404 | 0.307562 | 0.388761 |
| 5 | 0.201425 | 0.213459 | 0.264326 |
| 10 | 0.172105 | 0.178279 | 0.216030 |
| 20 | 0.150237 | 0.153424 | 0.182657 |
| 91.1876 | 0.117544 | 0.118000 | 0.136500 |
| 1000 | 0.087492 | 0.088489 | 0.099396 |

The current native hard setting is alphaSvalue=.118, order=2. Tune 14 supplies
ISR .1365/order=1 and MPI .130/order=1; StandardModel:alphaSnfmax is 6.
CMW is off for ISR. The native probe invokes the same AlphaStrong initialization
used by these Pythia8 components. The source ISR configuration has MSTP(64)=2
and PARP(64)=1; this audit evaluates common numerical scales, not the different
virtuality/pT evolution-variable definitions in the shower algorithms. A separate
source MPI context/scale audit remains needed before claiming MPI equivalence.

## Verified convention translation

Pythia8 AlphaStrong normalizes at 91.188 GeV. Translating the measured source
Lambda5 into that convention gives alphaSvalue=0.11754434294459773, order=1,
nfmax=5 and CMW off. Using these values, all ten evaluated scales from 1 to
1000 GeV match reference PYALPS within rel=2e-12 / abs=2e-13. Maximum absolute
difference is 1.11022e-16. This includes both sides of charm/bottom thresholds
and the high-scale region where allowing a sixth flavour would differ.
[Native diagnostic](../reference/coupling_probe.cpp) and
[values, settings and hashes](validation-effective-couplings.json).

This is a coupling-function comparison only, for these source cards and thresholds.
It does not validate low-scale freezing below the tested range, ISR ordering,
MPI regularization or the full generator. The translated hard/ISR coupling is now exposed through explicit runner
options; see [runner validation](validation-input-coupling.md). Existing spectrum/substructure results remain
historical comparisons with mismatched couplings; their 10% gate failures are
unchanged. The hard/ISR translation is being tested in fresh samples without fitting to
ratios. The subsequent [MPI audit](validation-mpi-coupling.md) adds explicit MPI
controls and verifies its common-scale coupling translation, while identifying
automatic native screening reduction and remaining model differences.
