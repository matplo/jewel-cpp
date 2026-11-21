# JEWEL PI0DEC control

`jewel-run-events ... --pi0-decay on|off` maps the custom JEWEL PI0DEC option to
Pythia8 `111:mayDecay`. Default is on, preserving the preceding native behavior.
The selected setting appears in the run header's hadronizer_settings list.
The switch works independently of the mapped fragmentation profile. It is
rejected by the parton-only runner, and repeated or malformed values fail before
output creation. Other particle lifetimes retain the existing 10 mm cutoff.

The source INITPYTHIA sets MDCY(PYCOMP(111),1) from PI0DEC after selecting
MSTJ(22)=2. An independent probe linked to the frozen Pythia6 object verifies
that a 1 GeV pion remains the sole final particle with decay off and produces
two final daughters with no surviving pion with decay on. Final energy is
1 GeV within 1e-12. [Probe](../reference/pi0_decay_probe.f90) and
[source/object hashes and results](validation-pi0-decay.json).

The native single-pion contract checks both modes and conserved total energy.
Three native vacuum hadron events per setting confirm that the public runner
emits final pions with decay off and none with decay on. Invalid value, repeated
option and parton-only misuse checks pass. Fragmentation, runner-decay and the
existing experimental ISR/MPI contracts all pass.

This establishes the switch's meaning and integration, not identical Pythia6/8
branching fractions, particle-data tables or decay distributions. The existing
jet comparison used PI0DEC on in both engines; this new optional control is not
a fitted explanation for the unresolved spectrum/substructure differences.

## Next fragmentation audit item

The explicit INITPYTHIA tune coefficients were already mapped. An additional
source default still needs a verified mapping: PARJ(45)=0.5 in the frozen Pythia6
block data, used by PYZDIS in the diquark-dependent Lund exponents. Installed
Pythia8 8.317 ZSelection.xml documents aExtraDiquark=0.97. Its useOldAExtra flag
also distinguishes an older implementation error fixed in 8.315. Effective
settings and both formula implementations must be checked before claiming an
exact mapping; no such tune change was made in this update.
