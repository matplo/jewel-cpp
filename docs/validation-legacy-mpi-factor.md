# Legacy MPI differential common factor

`legacy_mpi_common_factor` now ports PYSIGH's dedicated ISTSB=9 screened MPI
sampling factor. It includes the collision-energy prefactor, tau mixture,
rapidity mixture and the screened xT factor. The caller supplies coefficients
1, 2, 8, 9 and 10 explicitly, along with the GeV^-2-to-mb conversion and K-factor.
The implementation preserves `SH=max(1,tau*s)` and the original arithmetic order.

The source's tau normalization uses `atan(1/xT_squared - 1)`, not the square-root
argument appearing in the proposal's tangent mapping. This translation retains
the actual reference expression. No analytic replacement or tuning is applied.

The returned common factor is not a differential cross section by itself.
Alpha_s, channel matrix elements and the two PDFs still have to be evaluated
and combined in source order. In the audited production mode, PDF and coupling
scales are shifted by screening; standard Pythia8's MPI PDF-scale policy remains
a known mismatch. Heavy-flavour PDF suppression and channel thresholds must
also be reproduced before the acceptance evaluator is complete.

## Validation

The Fortran probe extracts the dedicated common-factor statements unchanged
from the hash-verified frozen Pythia6 source. Both engines independently compute
the factor from identical external inputs.

All 8,100 comparisons agree exactly on this host. Four coefficient mixtures
are crossed with three collision-squared energies (including one exercising the
SH floor), five xT-squared values from 1e-10 to 1-1e-8, three tau choices, five
relative rapidity positions, three screening values and three K-factors.
Zero K-factor behavior is checked separately from numerical tolerances.
Twelve invalid configurations are rejected, including zero mixture density.
The supported finite domain has 0<xT_squared<=tau<1; singular collapsed
phase-space endpoints are rejected rather than assigned an invented weight.

All five MPI component differential tests pass together. The preceding
phase-space wrapper's subprocess label was corrected from ISTSB=2 to ISTSB=9;
its exercised rapidity mapping is unchanged and all 17,115 outputs still agree
exactly. The saved phase-space report now records the corrected reference binary.

[Machine-readable common-factor results](validation-legacy-mpi-factor.json)

```sh
cmake --build build --target jewel-legacy-mpi-factor-probe jewel-legacy-mpi-factor-reference
ctest --test-dir build --output-on-failure -R '^legacy-mpi-(weight|overlap|scale|phase|factor)-differential$'
```

## Remaining work

Implement the ISUB=96 channel factors and PDF weighting, including the source
heavy-flavour thresholds, then validate their sum against complete PYSIGH calls.
That evaluator is needed by both the sampling-table/bound initializer and the
production accept/reject loop. Event flavour/colour construction and generator
integration follow. The existing runner still uses Pythia8 MPI; this component
has not been layered onto it or used to claim final-state jet agreement.
