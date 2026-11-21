# Inactive colour-octet onium MPI input

The translated-coupling sample failed at attempt 511 because MPI subsystem 2
contains PDG 9942003, mass 3.29692 GeV and colour tags 107/108. This is a Pythia8
colour-octet charmonium precursor, not a colourless hadron or an ordinary gluon.
The previous native initialization rejected all coloured inactive nonpartons.

The explicit native inactive-time policy now also admits the Pythia8 octet-onium
ID interval [9940000, 9960000), with status 4 and two positive distinct colour
tags. It preserves the mass, tags and inactive status. Primary hard pairs,
active octets, malformed colour pairs and arbitrary coloured hadrons remain
rejected. This does not turn the precursor into an active JEWEL gluon shower.

The local Pythia8 8.317 ParticleDataEntry::isOctetHadron defines this ID interval.
HadronLevel::decayOctetOnia decays these precursors to singlet plus gluon and
assigns the parent octet tags to the gluon before string fragmentation. The
existing native hadronization adapter performs this step successfully.

## Verification

- 1,440 octet initialization cases, 227,232 finite values: all pass against
  unchanged JEWEL 2.6.0 FINDVIRTUALITIES at rel=2e-12 / abs=2e-13; maximum
  absolute difference 1.8189894035458565e-12. Draw counts and discrete fields
  agree. The 1,728 inactive-time fields are checked separately: source positive
  infinity versus the documented native finite placeholder, not numerical parity.
- Original primary and colourless-passive differential checks pass. Native
  contracts check mass/colour preservation and six invalid configurations.
  An initial test-fixture colour-maximum error was corrected; the rerun passes.
- Subsystem-initialization and coupling-CLI integration checks pass.
- The same 511-attempt production run now accepts 510 events. Attempt 511
  completes evolution and hadronization, retaining the precursor mass and tags
  at the hadronization boundary, with only colourless final particles.
- Every record for attempts 1–510 matches the preceding sample exactly.
  Attempt 385 remains an ISR-frame failure. Accepted-event topology and
  energy-scaled momentum checks pass; the strict component diagnostic retains
  its three excursions (maximum component residual 2.08992040433547e-7 GeV).

```sh
ctest --test-dir build --output-on-failure \
  -R '^hard-initialization-octets-differential$'
```

Evidence: [differential results](validation-mpi-octets.json),
[recovered sample](validation-mpi-octets-recovery.json),
[unchanged prior attempts](validation-mpi-octets-identity.json),
[topology audit](validation-mpi-octets-audit.json).

This verifies retained-mass initialization and native hadronization of the
precursor. It does not establish equivalence of Pythia6/Pythia8 quarkonium
production or in-medium quarkonium interactions. The earlier 1500-attempt jet
comparison remains historical; its ratios have not been recomputed here.
ISR-frame failures and the full-generator 10% target remain unresolved.
