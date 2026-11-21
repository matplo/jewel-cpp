# Scattering tables along expanding-medium trajectories

The C++ and frozen JEWEL 2.6.0 Fortran implementations independently initialize
and evaluate their medium and scattering tables. The harness now adds 49,248
GETSSCAT cross-section comparisons along prescribed massless trajectories.
The [machine-readable evidence](validation-scattering-rays.json) records the
source/executable hashes, input hashes, configurations and comparison errors.

For each of the six [thermodynamic configurations](validation-scattering-thermodynamics.md),
36 rays cross three starting points (0,0), (2,1), (8,0) fm; momentum rapidities
-1.5, 0, 1.5; and four azimuths separated by pi/2. Each projectile has energy
10 GeV and starts at lab time 0.05 fm with z=0. Its position advances at p/E.
Each ray has 38 samples through 30.05 fm, including dense early-time samples
between 0.05 and 0.8 fm. Positive starting time keeps the probes inside the
light cone. Both engines receive identical Cartesian inputs.

Each sample queries quark and gluon projectiles with combined outgoing channel C,
in local-medium mode 0 and screening-bound modes 1 and 2. Modes 1 and 2 are
bounding queries, not local cross sections. The configurations include flat and
Woods-Saxon profiles, different screening parameters and table energies, and
longitudinal boosts enabled and disabled.

| Comparison | Values | Maximum absolute error |
|---|---:|---:|
| Full grids and axes | 2,406,600 | 0 |
| Existing integrals and cross sections | 3,324 | 2.22045e-16 |
| Cross sections along rays | 49,248 | 8.88178e-15 |
| Grid boundaries and extrapolation | 210,132 | 0 |

All 2,669,304 comparisons pass with unchanged relative tolerance 2e-12 and
absolute tolerance 2e-13. Finite values and zero/nonzero agreement are separately
required. All 12 before/after table dumps (six configurations, two engines)
are byte-identical after the ordinary and trajectory queries.

All 49,248 ray cross sections are nonzero, including queries outside the active
medium. GETSSCAT returns an integrated cross section, not a collision probability:
the density and evolution-time factors are applied separately. A nonzero cross
section alone does not imply scattering outside the medium. The separate
[medium probes](validation-medium.md) test active/inactive local fields.

These rays do not undergo branching, deflection or energy loss. This validates
the coupling of medium queries to initialized cross-section tables; it does not
establish complete-cascade or final-state jet agreement. No production physics
implementation or numerical tolerance changed.

All 13 selected medium/table regression suites passed in 53.96 seconds on the
current macOS arm64 build (AppleClang 21, GNU Fortran 16.2):

```sh
ctest --test-dir build --output-on-failure -j 3 \
  -R '^(medium|sampling|radiation|final-splitting|pdf|scattering|event-geometry|geometry-lengths)-(contracts|differential)$'
```

The trajectory checks are implemented in `tests/scattering_differential.py`.
