# Expanding medium and table validation

Recorded on 2026-11-20 (America/Los_Angeles), macOS arm64;
AppleClang 21.0.0.21000101 and GNU Fortran 16.2.0, Release builds.

All six reference-enabled CTest suites passed. Both C++-only contract suites
also passed. The original 1,320 kernel comparisons remain passing.
The new suite passed **10,785 scalar comparisons** against extracted,
unchanged JEWEL 2.6.0 Fortran routines. Source hashes and exact routine spans
are recorded in [validation-medium.json](validation-medium.json).

| Comparison | Values | Maximum absolute error |
|---|---:|---:|
| ei-table | 3000 | 0 |
| ei-interpolation | 18 | 0 |
| LOW | 300 | 2.0817e-17 |
| HIGH | 300 | 2.0817e-17 |
| SCAT | 75 | 0 |
| thickness-table | 800 | 0 |
| cross-section-table | 2400 | 0 |
| thickness-boundary | 24 | 0 |
| participants | 160 | 0 |
| medium-sample | 3440 | 2.2204e-16 |

Acceptance uses relative tolerance 2e-12 and absolute tolerance 2e-13.
The largest observed relative difference was 7.39e-16. Exact agreement is
specific to this compiler/platform configuration, not a cross-platform promise.

The tests compare all entries of the 1,000-node Ei table, 100-node thickness
tables and 200-row cross-section tables. Medium configurations include
A=208,16,197; central and displaced collisions; Woods-Saxon and uniform
initial temperature profiles; NF=0,3,5; several initial times and temperatures.
Sample points cover early-time growth, the initial-time transition, cooling,
late-time cutoff, overlap edges and rapidity limits. Ei-dependent kernels cover
NF=0,3,4,5,6, three Lambda values and multiple momentum scales.

The reference probe initializes COMMON data explicitly; it does not run MEDINIT.
DERIV dispatch is a harness implementation limited to the original mode-5
exp(-x)/x expression and mode-101 MEDDERIV call. The selected quadrature,
medium, table and kernel bodies are otherwise extracted without edits.
No temperature or table values from C++ are supplied to the Fortran reference.
Thickness boundary coordinates are read from the Fortran table to exercise
its actual knots, which are separately compared against the C++ table.

C++-only contracts additionally cover scalar quadrature against an analytic
integral in both directions, invalid parameters, invalid/nonfinite positions,
empty medium, light-cone behavior and out-of-table Ei requests. Deliberate
behavior changes outside the stock valid domain are documented in the README;
they are not included in the numerical parity claim.

Medium event/vertex sampling, thermal draws and bounds now have additional
coverage in [sampling validation](validation-sampling.md). Still unimplemented:
full splitting/Sudakov/scattering grids,
shower evolution, Pythia8 import/hadronization, or complete events.
