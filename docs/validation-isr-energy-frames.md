# Signed energies after recoil transport

`tools/audit_isr_energy_frames.py` transports each reconstructed spacelike
mother-minus-emission vector through the later recorded ISR recoil matrices
and the primordial matrix. It preserves all momentum components and the
original recorded virtuality. This is a diagnostic of a proposed common-frame
representation, not a physical production-time assignment.

| Seed | Branches | Negative energy at branching | Negative energy in final frame | Sign changes |
|---|---:|---:|---:|---:|
| 2600317 | 3935 | 270 | 250 | 40 |
| 2600318 | 4018 | 246 | 237 | 37 |
| 2600319 | 3990 | 235 | 220 | 27 |

Across 11943 branches, **707 still have negative energy in the final frame**.
A frame change alone therefore does not resolve the sign issue. It would be
incorrect to replace these energies with absolute values merely to obtain
positive formation rates.

Recomputed virtuality has a maximum absolute residual of 1.2085e-6 GeV^2 after
successive transforms; the largest residual divided by max(1 GeV^2, Q2) is
6.4667e-8. Both are reported diagnostics, not a new pass/fail tolerance or a
change to the frozen-Fortran comparison. Boosted subtraction can amplify
floating-point cancellation. Neither the vectors nor their stored virtualities
are adjusted to force invariant agreement.

[Machine-readable results](validation-isr-energy-frames.json) include source
sample and audit hashes. The corresponding positive/signed-energy timing
matrix now passes 1152 cases against the unchanged Fortran block; see
[the timing validation](validation-isr-times.md). Negative rates reverse time
intervals in that source expression. Formal numerical parity does not settle
how reconstructed Pythia8 spacelike histories should be assigned physical
JEWEL production times.

```sh
python tools/audit_isr_energy_frames.py primordial.jsonl --output energy-audit.json
```

The next integration must reconcile this history/energy representation with
JEWEL's virtuality and formation-time assumptions. The event runner remains
ISR-off, and the full matched Fortran/native jet comparison is still incomplete.
