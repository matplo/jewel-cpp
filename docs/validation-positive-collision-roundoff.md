# Positive collision roundoff and remaining event differences

Recorded 2026-11-21 after the accepted- and rejected-splitting arithmetic fixes.
The production physics and compiler settings are unchanged in this update.

## Event 9: identical branch scheduling, sensitive momenta

Both engines complete the preserved event 9 with 192 records, 64 centres and
20090 random draws. All 85 traced branch entries align by scheduling slot,
record count, RNG cursor and discard flag; there are no unmatched entries or
discrete trace differences. This trace does not prove equality of every record
inside each branch. The strict final comparison still has 208 scalar mismatches,
maximum absolute difference 1.13580e-8.

The extended diagnostic records all reshuffled collision momentum squares,
including positive values. It captures 64 entries in each engine. At the ninth
entry, both use the fallback with zero recoil squared momentum. In exact real
arithmetic its `e4 = sqrt(m42)` would give `e4² − m42 = 0`. With rounded inputs,
the fused subtraction gives:

| Quantity | C++ | Fortran |
|---|---:|---:|
| squared momentum, GeV² | 5.048207908552128e-18 | 3.244795232184443e-17 |
| square root, GeV | 2.246821734929616e-9 | 5.696310413051981e-9 |

An 80-digit Decimal calculation from the exact binary floating-point operands
reproduces each residual when rounded to double. Small input differences are
amplified by the square root near zero. The subsequent collision is the first
recorded collision whose scalar operands exceed the usual tolerance. At branch
entry 21, slot 49, cursor 2786, the selected particle differs at roughly 1e-8.
This evidence supports rounding amplification, rather than a divergent branch
choice, as the source of the remaining discrepancy in this event. It does not
establish global equivalence of the generators.

## Event 14: negative residuals in both engines

The default negative-only diagnostic remains operational. Event 14 produces
squared momenta −1.8895704699223268e-17 (C++) and −3.5723856597552886e-17
(Fortran) at the same fallback. Both cascades fail; their terminal draw counts
differ. No clamp was introduced, no failed event was counted as a pass, and the
reference routines were not modified to conceal the failure.

## Reproducibility and limits

Both diagnostic modes build temporary source copies and compare their entire
stdout byte for byte against the uninstrumented engine. That assertion passes
for both engines and both events. Diagnostic writes only add stderr records.
The original negative-only invocation remains supported:

```sh
python3 tools/diagnose_collision_roundoff.py build docs/angular-mode1-regression/9.input /tmp/jewel-event9-diagnostic --all-collisions
python3 tools/diagnose_collision_roundoff.py build docs/angular-mode1-regression/14.input /tmp/jewel-event14-diagnostic
```

Output directories must be new. [Recorded operands, trace comparison, compiler
commands and source/executable hashes](validation-positive-collision-roundoff.json)
provide the evidence. These events remain failed strict comparisons. The required
≥1000-jet spectra/substructure ratios, full feature coverage and final dynamic
report remain outstanding; neither exact branch scheduling nor tiny component
errors substitute for those requirements.
