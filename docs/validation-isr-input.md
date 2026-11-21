# Pythia8 ISR input boundary

The hard-input adapter supports `PythiaInputConfig::initial_state_radiation`
and `jewel-pythia8-input --isr on|off`. The default remains off; MPI, Pythia FSR
and hadronization remain disabled in this adapter. This is not yet ISR support
in `jewel-run-events` or the JEWEL cascade.

Snapshots retain the complete Pythia event, including ISR ancestry, colours,
scales and four-momenta. `isr_emissions` lists every status-43 emission root;
`isr_final` identifies its final copy. Hard outgoing roots and their final
copies remain separate. Copy walks require unchanged flavour, reciprocal
ancestry, unique IDs and only known copy statuses: 62/64, plus ISR recoil
status 44 when ISR is enabled. Both hard and ISR associations are independently
cross-checked against Pythia8's `iBotCopy()` navigation. Invalid, missing,
duplicated or overlapping emission/final-copy associations are rejected.

The reduced `prepare_pythia_dijet` adapter explicitly rejects ISR-enabled
snapshots before consuming random numbers. Previously, enabling ISR without
this work would leave all non-hard final partons as inert status-4 spectators.
The guard prevents advertising such behavior as JEWEL ISR evolution.

## Validation

Three seeds (2600317–2600319), 1000 attempts each, all return valid snapshots:

| Seed | ISR emissions | Events without emissions | Max momentum closure error [GeV] |
|---|---:|---:|---:|
| 2600317 | 3935 | 6 | 1.2672e-9 |
| 2600318 | 4018 | 3 | 1.5088e-9 |
| 2600319 | 3990 | 4 | 6.9268e-10 |

All 11943 emission chains and 6000 hard outgoing chains pass the explicit
walk and Pythia navigation checks. Closure sums the complete final input state,
including beam remnants, against the 5020 GeV incoming pp system. Acceptance
uses 1e-8 times that energy; the reported errors are the observed maxima.
[Machine-readable evidence](validation-isr-input.json) records input/executable/
source hashes and particle-status/flavour coverage.

The C++ input contracts test identical-seed reproducibility in both modes,
ISR metadata, momentum closure, corrupted associations and the transport guard
(with zero random draws). CLI checks verify default equals explicit off,
on-mode emission metadata, and rejection of missing/invalid/duplicate switches
before output creation. Input, boundary, shower-output and fragmentation
contracts all pass. A regenerated 100-event CT14nlo/0, alpha_s=0.118, order-2
ISR-off sample matches every original field of the prior sample exactly; only
the new false/empty ISR metadata fields are added.

## Remaining source mapping

Stock JEWEL INITPYTHIA sets MSTP(61)=1. `interpretpythiaevent` walks incoming
ancestry backwards from the matrix element and assigns production/end times,
using ISRSCAT to select the time origin. GENEVENT additionally classifies
non-hard partons and calls `findvirtualities` and `findmpivirtualities` before
MAKECASCADE. The native adapter has not mapped those time, virtuality, recoil,
rapidity and medium-evolution rules yet. Pythia6 and Pythia8 spacelike shower
records are not assumed identical.

Thus these are input-interface checks against Pythia8 and source inspection,
not Fortran ISR differential tests or a matched jet comparison. The full
Fortran/native >=1000-selected-jet ratio gate remains open.

```sh
build-pythia8/jewel-pythia8-input isr.jsonl 1000 2600317 --isr on
ctest --test-dir build-pythia8 -R 'pythia8-(input|isr-input|boundary|shower-output|fragmentation)' --output-on-failure
```
