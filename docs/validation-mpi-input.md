# MPI input subsystem capture

Validated 2026-11-21 with Pythia8 8.317. `jewel-pythia8-input --mpi on`
now captures MPI subsystem membership, with ISR independently on or off.
The default remains MPI off. Pythia FSR and hadronization remain disabled.

The adapter reads `Pythia::partonSystems`. It records each subsystem's current
incoming/outgoing IDs, final-copy associations, scale and invariant mass squared.
Original status-23 primary and status-33 secondary outgoing roots are associated
by their final-copy membership, not by pT equality or parentless-record heuristics.
Each pair must share two distinct incoming roots of the appropriate status,
close four-momentum, and belong to exactly one subsystem. All scattering and
status-43 emission roots must be represented; unrelated beam-remnant particles
cannot enter the subsystem output ledger. Full particle records and junctions
are retained separately.

## Evidence

Two samples use seed 2600417, 1,000 attempts each, pp at 5020 GeV, pThat
20–100 GeV, the adapter's internal PDF/tune settings, and MPI on.
All attempts returned events. An independent Python audit follows ancestry,
checks coverage and uniqueness, and checks pair and whole-event conservation.

| ISR | Subsystems (including primary) | Secondary pairs | ISR emissions |
|---|---:|---:|---:|
| off | 7,572 | 6,572 | 0 |
| on | 6,767 | 5,767 | 13,253 |

Maximum original-pair closure residuals are 1.28e-10 GeV (ISR off) and
7.75e-11 GeV (on). Whole-event residuals are 1.33e-6 and 2.70e-5 GeV,
respectively, within the stated 1e-8 times beam-energy tolerance.
Final subsystem closure is recorded separately as a diagnostic: its maximum
is 6.67e-9 GeV off and 2.70e-5 GeV on. It is not claimed to have original-pair
precision or used as evidence that MPI recoil transport has been implemented.
[Machine-readable evidence and hashes](validation-mpi-input.json).

Four native regression suites pass: input contracts, CLI, shower boundary,
and shower output. Tests reject missing/duplicated subsystem mappings,
incorrect ancestry, nonconserving roots and invalid MPI switches. Default
output equals explicit ISR/MPI-off output for a fixed seed. MPI snapshots
are rejected by reduced dijet preparation before any JEWEL random draw.

## Limits and next integration step

This is input capture, not MPI evolution. The single-system ISR ancestry and
primordial-recoil adapters reject MPI snapshots; their derived JSON fields
are null, meaning unavailable, rather than empty histories. Original records
remain available for implementing subsystem-specific transforms and timing.

The separate MPI virtuality kernel is compared against unchanged JEWEL 2.6.0
in [MPI initialization validation](validation-mpi-initialization.md).
That comparison does not make Pythia8's MPI model equivalent to Pythia6's,
and this input audit does not validate final jets. Connecting subsystem pairs
to evolution requires consistent recoil frames, formation times, explicit
JEWEL sampling channels and colour/remnant handling for all subsystems.

Reproduce with distinct output paths (the exporter refuses overwrites):

```sh
build-pythia8/jewel-pythia8-input mpi-off.jsonl 1000 2600417 --mpi on --isr off
build-pythia8/jewel-pythia8-input mpi-on.jsonl 1000 2600417 --mpi on --isr on
python3 tests/mpi_input_audit.py mpi-audit.json mpi-off.jsonl mpi-on.jsonl
```
