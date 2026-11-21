# Coherent merge initialization validation

Recorded 2026-11-20, macOS arm64, Release builds, AppleClang 21 and GNU Fortran 16.2.
`begin_coherent()` translates DOCOHSCATTERING lines 4724–4835, before the first
GETDELTAT call. The reference extracts those statements unchanged after checking
the complete source hash and section hash. Its driver supplies event records and
zeros unused storage; it does not substitute C++ results into the reference.

[Recorded results](validation-coherent-start.json): **330 cases, 36,270 values,
maximum absolute difference zero** on this platform. Relative tolerance is
2e-12 and absolute tolerance 2e-13; decisions, IDs, statuses, colours and flags
must agree exactly.

The matrix covers both selected siblings, three momentum orientations, quark
and antiquark parents, gluon-to-quark pairs, both gluon colour directions,
ordinary and shortened intervals, zero duration, negative duration, disabled
coherence, non-splitting mothers, inactive/expired siblings, negative splitting
angles, root mothers and status-2 selected records. There are 120 accepted merges
and 210 unchanged/rejected cases. Every active event row is compared, including
momenta, auxiliary vertex rotations, production and decay coordinates, branch
metadata, colours, statuses and stock primary/daughter links.

## Event ancestry

The merged record retains `parent=first` for the stock K(:,3) meaning and adds
`coherent_partner=second`. Its portable particle ancestry contains both IDs.
Both incoming siblings have status 20 and two equal daughter slots pointing to
the coherent record, exactly as in Fortran. Duplicate daughter slots are allowed
only for this explicit merge representation. Validation checks both ancestry
edges for cycles and requires both incoming links to agree.

C++ contracts check complete snapshot rollback, non-recycled IDs, corrupted
snapshots, cycles through the second input, and reordered restored records.
Sibling ordering uses record position rather than numeric stable IDs. Collision
contracts also apply a two-kick sequence to a merged projectile: the incoming
coherent record retains both origins, while newly created recoils/outgoing
records have only their immediate collision origin.

## Preserved quirks and explicit limits

When the sibling's decay shortens `duration`, `remaining` retains the original
requested interval. This matches the source's assignment order and must remain
visible to the upcoming coherent loop. The second sibling undergoes the original
rotation round trip; it is not replaced with its saved pre-rotation momentum.

The routine saves both original sibling records in its result; the caller owns
the full event/recoil snapshot needed for rollback. Missing mothers return
inactive rather than indexing a nonexistent row. At the 22,990-record guard the
C++ event is marked discarded without appending a merged record. Unassigned new
auxiliary fields are initialized to zero, matching the zeroed reference driver;
no parity claim is made for undefined/stale Fortran auxiliary slots.

This report covers initialization and genealogy. The subsequent loop, failed-kick
rollback, propagation, final resplitting and histograms now have
[full-routine validation](validation-coherent-evolution.md). Complete generator
and final-jet validation remain open.

## Regression checks

All 38 reference-enabled CTest suites pass (180.77 seconds), including the full
collision-sequence and splitting comparisons. All 15 C++-only contract suites
also pass. No Fortran library is linked into the production C++ core.
