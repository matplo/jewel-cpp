# On-shell projection numerical core

Recorded 2026-11-21, AppleClang 21 / GNU Fortran 16.2, macOS arm64 Release.
`project_on_shell` in `include/jewel/projection.hpp` implements the numerical
operations of JEWEL 2.6.0 `PUTPARTICLESONSHELL`. Native-runner integration and
Pythia8 nominal-mass lookup are still pending; the current runner continues to
record `put_on_shell=0`.

## Modes and result contract

- Mode 0 returns the records unchanged.
- Mode 1 keeps each spatial momentum and replaces its energy with
  `sqrt(|p|² + nominal_mass²)`, storing the nominal mass. This generally changes
  total energy; it is not an energy-conserving event adjustment.
- Mode 2 boosts the selected range to its centre-of-momentum frame, performs
  the source massless and massive rescalings, boosts back and applies the final
  individual energy reset. It preserves the source's 1e-6 relative energy
  tolerance, ten-step iteration limit and insufficient-energy path.

Nominal masses are supplied explicitly per particle. The API owns a result copy
and leaves input records untouched. The zero-based `first` index selects the
mode-2 range; modes 0/1 ignore it as in the source. Empty ranges are a defined
no-op extension. Undefined total-momentum/derivative states and nonfinite
results throw explicit errors.

The result includes iteration counts and convergence flags for both rescalings,
and an `insufficient_energy` flag. These are not silently promoted to success.
When the source exhausts ten iterations, its last computed energies and updated
scale are preserved. When nominal masses exceed the available energy, the source
skips the massive rescaling but still performs the final individual energy reset.
Neither path guarantees conservation of the original total energy.

## Reference harness and scope

The build verifies the stock JEWEL source hash and extracts the unchanged routine.
It links unchanged, separately hash-verified Pythia6 `PYP` and `PYROBO` routines.
The harness supplies identical nominal masses to both engines through a test
`PYMASS` lookup. This isolates the projection calculation: it does not validate
Pythia6/Pythia8 particle-data differences.

The stock mode-2 PPJJ branch sets `lstart=0` and then indexes Fortran arrays
from zero. The reference harness avoids that undefined access by selecting its
valid `PPYJ`/`PPZJ` paths, which begin at Fortran indices 1/2. Those correspond to
C++ `first=0/1`. The numerical bodies are unchanged. No claim is made of matching
an out-of-bounds PPJJ memory access. EEJJ/PPDY channel-level bypasses belong to
the future adapter, not this explicitly selected-range numerical API.

## Validation

All **208 cases / 4,600 scalar values pass** at abs=2e-13 / rel=2e-12.
Maximum absolute difference: **4.07149e-14**. Cases cover all three modes,
two range starts, varying multiplicities and momentum scales, massless and
massive nominal targets, a nominal-mass energy shortfall and three near-threshold
massive rescalings. Mode-0 records and excluded prefixes are checked exactly;
mode 1 must preserve spatial momenta exactly. Independent checks verify final
nominal masses and particle mass shells.

Native contracts cover unchanged inputs, an analytic mode-1 energy, convergent
mode-2 energy agreement, insufficient energy, a ten-step nonconverged exit,
invalid modes/ranges/masses, an empty event, undefined lightlike total momentum
and overflow rejection. Both reference-enabled and standalone/Pythia8 builds
pass their projection tests.

[Source/executable hashes and detailed results](validation-projection.json).

```sh
cmake --build build -j4
ctest --test-dir build -R '^projection-' --output-on-failure
ctest --test-dir build-pythia8 -R '^projection-contracts$' --output-on-failure
```

This establishes component parity over the defined tested domain. Wiring the
component after native subtraction, recording nominal-mass provenance and
convergence outcomes, validating complete events and completing the ≥1000-jet
Fortran/native physics comparison remain outstanding.
