# Stateful native MPI evolution

`LegacyMpiEvolution` in `include/jewel/legacy_mpi.hpp` and `src/legacy_mpi.cpp`
composes the screened PYMULT mode-6 components into owned event state. Each
`step` proposes a lower scale, samples phase space, checks the 0.01 remaining-
momentum reserve, requests a cross section, accepts/rejects, constructs the
pair, inserts its string pieces and updates beam fractions and statistics.

The cross-section callback receives the proposed scale, phase-space variables
and current beam state. It is called only after momentum availability passes.
The implementation does not yet supply a built-in PDF/channel callback. This
interface enables that integration without coupling the state machine to a
particular PDF library or to the reference common blocks.

Rejected proposals retain their lower ordering scale but do not change the
parton list, strings, beam fractions or accepted-pair statistics. Accepted
pairs update the two remaining fractions, cumulative used fractions, interaction
count, additional-pair count, summed pT and first accepted pT in source order.
The native state stops at the scale cutoff or interaction count 240. Calls
after termination consume no randomness. Initial hard partons and string pieces
are supplied explicitly. The supported composition omits optional MPI showers;
it is not installed as a replacement for Pythia8 MPI in the event runner.

## Reference harness

The [extractor](../reference/extract_mpi_evolution.py) checks the bundled source
hash and copies the PYMULT scale, phase-space, acceptance, pair, string and
bookkeeping blocks unchanged. The wrapper supplies initial strings and identical
cross sections instead of calling PYSIGH. Its PYKLIM/PYKMAP wrappers retain the
previously validated massless hadron/hadron mapping. Event-capacity errors,
initial string reconstruction and optional PYSHOW calls are outside this harness.
Packed colour pointers are not compared; ordered KSTR topology is compared.

Both native and Fortran executables independently retain state across each
sequence. They receive identical ten-value tapes per step and expose the actual
number consumed; unused tape entries are not treated as consumed randomness.

[Results](validation-legacy-mpi-evolution.json): 90 constructed sequences,
7,200 step calls and 710,092 compared values pass. Maximum absolute difference
is 4.440892098500626e-16, with unchanged relative/absolute tolerances 2e-12/2e-13.
Decisions, counters, flavour IDs, endpoints and random consumption must match
exactly. The outcomes are:

- 592 accepted additional pairs;
- 1,190 momentum-reserve rejections;
- 609 cross-section rejections;
- 4,809 stopped returns, including repeated calls after termination.

The comparison includes full accumulated parton momenta and ordered strings
after every step. Independent assertions check downward ordering, preservation
of state on rejection, used-plus-remaining beam fractions, callback invocation,
accepted-draw counts and zero draws after termination. Configurations exercise
three phase-space mixtures, quark-pair and gluon attachment policies, depleted
beams, zero enhancement, pre-cutoff starts and starts at interaction count 239.
These are constructed sequences, not 90 complete collision events.

The [Pythia8-enabled build](validation-legacy-mpi-evolution-pythia8-build.json)
passes the same comparison. All eleven selected native MPI suites pass after
relinking. Reproduce with:

```sh
cmake -S . -B build
cmake --build build --target jewel-legacy-mpi-evolution-probe jewel-legacy-mpi-evolution-reference
ctest --test-dir build --output-on-failure -R '^legacy-mpi-evolution-differential$'
```

Next integration work is the native differential cross-section provider,
initial hard-event string extraction, Pythia8 colour tags and remnants, and
optional MPI shower handling. Full generator and jet/substructure validation
remain open; this state-machine comparison is not a physics-distribution gate.

The subsequent [connected sequence replay](validation-legacy-mpi-native-sequences.md)
replaces injected cross sections with the native provider and validates against
complete recorded PYMULT sequences with MPI showers disabled.
