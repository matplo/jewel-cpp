# Native MPI string-piece attachment

`legacy_mpi_attach_strings` translates PYMULT's ordered KSTR topology update
into explicit endpoint indices. The last two supplied momenta are the new MPI
pair; existing pieces must reference preceding particles. The caller supplies
the retained RFLAV, PARP(85), and first pair flavour from pair generation.
The function returns a new ordered piece list and consumes no random numbers.

When RFLAV < PARP(85) and an existing piece is available, the two new gluons
are inserted sequentially. Each uses the source distance expression, including
its denominator floor of 1 GeV squared. The first piece initializes the minimum
even when its distance exceeds the source's initial 1e8 sentinel; subsequent
pieces replace it only for a strictly smaller distance. Equal distances therefore
retain the first piece. Each insertion replaces the chosen piece's second
endpoint and appends the new continuation. The second insertion sees the list
already changed by the first.

Otherwise, a gluon pair adds two oppositely directed pieces (a closed loop),
while a quark-antiquark pair adds one piece. Existing pieces retain their order.
Finite inputs, valid old endpoint indices, distinct endpoints and compatible
pair flavour are required. Incoming physical strings are supplied by the caller;
this routine does not initialize hard-event strings or assign Pythia8 tags.

## Validation

The hash-verified [extractor](../reference/extract_mpi_strings.py) copies the whole
PYMULT attachment block unchanged into a Fortran harness. The harness initializes
packed K links to zero and compares only the resulting ordered KSTR endpoint
list. KSTR selection and updates do not depend on those packed links. This
explicitly does not validate the packed K-link updates or complete colour flow.

[Results](validation-legacy-mpi-strings.json): all 44,956 integer comparisons in
2,180 cases agree exactly. There are 870 sequential-insertion cases, 1,000 gluon
loops and 310 quark strings. Cases cover open chains, closed loops, empty piece
lists, exact distance ties, collinear endpoints, denominator-floor operation,
large distances and adjacent doubles around the attachment threshold. Seven
invalid-input cases are rejected. Subnormal text inputs are excluded because
the probe parser does not support them.

Independent structural checks require valid endpoints, the expected number of
new pieces, preservation of old endpoint degrees under insertion, and one
incoming/outgoing piece for each inserted gluon. The
[Pythia8 build](validation-legacy-mpi-strings-pythia8-build.json) passes the same
matrix. All ten selected MPI component differential suites pass after relinking.

```sh
cmake -S . -B build
cmake --build build --target jewel-legacy-mpi-strings-probe jewel-legacy-mpi-strings-reference
ctest --test-dir build --output-on-failure -R '^legacy-mpi-strings-differential$'
```

This is a native topology component, not a complete colour adapter. Initial
string extraction, conversion to Pythia8 colour tags, remnant updates, optional
MPI showers and native event-loop integration remain unfinished. Final-state
jet/substructure agreement is still unestablished.
