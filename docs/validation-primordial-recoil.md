# Primordial recoil and composed frame transport

`reconstruct_primordial_recoil_frame` follows the single-system transformation
in Pythia8 8.317 BeamRemnants: `toCMframe(oldIncomingA, oldIncomingB)` followed
by `fromCMframe(newIncomingA, newIncomingB)`. It uses the actual before/after
status-61 records, preserving their beam association and ancestry.

The adapter returns incoming source IDs, a row-major Lorentz matrix in
(energy,px,py,pz) order, before/after copy IDs and the maximum observed
momentum-component error. It checks both incoming status-61 and outgoing
status-62 copies. `apply_primordial_frame` and
`apply_primordial_frame_displacement` transform momenta and spacetime
displacements respectively. They do not generate a primordial-kT distribution,
choose an origin, or assign physical production times.

The hard-input CLI now exports `primordial_recoil_frame` in both ISR modes.
Multiple primordial systems and unsupported incoming histories are rejected.
Additional rescattering mismatch corrections are not implemented; inconsistent
copy momenta fail the closure check rather than receiving an assumed mapping.
This remains the reduced single pp hard-system adapter, with MPI disabled.

## Verification

| Sample | Events | Primordial copies | Max component error [GeV] |
|---|---:|---:|---:|
| ISR on, seed 2600317 | 1000 | 7935 | 1.4598e-10 |
| ISR on, seed 2600318 | 1000 | 8018 | 8.0036e-11 |
| ISR on, seed 2600319 | 1000 | 7990 | 5.6307e-11 |
| ISR off, seed 2600317 | 1000 | 4000 | 4.7021e-10 |

All 4000 matrices and 27943 copies pass the existing reconstruction gate,
abs=2e-9 GeV plus rel=2e-11 times the component magnitude. Independent Python
arithmetic checks all serialized matrices, both incoming IDs and complete
status-61/62 coverage. Lorentz-metric closure has maximum error 2.3844e-12.
Round trips of timelike, spacelike and zero displacements have maximum
component error 7.5269e-12 fm; both tests require less than 1e-9.

The audit then follows each hard outgoing and ISR-emitted root through every
recorded ISR recoil copy and the primordial stage. All **19943 final partons**
are reached without missing or duplicate transitions. The maximum composed
momentum-component difference is **1.9899e-9 GeV**, within the separately
recorded cumulative gate abs=1e-8 GeV plus rel=1e-10. The gate accommodates
rounding accumulated through multiple transformations; it is not the strict
frozen-Fortran numerical-routine tolerance or a physics-distribution test.

Every pre-existing field in the 3000 ISR-on snapshots remains exactly unchanged.
The new metadata does not change generator random numbers, particle momenta,
weights, existing ISR chains or their recoil matrices. Input contracts cover
both ISR modes, spacetime interval preservation and rejection of a deliberately
corrupted outgoing copy. Input, ISR CLI, hadronization boundary, shower-output
and ISR-timing suites all pass.

[Machine-readable evidence](validation-primordial-recoil.json) records all
sample/source/executable hashes, residuals and tolerances. The diagnostic
comparison between Python and C++ maxima allows the same dot-product rounding
bound documented in validation-isr-frames.md; this does not change momentum
closure acceptance.

```sh
build-pythia8/jewel-pythia8-input primordial.jsonl 1000 2600317 --isr on
python tests/primordial_audit.py primordial.jsonl --output audit.json
ctest --test-dir build-pythia8 -R '^(pythia8-(input|isr-input|boundary|shower-output)|isr-times)' --output-on-failure
```

## Remaining physics connection

The ordinary ISR and primordial transformations now reproduce the entire
recorded recoil path to final hard/emitted partons in these samples. This
provides frame transport, not JEWEL's spacetime history. Reconcile signed
spacelike energies and the formation-time frame, anchor production coordinates,
and implement non-hard-parton virtuality initialization before enabling ISR
medium evolution. The event runner remains ISR-off. No Fortran/native
>=1000-selected-jet ratio gate has been passed by this component milestone.
