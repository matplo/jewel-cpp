# Inactive massive nonpartons in MPI pairs

Native MPI initialization now retains inactive, colourless nonpartons with PDG
absolute value greater than 21. The original generator mass is retained in the
pair-rest-frame energy and momentum reconstruction, exactly as FINDVIRTUALITIES
uses retained masses for these legs. They remain status 4 and never enter the
parton cascade scheduler. Colourless nonpartons reach Pythia8 hadronization and
decay handling with their identities, masses and reconstructed four-momenta.
Primary non-QCD production channels are not implemented by this change.

## Explicit time convention

In stock JEWEL, a positive retained mass enters the formation-time branch even
though inactive status forces sampled virtuality Q=0. The expression produces
positive infinity after consuming a random draw. The native state requires
finite values, and this shower time is unused for status-4 records.

The opt-in `inactive_nonparton_lifetime` convention consumes the same source draw
and stores the medium lifetime as a finite unused placeholder. It does not set
or approximate the physical decay lifetime of a hadron. The native outgoing-
flavour adapter enables it; the strict core/source adapter defaults retain the
previous rejection. It applies only to inactive colourless MPI nonpartons;
active or coloured unsupported hard legs still fail. Run headers record
`inactive_nonparton_time_policy` explicitly. Massless photon behavior is unchanged.

## Independent initialization comparison

All 1,440 retained-mass cases pass the finite-field comparison against unchanged
JEWEL 2.6.0 FINDVIRTUALITIES: 227,232 values at rel=2e-12, abs=2e-13, maximum
absolute difference 1.81899e-12. The matrix covers PDG 10441, 445 and 20443 on
either leg, a two-nonparton pair, boosts, scales, MPI-FSR settings and sampling
channels. Both nonparton masses are nonzero. RNG counts agree, including 170
retry cases (maximum ten trials).

The 1,728 inactive-time fields are checked separately: the unchanged source must
return positive infinity and C++ must return the configured finite placeholder.
They are not included in the numerical-parity count and are not silently ignored.
The report identifies the convention and hashes both input forms (native
MPIPASSIVE versus source MPIINIT); reference routine bodies are unmodified.
[Machine-readable comparison](validation-mpi-passive-initialization.json).

Existing primary, quark/gluon MPI and photon differential suites pass. Updated
core contracts check retained mass, preserved source RNG consumption, finite
placeholder time, total pair energy and default-policy rejection. Native subsystem
and experimental ISR/MPI integration contracts pass.

## Generated medium events

The seed-2600417 run with PPJJ ISR/MPI and pThat 20–100 GeV now accepts all 220
attempts, compared with 218 previously. Quarkonium attempts 66 and 115 are newly
accepted, with generator masses retained. All 218 previously accepted events
have exactly unchanged pre-hadronization records. Later final hadrons can differ
because recovering events advances the shared hadronizer random stream.
[Identity and mass evidence](validation-mpi-passive-identity.json).

All 220 accepted events pass the native topology and momentum audit, including
78 junction events. Maximum momentum-component residual is 6.13027e-8 GeV;
the stricter component diagnostic also has no failures.
[Full sample audit](validation-mpi-passive-medium.json).

Both recovered initialized states complete through the frozen Fortran cascade
with matching full headers and RNG counts. Strict scalar comparison still fails:
44 of 13,821 values for attempt 66 (maximum absolute difference 8.57492e-12),
and 14 of 25,862 values for attempt 115 (maximum 1.63465e-11). These replays use
the same already initialized finite inactive records in both engines; they do
not establish full stock-generator equivalence or source lifetime parity.
[Replay evidence](validation-mpi-passive-replay.json).

The sample demonstrates recovery of these two events, not universal event
acceptance. ISR frame failures, first-angle junction treatment when enabled,
other production channels, full-cascade discrepancies, and final jet/substructure
ratio validation remain open. Numerical tolerances and jet acceptance gates are
unchanged.
