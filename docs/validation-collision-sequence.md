# Full collision-sequence validation

`apply_collisions` implements the enclosing DOKINEMATICS sequence around the
previously tested momentum, colour/timing and storage helpers. The reference
now calls the complete unchanged routine extracted from hash-verified JEWEL
2.6.0, rather than a section wrapper. The production library remains C++ only.

The matrix contains 576 calls. All 534 wholly finite reference calls agree
within relative 2e-12 / absolute 2e-13 for ordinary numerical fields; discrete
statuses, flavours, active genealogy, colour labels, retry flags, shortened
transfer ranges and random counts agree exactly. There are 83,518 finite-field
comparisons, including finite fields from one otherwise nonfinite reference
call. Maximum raw absolute difference is 5.33e-14. Completed calls contain up
to six active records and consume up to 31 random draws; 130 compared calls
return RETRYSPLIT. Nine near-lightlike dummy masses use the separately reported
signed-invariant roundoff check described in the collision-step report.

Coverage includes all four KINMODE values and all four recoil storage modes,
quarks/antiquarks/gluons, ordinary and recoil projectiles, timelike and spacelike
record masses, single/two accepted transfers, zero/rejected transfers among
valid ones, a non-first initial transfer index, target-mass increase/decrease,
initial/final-state energy fractions and coherent/ordinary lifetime choices.
Target-mass retries shorten the transfer range as in stock. The source's final
lifetime overwrite is retained, including the additional random draw after a
last-transfer lifetime was already sampled. All active records and the full
allocated recoil-storage tail are compared.

## Reference failures remain explicit

- Twelve cases do not furnish a completed finite reference result. Eight
  unchanged Fortran calls exceed the harness's one-second timeout and are
  terminated; four exhaust the supplied 5,000-value random tape (STOP 6).
  C++ raises its 100,000-iteration guard or replay-exhaustion exception. These
  are not counted as parity passes. A separately inspected first timeout case
  also failed to complete within five seconds.
- Source inspection explains the repeated retry state: after rolling back N
  and shortening N2 to the first negative transfer, all remaining transfers can
  be rejected while LPART still refers to a removed record's mass. That stale
  mass can repeatedly fail the target check. This port preserves the tested
  retry decisions but bounds execution; it does not silently reinterpret them
  as a successful collision.
- Thirty calls contain nonfinite reference values. C++ rejects 29. In one
  near-lightlike dummy case, C++ returns a finite mass while Fortran's square
  root produces NaN. The finite fields are still compared, and the signed
  dummy invariant is checked against a propagated roundoff bound. That call
  remains outside the wholly finite count. All individual cases are retained
  in the JSON.

These limitations must be handled by the eventual event-level error/discard
policy; this report does not claim that every source parameter combination
produces a usable event.

## Owned event graph and rollback

A recoil has no stock primary parent (K(:,3)=0), but its incoming projectile
lists it as an outgoing record. `ShowerParticle::collision_source` represents
that connection separately. `Particle::parents` exposes the physical ancestry,
while `ShowerParticle::parent` preserves the primary-parent convention.
Validation checks both ancestry kinds for cycles and reciprocal outgoing links.
The reference comparison maps stable IDs back to active row numbers.

Rejected sequence attempts remove new nodes without reusing their stable IDs.
Original incoming/scratch round-trip arithmetic and decay-time updates survive,
as do allocated recoil-storage tail entries; the used count and maximum colour
are restored. When truncating, the C++ graph restores baseline daughter links
instead of retaining references to removed rows. The test records any reference
links beyond N before mapping them to zero; none occurred in this matrix, so
that representational cleanup has no raw-record parity claim here.

The original recoil classification and originally requested target mass stay
fixed across transfers and retries. Local incoming status changes and fallback
mass targets must not replace those inputs. This distinction was necessary for
both recoil promotion and the KINMODE=3 temperature-dependent recoil mass.

C++ contracts independently cover two-transfer genealogy, colour allocation,
ledger counts, final decay time, cycle rejection, rollback ID stability, invalid
IDs/ranges, zero-transfer target retries and the 22,990-record capacity guard.
The collision operation can partially mutate event/ledger state before an
exception; callers must restore a snapshot or discard that candidate. Random
draws from rejected attempts are intentionally not rewound.

The full reference-enabled regression suite passes 33/33 tests. The standalone
C++-only build passes 13/13 contract tests.

[Detailed results, failure cases and source/executable/input hashes](validation-collision-sequence.json).
Full shower scheduling, coherent/initial-state evolution, Pythia8 integration,
custom geometry/output completion and final-jet comparisons remain outstanding.
