# Hard junctions with first-splitting angular veto disabled

Native subsystem initialization now omits the unused colour-angle diagnostic
when `first_angle_ordering=false`. Source `FINDVIRTUALITIES` computes the two
GETTHETAM values at lines 1948–1952, but uses them only inside the
AOFIRSTSPLIT veto at lines 2071–2093 of the frozen JEWEL 2.6.0 core.
GETTHETAM does not consume random numbers. No junction opening angle is invented.

The core defaults to evaluating the diagnostic, retaining existing source replay.
`evaluate_colour_angles=false` is rejected before random draws if the primary
first-angle veto is enabled. The result explicitly marks whether angles were
evaluated; placeholder -1 values must not be interpreted as measured angles.
The native outgoing-flavour adapter selects omission only with the veto off.
Run headers record `hard_colour_angle_policy`. Junction topology remains intact
through the shower and explicit Pythia8 junction hadronization.

This does not implement angular ordering for a primary junction leg when the
first-splitting veto is enabled, nor does it reproduce Pythia6 event generation.

## Validation

Hard-initialization, subsystem-initialization and experimental runner contracts
pass. The primary and MPI differential suites against unchanged Fortran routines
also pass (288 primary cases / 45,792 values, 1,152 MPI cases / 183,168 values).
A direct core test verifies identical sampled kinematics, masses, fractions,
angles, decay times, splitting flags and RNG counts with diagnostics omitted.
Another verifies missing partners can be bypassed only with the veto disabled.

At seed 2600417, 150 attempts with ISR/MPI and pThat 20–100 GeV produce 148
accepted events in both zero-interaction and interacting expanding-medium modes.
The preceding samples accepted 144. All four missing-partner failures (109,
114, 121, 146) are now accepted; the remaining failures are non-QCD MPI pairs.
Both topology audits pass, including unchanged colour junctions and conserved
four-momentum. Their stricter component diagnostics also have no failures.

In the zero-interaction sample, all 144 previously accepted events have exactly
identical shower records, draw counts and hadronization inputs. The first 107
are identical including final hadrons. Later hadrons differ because the newly
accepted events consume draws from the shared hadronizer random stream.
See [identity evidence](validation-unused-angle-identity.json),
[zero-interaction audit](validation-unused-angle-zero-interaction.json) and
[medium audit](validation-unused-angle-medium.json).

Replaying the four recovered zero-interaction states through the frozen Fortran
cascade gives matching headers and draw counts for all four. Three pass strictly;
attempt 146 has 10 scalar mismatches out of 5,971 values, with maximum absolute
difference 6.42e-12. Tolerances remain rel=2e-12, abs=2e-13. This is a recorded
failure, not waived numerical agreement. The replay starts from identical native
initialized states; it does not compare Pythia6/Pythia8 hard generation or
undefined source GETTHETAM behaviour on junction inputs.
[Replay evidence](validation-unused-angle-zero-replay.json).

A separate seed-17 true-vacuum diagnostic produced 147/150 accepted events and
passed topology checks. It cannot be paired by attempt number with the historical
seed-2600417 samples. The matched-seed results above supersede that initial
cross-seed recovery comparison. Full generator and jet-ratio validation remain
incomplete; the historical jet-ratio reports predate this change.

## Interacting-medium replay

All 144 previously accepted interacting-medium events also retain identical
pre-hadronization records; [matched identity evidence](validation-unused-angle-medium-identity.json).
For the four newly accepted events, the independent frozen cascade replay has
one strict pass (114), two scalar mismatches (109, 146), and a nonfinite Fortran
cascade (121). The three completed reference cascades match headers and RNG
counts; the nonfinite reference is not a parity pass. No tolerance, reference
routine, or failure classification was changed to accept these results.
[Full medium replay evidence](validation-unused-angle-medium-replay.json).
