# Mapped Pythia8 ISR timing inputs

`prepare_subsystem_isr_timing` connects the subsystem ancestry/recoil mapping
to the JEWEL timing interface. It constructs two backward timing chains per
Pythia subsystem and retains the stable final emission IDs for later attachment
to native records. This is a candidate common-final-frame prescription, not
proof that Pythia6 and Pythia8 encode equivalent shower histories.

For each branch, the reconstructed spacelike daughter is transported through
later recoil frames of its subsystem and then its primordial frame. Its signed
energy is retained; recorded mass is `-sqrt(Q2)` from the original branch.
The next backward branch supplies the incoming kinematics of each timing step;
the last incoming line receives the beam sentinel. Emitted lines use final
Pythia momenta and recorded masses. Nominal masses come from the input engine's
`particleData.m0`, exposed through `Pythia8Input::nominal_mass`, without RNG calls.
Empty chains retain no emitted timing records and preserve the hard-line draw
through `assign_isr_emission_times`.

Before accepting a chain, the mapped emission momentum is transported to its
final pre-energy-repair copy and checked with the existing composition gate
`abs(error) <= 1e-8 GeV + 1e-10 * abs(component)`. Individual frame reconstruction
also retains its existing checks. No tolerance was relaxed. This emission check
is not a replacement for the outstanding hard-outgoing-parton composition audit.

## Generated inputs and Fortran replay

Two seeds, 2600417 and 2600418, generate 1,000 attempts each with Pythia8 8.317:
pp at 5020 GeV, pThat 20–100 GeV, ISR and MPI enabled, FSR and hadronization off,
and the input adapter's existing tune/PDF settings. The timing probe uses
x=1.2 fm, y=-0.7 fm, lifetime=20 fm, formation factor=1, scattering enabled and
a fixed random tape of 0.5. It does not run JEWEL evolution or model a medium.

| Seed | Mapped events | Replayed subsystems | Emissions | Timing values | Negative incoming energies |
|---|---:|---:|---:|---:|---:|
| 2600417 | 999 | 6,764 | 13,249 | 66,245 | 1,362 |
| 2600418 | 998 | 6,981 | 13,683 | 68,415 | 1,357 |

All **134,660 emitted time/position values agree exactly** with the unchanged
Fortran timing block on this platform. All 13,745 random-count comparisons
agree. The audit also compares the probe's direct timing output with the C++
replay and checks all emitted stable IDs against captured Pythia8 histories.

Independent reconstruction from those captured snapshots checks 488,124 input
values. Its maximum difference is 2.76566e-9 GeV, under the explicit absolute
1e-8 / relative 1e-10 mapping check; this uses Python `fsum` rather than the C++
matrix accumulation order. It is a kinematic consistency check, separate from
the stricter unchanged timing comparison (absolute 2e-13 / relative 2e-12).
The largest accepted emission-copy residual is 7.95990e-9 GeV. No timing-domain
failure or zero-record-mass/positive-nominal-mass emission was observed.

The three prior individual-frame failures remain visible: attempt 273 for
seed 2600417 and attempts 458 and 613 for seed 2600418. They are recorded as
mapping failures with the original error, not silently dropped or accepted.
The full attempt ledger is checked. Existing composed hard-outgoing transport
failures are outside this emission timing check and remain unresolved.

Direct-beam intermediate infinities remain excluded from emitted output:
41,220 nonfinite reference values are explicitly counted in incoming records.
All surviving emission times are finite. Both complete sample/fixture pairs
reproduce byte-for-byte with the final built probe.

The Pythia8 input contracts also pass with the new connection, covering ISR-on
and ISR-off histories, emission coverage, finite timing outputs, exact draw
counts, invalid nominal masses and paired generator-sequence preservation.
[Machine-readable evidence](validation-mapped-isr-timing.json) retains all
mapping failures, sample/snapshot/fixture hashes, executable hashes and source
hashes. This is not full-generator or jet/substructure validation.

## Reproduce

```sh
build-pythia8/jewel-pythia8-input snapshots.jsonl 1000 2600417 --isr on --mpi on
build-pythia8/jewel-isr-timing timing.jsonl timing.fixtures 1000 2600417 1
python tests/mapped_isr_timing_audit.py \
  build/jewel-isr-times-probe build/jewel-isr-times-reference \
  snapshots.jsonl timing.jsonl timing.fixtures timing-audit.json
ctest --test-dir build-pythia8 -R '^pythia8-input-contracts$' --output-on-failure
```

The probe refuses existing output paths. Mapping and timing failures stay in
the JSON ledger; only successful systems produce replay fixtures. Stable-ID [attachment to native records](validation-native-isr-timing.md) is now
implemented separately. The next adapter work must account for subsequent
virtuality/record changes and connect selection and evolution.
The native event runner still rejects ISR/MPI snapshots pending that work.
