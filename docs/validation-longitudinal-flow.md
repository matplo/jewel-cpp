# Longitudinal medium flow in the native runner

Recorded 2026-11-21. The native parton and hadronized runners accept
`--longitudinal-boost on|off`. Omission retains the earlier `off` behavior;
stock medium-simple initializes its BOOST flag to true.

This setting uses the existing port of the source momentum-distribution boost,
with local rapidity y = 0.5 log((t+z)/(t-z)). It is not a change to the optical
geometry or a new transverse-flow model. Maximum scatterers for all four table
families, local scattering rates, transfer sampling and every corresponding
replay configuration use the same flag. The JSON header records it in
`longitudinal_scatterer_boost`.

```sh
build-pythia8/jewel-run-events flow.jsonl 20 2601001 medium replay-inputs \
  --centrality 0 10 --table-energy jewel --longitudinal-boost on
ctest --test-dir build-pythia8 -R '^runner-flow-contracts$' --output-on-failure
ctest --test-dir build \
  -R '^(sampling|transfer|event-geometry)-differential$' --output-on-failure
```

## Validation

[Machine-readable results](validation-longitudinal-flow.json) include source,
executable and native sample provenance. Flow-sensitive differential suites pass
against frozen JEWEL 2.6.0, with abs=2e-13 and rel=2e-12:

- Sampling: 181 cases including 108 scatterer cases with boost on/off;
  maximum scatterer absolute error 1.38778e-17.
- Transfer sampling: 234 cases (10,062 scalar comparisons), including boost
  on/off, fresh/recalculated/retried transfers; maximum error 2.22045e-16.
- Event-local geometry/rates: 3,030 scalar comparisons, including initialized
  flow-on tables preserved across impact-parameter changes; max error 1.42109e-14.

The runner integration test checks omitted versus explicit off, option rejection,
all five replay boost flags, and exact agreement of on-mode native particle
momenta and random-draw count with a separately initialized C++ replay probe.

## Small native sample and strict Fortran replay

At 0–10% centrality with the EOVEST scale, 18/20 attempts were accepted and
hadronized. Accounting, record integrity, geometry and conservation checks pass.
Attempt 4 was discarded by the cascade; attempt 18 encountered nonfinite
kinematics. Neither failure is silently removed from accounting. With flow off,
the full 20-attempt output exactly reproduces the preceding EOVEST baseline.

For identical initial states and random streams with flow on:

| Attempt | Records | Centres | Random draws | Strict C++/Fortran result |
|---|---:|---:|---:|---|
| 1 | 87 | 26 | 8,788 | Fails: 11 scalar mismatches, max abs 3.32001e-12 |
| 2 | 51 | 16 | 15,234 | Passes: max abs 4.62685e-14 |

Counts agree between implementations in both cases. The strict failed case
remains a failure even though the differences are small. These two replays and
the 20-event sample do not establish full-generator physics equivalence, nor
the requested >=1000-jet, 10% spectrum/substructure gate. ISR/MPI, remaining
settings/features and cascade discrepancies still need work.
