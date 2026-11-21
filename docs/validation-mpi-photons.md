# Inactive photons in MPI subsystem initialization

The primary PPJJ interface still requires quarks/gluons. An MPI pair may now
also contain a photon (PDG 22) provided it is inactive (status 4), exactly
massless, and colourless. The photon retains its status, identity, zero mass,
and colour tags. Both pair momenta are reconstructed together when its coloured
partner receives a shower mass; four-momentum is not discarded by skipping a leg.

This follows the finite massless branch of frozen JEWEL FINDVIRTUALITIES:
status 4 forces QMAX=0, GETMASS returns zero virtuality, a nonparton's existing
mass is retained (zero here), and a zero-mass leg receives the medium lifetime.
The resulting inactive photon does not shower or scatter, and passes through
native hadronization. A placeholder sampling channel does not affect zero-QMAX
sampling; both channel choices are checked against the source.

Massive nonpartons remain explicitly rejected. In the source, an inactive
massive nonparton can retain positive mass while its sampled Q is zero; the
formation-time expression then divides by zero. Supporting these records needs
an explicit inactive-lifetime treatment and separate validation. This change
does not set massive quarkonium to zero mass and does not claim V+jet support.

## Independent reference and contract checks

The new MPI photon matrix compares 1,440 cases / 228,960 values against unchanged
FINDVIRTUALITIES and GETMASS routines. All pass with rel=2e-12, abs=2e-13; maximum
absolute difference is 1.36424e-12. Cases include either photon leg, two photons,
quark/gluon partners, three boosts, six primary scales, both MPI-FSR settings,
first-angle switches and both inherited sampling-channel choices. There are 171
retried cases (up to ten trials). Complete records and RNG counts are compared.
[Reference evidence](validation-mpi-photon-initialization.json).

Existing primary and MPI differential tests pass. Core and native subsystem
contracts and the experimental ISR/MPI integration test pass. New contracts
check that an inactive photon remains massless with the prescribed lifetime,
and reject an active, massive, or coloured photon and a primary photon before
random draws. No reference routine or numerical tolerance was changed.

## Generated events

All 148 previously accepted events in the seed-2600417 150-attempt medium sample
remain identical, including final hadrons. The two failures in this range are
PDG 10441 quarkonium, not photons.
[Identity evidence](validation-mpi-photon-unchanged.json).

Extending the same run to 220 attempts accepts 218, with only those two massive
nonparton failures. Attempt 217, previously rejected because its MPI pair contains
a photon, now completes. Its photon is retained exactly in the final hadronic
event. All accepted events pass native topology, colour and momentum-conservation
checks; the stricter component diagnostic has no failures.
[Photon record](validation-mpi-photon-event.json) and
[full sample audit](validation-mpi-photon-medium.json).

These are component and boundary checks. Full generator equivalence, unsupported
massive nonpartons, and the final jet/substructure ratio requirement remain open.

## Full-cascade replay limitation

The initialized state of attempt 217 was also run through the native and frozen
Fortran cascade probes. The native cascade completes; the bounded Fortran
reference reports a nonfinite cascade (guard reason 1 after 29 branch calls), with a different record
and draw count. This is not a parity pass. The passing initialization matrix
above must not be read as a full-event agreement claim.
[Failed cascade replay evidence](validation-mpi-photon-medium-replay.json).

## First nonfinite operation identified

The first 29 branch visits agree in selected-particle state within the unchanged
comparison tolerance, and exactly in visit order, record counts and RNG cursor.
This boundary trace is not a comparison of all state inside each branch.
[Trace summary](validation-mpi-photon-branch-prefix.json).

At branch 29, DOKINEMATICS takes the square root of p32=-8.456990869161364e-18
GeV^2 after setting p42=0 and E4=sqrt(m42), where m42=0.2379884093376736 GeV^2.
Thus p32=E4^2-m42 is algebraically zero; floating-point evaluation gives a tiny
negative value. The existing native zero-boundary handling already restores
zero only in this explicit fallback. This is the previously documented collision
roundoff case, not an active photon scattering. Diagnostic writes reproduce
identical stdout to the uninstrumented engines.
[Instrumented evidence](validation-mpi-photon-roundoff.json).

A separate temporary Fortran copy was modified to apply exactly that zero-boundary
correction. The frozen reference and production implementations were not changed.
This modified diagnostic completes and agrees with C++ on the full output header,
including 639 records, 217 stored scattering centres and 628,994 random draws.
It still has 28 strict scalar mismatches among 21,453 values, with maximum absolute
difference 6.86384e-12. It is neither a stock-reference parity pass nor a strict
modified-reference pass. [Explicit patch and result](validation-mpi-photon-modified-reference.json).

Replay reports now parse GUARD_STATS into structured reason and branch-call
fields. The final output-header field is a coherent-history index, not a guard
reason; the earlier label of reason 4 was corrected to reason 1 after 29 calls.
No numerical tolerance or acceptance gate was relaxed.

Subsequent work adds [inactive massive MPI nonpartons](validation-mpi-passive.md)
under an explicit finite-time convention; the photon results above remain historical.
