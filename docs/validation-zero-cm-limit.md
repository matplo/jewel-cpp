# Finite zero-momentum collision fallback

Medium attempt 11 was traced after correcting imported fractions. Both native
and Fortran execution enter branch 59 with matching scheduling and RNG cursor
57,676. A subsequent DOKINEMATICS collision fallback explicitly sets recoil
CM momentum squared to zero and recoil energy to `sqrt(m42)`, then recomputes
the outgoing momentum squared as `e4*e4-m42`.

For this event the values are:

```
m42 = 1.2781802594653007 GeV^2
e4  = 1.1305663445659881 GeV
p42 = 0 GeV^2
p32 = -3.7703347712061184e-17 GeV^2
```

The exact algebraic value of p32 is zero in this branch. Rounding of sqrt,
followed by fused subtraction, instead makes it slightly negative. Taking its
square root contaminates the outgoing momenta. The native implementation stops
at draw 58,078; Fortran propagates the NaNs into later collisions and stops
under the harness guard at draw 58,083.

## Numerical policy

Only inside this explicit fallback, when `p42 == 0` and computed `p32 < 0`, C++
sets p32 to its algebraic value zero. No general negative-momentum clamp, new
random draw or tolerance change is introduced. Other branches and positive
results retain the source arithmetic. This is a deliberate numerical extension
at a source-nonfinite limit, not a claim to reproduce Fortran NaNs.

`JEWEL_COLLISION_TRACE=1` enables diagnostic stderr markers with the original
negative residue, squared mass and energy whenever this correction is used.
It also reports any negative squared momenta still reaching the final rescale.

## Evidence and regression scope

All 381 finite one-transfer reference cases retain their original comparisons.
Three previously nonfinite reference cases now produce finite native results;
they are recorded separately, not counted as parity passes. Independent checks
require unchanged RNG counts, matching outgoing velocities (zero relative CM
momentum), on-shell outgoing masses and four-momentum conservation to 1e-10.

Sequence tests distinguish repaired nonfinite physical records from the existing
dummy-mass cancellation handling. Recovered cases require a recorded correction
with residue bounded by `4*epsilon*m42` and finite native outputs. Finite-source
sequence comparisons retain their previous tolerances.

The same 20-event experimental ISR+MPI medium sample now completes **20/20**,
compared with 16/20 before this correction and 2/20 before the imported-fraction
fix. Accepted weight and generator weight sum are both 20; no failed event is
silently removed. The full-pipeline regression now covers all 20 attempts.
This small sample does not establish final-jet agreement or general stability.

[Machine-readable evidence](validation-zero-cm-limit.json) includes the measured
failure, source-nonfinite recoveries, finite-reference results and sample ledger.
Remaining junction, non-QCD, frame, numerical-parity and jet-validation work
continues under the full project objective.
