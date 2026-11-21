# Native/Fortran jet comparison report

The [interactive report](jet-comparison.html) now plots native and Fortran
weighted distributions and their ratios. It supports pT, mass, girth, pTD,
Soft Drop mass and zg, a 0.9–1.1 band, event-level error propagation, a bin
table, CSV download, and full selection/run provenance. Repeat `--pair` to
include more settings in the sample selector.

```sh
python3 tools/compare_jet_samples.py docs/common-event-analysis.json docs/jet-comparison.json --pair 0:2
python3 tools/render_jet_comparison.py docs/jet-comparison.json docs/jet-comparison.html
python3 tests/jet_comparison.py
```

Inputs are produced by the existing common hepyy/cppyy FastJet analysis. The
comparison does not rerun clustering, rescale yields, renormalize shapes or
merge sparse bins. Both samples must have the same observables and bin edges.

For independent samples, the ratio error is
`sqrt((sigma_native/reference)^2 + (native*sigma_reference/reference^2)^2)`.
It remains defined at zero numerator. A nonpositive reference produces an
undefined ratio; negative weighted numerator yields are also undefined in
this implementation. All undefined bins remain visible and prevent a
numerical pass. Event standard errors are a large-sample approximation, not
an exact low-count confidence interval. Correlated/paired event ensembles
would require a covariance term and are not supported here.

A numerical pass requires at least 1,000 selected jets in **each** sample and
all displayed central ratios within 10%. Uncertainties do not widen that
threshold. This is separate from physics agreement: matched settings and a
common normalization still need a configuration audit. The report deliberately
cannot certify that audit merely from equal histogram values. Underflow,
overflow and undefined-observable accounting remain in each sample's provenance;
they are not additional in-range bins.

## Current evidence and limits

The checked-in example compares the historical native vacuum sample (1,081 jets)
with the three-event Fortran smoke sample (8 jets). Native denominators count
attempts, whereas imported Fortran denominators count written events. Their
hard cuts and ISR policies also differ. Consequently these ratios are diagnostic
only and **do not satisfy the requested matched 1,000-jet validation**.

Six contract tests pass, covering ratio thresholds, independent uncertainties,
zero yields, sample counts, invalid inputs and visible denominator mismatch.
The generated JavaScript passes syntax checking. A DOM smoke test exercised all
six observable selections, table updates, CSV exports and validation notices.
The rendered layout has not yet been visually inspected in a browser.
