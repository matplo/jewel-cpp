# Reconstructing Pythia8 ISR recoil frames

`reconstruct_isr_recoil_frames` implements the ordinary initial-initial recoil
prescription from Pythia8 8.317 SimpleSpaceShower::branch using the saved event
records. This supplies the transformations between shower stages without
inventing a rotation convention or fitting a matrix to the final recoil copies.

The reconstruction identifies the adjacent status-41/42 incoming pair and
checks that the recoiler belongs to the opposite beam's mapped chain. It uses
their old/new energies, the reconstructed spacelike virtuality and the emitted
mass to reconstruct the source's old-rest-frame kinematics. The emitted
particle's recorded azimuth fixes the source azimuthal rotation. The resulting
boost/rotation sequence follows Pythia8's Mtot/MfromRest prescription.

Source inputs must be ordinary massless, collinear initial-initial records with
the correct beam directions. Invalid domains and unsupported recoil topologies
are rejected. The matrices are chronological, with before/after particle IDs
for each transformed status-44 copy. Every reconstructed matrix is checked
against its observed copies before being returned.

## Momentum and displacement APIs

The header `jewel/pythia8_isr.hpp` exposes the frame records,
`apply_isr_frame` for momenta and `apply_isr_frame_displacement` for spacetime
displacements. The latter acts on (dt,dx,dy,dz) relative to an independently
chosen origin; it does not choose a hard-collision origin, supply translations
or assign production times. The hard-input JSON field `isr_recoil_frames`
records row-major matrices in (energy,px,py,pz) order and copy provenance.

Primordial transverse-momentum reshuffling (status 61/62), initial-final dipole
recoil, secondary scattering and MPI are not represented by these matrices.
They must not be silently treated as covered ordinary initial-initial recoil.

## Verification against actual Pythia8 records

| Seed | Events | Matrices | Recoil copies | Max component error [GeV] |
|---|---:|---:|---:|---:|
| 2600317 | 1000 | 3935 | 14865 | 3.8392e-10 |
| 2600318 | 1000 | 4018 | 15290 | 4.0961e-10 |
| 2600319 | 1000 | 3990 | 15081 | 5.5548e-10 |

All 11943 matrices and 45236 copies pass abs=2e-9 GeV plus rel=2e-11 times
the recorded component magnitude. These are reconstruction tolerances, not
relaxed tolerances for the separate frozen-Fortran timing comparison. All
pre-existing snapshot fields, including particle momenta and ancestry, remain
exactly unchanged. The final build reproduces all audited files byte for byte.

The independent Python audit checks M^T g M = g (max absolute error
4.4717e-11), and displacement round trips using g M^T g as the inverse
(max component error 4.6344e-11 fm). Both gates use 1e-9. Python also recomputes
all transformed momenta from the serialized matrices. Comparing its diagnostic
maximum with C++ permits 32 ULP of the largest absolute dot-product term sum,
accounting for fused versus unfused arithmetic in cancellation-prone boosts;
this does not change the recorded-copy acceptance gate.

Input contracts check displacement intervals and rejection of a corrupted
recoil-copy momentum. Five related suites pass: Pythia8 input, ISR CLI,
hadronization boundary, shower output and ISR timing.
[Machine-readable evidence](validation-isr-frames.json) includes source,
Pythia source, executable and sample hashes.

```sh
build-pythia8/jewel-pythia8-input frames.jsonl 1000 2600317 --isr on
python tests/isr_frames_audit.py frames.jsonl prior-chains.jsonl audit.json
ctest --test-dir build-pythia8 -R '^(pythia8-(input|isr-input|boundary|shower-output)|isr-times)' --output-on-failure
```

## Remaining integration

These transformations supply a tested part of recoil-frame transport, not a
finished prescription for JEWEL production times. Reconcile branch-local
spacelike vectors (including their signed energies), map primordial recoil and
choose consistent origins before connecting the timing calculation. Non-hard
virtuality initialization and medium evolution also remain unfinished. The
native event runner continues to reject ISR snapshots. There is no new claim
of Pythia6/Pythia8 ISR equivalence or a passing >=1000-jet Fortran/native ratio.
