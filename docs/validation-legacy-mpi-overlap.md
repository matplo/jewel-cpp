# Legacy MPI overlap normalization and hard-impact sampling

`LegacyMpiOverlap` in `include/jewel/legacy_mpi.hpp` and `src/legacy_mpi.cpp`
now implements the double-Gaussian branch of PYMULT overlap initialization and
its hard-process impact-parameter proposal. It takes the saved integrated
partonic cross section and non-diffractive cross section explicitly. It does
not silently rebuild normalization when the live screening parameter changes.

The normalization follows the original bracketing/interpolation iteration and
radial integration: step size, stopping criteria, exponent caps, division-point
selection and arithmetic order are retained. Outputs include VNT146, VNT147,
mean impact parameter, dividing impact parameter and PIK. Invalid inputs and
nonconvergent or nonfinite iterations produce errors rather than fallback data.
The supported profile has core fraction 0..1 and radius ratio 0.1..1; the supplied
integral must satisfy the original 1.1*sigma_ND initialization criterion and
its floored-denominator target must exceed one.

The hard-impact proposal consumes one random draw to choose among outer/outer,
outer/core and core/core contributions, then one radial draw. It returns squared
impact parameter, VINT(148) enhancement and VINT(139) relative impact parameter.
The component does not apply the later process-dependent rejection loop, and
does not sample low-pT minimum-bias impact parameters.

## Fortran comparison

`reference/extract_mpi_overlap.py` verifies the frozen Pythia6 source hash and
extracts the initialization and sampling statements unchanged into a probe.
The reference receives the same parameters and two-value random tape as C++.
No native normalization or sampled output is supplied to Fortran.

The comparison covers 1,633 configurations and 16,330 scalar values: five core
fractions, three radius ratios, four integrated/ND cross-section ratios,
mixture boundaries and adjacent representable values, radial draws down to the
smallest normal double and up to the double immediately below one, plus the
captured pp initialization. Subnormal decimal inputs are excluded because the
text probe cannot reliably read them; no subnormal-boundary parity is claimed.

All comparisons pass with relative tolerance 2e-12 and absolute tolerance 2e-13.
Zero/nonzero decisions agree, and every proposal consumes exactly two draws in
both implementations. Maximum absolute error across outputs is 5.68434e-14.
Twelve invalid inputs are rejected, including an unsolvable normalization below
the floored-denominator target. The captured source initialization reproduces
VNT146=0.83059932866738195 and VNT147=0.091240840189946285 within tolerance.

The preceding hard-weight differential test also passes. Its live-state replay
was checked after adding the sampler and still reproduces 1,000 captured
Fortran weights exactly. These are component checks; no new full native event
sample or final-state jet agreement is claimed by this update.

## Remaining integration

The [screened scale-ordering proposal](validation-legacy-mpi-scale.md) is now
ported and independently validated. The saved cross-section table initializer,
successive interaction kinematics,
process-dependent rejection and flavour/colour handling remain to be ported
and connected. The existing event runner still uses Pythia8 MPI. Adding this
proposal on top of Pythia8's sampled MPI without replacing its selection would
not reproduce the reference algorithm, so it is not enabled in production.

[Machine-readable evidence](validation-legacy-mpi-overlap.json)

```sh
cmake --build build --target jewel-legacy-mpi-overlap-probe jewel-legacy-mpi-overlap-reference
ctest --test-dir build --output-on-failure -R '^legacy-mpi-(weight|overlap)-differential$'
```
