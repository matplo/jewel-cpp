# Branch construction validation

All 21 reference-enabled tests and all eight C++-only contract tests pass.

MAKESPLITTING is extracted unchanged from hash-verified JEWEL 2.6.0 and compared
with C++ using identical initial event records and finite random tapes. Reference
GETMASS, REJECTSPLITTING, medium fields and Pythia6 kinematics are original bodies;
no C++ branch values are supplied to the Fortran calculation.

All 155 cases pass: 150 construct daughter pairs, two take angular-rejection
repair paths, two discard the event and one returns without splitting. The
comparison covers 20,202 record scalars plus particle counts, colour allocation,
discard flags and draw counts. All discrete results and random-draw counts match
exactly (up to 76 draws). Maximum absolute difference is 6.3949e-13 and maximum
relative difference is 1.7346e-11. Each value satisfies the combined tolerance
`abs(error) <= max(2e-13, 2e-12 * max(abs(C++), abs(reference)))`; near-zero differences can
therefore have a relative error above 2e-12. Tolerances were not relaxed.

Platform: macOS arm64, AppleClang 21.0.0 and GNU Fortran 16.2.0, Release.
Cases cover ANGORD=0–5; quark, antiquark, gluon and gluon-to-quark-pair branches;
fraction 0.05/0.4/0.95; active/cold medium locations; aligned/rotated parent
momenta; status 1/2 parents; and formation factors 1/2. Six second-generation
cases use first-generation Fortran records as frozen common inputs to both
implementations, one for each angular-ordering mode. These are local history
checks, not a self-running shower. Two failure cases cover zero fraction and
kinematically impossible splitting; the reference's early-return record state
is compared rather than ignored.

Independent checks verify four-momentum conservation, daughter mass shells and
net colour connections for every created pair. All event records also pass
C++ genealogy validation. C++ contracts cover the no-draw on-shell return,
zero-fraction discard and invalid formation scale.

MAKESPLITTING only assigns the daughters' production time, not spatial production
points; the latter are pending caller work. Fresh C++ records initialize pending
fields to zero, and the reference driver explicitly initializes unused slots to
zero to make this comparison defined. This does not validate MAKEBRANCH's later
production-point assignment. Record colours, momentum, genealogy, angles,
fractions, flags, auxiliary coordinates and decay/evolution times are compared.

[Detailed results and source/executable hashes](validation-splitting.json).
Full transport, recoil/colour evolution, Pythia8 integration and jet-observable
agreement remain unvalidated.
