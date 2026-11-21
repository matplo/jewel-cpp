# Actual Pythia8 ISR ancestry

`map_pythia_isr_ancestry` now follows both hard incoming legs back to their
beams. Status-41 incoming radiators identify the emitted status-43 sibling;
status-42 recoil copies and status-61 primordial-momentum copies remain explicit
in the backwards line list. Every edge must be reciprocal. The mapper rejects
cycles/shared incoming lines, unsupported statuses, invalid copy flavours,
missing siblings, common beams or emissions not accounted for by the two chains.
The hard-input CLI exports these associations as `isr_chains`.

## Why a direct timing connection is not yet correct

In these Pythia8 samples the stored incoming bookkeeping particles are massless.
Their record masses cannot simply replace the spacelike masses used by JEWEL's
Fortran formation-time calculation. For each branching, the mapper reconstructs
`q = p_mother - p_emitted` and `Q2 = -q^2`, in that branching's recorded recoil
frame, retaining both source IDs and the original four-vectors.

This reconstruction is consistent with the mother/sister construction in
Pythia8 8.317 SimpleSpaceShower::branch. It must not be mistaken for the stored
on-shell daughter momentum: recoil copies represent different stages of the
shower. The output deliberately does not combine stages into a timing chain.

A spacelike vector can have either energy sign depending on the frame. **751
of the 11943 reconstructed branches have negative energy** in their recorded
frames. These signs are retained; neither absolute-value replacement nor an
energy clamp is applied. The timing API now preserves the source algebra for signed energies, but that
does not make these raw stage-local vectors a validated physical timing input. A common frame and coordinate/recoil transport mapping
are still needed, followed by non-hard-parton virtuality initialization and
medium evolution. `jewel-run-events` continues to reject ISR snapshots.

## Verification

Three seeds, 1000 events each, reproduce every field in the prior snapshots
exactly except for the new `isr_chains` metadata:

| Seed | Incoming chains | Mapped branches | Negative-energy branches |
|---|---:|---:|---:|
| 2600317 | 2000 | 3935 | 270 |
| 2600318 | 2000 | 4018 | 246 |
| 2600319 | 2000 | 3990 | 235 |

All roots in the existing ISR emission ledger occur exactly once in the mapped
chains. Independent Python subtraction reproduces all four reconstructed
components exactly. An 80-digit Decimal calculation from the same rounded
components checks Q2, bounded by 32 ULP of max(1, sum of squared components).
The maximum observed difference is 2.1391e-10 GeV^2, consistent with cancellation
in boosted vectors. All reconstructed Q2 values are positive in this sample.

For 9680 massless/light-flavour branchings, an additional diagnostic compares
Q2 with pT_evol^2/(1-z), using the recorded emission scale and daughter/mother
energy ratio. Its largest absolute residual is 1.7588e-8 GeV^2. This residual
is reported, not presented as a strict Fortran-parity gate. Heavy-flavour
threshold branches need a mass correction in Pythia8 (including its special
near-threshold evolution), so this diagnostic does not apply that massless
formula to them.

The input contracts check chain coverage, beam identity, corrupted reciprocal
ancestry and retention of negative spacelike energies. Five related suites pass:
Pythia8 input, ISR CLI, hadronization boundary, shower output and ISR timing.
[Machine-readable evidence](validation-isr-ancestry.json) includes sample,
executable, mapping-source and inspected Pythia source hashes.

```sh
build-pythia8/jewel-pythia8-input chains.jsonl 1000 2600317 --isr on
python tests/isr_ancestry_audit.py chains.jsonl prior-snapshots.jsonl audit.json
ctest --test-dir build-pythia8 -R '^(pythia8-(input|isr-input|boundary|shower-output)|isr-times)' --output-on-failure
```

This advances the Pythia8-to-JEWEL mapping. It does not establish equivalent
Pythia6/Pythia8 ISR physics, completed ISR transport, or the >=1000-selected-jet
Fortran/native ratio requirement.
