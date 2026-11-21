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
reference reports a nonfinite cascade (guard reason 4), with a different record
and draw count. This is not a parity pass. The passing initialization matrix
above must not be read as a full-event agreement claim.
[Failed cascade replay evidence](validation-mpi-photon-medium-replay.json).
