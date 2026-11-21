# Legacy MPI screened scale ordering

`LegacyMpiScaleSampler` in `include/jewel/legacy_mpi.hpp` and
`src/legacy_mpi.cpp` now ports the PYMULT mode-6 screened next-scale proposal
for MSTP(82)>=2. Its inputs are live screening squared, saved overlap average,
sampled overlap enhancement, the differential-cross-section upper bound and
the non-diffractive cross section. The bound is not the integrated partonic
cross section: substituting the saved integral would change the sampler.

The result retains both the source's candidate scale and a proposal flag.
At or below `0.01001 * screening_squared`, it stops without drawing a random
number. Otherwise it draws once, applies the source inverse-Sudakov formula,
and stops if the candidate is nonpositive. Positive candidates are floored at
`0.01 * screening_squared`; the next call then stops at the pre-draw cutoff.
Even zero proposal normalization follows the source draw-and-stop behavior.

This is an ordering proposal, not an accepted interaction. A subsequent
kinematic/PDF rejection must retry from the returned scale, not reset to the
previous upper scale. The [massless phase-space and momentum-fraction proposal](validation-legacy-mpi-phase.md)
is now ported and tested separately. Differential cross-section acceptance,
flavour and colour construction are not yet connected.

## Validation

The Fortran probe extracts the normalization factor and ordering statements
unchanged from the hash-verified frozen source. The harness supplies inputs
and a one-value random tape; proposal/termination decisions and consumed draws
are checked exactly, separately from numerical tolerances.

All 1,621 cases (6,484 scalar comparisons) agree exactly on this host. Cases
include both measured screening scales, a larger screening value, the cutoff
and its adjacent doubles, zero overlap/bound, exponential tails and points
straddling cancellation to zero and the returned-scale floor. Across the matrix
there are 721 pre-draw stops, 451 post-draw stops and 449 proposals.

Eight complete source-generated scale sequences are replayed through C++:
lengths 21, 17, 28, 19, 21, 21, 19 and 23 calls, all ending at the same point
with identical random consumption. The sequence inputs come from Fortran's
previous output rather than a duplicated Python formula. Nine invalid input
cases are rejected. Relative tolerance remains 2e-12 and absolute tolerance
2e-13; neither tolerance relaxes stopping or draw-count decisions.

All three legacy MPI differential tests pass together: hard weight, overlap
normalization/sampling and scale ordering. [Machine-readable scale results](validation-legacy-mpi-scale.json).

```sh
cmake --build build --target jewel-legacy-mpi-scale-probe jewel-legacy-mpi-scale-reference
ctest --test-dir build --output-on-failure -R '^legacy-mpi-(weight|overlap|scale)-differential$'
```

## Integration status

The three components can now represent the saved overlap state, propose a hard
impact parameter, evaluate the hard-event survival weight and propose decreasing
MPI scales. They remain independent of Pythia8's active MPI generator. The
cross-section table/bound initializer and differential kinematic/PDF acceptance
are the next missing dependencies; using a guessed bound or a weight layered on
top of Pythia8's own MPI would not reproduce the reference.

No native event-generation default changed and no new full-event jet parity
is claimed. The requested 10% jet/substructure gate remains unresolved.
