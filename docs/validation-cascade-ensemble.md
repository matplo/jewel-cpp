# Controlled expanding-medium cascade comparison

This development checkpoint is not full-generator validation. The expanding
medium and numerical tables have separate passing [component validation](validation-medium.md).
The [interactive report](cascade-ensemble-report.html) compares controlled
parton showers with coherence off and on, using unchanged JEWEL 2.6.0 physics
routines as the reference. Pythia hard-event generation and hadronization are
not included.

| Coherence | Attempts per engine | Accepted C++ / Fortran | Selected jets C++ / Fortran |
|---|---:|---:|---:|
| Off | 1,000 | 992 / 994 | 1,576 / 1,580 |
| On | 1,000 | 988 / 993 | 1,543 / 1,549 |

Both settings use fixture 55: a controlled 100 GeV parent with mass 10 GeV,
expanding medium, rate multiplier 1, KINMODE=1, angular ordering off and
SCATRECOIL=false. They are single-shower fixtures, not physical hard dijet events.
Jets use native hepyy/cppyy FastJet 3.5.1, anti-kt R=0.4. All terminal shower
partons (status 4/5) are clustered before applying pT >= 5 GeV and |eta| < 2;
status-3 recoils are excluded. Observables are jet pT, girth and pTD.

Histograms are normalized per attempted event; failed attempts contribute zero
jets and remain visible. Ratio uncertainties use paired event-level sampling,
including correlations between jets in an event. Empty reference bins have
undefined ratios. The coherence-off girth bin [0.04, 0.06) has ratio 49/44 = 1.1136, outside
the requested ±10% band. All other nonempty reference bins in these samples
are within the band; statistical precision remains limited.
More than 1,000 selected jets alone does not make this a passing validation.

## Reproducibility and reference boundaries

Both probes accept generated streams using splitmix64-open52-v1, with seed and
consumed-draw count recorded. This is a shared test RNG, not Pythia6's RNG.
The Fortran harness uses an independent implementation; C++ physics does not
call Fortran. The report records input, executable, reference-source and loader
hashes, settings and analysis versions. Seeds in these samples are positive
and within the signed 64-bit range supported by both probes.

GCASCADE is a diagnostic scheduler that checks for nonfinite state after each
unchanged MAKEBRANCH call and limits branch calls. It bounds a known source
scheduler loop; it does not repair reference physics or guarantee termination
inside an individual branch. The original CASCADE operation is retained.

The generated-stream baseline passes 42 cases / 113,328 scalar comparisons,
maximum absolute error 3.28e-14, with matching discrete fields and draw counts.
The expanded guarded 58-case run records nine mismatches across six cases;
its JSON is retained as a diagnostic, not a passing certificate. The latest regression
run passed 45/45 tests (160.73 seconds), excluding the known-failing expanded
cascade-differential.

## Findings and next implementation step

GENERATEZ uses both QG and GQ labels for its quark-pair branch in the source.
The C++ sampler now accepts the GQ alias; direct and differential checks cover
both labels. This corrected one rejected event in the coherence-off sample.

The main remaining rejection difference is transactional collision handling.
For seed 1260555, fused arithmetic produces a temporary squared momentum of
-3.5593068880265875e-17. The original compiled Fortran uses the same fused
operation, so simply clamping the value would change numerical policy.
Fortran later retries the candidate and recovers; strict C++ validation throws
before that recovery.

A diagnostic PYROBO wrapper observed the unchanged compiled reference physics
object. Its acceptance pattern matched the original for all 1,000 off events:
41 events with nonfinite intermediate collision records recovered to finite
accepted events, while six ended invalid. See
[candidate diagnostics](cascade-transient-diagnostics.json). Instrumenting the
physics body itself changed optimization and was not used as the oracle.

Collision sequences now operate on a private event and recoil-storage copy.
An internal rotation path preserves temporary nonfinite outgoing candidates
until the source retry decision. Truncation removes rejected candidate records;
normal restore validation checks the retained state. Strict validation runs
before committing the sequence. Exceptions leave the caller's event and recoil
storage unchanged; random draws are intentionally consumed, not rewound. Public
kinematics and particle append retain their finite-input requirements.

Seed 1260555 now reproduces the reference's 103 records and 139,897 random draws;
all compared values pass the existing tolerances (maximum absolute difference
2.84e-13). A [dedicated cascade regression](cascade-collision-retry.json) covers it. A separate contract checks
that a failed sequence after its first collision publishes no partial state.

The [previous ensemble summary](cascade-pre-retry-summary.json) retains the
pre-fix results: 952/938 accepted C++ events with coherence off/on. The current
results are 992/988. Residual failures and trajectory differences remain; the
new [58-case generated-stream replay](cascade-retry-expanded.json) records 18
mismatches, including discrete/trajectory discrepancies. It uses different
random inputs from the earlier finite-tape guarded report, so mismatch totals
are not a before/after comparison.
Event compression, run initialization, Pythia8 import/hadronization and custom
output remain separate unfinished milestones.

## Running the comparison

Use an environment with hepyy, cppyy and hepyy-utils installed:

```sh
python tools/cascade_ensemble.py build/jewel-medium-probe \
  build/jewel-medium-reference build/cascade-ensemble-coherence-off.json \
  --events 1000 --case 55 --coherence off
python tools/cascade_ensemble.py build/jewel-medium-probe \
  build/jewel-medium-reference build/cascade-ensemble-coherence-on.json \
  --events 1000 --case 55 --coherence on
```

The HTML is self-contained. Its JavaScript syntax was checked; browser visual
inspection was not performed because the browser rejected local-file access.

The [remaining acceptance disagreements](cascade-remaining-failures.json) list
six off and seven on seeds, including cases accepted only by C++. They are
retained for further diagnosis; failed events have not been dropped from rates.
