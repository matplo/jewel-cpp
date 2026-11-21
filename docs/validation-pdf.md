# Partonic PDF validation

Recorded 2026-11-20 (America/Los_Angeles), macOS arm64, AppleClang 21.0.0
and GNU Fortran 16.2.0, Release. All 12 reference-enabled CTest suites and
all five C++-only contract suites pass.

The suite compares unchanged GETPDF, GETPDFXINTEXACT, PDFINT and GETPDFXINT
against C++ using the previously validated initial-state Sudakov tables.
The reference harness dispatches stock DERIV expressions for modes 7–10
and 21–24. It does not substitute beam PDFs or a different evolution model.

| Comparison | Scalars | Maximum absolute error |
|---|---:|---:|
| table | 20000 | 0 |
| density | 960 | 0 |
| exact-integral | 80 | 0 |
| table-lookup | 96 | 0 |

Four configurations cover NF=0/3/5, different Q0/Lambda settings, medium boost
off/on, SCALEFACM=1/2 and EMAX up to 6,500 GeV. Each compares every entry of
all four 1,000-node tables. Density probes include both endpoints, near-endpoint
fractions and threshold scales. Integral lookups include extrapolation below
and above the tabulated scale range. Relative tolerance is 2e-12 and absolute
tolerance 2e-13; all supported-domain comparisons are exact on this build.

## Explicit undefined-domain comparison

With NF=3, Q0=1, Lambda=0.2, EMAX=50, boost off, and QQ x=0, Q=100,
the stock reference returns NaN after extrapolation drives the Sudakov
denominator to zero. C++ throws `zero Sudakov denominator`. The test verifies
both outcomes separately. Direct density/integration parity probes stay within
the configured Sudakov support; this undefined case is not silently counted
as agreement or removed from the record. The integral-table extrapolation API
retains its independent stock behavior.

[Machine-readable comparison and provenance](validation-pdf.json).
Full scattering rates, shower evolution, Pythia8 integration and jet-level
acceptance remain separate unfinished gates.
