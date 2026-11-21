# Minimum-invariant-mass string constructor

`make_minimum_strings` in `src/strings_minimum.cpp` ports JEWEL 2.6.0
`MAKESTRINGS_MINL`, the constructor selected by HADROTYPE=1. The existing
HADROTYPE=0 constructor remains unchanged. Stock MAKESTRINGS dispatches only
these two modes.

The port preserves the source behavior:

- Select the highest-energy active parton within the rapidity domain.
- Split a selected gluon into an oppositely signed down-quark pair using the
  existing GENERATEZ QG sampler, including its random-draw sequence.
- Choose partners by the source invariant-product criterion, rapidity separation,
  opposite-sign endpoint and different-parent requirements.
- Append an artificial opposite endpoint of energy 10 GeV along either beam
  direction when no partner is available, with the source random sign choice.
- Preserve insertion shifts, parent/daughter slot updates, partial record on
  capacity rejection, colourless-particle copying and CLEANUP's physical-slot
  metadata behavior.

The rapidity operation is Pythia6 PYP(i,17): true rapidity reconstructed from
stored mass and momentum, not pseudorapidity or a new Pythia8 tune prescription.
The implementation preserves the source cut at |y| > ETAMAX+1, the strict
colourless-copy boundary, partner separation <4, and invariant-product cutoff
of 1e8 GeV squared. It does not silently restore momentum added by artificial
endpoints or substitute colour reconnection for this algorithm.

The result retains original input-slot provenance separately. Both halves of a
split gluon point to its original slot; artificial endpoints use the explicit
`size_t::max` sentinel. Physical colour fields after CLEANUP are not a valid
Pythia8 string graph and must not be exported directly.

## Reference evidence

The reference extractor verifies the stock JEWEL source hash and extracts
MAKESTRINGS_MINL, GENERATEZ and CLEANUP without changes. The Fortran harness
supplies a recorded random tape and only the PYP J=17 expression, copied from
the pinned Pythia6 source. Other PYP modes terminate explicitly. The extractor
also recognizes GENERATEZ's labeled END statement to avoid accidentally including
the next routine. Existing vacuum-string extraction remains unchanged.

All **158 cases, 2,042,832 scalar record values and 424 random draws** agree
exactly on this build. Four capacity-limit cases reject identically. Tests cover
mixed quark/gluon chains, actual record compression, isolated quarks/antiquarks/
diquarks/gluons, axial and zero-momentum gluons, colourless rapidity boundaries,
same/different parents, collinear pairs, the mass cutoff, empty records and
record capacity. Full K/P/V, production metadata and physical colour slots are
compared, not just a count of output strings.

The empty compressed case required preserving the stock no-op rather than calling
the generic C++ compression function, which rejects a zero-sized record.
Existing vacuum-string contract/differential, compression differential and
branching differential tests also pass. [Hashes and full case list](validation-minimum-strings.json).

```sh
ctest --test-dir build -R '^(minimum-strings-differential|strings-differential|strings-contracts|compression-differential|branching-differential)$' --output-on-failure
```

## Pythia8 integration

The ordered-string export and `--hadrotype 1` runner mode are now implemented.
See [adapter semantics and hadronization validation](validation-minimum-hadronization.md)
for fresh colour assignment, synthetic-endpoint provenance and the generated
quark mass-field adaptation. This component's exact Fortran-boundary result
remains distinct from the later Pythia8 adapter and from hadron-level physics
agreement. The requested final-jet ratio gate remains open.
