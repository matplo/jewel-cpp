# MPI pair virtuality initialization

The C++ FINDVIRTUALITIES translation now supports an explicitly selected MPI
quark/gluon pair through `HardInitializationConfig::mpi_pair`. It retains the
existing transactional state update and RNG behavior. MPI pair discovery,
Pythia8 MPI generation and event-runner MPI integration are not enabled here.

## Preserved source rules

For an active leg with MPIFSR enabled, the initial mass upper bound is

```
Qmax = pT * exp(double(float(0.3)) * abs(y1-y2) / 2) / 2
```

The rapidities use the original record masses and PYP mode-17 semantics.
Both scales are calculated before the pair's rest-frame boost round trip,
as in the source. A status-4 leg or MPIFSR=false sets its bound to zero.
The primary hard pThat scale does not set the MPI quark/gluon bound.

MPI initialization does not obtain colour-opening angles: both limits are -1,
so the first-angle-ordering switch cannot impose the hard-process colour-angle
veto on this pair. The existing mass sampling, two-body momentum adjustment,
kinematic/fraction retries and decay-time assignment are then used.

**TYPE1/TYPE2 are explicit caller inputs.** The Fortran MPI call uses global
sampling-channel variables; the routine does not derive them afresh from the
selected pair flavours. C++ therefore requires `mpi_sampling_channels` (QQ or
GC for each leg) when `mpi_pair=true`. Missing/unsupported channels are rejected.
The port does not silently replace the source semantics with flavour-based
channel inference. A future event adapter must define and record this context.

## Frozen reference validation

The harness invokes the unchanged, hash-verified stock FINDVIRTUALITIES body
with `mpiscat=true`, supplied MPIFSR and explicit TYPE1/TYPE2. Only harness
configuration changed; the extracted source routine was not modified.

**1152 cases and 183168 values pass** at the existing rel=2e-12, abs=2e-13
numerical tolerance, with exact discrete-state and random-draw comparisons.
Maximum absolute difference is 1.3643e-12 (within relative tolerance for those
values). There are 334 cases with retries, maximum 11 trials and 166 draws.

The matrix covers gg, qqbar, qg and antiquark-gluon pairs; independent
MPIFSR and first-angle switches; an inert status-4 leg; massive input records;
boosted frames; and sampling channels both matching and differing from pair
flavours. Pair rapidity differences span 0 to 4.2894, exercising the exponential
scale rather than only common longitudinal boosts. The main hard scale also
varies to verify that it does not replace the MPI prescription.
[Machine-readable evidence](validation-mpi-initialization.json).

Contracts independently check MPIFSR=false, absent colour partners, explicit
channel requirements, independence from primary pThat and transactional state
behavior. The original PPJJ comparison still passes all 288 cases and 45792
values. Five native suites pass after rebuilding without Fortran: input, ISR
CLI, hadronization boundary, hard initialization and shower output.

```sh
ctest --test-dir build -R '^(hard|mpi)-initialization-' --output-on-failure
```

## Remaining scope

Stock `findmpivirtualities` discovers candidate pairs by parent-zero records
and transverse-momentum equality within 1e-4; its source contains a beam-remnant
baryon warning. That discovery loop is not ported or assumed to match Pythia8
subsystem identities. This milestone implements the initialization operation
for an explicitly selected quark/gluon pair. MPI ancestry, pair selection,
sampling-channel context, colour topology, medium evolution and full-event
validation remain to be connected.

This operation is also distinct from ISR timing: source ISR emissions inherit
history from `interpretpythiaevent`, rather than simply being all initialized
as MPI pairs. The unresolved signed-energy/formation-time mapping is not
bypassed by this extension. The full >=1000-jet Fortran/native ratio gate
remains outstanding.

## Updated source-context finding

`findspeclines` sets global TYPE1/TYPE2 from the primary outgoing legs (gluon
maps to GC, otherwise QQ), before the primary `findvirtualities` call and the
later `findmpivirtualities` loop. The future subsystem initializer must carry
that primary context through MPI calls. It must not derive sampling channels
from each MPI pair. The [MPI-enabled parent trace](validation-mpi-scheduling.md)
also resolves the distinction between stable native ancestry and source
scheduling-parent zero. Pair sequencing and full-runner integration remain open.
