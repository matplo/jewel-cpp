# MPI string topology to Pythia8 hadronization

`legacy_mpi_colour_tags` converts an ordered, directed string-piece graph to
unique colour/anticolour tags. Each piece links its first endpoint's colour to
its second endpoint's anticolour. It checks every required colour port against
explicit particle colour types: singlet, triplet, antitriplet or octet.

`Pythia8Hadronizer::hadronize_string_pieces` obtains these types from Pythia8's
particle data, assigns tags on a copy of the input, then uses the existing
hadronization boundary. The original momenta, IDs and input tags are not mutated.
The input must be a complete outgoing state. Incoming/history particles are
rejected, and no missing remnant or endpoint is synthesized. Junction graphs
and sextet representations are outside this new string-piece interface; the
existing explicit-junction hadronization API remains available separately.

The mapper rejects reused colour ports, self-links, out-of-range endpoints,
missing/wrongly oriented endpoints, unsupported representations and invalid or
overflowing tag ranges. No random draws are used for tag assignment.

## Validation

[Results](validation-legacy-mpi-colour-boundary.json) cover 80 constructed
outgoing states, each with two pairs produced by the native MPI pair sampler
and attached by the native string update. Twenty cases each exercise:

- insertion of gluons into an existing quark string;
- separate closed gluon loops;
- separate quark-antiquark strings;
- gluon insertion between diquark/antidiquark endpoints, using Pythia8's
  colour representation rather than guessing orientation from the PDG sign.

All 80 states hadronize successfully with the mapped JEWEL fragmentation
profile, producing 6,151 final particles. All final particles are colourless;
maximum four-momentum residual is 6.1765037528971334e-12 GeV. Eleven invalid
cases are rejected. The diquark endpoints are supplied test inputs, not a
validated beam-remnant reconstruction algorithm.

The new boundary test, the recorded native MPI sequence replay, the existing
Pythia8 boundary contracts and fragmentation contracts all pass after rebuilding.
No Fortran hadron-level or jet comparison is implied by this closure test.
The preceding MPI topology and sequence comparisons remain separate evidence
that the native string-piece update follows the reference.

```sh
cmake --build build-pythia8 --target legacy-mpi-colour-boundary-test
ctest --test-dir build-pythia8 --output-on-failure -R '^legacy-mpi-colour-boundary$'
```

Complete collision integration still requires projecting the hard/MPI history
onto an outgoing colour-complete state with the correct remnants. Independent
MPI initialization, optional MPI showers and the required final-state jet and
substructure agreement remain unfinished. This API must not be called on the
raw captured MPI state as though its incoming/history entries were outgoing.
