# Scheduler development checkpoint — 2026-11-20

This is an incomplete integration milestone, not a validation certificate.
`Cascade::branch()` and `Cascade::evolve()` connect MAKEBRANCH and MAKECASCADE
using the previously validated initial/final evolution, splitting, coherent
scattering and collision routines. The implementation is in `src/cascade.cpp`.

The first 42-case matrix passed 112,788 scalar comparisons, with maximum absolute
difference 5.80e-14. It covered individual medium branches, time initialization,
and vacuum cascades. The expanded 58-case matrix is intentionally retained in
`tests/cascade_differential.py`; it is currently failing and must not be described
as validated. All 18 C++-only contracts passed after the slot-metadata changes.
The other 43 reference-enabled tests pass (140.71 s with two workers), explicitly
excluding the expanded `cascade-differential` test. The separately rerun
[42-case baseline](cascade-baseline-validation.json) also passes with the current
slot-metadata implementation. Run it with:

```sh
python3 tests/cascade_differential.py build/jewel-medium-probe \
  build/jewel-medium-reference build/cascade-baseline-validation.json --baseline
```

The default invocation retains all 58 cases and is still failing.

## Integration findings

GENEVENT initializes unused STARTTIME slots to -1e6. Constructors that leave
STARTTIME unassigned must inherit that value until MAKEBRANCH initializes the
slot from the production time. `ShowerState(-1e6)` provides this behavior; the
zero default remains available for the existing isolated reference fixtures.
Partial truncation also retains the MEDIND flag. Full snapshots restore both
metadata tails and the unused-time default. Explicit daughter evolution times
remain explicit. Particle identifiers are never recycled.

An isolated high-rate coherent cascade (matrix case 45, seed 260045, gluon,
100 GeV parent, 10 GeV mass, cross-section multiplier 5, SCATRECOIL=false) was
traced after each branch using temporary diagnostic copies. The first discrete
mismatch is at zero-based branch step 32, selected physical slot 95. Both sides
have 131 records and colour maximum 266. Before this step, the maximum scalar
difference is 1.15e-12; record counts and RNG cursors match.

The thermal mass is 0.79494487676389558. The resulting recoil mass is
0.79494487676388859 in C++ and 0.79494487676389869 in Fortran. The source's exact
`P(N-1,5) > P(1,5)` comparison therefore consumes one extra lifetime random draw
in Fortran: cursors 147171 versus 147172. The recoil decay times become 0 versus
3.7325782847333855. Later trajectories differ. The final diagnostic runs have
345 records / 241974 draws in C++ and 280 records / 711882 draws in Fortran.
No physics tolerance or forced branch decision has been added to hide this.
Temporary trace instrumentation is diagnostic only, not the validation oracle.

A separate status-2 recoil branch (seed 260000, selected slot 4, rate multiplier
1) gives a NaN daughter angle in unmodified Fortran after MAKEINSPLIT assigns
fraction -1 to a zero-mass radiated daughter. C++ rejects the nonfinite record.
This is not counted as finite parity. Recoil-rescattering cascade validation
remains open; the current expanded whole-cascade matrix uses the stock default
SCATRECOIL=false, while individual branch fixtures also exercise true.

Some high-rate cascades require more than one million random draws. The harness
now supports two-million-value tapes. The expanded batch times out in
Fortran at 240 seconds. Isolating matrix case 52 (seed 260052, antiquark,
coherence on, multiplier 1, SCATRECOIL=false) explains the timeout: after branch
step 35, selected slot 68, the source has NaN momentum components and energy in
slot 76. C++ rejects nonfinite kinematics at draw 63729. Fortran continues, then
repeatedly schedules slot 81 with 82 records, draw cursor 63783 and colour
maximum 304 unchanged. A 30-second diagnostic captured 66,649 repetitions of
that same step. Its large trace was reduced to the first 100 steps and a count
summary. Tape exhaustion, nonfinite events and timeouts are failures, not
successful exclusions.

## Outstanding work

- Resolve or explicitly characterize each expanded-matrix failure, including
  the characterized nonfinite kinematics and Fortran infinite loop; establish statistical
  validation where roundoff-dependent random consumption prevents trajectory
  replay. Do not widen tolerances on discrete fields.
- Verify and expand initial-split retry, induced splitting and angular-order
  coverage using complete branches/cascades.
- Broaden compression validation beyond the opt-in native integration described
  below, including all angular-ordering modes and unusual prefix genealogy.
- Add run initialization, Pythia8 hard-event import and hadronization, all custom
  JEWEL exports, and the requested final jet/substructure comparison and dynamic
  HTML report. The full thread goal remains incomplete.

## Generated-stream ensemble follow-up

The [ensemble validation notes](validation-cascade-ensemble.md) and
[interactive report](cascade-ensemble-report.html) extend this checkpoint with
a passing 42-case generated-stream baseline, bounded expanded diagnostics and
1,000 attempted showers per engine in each coherence setting. A missing GQ
GENERATEZ alias is fixed. Excess C++ rejections are traced to temporary
nonfinite collision candidates that the source can recover through retries;
transactional candidate handling remains unresolved. The report is diagnostic
and does not satisfy the full-generator validation milestone.

### Transactional collision retry follow-up

`apply_collisions` now stages event and recoil storage privately, permits the
source's temporary nonfinite outgoing candidates and validates before commit.
The first failing seed now matches all reference values within existing
tolerances. The refreshed ensemble report improves C++ accepted counts to
992 (off) and 988 (on), versus 994 and 993 reference events. Remaining
nonfinite and discrete trajectory discrepancies are still tracked; this
does not complete the generator milestone.

### Compression boundary checkpoint

The [compression routine](validation-compression.md) now matches the unchanged
Fortran reference exactly across 222 cases, including inactive tail slots.
This supplies the source-compatible record transformation and retained-slot
mapping. Scheduler integration remains pending: moved genealogy is cleared
while evolution times and medium-induced flags remain attached to physical
slots, requiring an explicit boundary with the stable-ID ancestry graph.

### Native compression integration

Native compaction now archives removed ancestors and keeps source scheduling
links separate from physical ancestry. The scheduler supports the stock
20,000-record trigger through an opt-in flag. Four vacuum cascades, an exact
threshold case and a coherence-enabled medium fixture pass existing replay
tolerances. The threshold case required porting PEVREC's V=MV side effect.
One additional medium fixture retains four stored-transfer discrepancies.
See [compression validation](validation-compression.md) for evidence and limits.
