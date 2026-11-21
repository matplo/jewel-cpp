# ISR ancestry for every MPI subsystem

Validated 2026-11-21 with Pythia8 8.317. `map_subsystem_isr_ancestry` returns two
incoming chains per subsystem, ordered by the subsystem ledger. It starts at the
original incoming roots shared by that scattering's outgoing pair, walks status
41/42 ISR histories and the status-61 primordial copy, and ends at the beam.
The terminal incoming record must equal that subsystem's recorded incoming ID.

Each status-41 radiator must supply exactly one status-43 emitted sibling.
Its final copy must belong to the same subsystem. Incoming chains cannot share
non-beam records; emission roots cannot be duplicated or omitted across the
whole event. ISR-off input also returns chains, with no radiation branches.
Malformed ancestry, swapped subsystem ownership and missing source energies
are rejected explicitly.

For each branch, the spacelike daughter is reconstructed as mother minus emitted
momentum using captured **pre-beam-energy-repair** energies. The source-stage
capture is described in [primordial validation](validation-mpi-primordial.md).
The JSON `subsystem_isr_chains` field labels this momentum stage explicitly.
The original single-system API retains its existing behavior; its MPI guard
remains in place. These new chains have not been substituted into JEWEL timing.

## Independent audit

Four samples each contain 1,000 events: two seeds, ISR off/on, MPI on, pp at
5020 GeV, pThat 20–100 GeV, Pythia FSR and hadronization off.

| Seed | ISR | Subsystems | Emission branches | Negative stage-local energies |
|---|---|---:|---:|---:|
| 2600417 | off | 7,572 | 0 | 0 |
| 2600417 | on | 6,767 | 13,253 | 1,586 |
| 2600418 | off | 7,164 | 0 | 0 |
| 2600418 | on | 6,997 | 13,709 | 1,593 |

All 57,000 incoming chains and all 26,962 emission branches pass the independent
ancestry/ownership audit. Every pre-existing JSON field is exactly equal to the
preceding stage's corresponding event, including primordial mappings, generation
outcomes, weights and momenta. No event was dropped.

All exported spacelike components equal the independently subtracted source
components. The virtuality-squared calculation is checked against an 80-digit
Decimal evaluation of those components, with a 32-ULP allowance based on the
sum of squared components. Maximum absolute residual is 2.26e-10 GeV². This
checks arithmetic, not an independent physical model or a JEWEL equivalence
claim. The 3,179 negative spacelike energies are retained without clamping;
energy sign is frame-dependent and does not determine a physical production time.

[Audit results and provenance](validation-mpi-isr-ancestry.json).

Four native regression suites pass: input, CLI, shower boundary and shower
output. Additional input contracts reject broken ancestry, swapped terminal
subsystem records and absent pre-repair emission energy.

Remaining integration work: reconstruct subsystem ISR recoil transforms,
compose them with primordial transforms in the correct energy stage, establish
physical time assignments, then connect all scattering pairs and radiation to
JEWEL evolution. The reduced dijet evolution adapter still rejects MPI snapshots.
No Fortran kernel, reference tolerance or final-jet validation claim changed.
