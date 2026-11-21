# Thermal constituent subtraction kernel

Recorded 2026-11-21, AppleClang 21, macOS arm64 Release. The new
`subtract_thermal_momenta` API in `include/jewel/subtraction.hpp` adapts the
JEWEL 2.6.0 `subtraction.C` helpers to owned C++ vectors. It does not depend on
Pythia, Fortran COMMON blocks or a fixed-size input array. An optional native-runner adapter now uses it; see
[native integration](validation-native-subtraction.md). The [on-shell numerical core](validation-projection.md)
and its [native adapter](validation-native-projection.md) are now validated
separately.

## Preserved numerical behaviour

Particles and thermal centres are represented by transverse momentum, transverse
mass minus transverse momentum, rapidity and azimuth. All particle/thermal pairs
are sorted by rapidity–azimuth distance. Pairs at or below the radius subtract
transverse momentum and transverse-mass excess independently, exhausting the
smaller quantity each time. Azimuth differences wrap across ±pi.

Particles with positive remaining transverse momentum are reconstructed in input
order. The original helper also emits **positive-momentum residual thermal
particles**, assigning pion IDs 211, −211 or 111 with equal probability. The port
preserves this behaviour and explicitly tags these outputs `thermal_residual`.
Their source ID is zero; retained event particles keep their input source ID.
The input record's mass is not used to compute transverse mass; the helper
recomputes it from the supplied four-momentum, as in the original.

The random source is injected. Differential tests use the same seeded C-library
`rand()/RAND_MAX` sequence as the unchanged helper. Production callers can choose
another recorded stream, but that would not reproduce the upstream random ID
sequence. Equal-distance ties use the same platform `qsort` convention as the
source; no cross-platform tie-order guarantee is claimed.

## Reference validation

The harness hashes the externally supplied `subtraction.C` before copying it to
the build directory. It calls the original `readEvent`, `readThermals`,
`buildPairs`, `doSubtraction` and `collectResult` bodies with an explicit zero
offset. Production does not call or link this reference implementation.

All 132 cases / 5,106 scalars pass at abs=2e-13 / rel=2e-12. The maximum absolute
difference is 1.77636e-15. Particle counts and PDG IDs agree exactly. Cases include
several radii, complete/partial consumption, residual thermals, massive inputs,
equal-distance pairs, phi wrapping, adjacent radius values and empty inputs.
Native contracts separately verify exact collinear subtraction, residual tagging,
source IDs, unchanged inputs, draw consumption and invalid-argument rejection.

The source forms `&vector[0]` for an empty distance vector. Both the typed port
and reference driver avoid invoking that undefined access when no pairs exist;
all nonempty helper calls use unchanged bodies. Non-forward/lightlike inputs
are skipped with counters rather than writing diagnostics to standard output.
Nonfinite inputs and invalid radii throw explicit errors. The default
`Reference` mass policy rejects non-real mass results. The opt-in `SignedMass`
policy records negative invariant squares as `-sqrt(-m²)` and counts those
fields without changing any four-momentum; this represents cases where the
original helper emits NaN mass metadata. It is not on-shell projection.

[Source/executable hashes and numerical results](validation-subtraction.json).

## Upstream wrapper issues and remaining integration

The Fortran-facing wrapper uses `strcmp` as a boolean equality test, and its
`PPZJ || PPWJ` condition is always true. It consequently sets the starting offset
to two for every process. `readEvent` applies that offset, and `collectResult`
applies it a second time to the already filtered vector. Its `char*` process
argument additionally assumes termination when comparing strings.

This API deliberately accepts **already selected particles** and does not
reproduce those process-offset operations. That is an explicit boundary change,
not a claim of equivalence to the full `DOSUBTRACTION` wrapper. The reference
comparison establishes the actual subtraction kernel at offset zero. The native
adapter now supplies final-particle selection, thermal-ledger mapping, recorded
random-stream policy, output provenance and analysis routing, with integration tests.
The separate `PUTONSHELL` modes and full event-level physics comparisons also
remain outstanding.

```sh
cmake --build build -j4
ctest --test-dir build -R '^subtraction-' --output-on-failure
```
