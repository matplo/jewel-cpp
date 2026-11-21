# Bounded ISR reconstruction recovery

The source-matrix diagnostic showed that recovering ISR inputs from particle
output loses precision in highly asymmetric branches. The production adapter
now preserves the original reconstruction whenever it passes. Only after it
fails, for mapped light/massless initial-initial branches with captured beam
fractions, it tries a bounded fallback:

1. Recover the azimuth from the largest transverse recoil displacement.
2. Consider the squared stored scale and its two adjacent representable values
   in each direction. A candidate is allowed only when its square root equals
   the stored scale exactly.
3. Reconstruct the source-prescription matrix and require every recoil copy to
   pass the original `2e-9 + 2e-11 * abs(component)` check. If none passes, retain
   the original rejection. No frame is accepted merely because it is closer.

Recovered hard-input frames record `source_rounding_recovery`, including the
azimuth method and squared scale. No changes are made to the generated event,
random numbers, JEWEL shower kernels or tolerances. The candidates represent
floating-point round-trip ambiguity, not adjustable physics parameters.

## Adapter verification

All 1500 translated-coupling input events now map, with exactly three recovered
frames at attempts 385, 1089 and 1355. The other **1497 event records are exactly
unchanged**. The recovered squared scales round back to the stored values.
Against independently captured Pythia source matrices, maximum entry differences
are 2.32e-10, 2.96e-12 and 3.06e-11 respectively. This strongly reduces the
previous differences without requiring bitwise equality of matrices.

The audit checks 11168 ISR frames, 33958 recoil copies and 22334 composed final
partons. Recoil-copy and norm-scaled Lorentz-metric checks pass. **The complete
composition audit still fails:** four residuals at attempts 72 and 302 are
identical to the baseline, with maximum 8.58e-7 GeV. No new composition failures
appear in the recovered events. These unresolved residuals are saved explicitly.

The 1500-event C++ recovery contract passes and rejects all three independently
corrupted recoil-copy fixtures. ISR CLI, ISR-time, experimental ISR/MPI and
coupling CLI regressions pass. The old CLI expectation that a particular sample
must contain rejected frames was updated to require observable recovery metadata.

```sh
cmake --build build-pythia8 -j 4
build-pythia8/isr-recovery-test CT14nlo/0
ctest --test-dir build-pythia8 --output-on-failure \
  -R '^(pythia8-isr-input-cli|isr-times-contracts|experimental-isr-mpi-contracts|input-coupling-cli)$'
```

The recovery test is also registered with CTest when JEWEL_TEST_EXTERNAL_PDF
is CT14nlo/0, the installed set/member used by its numerical fixture.

## Complete vacuum rerun and jets

The same seed 2600421 and translated-coupling setup now accepts **1500/1500**
attempts. Together with octet-onium support, this recovers attempts 385, 511,
1089 and 1355. All 1496 previously accepted pre-hadronization event records are
exactly unchanged. Later hadrons can change because the recovered events consume
the continuous hadronizer random stream.

The common hepyy/cppyy analysis finds **2070 native jets and 1357 Fortran jets**.
Cuts, bins, weights and attempted-event normalization are unchanged.

| Jet pT (GeV) | Native / Fortran |
|---|---:|
| 10–15 | 1.134 |
| 15–20 | 0.944 |
| 20–30 | 0.997 |
| 30–40 | 0.922 |
| 40–60 | 1.174 |
| 60–80 | 0.778 |

All six observable gates remain not passed. Topology and energy-scaled momentum
checks pass; eight stricter component diagnostic excursions are retained,
maximum 2.55e-7 GeV. These are not full-generator parity claims. MPI context,
shower-scale differences, baseline composition residuals and the other feature
and physics gaps remain open.

- [Interactive comparison with historical pairs](isr-recovered-vacuum-comparison.html)
- [Recovery and source-matrix evidence](validation-isr-recovery.json)
- [Full frame/composition audit](validation-isr-recovery-frames.json)
- [Baseline frame/composition audit](validation-isr-recovery-prior-frames.json)
- [Topology audit](validation-isr-recovery-topology.json)
- [Pre-hadronization identity](validation-isr-recovery-identity.json)
