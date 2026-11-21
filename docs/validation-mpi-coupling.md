# MPI coupling and screening audit

**Correction:** the original trace sampled initialization, not production.
The [phase-aware audit](validation-mpi-normalization.md) reproduces the full
reference sample and shows both generators reach 2.326 GeV screening.
Production Fortran MPI uses shifted PDF scales; standard Pythia8 does not.

A logging-only patch to the frozen Pythia6 PYSIGH routine records MPI scale
and coupling evaluations during initialization and one generated JEWEL event.
The diagnostic event HepMC is byte-identical to the uninstrumented baseline.
60 traced evaluations cover MINT(35)=0, 1 and 3. The evidence includes source,
executable, card, log and output hashes.

The source uses MSTP(81)=1 (legacy MPI), MSTP(82)=4, MSTP(32)=8,
MSTP(33)=0 and PARP(34)=1. Its MPI PYALPS calls use one-loop Lambda4=.118 GeV,
with NF between 3 and 5. The common-scale translation remains
alphaSvalue=0.11754434294459773, order=1, nfmax=5 and no CMW rescaling.
All 60 compiled Pythia8 AlphaStrong values match the traced Fortran values
within rel=2e-12 / abs=2e-13; maximum absolute difference 8.33e-17.
The previous native MPI coupling is 11–25% larger at those same scales.

The source renormalization scale is pT²+pT0². Its screening law is
pT0 = 2 GeV * (sqrt(s)/1800 GeV)^0.25, giving **2.5845670224 GeV** at 5020 GeV.
Pythia8 uses the same renormalization-scale form, but Tune 14's defaults give
2.1227070758 GeV. The earlier unshifted-PDF-scale observations were from the second initializer,
not production. The phase-aware trace confirms production Fortran PDF
Q² = pT²+pT0², while standard Pythia8 uses pT².

## Explicit native controls

```sh
--mpi-alpha-s 0.11754434294459773 --mpi-alpha-s-order 1 \
--mpi-screening 2 1800 .25
```

The screening arguments are pT0Ref [GeV], ecmRef [GeV], and the energy exponent.
They select Pythia's power-law parametrization. Defaults are unchanged unless
requested. Value/order pairing, finite values, Pythia's parameter ranges,
duplicate options and missing arguments are checked. Requested settings are
verified after initialization and recorded in the run header. The extended
coupling CLI test passes 48 invalid-input checks and both three-event runner
cases; hard-range and Pythia input regressions also pass.

**Settings preservation does not prove effective pT0 preservation.** In the
1500-attempt run with the source-derived inputs, Pythia8 initialization reports:

- pT0=2.58 GeV: sigmaInteraction=51.84 mb, rejected.
- pT0=2.33 GeV: sigmaInteraction=64.53 mb, accepted.

The installed source requires sigmaInt > 1.1 sigmaND and multiplies pT0 by 0.9
on failure. One observed reduction therefore implies an effective
**2.3261103202 GeV**, despite unchanged requested settings. This value is inferred
from the trace and source algorithm; the log itself prints only two decimals.
The full Fortran initialization also reduces pT0 to this value. The run is
**not fully MPI-matched** because PDF scales and other MPI conventions differ. No normalization or cross-section
parameter was adjusted to force acceptance of the requested screening scale.

Other differences remain: legacy versus interleaved MPI, double-Gaussian versus
Tune 14 exponential-power overlap, PDF treatment and event-history conventions.
The subsequent [normalization audit](validation-mpi-normalization.md)
records integrated rates, nondiffractive normalization and production PDF scales.

## Fresh jet comparison

All 1500 native attempts succeed. The common hepyy/cppyy analysis selects
**1981 native jets versus 1357 Fortran jets**. Cuts, bins and attempted-event
normalization are unchanged. All six observable gates remain not passed.

| Jet pT (GeV) | Native / Fortran |
|---|---:|
| 10–15 | 1.108 |
| 15–20 | 0.903 |
| 20–30 | 0.956 |
| 30–40 | 0.858 |
| 40–60 | 0.942 |
| 60–80 | 1.222 |

Topology and energy-scaled momentum checks pass. Twenty stricter component
excursions are preserved in the audit, maximum 2.46e-7 GeV. The sample changes
MPI coupling and requested screening together; it cannot isolate either effect.

- [Interactive comparison with historical pairs](mpi-policy-vacuum-comparison.html)
- [Coupling/scale values and provenance](validation-mpi-coupling.json)
- [Requested versus effective screening](validation-mpi-effective-screening.json)
- [Runner-option validation](validation-mpi-input.json)
- [Topology audit](validation-mpi-policy-topology.json)

For reproduction, `reference/mpi_coupling_trace.patch` adds one logging call to
a temporary Pythia6 source copy; link `reference/mpi_coupling_trace.f90` with it.
Use the same reference cards with NEVENT=1, retaining separate baseline and
instrumented directories. `reference/mpi_coupling_probe.cpp` initializes the
preceding native configuration and reads the traced Q² values from stdin to
compare current and translated AlphaStrong evaluations. Its XML path/settings
record this installed diagnostic environment. No installed recipe or Fortran
physics routine was modified.
