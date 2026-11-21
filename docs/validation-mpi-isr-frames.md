# Subsystem ISR recoil: implemented, validation incomplete

Latest update: [public-hook source fractions](validation-isr-fractions.md) reduce
the failures below to 3 rejected events and 31 composed partons. The original
results in this document are retained as the preceding baseline.

Recorded 2026-11-21, Pythia8 8.317. The adapter now reconstructs chronological
ISR recoil matrices per subsystem using the existing SimpleSpaceShower
source-prescription formula and captured pre-beam-energy-repair momenta.
This is an experimental mapping: **the composed-transport audit fails**.
It must not yet supply production-point or formation-time transport.

`reconstruct_subsystem_isr_frames` uses the validated subsystem ancestry,
selects the opposite incoming recoil chain, and predicts every status-44
copy. No recoil-copy momenta are used to fit a matrix. It supports ordinary
massless initial-initial recoil only. Existing single-system behavior is retained.
Export adds `subsystem_isr_frames` and `subsystem_isr_frame_error`, preserving
all raw events. If a reconstruction fails, the frame array is null and the
error is explicit. The original ancestry and primordial results remain available.

## Evidence and failures

Four 1,000-event samples use seeds 2600417/2600418 and ISR off/on with MPI on,
pp at 5020 GeV and pThat 20–100 GeV. All pre-existing event fields and generation
statistics are exactly preserved relative to the preceding ancestry snapshots.

| Seed | ISR | Events with individual maps | ISR frames | Checked ISR copies | Composed partons | Failed composed partons |
|---|---|---:|---:|---:|---:|---:|
| 2600417 | off | 1000 / 1000 | 0 | 0 | 15,144 | 0 |
| 2600417 | on | 995 / 1000 | 13,176 | 40,996 | 26,626 | 65 |
| 2600418 | off | 1000 / 1000 | 0 | 0 | 14,328 | 0 |
| 2600418 | on | 993 / 1000 | 13,622 | 42,470 | 27,508 | 42 |

Twelve ISR-on events cannot provide complete individual maps: ten exceed the
copy residual gate and two fail the massless-collinear input check. One traced
collinearity failure has a pre-repair energy differing from |pz| by about
1.33e-10 GeV at energy 0.969 GeV; the source of that deviation is still under
investigation. Individual component tolerance remains 2e-9 GeV + 2e-11 |p|.
No tolerance was widened. Failure details identify event, emission and copied
records where available.

The independent audit follows every original scattering/emission parton through
its chronological ISR copies and the subsystem primordial transform to its final
pre-repair momentum. Within events with complete individual maps, **107 partons
in 61 events fail** the existing composed gate of 1e-8 GeV + 1e-10 |p|.
Maximum discrepancy is 2.5281e-5 GeV, seed 2600417, attempt 785, subsystem 3,
root 76, final record 106. The audit writes all failed comparisons and exits
nonzero. It does not turn a composition failure into a passing component result.
This is additional to the twelve events rejected during reconstruction.

On accepted individual maps, the largest independently computed copy residual
is 1.30e-8 GeV, within the combined absolute/relative gate. The Lorentz-metric
check uses 1e-7 * max(1, ||M||_inf²); the maximum absolute metric residual is
4.19e-7. These observations do not establish sufficient numerical stability
for arbitrary spacetime displacements.

[Complete results, failed-event records and hashes](validation-mpi-isr-frames.json).

Four native contract/CLI/boundary/output suites pass. Those tests verify supported
small cases, corruption rejection and retention of unsupported mappings, **not**
full large-sample transport acceptance. A 300-event CLI test requires mapping
failures to remain represented alongside successes.

Next work must trace the failed source kinematics and accumulated errors without
fitting away discrepancies or relaxing reference tolerances. Physical timing,
MPI evolution and final jet comparisons remain unimplemented integration gates.
No Fortran physics routine or its acceptance policy changed.
