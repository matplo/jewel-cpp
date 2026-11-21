# Attaching ISR timing to native records

`attach_pythia_isr_times` in `include/jewel/pythia8_shower_input.hpp` installs
mapped emission production coordinates and decay/end times in `ShowerState`.
It uses explicit source-final-ID to native-ID bindings rather than array order.
The result records each native/source pair and its subsystem/beam-side indices.
The common-frame timing prescription remains a candidate; this attachment does
not establish equivalent Pythia6/Pythia8 histories or a complete ISR shower.

## Contract and ordering

Call this after importing the complete final Pythia record and before surviving
parton preparation or any other momentum/mass changes. The supplied source map
must be one-to-one, cover every final source particle, and refer to live native
records. Additional native bookkeeping anchors are allowed. The helper checks
source/native PDG, mass and four-momentum equality, subsystem/beam/process roots,
emission ownership, uniqueness and complete ISR coverage before consuming RNG.

Only `particle.production` (fm) and `decay_time` (fm) change. Momentum, recorded
mass, status, ancestry, generator vertices, colour and evolution metadata remain
unchanged. Hard legs and remnants without an ISR-emission binding retain their
previous production and decay values. The helper does not activate particles,
assign their virtualities or invent their shower ancestry.

State updates are transactional across all subsystems: a later timing failure
does not publish any earlier updates. Consumed random draws are not rewound.
Chains are evaluated in subsystem order, then side 0 and side 1; this is the
candidate Pythia8 subsystem prescription, not a claim about full Pythia6 MPI RNG
ordering. Empty chains retain the previously validated source draw behavior.

## Generated-record validation

The probe now imports every final particle into a native record in **reverse
source order**, after an inert anchor. This deliberately makes native indices
different from source IDs. It attaches times and compares each installed value
to the direct timing result before writing an emission's `native_id`. It checks
that non-emission records retain distinct test production/decay markers and that
status, record mass and evolution time are preserved.

The independent Python audit derives the expected native IDs from the captured
Pythia8 snapshots and checks every attachment. It also repeats the common-frame
kinematic audit and unchanged Fortran timing replay.

| Seed | Mapped events | Native ISR attachments | Timing values |
|---|---:|---:|---:|
| 2600417 | 999 | 13,249 | 66,245 |
| 2600418 | 998 | 13,683 | 68,415 |

All **26,932 attachments** have the expected native IDs. All **134,660 timing
values agree exactly** with Fortran and with the installed native records.
The two replay fixture files remain byte-identical to the prior timing-only
run, and removing the new `native_id` fields makes every JSON ledger record
identical to the prior run. The three known frame failures remain unchanged;
none is silently accepted or removed. Configuration, random-tape policy and
remaining frame-composition limitations are documented in
[the mapped timing validation](validation-mapped-isr-timing.md).

The generated-event contracts cover both ISR-on and ISR-off inputs and verify:
complete emission coverage, source-order independence, preservation of momenta
and auxiliary vertices, exact RNG use, missing-source-map rejection, rejection
after an imported momentum changes, and rejection of a hard final particle
misidentified as an ISR emission. These preflight failures consume no draws.
A deliberately exhausted random tape also verifies that updates are rolled back
after the first chain has already consumed its draws. Paired Pythia generators
retain their matching event sequences. Both the input and existing adapter
boundary contract suites pass.

[Machine-readable evidence](validation-native-isr-timing.json) includes source,
probe and sample hashes, complete audit results, and unchanged failure records.
To reproduce, use the [timing-probe commands](validation-mapped-isr-timing.md#reproduce)
with new output paths, then run:

```sh
ctest --test-dir build-pythia8 \
  -R '^pythia8-(input|boundary)-contracts$' --output-on-failure
```

The native event runner remains ISR/MPI-off pending construction of the full
surviving record and its ancestry, preparation/virtuality initialization and
cascade integration. Full jet/substructure agreement has not been established.


The probe now uses the reusable [native final-record importer](validation-native-import.md)
with one process anchor per subsystem. The independent audit reads the recorded
anchor count when reconstructing native IDs. The earlier attachment results
above remain the evidence for their recorded source/probe hashes.
