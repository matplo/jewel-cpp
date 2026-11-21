# Surviving-parton massless preparation

`make_parton_massless` in `include/jewel/hard_initialization.hpp` ports
JEWEL 2.6.0 `MAKEMASSLESS` (stock source lines 8588–8632). It accepts the
emitted parton and two hard legs as distinct active `ShowerParticle` records.

Stock `GENEVENT` invokes this before `FINDVIRTUALITIES`: a non-hard quark or
gluon that is not above its nominal-mass threshold, is inside the pseudorapidity
acceptance, and has positive recorded mass is converted to massless kinematics.
This helper provides that conversion; the caller still has to implement the
source selection and call order when assembling ISR-enabled events.

The port boosts all three momenta and auxiliary generator vertices into the
hard-pair rest frame, transfers the emitted parton's removed rest-mass energy
to the hard pair, sets the emitted recorded mass to zero, and boosts back.
It preserves the source arithmetic and assignment order. In particular, each
hard momentum component is rescaled using a freshly evaluated magnitude after
the preceding component assignment. The source's `sqrt(abs(p1new2))` and its
separate energy expressions using the signed `p1new2` are retained.

Production coordinates (`MV`), ancestry, status, decay/evolution times, colour
and shower metadata are preserved. Source `PYROBO` acts on the auxiliary
`V` record, represented here by `generator_vertex`. Inputs must be finite and
records active and distinct. Undefined directions, zero pair-energy denominators
and nonfinite results raise an error; changes are only published after all three
results validate. No random numbers are used.

## Fortran comparison

The reference extractor verifies the original core and modified Pythia6 source
hashes and extracts the unedited `MAKEMASSLESS`, `PYROBO` and `PYP` routines.
The probe supplies their COMMON data; it does not substitute C++ calculations.
Reference extraction reruns when its manifest or extractor changes.

The 192-case matrix covers hard momentum scales 10 and 100 GeV, massless and
unequal-mass hard pairs, emitted masses 0, 0.13957 and 4.8 GeV, all three axes
and a mixed direction, and four common boosts including longitudinal beta=0.8.
Momenta, all three recorded masses and auxiliary vertices are compared.

All **5,184 scalar values pass**, with maximum absolute difference
3.552713678800501e-15, under unchanged absolute 2e-13 / relative 2e-12 tolerances.
[Machine-readable evidence](validation-massless.json) includes source,
executable and input hashes.

The matrix also measures two properties without imposing a new correction:

- Maximum total four-momentum closure residual: 2.2737367544323206e-13 GeV.
- Maximum hard-leg `abs(E^2 - p^2 - recorded_mass^2)`: 56.72485571913421 GeV^2.

The latter is a consequence of the literal sequential rescaling: a recorded
hard mass need not equal the mass calculated from the new four-momentum.
Replacing this with simultaneous vector rescaling would change the reference
algorithm. This port preserves the behavior and does not claim on-shell hard
legs at this intermediate stage.

All six selected CTest suites pass: massless contracts/differential,
kinematics differential, hard-initialization contracts/differential and
MPI-initialization differential. Contracts additionally check production metadata
preservation, alias rejection and unchanged records following a failed conversion.

```sh
cmake --build build -j 4
ctest --test-dir build --output-on-failure \
  -R '^(massless|kinematics|hard-initialization|mpi-initialization)-(contracts|differential)$'
```

This closes one preparation dependency. The native runner remains ISR/MPI-off;
full history-to-evolution integration and matched jet/substructure validation
are still outstanding.


The [combined surviving-parton preparation](validation-preparation.md) now
implements the source selection and ordered calls to this routine, with its
own Fortran comparison. Wiring that block into the full ISR adapter remains
separate work.
