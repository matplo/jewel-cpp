# Backward ISR production-time port

`assign_isr_times` implements the backward timing block of JEWEL 2.6.0
`interpretpythiaevent`. Its input is a hard incoming line and an ordered chain
of incoming/emitted siblings leading backwards to a beam. Its output contains
production coordinates and production/end times for every supplied line.
This is a core operation; actual Pythia8 ancestry mapping is still pending.

## Preserved source behavior

- Formation intervals use `log(1-u)/lambda`, with
  `lambda = 1/(ftfac * E * double(float(0.2)) / record_mass^2)`.
  The promoted default-REAL constant and expression order are retained.
- The hard incoming line ends at 0 when ISRSCAT is true, or -1e6 fm otherwise.
  Its first position uses the absolute sampled production time, as in the
  reference; subsequent incoming lines propagate from the preceding vertex.
- Velocities divide by max(|p|, E), including spacelike incoming momenta.
- The terminal beam line uses the reference sentinel coordinates
  (1e6, 1e6, +/-1e6, -1e6), with end time equal to its child's production time.
  These sentinel coordinates are not interpreted as physical production points.
- An emitted particle starts at its sibling's production vertex. Exact equality
  of record and supplied nominal mass sets its end time to the medium lifetime
  without a random draw. Otherwise its positive lifetime is sampled.
- RNG order is hard incoming, then each nonterminal incoming followed by its
  off-shell emission. There is no incoming-time draw for the terminal beam.

The API validates finite inputs, positive formation factor, nonzero sampled-line
energies and defined nonzero formation rates and a chain ending at one identified beam. Undefined domains raise an
error rather than receiving arbitrary momentum, rate or time clamps. Empty
chains represent hard incoming lines directly attached to beams. Beam sentinel
coordinates may not follow physical time ordering in extreme test inputs;
they are preserved literally.

## Frozen Fortran comparison

`reference/extract_isr_times.py` verifies the frozen stock JEWEL source hash
and extracts the entire timing block without editing its statements. The test
harness supplies arrays, ancestry, initial configuration, nominal masses and a
fixed random tape. Its PYP stub supports only mode 8 using the original
sqrt(px^2+py^2+pz^2) expression; PYMASS returns the supplied nominal mass, and
PYR consumes the tape. This isolates timing from Pythia6/Pythia8 particle-data
and RNG differences. It is not a whole-interpretpythiaevent comparison.

**1152 cases pass: 46,080 time/position values and 1152 draw counts.** Maximum
absolute difference is zero on this AppleClang/GNU Fortran build. Numerical
acceptance remains rel=2e-12, abs=2e-13, with exact draw-count equality. Exact
floating-point agreement is not promised across platforms.

The matrix crosses ISRSCAT off/on; formation factors 0.25, 1 and 2.5;
ancestry depths (0,0), (1,1), (2,3), (4,1); incoming energies +/-1, +/-100 and +/-1000 GeV; emitted energies +/-1 GeV;
and random draws 1e-12, 0.25, 0.9 and 1-1e-12. Both beam directions, spacelike
incoming momenta, on-shell massless emissions, off-shell emissions and an
adjacent representable mass above the nominal mass are included.
[Machine-readable evidence](validation-isr-times.json) records source,
executable and input hashes.

C++ contracts independently check time orientation, sentinel coordinates,
on-shell draw suppression, off-shell lifetimes, sibling-vertex continuity and
invalid chains/configurations. Both reference-enabled tests pass. The native
build, without Fortran, also passes timing, Pythia8 input and ISR CLI contracts.

```sh
ctest --test-dir build -R '^isr-times-' --output-on-failure
ctest --test-dir build-pythia8 -R '^(isr-times|pythia8-input|pythia8-isr-input)' --output-on-failure
```

## Remaining integration

Map actual Pythia8 backwards spacelike chains, their recoiled copies and emitted
siblings to this input while retaining source provenance and consistent frames.
Then port the non-hard-parton virtuality initialization and evolution policy,
including ISRSCAT and rapidity selection, before enabling ISR in the event
runner. Its existing rejection of ISR snapshots remains intentional until that
integration is verified. Neither this numerical test nor the earlier 3000
Pythia8 snapshots establishes Fortran/native jet-spectrum equivalence.

## Signed-energy correction

The original C++ translation required positive energies/rates. Inspection of
the unedited source shows that this restriction is not in the expression.
The port now preserves finite signed rates: a negative incoming energy gives
a production time later than its end time, and a negative off-shell emitted
energy gives an end time earlier than production. These are algebraic outcomes
of the source, not assertions of physical causality. No absolute value, sign
flip or clamp is introduced. Zero/undefined rates remain errors.

The expanded matrix includes 864 cases with at least one negative-energy
configuration and all original positive-energy cases. All values and draw
counts agree exactly with the reference on this build. Contract tests also
check interval reversal and reject zero energy before random consumption.
The reference-enabled and five related native suites pass.

The [frame diagnostic](validation-isr-energy-frames.md) shows that transporting
to the final event frame does not generally make reconstructed incoming energies
positive. A physical Pythia8-to-JEWEL timing prescription therefore remains
separate from this correction to the literal numerical translation.
