# Native MPI evolution with calculated cross sections

The native sequence executable now connects `LegacyMpiEvolution` to
`LegacyMpiCrossSection`, using LHAPDF and Pythia8 AlphaStrong. It receives only
initial event state, explicit model settings and a reference random tape.
It proposes every subsequent scale and phase-space point, calculates the cross
section, decides acceptance, constructs pairs and updates beam fractions and
ordered string pieces. Intermediate states and cross sections are not supplied
to the native executable.

## Controlled reference and instrumentation

A 100-attempt JEWEL vacuum run uses the preceding matched pp setup with
`MPIFSR F`. All other cards are retained apart from the event count. The first
ten PYMULT mode-6 invocations are recorded. This deliberately isolates the
no-MPI-shower composition; it does not replace the eventual MPIFSR-on requirement.

`reference/mpi_sequence_trace.patch` adds read-only state and PYR-output logging.
`reference/mpi_sequence_trace.f90` emits the state snapshots and initializes the
logging flag. It records the actual initial partons and KSTR pieces, including
history entries, then snapshots the state at every proposal boundary and exit.
PYR itself remains the source generator; logging reads its result after the
source calculation. A separate uninstrumented run uses exactly the same cards.
The resulting HepMC files are byte-identical. [Live-run evidence](validation-legacy-mpi-native-sequences.json)
records executable, cards, helper, patch and output hashes.

## Results

All ten recorded sequences pass:

- 759 proposal steps, producing 42 accepted additional pairs;
- 5,264 random draws, with all 769 state-boundary checkpoints identical;
- 411,291 scalar state values identical;
- 117,630 integer values (counts, flavours, ordered endpoints) identical;
- 678 native cross-section evaluations passing unchanged tolerances, with
  maximum relative difference 9.891197425295553e-16.

The native replay consumes every captured draw exactly once. Its termination,
accepted/rejected decisions, momenta, remaining/used beam fractions, cumulative
pT statistics and string topology agree at every recorded boundary. Small
cross-section arithmetic differences do not change any decision in these tapes.
This is replay evidence on ten sequences, not a statistical equivalence claim.

## Saved regression fixture

`reference/fixtures/legacy-mpi-native-sequences.json.gz` stores the initial
settings/states, random tapes, expected snapshots and reference provenance.
The fixture is 118,488 bytes. The default Pythia8/LHAPDF build now registers:

```sh
ctest --test-dir build-pythia8 --output-on-failure -R '^legacy-mpi-native-sequences$'
```

This fresh native replay [passes against the saved fixture](validation-legacy-mpi-native-sequences-fixture.json).
The report explicitly labels the reference evidence as archived. It does not
pretend that each fixture test regenerated the full Fortran run. CT14nlo/0 must
be available to LHAPDF. The native probe takes the audited gluon-decay file and
alpha_s(MZ) as explicit arguments; order=1, nfmax=5 and CMW=false are fixed for
this comparison.

The same analyzer accepts a trace directory to repeat the live comparison:

```sh
python3 tools/analyze_mpi_native_sequences.py \
  build-pythia8/jewel-legacy-mpi-native-sequence-probe TRACE_DIRECTORY result.json \
  --width-config reference/legacy-gluon-decays.dat --alpha-s .11754434294459773
```

Native hard-event string extraction, normalization/overestimate initialization,
Pythia8 colour-tag and remnant integration, optional MPI showers, and connection
to the full event runner remain unfinished. The replay starts from reference
initial state and recorded randomness; it is not an independently initialized
collision generator. No jet or substructure agreement is claimed by this test.
