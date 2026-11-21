# Native legacy MPI channel evaluator

`legacy_mpi_channels` in `src/legacy_mpi_channels.cpp` now translates the full
ISUB=96 PYSGQC channel block: quark/quark, quark/antiquark annihilation, quark/gluon,
gluon/light-quark, gluon/charm and bottom, and gluon/gluon terms. It preserves
channel order, incoming flavours and source channel codes, including identical-
flavour interference and the separate heavy-pair angular treatment.

The source heavy-pair opening condition is strictly `s_hat > 4.04*m^2`, with
mass-dependent beta and t/u reconstruction. Light flavours use their own
mass-dependent weights and enable flags. These are not replaced by Pythia8's
default heavy-flavour settings.

The evaluator retains both the unweighted matrix value and the PDF-weighted
value per channel. It multiplies by beam A's x*f first, beam B's second, then
sums in source order. The API's 11 PDF entries are ordered antiquarks -5..-1,
gluon, quarks 1..5. Signed finite PDF entries are preserved rather than silently
clamped; the synthetic test includes signed inputs to check that convention.

The caller still supplies alpha_s, the common factor, FACA, the PYWIDT-derived
annihilation-width sum, masses/enable flags, and the evaluated PDFs. This is not
yet a self-contained cross-section evaluator or an event-generation backend.
The currently supported interference modes are MSTP(34)=0 and 1.

## Independent differential checks

The Fortran probe extracts the complete channel block unchanged from the
hash-verified frozen source. PYWIDT's summed output and PDFs are supplied as
external inputs in both engines. The test covers 840 configurations and 156,400
channel records, with 313,640 numerical comparisons. All numeric values agree
exactly on this host; channel counts, identities and ordering also match.

The matrix crosses two mass sets, both interference modes, five scattering
angles, flavour-enable masks, varying common/FACA factors and nonuniform PDFs.
Charm and bottom thresholds are queried at and around their candidate opening
points, including adjacent doubles and relative offsets. Eleven invalid inputs
are rejected. Relative tolerance remains 2e-12 and absolute tolerance 2e-13;
zero/nonzero agreement is checked separately.

## Actual reference-call replay

A read-only diagnostic records the first 100 ISUB=96 calls during initialization
and the first 100 after PYINIT, using explicit phase markers. The captured inputs
include actual matrix kinematics, couplings, PYWIDT sum, masses, enable flags and
PDFs. Native replay matches all 37,580 channel records and 75,360 numeric values
exactly, including totals calculated by complete PYSIGH calls.

The instrumented full 1,000-attempt run reproduces the baseline HepMC byte for
byte, SHA-256
`64a5289388223a4fb5e1b272007a431c773b5125cb3277e66d6718733e275ca3`.
No random draws or physics values were changed. This validates the evaluator
with real inputs; it does not independently validate the external PDF or
annihilation-width calculations that produced them.

All six legacy MPI differential suites pass together. See the
[differential evidence](validation-legacy-mpi-channels.json),
[live-call evidence](validation-legacy-mpi-channels-live.json), and
[diagnostic Pythia6 patch](../reference/mpi_channels_trace.patch).

```sh
cmake --build build --target jewel-legacy-mpi-channels-probe jewel-legacy-mpi-channels-reference
ctest --test-dir build --output-on-failure -R '^legacy-mpi-(weight|overlap|scale|phase|factor|channels)-differential$'
```

To reproduce the live trace, rebuild a copy of the reference with the diagnostic
Pythia6 patch, `reference/mpi_phase_core.patch`, and `reference/mpi_phase_trace.f90`
(the latter supplies the PYINIT phase markers). Run the unchanged baseline cards
in a fresh directory. Preserve `pythia.f`, `core.f`, `trace.f90`, `reference`,
`instrumentation.patch`, `run.log`, cards and event output for provenance.

```sh
python3 tools/analyze_mpi_channel_trace.py build/jewel-legacy-mpi-channels-probe \
  /path/to/trace-directory /path/to/baseline/events.hepmc live-channels.json
```

## Remaining integration

Connect native PDF evaluation at the reference scale, reproduce its threshold
suppression and PYWIDT annihilation sum, and validate complete differential
cross sections. Then construct the sampling tables and bounds and connect the
accept/reject sequence and flavour/colour event construction. The current event
runner continues using Pythia8 MPI; full jet/substructure agreement is unresolved.

The [native gluon-width calculation](validation-legacy-gluon-width.md) now
replaces the captured annihilation sum in an additional composed replay,
retaining exact agreement for all 75,360 numerical values.
