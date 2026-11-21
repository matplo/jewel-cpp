# Native gluon annihilation-width calculation

`legacy_gluon_width` in `src/legacy_mpi_channels.cpp` now translates the gluon
branch of PYWIDT. It calculates partial widths, the six enabled-width buckets
and the exact sum of buckets 1..4 used by the ISUB=96 annihilation channel.
The API supplies daughter masses, MDME mode, secondary weight and the common
fudge multiplier explicitly. The general resonance/fudge-selection policy
outside this branch is not ported by this component.

The threshold checks both daughter masses before evaluating the source's
first-daughter expression. Negative modes skip a channel entirely; mode zero
retains its partial contribution but does not enter the enabled sum. Modes
1..5 feed their respective buckets, while bucket 5 is excluded from the
annihilation sum. Only channels 6..8 apply WIDS secondary factors, and channels
above 8 retain zero partial width in this branch. These distinctions are
preserved rather than collapsed to a boolean enabled flag.

## Validation

An independent Fortran probe extracts the branch unchanged from the frozen,
hash-verified source. All 37,932 scalar comparisons across 2,349 configurations
agree exactly on this host. The matrix covers threshold-adjacent values, enable
modes, zero/nonzero fudge and secondary factors, unequal daughter masses, and
0/1/9/400-channel tables. Six invalid cases are rejected. Numerical tolerances
remain relative 2e-12 and absolute 2e-13, with exact zero/nonzero decisions.

A phase-aware read-only trace captures 100 initialization and 100 production
calls from the full reference. Native replay matches all 3,400 values exactly.
The instrumented 1,000-attempt run reproduces the baseline HepMC byte for byte:
`64a5289388223a4fb5e1b272007a431c773b5125cb3277e66d6718733e275ca3`.

The observed table has enabled d/u/s/c/b channels, a top channel with mode zero,
two skipped heavy channels, and a ninth massless channel whose width is zero
under this branch. Top mass is 175 GeV in this reference configuration; it is
not imported from a modern Pythia8 default. The measured fudge is one.
The recorded [decay configuration](../reference/legacy-gluon-decays.dat) is an
explicit test fixture, not a universal production default.

## Composition with the channel evaluator

The previous 200-call ISUB=96 channel trace was replayed again with the native
width calculation replacing its captured annihilation-width input. All 37,580
channel records and 75,360 numerical values still match exactly. This removes
one captured numerical input from the composed native evaluator; PDFs,
coupling, kinematics and common factors remain supplied by that trace.
All seven MPI/width differential suites also pass.

Evidence: [differential](validation-legacy-gluon-width.json),
[live width calls](validation-legacy-gluon-width-live.json),
[composed channel replay](validation-legacy-width-channel-composition.json).

```sh
ctest --test-dir build --output-on-failure -R '^(legacy-gluon-width|legacy-mpi-(weight|overlap|scale|phase|factor|channels))-differential$'
python3 tools/analyze_mpi_channel_trace.py build/jewel-legacy-mpi-channels-probe \
  /path/to/channel-trace /path/to/baseline/events.hepmc composition.json \
  --width-config reference/legacy-gluon-decays.dat
```

For live width reproduction, apply `reference/gluon_width_trace.patch` to a
copy of the frozen Pythia6 source and use the existing PYINIT phase-marker
core patch/helper. Rebuild the full reference with the diagnostic object and
run unchanged cards into a fresh directory. Preserve the source as `pythia.f`,
core as `core.f`, helper as `trace.f90`, executable as `reference`, patch as
`instrumentation.patch`, stdout as `run.log`, plus cards and event output.
Then run `tools/analyze_gluon_width_trace.py` with the native width probe,
trace directory, baseline HepMC path and output JSON.

Native PDF evaluation and threshold suppression, complete cross-section
assembly, table/bound initialization and event construction remain before
this can replace the current MPI backend. Full jet/substructure parity remains
unresolved; no event-generation default changes in this update.
