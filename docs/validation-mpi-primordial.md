# MPI primordial recoil: partial reconstruction

Recorded 2026-11-21, Pythia8 8.317. This implementation reconstructs a
BeamRemnants-style primordial recoil matrix for each ordinary beam-initiated
subsystem. It remains incomplete: 26 of the 2,000 audited events cannot yet
supply a validated set of matrices. Do not use this result as evidence of
complete MPI transport, formation-time assignment, or JEWEL equivalence.

`reconstruct_subsystem_primordial_frames` uses the subsystem's status-61 incoming
copies and their old daughters, then constructs `toCMframe(oldA,oldB)` followed
by `fromCMframe(newA,newB)`. This follows the locally inspected Pythia8
`BeamRemnants.cc` Msys construction and subsequent status-61/62 copies.
Every original-to-copy momentum is checked. The implementation requires complete
coverage and refuses rescattered incoming records and unsupported copy histories.
Existing single-system APIs and their tolerances are unchanged.

## Numerical policy and failures

Reconstruction uses already rounded transformed incoming momenta; low-mass,
highly boosted systems amplify that rounding. The original single-system
absolute/relative test alone failed, e.g. a 3.72e-8 GeV discrepancy for a
549 GeV momentum component. The new subsystem API explicitly adds a conservative
conditioning allowance:

`64 * epsilon(double) * ||toCM||_inf * ||fromCM||_inf * momentum_scale`.

This is a numerical error estimate, not a rigorous bound or physics tolerance.
The scale is the largest absolute old four-vector component, new energy, or
1 GeV. Reconstruction is rejected if the allowance exceeds 1e-7 of this scale.
Otherwise each component must agree within 2e-9 GeV + 2e-11 times its absolute
value + this allowance. The largest allowance and actual residual are exported
separately. No such allowance was added to the Fortran kernel comparisons.

Snapshot export retains raw events when reconstruction fails. It writes
`primordial_subsystem_frames: null` and a non-null `primordial_subsystem_error`.
Successful events contain one frame per subsystem, in subsystem-index order,
and a null error. It never drops an event or substitutes an empty history.
The old singular primordial field remains null for MPI snapshots.

## Audit

The same seed/configurations as the MPI input audit were regenerated. Every
pre-existing JSON field agrees exactly, including all momenta and weights.

| ISR | Events with all frames | Rejected events | Frames | Checked copies |
|---|---:|---:|---:|---:|
| off | 984 / 1000 | 16 | 7,442 | 29,768 |
| on | 990 / 1000 | 10 | 6,692 | 39,858 |

Of the 26 rejected events, 18 exceed the conditioning limit and 8 fail the
momentum-copy comparison. The latter are all ISR-on; their cause is not yet
established. They must not be attributed to floating-point rounding without
further evidence. The current event-level failure policy withholds all subsystem
matrices if even one subsystem fails.

The independent audit checks full copy coverage, matrix application and the
Lorentz metric. Maximum checked-copy residual is 6.90e-7 GeV. The largest
reported conditioning allowance is 1.48e-4 GeV. Absolute metric residuals reach
1.22e-5; the audit uses a norm-scaled gate of 1e-7 * max(1, ||M||_inf²),
so this is not a claim of absolute 1e-7 metric accuracy for boosted matrices.
[Detailed evidence](validation-mpi-primordial.json).

Native input/CLI/boundary/shower-output suites pass. Tests include deliberately
corrupted copied momentum and a 100-event CLI sample requiring rejected mappings
to remain in the output. Further work must resolve rejected mappings and map
subsystem ISR before connecting these transforms to JEWEL time/evolution state.
