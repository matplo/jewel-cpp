# External PDF and hard-coupling configuration

Recorded 2026-11-21. The native Pythia8 input adapter can now select an installed
LHAPDF6 set/member and an explicit hard-process alpha_s policy. Defaults remain
internal PDF option 13 and tune 14; existing ensemble results are unchanged.

## Configuration

Both `jewel-pythia8-input` and the native parton/hadron runners accept:

```sh
--lhapdf CT14nlo/0 --hard-alpha-s 0.118 --hard-alpha-s-order 2
```

The C++ API exposes `PythiaInputConfig::external_pdf`, `hard_alpha_s` and
`hard_alpha_s_order`. An external set requires both coupling fields. Explicit
coupling without an external set is also supported, but value and order must
always be supplied together. The value must be finite and within Pythia's
supported [0.06, 0.25] range; the Pythia running-order selector is 0–3.
Set names use `SET/MEMBER`, with a nonnegative integer member and no whitespace.
Malformed names, incomplete policies, out-of-range orders and repeated CLI
options are rejected.

Settings are applied after the tune, recorded in the run header, and checked
again after Pythia initialization. The adapter does not silently substitute a
PDF or accept an initialization failure. It does not change JEWEL's independent
shower/scattering Lambda values, the hadronizer, or ISR/MPI switches.

The LHAPDF plugin must be discoverable by the native dynamic loader, and its PDF
files must be installed. For the development macOS environment this was done by
setting `DYLD_LIBRARY_PATH` to the Pythia8 library directory and
`LHAPDF_DATA_PATH` to the LHAPDF data directory. Use the appropriate loader
configuration on other platforms. The optional plugin is a runtime requirement
for external PDFs; it is not needed by the default internal-PDF path.

```sh
# With those library/data paths configured:
build-pythia8/jewel-pythia8-input hard.jsonl 100 2601001 \
  --lhapdf CT14nlo/0 --hard-alpha-s 0.118 --hard-alpha-s-order 2
build-pythia8/jewel-run-events medium.jsonl 20 2601001 medium \
  --lhapdf CT14nlo/0 --hard-alpha-s 0.118 --hard-alpha-s-order 2 \
  --jewel-fragmentation
```

## Observed checks

The installed CT14nlo metadata identifies set index 13100, alpha_s(MZ)=0.118
and `AlphaS_OrderQCD=1`. That LHAPDF order index denotes NLO, whereas Pythia8's
running-order selector **2** denotes two-loop running. The indices must not be
copied numerically from one API into the other.

- Default input contracts still pass, including ancestry and deterministic
  event generation. New negative cases reject missing coupling policy,
  embedded setting text and unsupported running order.
- Two independent CT14nlo-configured engines produce 100 identical hard-event
  snapshots. All 100 CLI attempts also return events, with finite momenta and
  maximum full-event momentum residual 3.3652e-11 GeV.
- The final CLI executable reproduces the initial CT14nlo sample byte-for-byte.
- With mapped JEWEL fragmentation, native end-to-end smoke samples accept 20/20
  vacuum events and 17/20 medium events. Full record, failure-weight and
  hadronization-conservation audits pass; failed attempts remain in output.

[Hard-input results, settings and executable/plugin/PDF-grid hashes](validation-external-pdf.json) ·
[Native end-to-end audit](validation-external-pdf-native.json).

The standalone integration executable is:

```sh
build-pythia8/pythia8-external-pdf-test CT14nlo/0
```

To register it with CTest, configure
`-DJEWEL_TEST_EXTERNAL_PDF=CT14nlo/0` and run CTest with the same plugin/data
paths. It is opt-in because the ordinary native build does not require an
external PDF installation. The internal-PDF contract remains registered normally.

## Limits on the Fortran comparison

This removes the inability to request the same PDF member used by the inspected
Fortran setup. It does not prove the same hard-generation distribution. In
particular, the modified Pythia6 initialization derives its coupling through
its own PDF metadata/fallback logic; its actual running and scale prescription
still require a matched benchmark. The explicit two-loop choice above follows
CT14nlo metadata and is not presented as a verified reconstruction of that
legacy prescription.

ISR history handling, sampled geometry, other tune defaults, cascade numerical
failures and the full-generator jet/substructure comparison remain open. The
large native report still uses its recorded internal NNPDF/Monash settings.
