# MPI source scheduling versus retained ancestry

The previous native importer gave all hard legs original-record parents. That
preserves provenance but incorrectly extends primary-pair physics semantics to
MPI. Stock `findmpivirtualities` searches for parent-zero candidates, while
`GETTHETAM` suppresses a colour-partner angle when its parent index is at most 2.
Using a native MPI provenance anchor beyond slot 2 can therefore introduce an
angle that stock JEWEL would suppress.

Native MPI hard legs now retain stable ancestry to their original source
records, with separate scheduling slots `{0,0,0}`. The existing state API
exposes those zero parents to colour-angle and coherence calculations.
Primary hard legs retain their original-record scheduling parents. Beam and
ISR scheduling remain unchanged. Survivor preparation preserves both types
of links. This change does not enable full ISR/MPI cascade evolution.

## Stock MPI-enabled trace

The isolated stock JEWEL 2.6.0 trace ran 20 vacuum events with MPI enabled,
sqrt(s)=5020 GeV, pThat 50–400 GeV, eta_max=3.1 and job seed 901. The traced
and uninstrumented executions produced byte-identical HepMC and random-state
files. After event interpretation, and again before the cascade, records
beyond the eight header slots contained:

- 172 parent-zero QCD records and four parent-zero non-QCD records;
- 20 primary records parented to slot 7 and 20 parented to slot 8;
- 83 ISR records parented to incoming slots 3/4;
- 59 beam-parented remnant records.

The four parent-zero non-QCD records support the source's warning about baryon
remnants in its MPI discovery loop. The native implementation continues to
use explicit Pythia8 subsystem identities; it does not treat every parent-zero
record as an MPI hard leg. These source counts are not a one-to-one comparison
of Pythia6 and Pythia8 histories.

## Native validation

Two 1,000-attempt ISR+MPI samples check the separate scheduling-parent fields
for every hard pair in the 1,997 mapped events: 3,994 primary hard legs retain
original parents and 23,496 MPI hard legs expose zero. Stable ancestry remains
unchanged for all 27,490 hard legs. The same three frame-mapping failures remain
recorded. Timing fixtures are byte-identical to the prior parentage run and all
134,660 emitted timing values still match Fortran exactly.

A colour-angle contract explicitly gives a partner a nonzero provenance parent
and zero scheduling parent, then checks the suppressed angle. Generated input
contracts also check scheduling parents and their preservation through survivor
preparation. Primary and MPI initialization differential suites pass unchanged.

[Machine-readable evidence](validation-mpi-scheduling.json) contains the stock
trace report, parent counts and both native audits.

The next initialization adapter must also preserve the primary TYPE1/TYPE2
context established by `findspeclines`. Source ordering, complete subsystem
initialization and full cascade/jet validation remain unfinished.
