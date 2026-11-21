# Adaptive native MPI screening initialization

`initialize_legacy_mpi` wraps the native stratified integrator in the source
PYMULT acceptance/retry rule. It accepts when the calculated integral is at
least `1.1 * sigma_ND`; otherwise it multiplies pT0Ref by 0.9, recalculates the
energy-dependent pT0 and screening fraction, rebuilds the cross-section provider
and performs another complete integration pass. Random consumption continues
across retries. Rejected tables are retained as diagnostics; the last table is
the accepted one. A caller-specified pass limit throws on nonconvergence and
never silently accepts a failed pass.

## Controlled reference configuration

To exercise rejection, `reference/mpi_retry_configuration.patch` sets
PARP(82)=4 GeV immediately before PYINIT calls in a temporary reference build.
This is an explicit initial-setting variation, not a modification of PYMULT's
physics formulae and not a change to the installed recipe/default generator.
The same configuration-only core is used in both the uninstrumented baseline
and the logging build. Cards otherwise match the preceding 100-attempt run
with optional MPI showers disabled.

`reference/mpi_retry_trace.patch` and `reference/mpi_retry_trace.f90` log each
integration pass, sampled point, random draw, table and final normalization.
The instrumented and uninstrumented HepMC files are byte-identical.
[Live evidence](validation-legacy-mpi-retries.json) records source/configuration
patches, executables, cards and output hashes.

## Results

All four channels independently reject three passes and accept the fourth:
4 -> 3.6 -> 3.24 -> 2.916 GeV in pT0Ref. The native executable receives only the
initial settings and continuous random tape, not the reference retry decisions,
subsequent screening values or cross sections.

| Channel | Accepted integral (mb) |
|---|---:|
| pp | 56.708985020845645 |
| pn | 57.01236772536843 |
| np | 55.14899129166581 |
| nn | 56.504138931747406 |

Across 16 passes, all 32,000 points and 192,000 draws are accounted for. Pass
counts, acceptance decisions, bin counts, screening parameters and consumed-draw
counts agree exactly. Maximum relative difference in integrated cross sections
is 7.52e-16; for final overlap normalization it is 5.47e-15. Individual sampled
kinematics/cross sections pass the unchanged combined relative/absolute tolerances
2e-12/2e-13; small values can pass via the absolute tolerance. No tolerance was
relaxed.

`tests/legacy_mpi_initialization_contracts.cpp` additionally checks exact equality
at the floating-point acceptance threshold, continuation after a failed pass,
correct screening reduction, rejection of a zero pass limit and an explicit
exception when the pass limit is reached. The equality fixture uses an exactly
representable threshold so decimal rounding is not mistaken for a branch error.

The compressed fixture `reference/fixtures/legacy-mpi-retries.json.gz` supports
fresh native replay without regenerating the full Fortran run. Its
[report](validation-legacy-mpi-retries-fixture.json) labels that evidence as archived.
The retry fixture, initialization contracts, preceding no-retry initializer
fixture and connected MPI sequence fixture all pass.

```sh
ctest --test-dir build-pythia8 --output-on-failure \
  -R '^(legacy-mpi-native-retries|legacy-mpi-initialization-contracts)$'
```

Phase-space optimization coefficients and sigma_ND are still supplied inputs.
The secondary PYMIGN initializer and shared-state lifecycle, full event
history/remnants, optional MPI showers and event-runner integration remain
unfinished. The recorded initialization comparison does not establish final
jet or substructure agreement.
