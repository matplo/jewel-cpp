# ISR azimuth precision diagnostic

Recorded 2026-11-21. This extends the isolated Pythia 8.317 source tracer,
not the production adapter. It records the sampled azimuth, polar rotation,
rest-frame sister momentum, initial rest boost, and final rest-to-lab matrix.
The installed Pythia source and libraries remain unchanged.

Across seeds 2600417 and 2600418, all 2,000 instrumented events reproduce the
adapter's complete particle lists exactly (PDG, status and four-momentum).
All 26,962 accepted ISR branches have unique matching trace records.

Inferring the sampled azimuth from the stored emission's transverse momentum
loses precision when large intermediate products cancel. The maximum observed
angle discrepancy is 2.44522e-10 radians. Angles are compared modulo pi because
the pre-azimuth transverse component can have either sign.

## Factor-based error estimate

Let B be the recorded x-z boost, v the rest-frame sister momentum, and theta
and phi the recorded rotation angles. Since v_y=0 and B does not mix y, the
ideal transverse components are proportional to (cos(phi), sin(phi)). The
common amplitude uses only the x and z rows of B. The final longitudinal boost
does not change these transverse rows.

With u=2^-53 and gamma7=7u/(1-7u), define

```
S = |cos(theta)| sum_i |B_xi v_i| + |sin(theta)| sum_i |B_zi v_i|
E_x = [u + gamma7(1+u)] |cos(phi)| S + gamma7 sum_i |M_xi v_i|
E_y = [u + gamma7(1+u)] |sin(phi)| S + gamma7 sum_i |M_yi v_i|
angle estimate = asin(min(1, hypot(E_x,E_y)/pT)) + 8u*pi
```

The terms account for rotation-coefficient products, matrix formation, and
matrix-vector evaluation. Treating B and theta as common fixed inputs is
appropriate for this azimuth relation, but does not bound their error in a
complete Lorentz transformation. The final allowance covers ordinary libm
angle/trigonometric rounding; this is a diagnostic estimate under binary64
arithmetic assumptions, not a certified cross-platform interval bound.

All 26,962 angle errors fall within this estimate. Maximum error/estimate is
0.13484 for seed 2600417 and 0.18599 for seed 2600418. Including only the final
matrix-vector dot-product error was insufficient; matrix-formation error must
also be included. [Results and input hashes](validation-isr-azimuth.json).

This explains why stored emission momenta cannot recover the sampled angle to
machine precision. It does **not** establish the accuracy of complete ISR recoil
composition, and does not justify widening its acceptance gate. The existing
three rejected reconstructed frames and 31 failed composed-parton checks remain
unresolved. No production tolerance or physical ISR timing was changed.

## Reproduction

Build the isolated tracer using `tools/build_pythia_isr_trace.py` and run its
probe with the settings from each adapter run header. Then run:

```sh
python3 tools/audit_pythia_isr_azimuth.py result.json \
  source-2600417.jsonl adapter-2600417.jsonl \
  source-2600418.jsonl adapter-2600418.jsonl
```

The audit checks exact particle reproduction and complete branch coverage before
calculating errors; mismatches fail the run. The retained build provenance is
in [the diagnostic manifest](validation-isr-azimuth-build.json).
