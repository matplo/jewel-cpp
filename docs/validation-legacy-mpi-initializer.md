# Native MPI initialization tables and overlap normalization

`integrate_legacy_mpi` in `src/legacy_mpi.cpp` implements one PYMULT mode-1
integration pass. It samples all twenty stratified xT-squared bins, preserves
source draw and accumulation order, and returns each bin's sum/count plus the
integrated cross section. Each point consumes six draws. Unlike production,
initialization does not apply the 0.01 remaining-momentum reserve cut.

The native initializer executable connects this routine to `LegacyMpiCrossSection`
with LHAPDF and Pythia8 AlphaStrong, then feeds its calculated integral to
`LegacyMpiOverlap`. It therefore constructs the tables and double-Gaussian
normalization without receiving reference cross sections or normalization values.
Its sums/counts and overlap average are also the inputs required by the existing
`LegacyMpiTable` hard-weight component.

## Full-reference comparison

The read-only `reference/mpi_initializer_trace.patch` and logging initializer
`reference/mpi_initializer_trace.f90` capture the actual PYMULT integration passes,
random draws, sampled kinematics, cross sections, bins and saved overlap values.
The controlled 100-attempt run uses the same cards as the preceding native MPI
sequence comparison. Its HepMC output remains byte-identical to the uninstrumented
baseline. [Live evidence](validation-legacy-mpi-initializer.json) records hashes
and the settings for all four initialization channels.

Each channel uses 100 samples per bin. The 8,000 points and 48,000 draws yield:

| Beam channel | Native integrated cross section (mb) |
|---|---:|
| pp | 124.73191966455613 |
| pn | 147.32364488230098 |
| np | 146.96764844489297 |
| nn | 155.6085797570541 |

All 40,272 compared quantities pass. Maximum relative differences are 3.33e-14
for sampled kinematics, 1.48e-13 for individual cross sections, 7.69e-16 for bin
sums, 5.70e-16 for integrals and 7.16e-16 for overlap normalization. Bin indices,
counts and consumed-draw counts agree exactly. Relative/absolute tolerances
remain 2e-12/2e-13.

These channels do not all use the same screening input: pp starts with
1.060298518002389e-6, while pn/np/nn use 8.588417995819353e-7. This reflects the
reference's intervening secondary-initializer change, already documented in the
MPI saved-state audit. The native comparison supplies each captured screening
configuration explicitly; it does not yet reproduce that shared initialization
lifecycle by itself. The overlap core fraction/radius are .5/.4 and sigma_ND is
45.733890062687706 mb in each case.

## Regression fixture and limits

`reference/fixtures/legacy-mpi-initializer.json.gz` contains the captured settings,
draws, expected samples/tables/normalization and provenance. The
[fixture replay](validation-legacy-mpi-initializer-fixture.json) passes and labels
its reference evidence as archived. Run:

```sh
ctest --test-dir build-pythia8 --output-on-failure -R '^legacy-mpi-native-initializer$'
```

The initializer, connected native MPI sequences and colour-boundary tests all
pass after rebuilding. CT14nlo/0 is required. The current fixture covers four
accepted integration passes; adaptive screening retries are not yet implemented
by this one-pass API. Phase-space optimization coefficients and non-diffractive
cross sections are supplied settings, not independently initialized here.

Completing the initialization lifecycle, overestimate optimization, hard-event
history/remnants, optional MPI showers and full event-runner integration remains
necessary. This comparison establishes numerical construction of MPI tables and
normalization, not the required final-state jet/substructure agreement.
