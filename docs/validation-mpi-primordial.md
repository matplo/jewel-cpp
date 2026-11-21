# MPI primordial recoil at the correct energy stage

Updated 2026-11-21, Pythia8 8.317. All 4,000 audited events now provide complete
subsystem primordial transforms. This resolves all 26 rejected events from the
preceding implementation. MPI ISR transport and JEWEL evolution remain separate,
unimplemented integration steps; these are adapter results, not final-jet parity.

## Source finding and implementation

Pythia8 constructs each subsystem's Msys in `BeamRemnants.cc` from original
incoming momenta and the beam-slot target momenta. It applies that matrix to
incoming status-61 and outgoing status-62 copies. Subsequently
`BeamSetup::boostAndVertex` recalculates **every record's energy** as
`sqrt(max(0, mass² + |p|²))`, including historical records. Consequently, final
record energies are not the original before/after Lorentz-transform inputs.
Trying to reconstruct the source transform from them caused the prior failures.

The adapter now captures two pieces of source-stage information:

- `primordial_targets_px_py_pz_energy` per subsystem, from Pythia's original
  beam slots, checked against subsystem incoming IDs and beam-origin flags.
- `energy_before_beam_repair` per particle, captured by a parton-level observer
  before `boostAndVertex`. The observer never vetoes, edits records or consumes
  random numbers. Spatial momenta must remain identical across that stage;
  unsupported subsequent rotations/boosts fail explicitly.

`reconstruct_subsystem_primordial_frames` uses those inputs with the source's
`toCMframe(oldA,oldB)` / `fromCMframe(targetA,targetB)` prescription. Each exported
frame declares `momentum_stage: before_beam_energy_repair`. To apply it to a
captured momentum, use the particle's pre-repair energy with its stored spatial
components. The final energy repair is checked independently. It is not a
Lorentz transformation and must not be folded into a spacetime boost.

The previous extra conditioning allowance and rejection limit were removed.
Every copied component now uses the original gate: 2e-9 GeV + 2e-11 times the
absolute reference component. Missing source inputs and unsupported ancestry
still fail explicitly. Export retains raw events and records an error/null
frame set if reconstruction fails; none fail in the current audit. The existing
single-system API and its output remain unchanged.

## Validation

All samples are pp at 5020 GeV, pThat 20–100 GeV, MPI on, Pythia FSR and
hadronization off, using the existing adapter PDF/tune configuration.

| Seed | ISR | Mapped events | Subsystem frames | Checked copies |
|---|---|---:|---:|---:|
| 2600417 | off | 1,000 / 1,000 | 7,572 | 30,288 |
| 2600417 | on | 1,000 / 1,000 | 6,767 | 40,321 |
| 2600418 | off | 1,000 / 1,000 | 7,164 | 28,656 |
| 2600418 | on | 1,000 / 1,000 | 6,997 | 41,697 |

The independent audit checks full copy coverage, staged matrix application and
final energy repair. Its maximum copy residual is 3.06e-9 GeV, within the
combined absolute/relative gate. Maximum final-energy repair residual is
4.55e-13 GeV. The two seed-2600417 samples additionally reproduce **every
pre-existing event field exactly**, excluding only the newly introduced
metadata. This includes all momenta, weights, generation outcomes and summary
statistics. The fresh-seed samples have no pre-change comparison files and are
not included in that exact-reproduction claim.

Highly boosted source matrices still have finite-precision metric residuals:
the largest absolute residual is 3.30e-5. The independent audit uses
1e-7 * max(1, ||M||_inf²) for the Lorentz-metric check. This does not establish
absolute 1e-7 metric accuracy, nor arbitrary spacetime-transport precision.
[Evidence, executable/audit hashes and Pythia source hashes](validation-mpi-primordial.json).

Four native regression suites pass: input, CLI, shower boundary and shower
output. Tests reject absent beam targets, absent pre-repair energies and corrupted
copied momenta. The CLI regression includes a 100-event sample containing formerly
rejected events and now requires every subsystem reconstruction to succeed.

No JEWEL Fortran physics routine or differential-test tolerance changed. This
resolves the primordial-stage mapping issue; subsystem ISR history/frame mapping,
physical timing, multi-pair evolution and final-state jet comparisons remain open.
