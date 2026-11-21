# First kernel validation

Recorded on 2026-11-20 (America/Los_Angeles), macOS arm64.
AppleClang 21.0.0.21000101; GNU Fortran 16.2.0; Release CMake build.

Both CTest suites passed: core contracts and Fortran differential comparison.
All 1,320 numerical comparisons were identical at the printed 17-digit precision.
All GETT draw counts matched; sampled cases used up to 12 draws.
The machine-readable record is [validation-first-kernels.json](validation-first-kernels.json).
It includes the stock-source hash, routine spans, test-input hash and executable hashes.

These results cover only the nine listed kernels. They do not establish correctness
of a shower, medium model, collision generator, hadronizer or jet observables.
The tolerances are portable numerical acceptance thresholds, not physics tolerances.
Re-run the commands in the README on other compilers and platforms.

The source-freeze utility was exercised on the original stock source and license;
its snapshots are local validation artifacts, excluded from Git. Reference extraction
rejects a changed source hash before compiling any routine.
