# MPI double-Gaussian overlap

The native hard-input and event runners now accept
`--mpi-double-gaussian FRACTION RADIUS`. The JEWEL reference values are `.5 .4`.
This selects Pythia8 `MultipartonInteractions:bProfile=2`, sets the core matter
fraction and inner/outer radius ratio after Tune 14, and verifies all three
settings after initialization. The default remains unchanged. These parameters
describe proton MPI overlap, not the expanding nuclear medium.

The frozen Pythia6 `PYMULT` expression for MSTP(82)=4 and the Pythia8 8.317
bProfile=2 expression have identical normalization 1/(2*pi), weights
(1-f)^2, 2*f*(1-f), f^2, and squared widths 1, (1+r^2)/2, r^2.
Both cap the negative exponential argument at 50. No impact-parameter rescaling
is required for these expressions. Matching the expression does not prove that
the full impact-parameter sampling and MPI evolution agree.

`tools/audit_mpi_overlap.py` extracts the coefficient assignments and overlap
expressions unchanged from the two source files, compiles independent probes,
and compares 4,020 points: five fractions, four radii and 201 impact parameters
from 0 to 10. All pass relative tolerance 2e-13 with zero absolute tolerance;
maximum relative difference is 7.35493e-15 and absolute difference 4.44089e-16.
The JSON records full-source hashes, extracted probe code and the query hash.
This is expression-level validation, not a full-generator reference test.

The CLI integration test passes both three-event runners, the alternate coupling
policy and 66 invalid inputs. Hard-range and Pythia8-input contracts also pass.
The native build was rebuilt successfully.

## Event comparison

A 1,500-attempt vacuum sample uses the preceding MPI coupling/screening policy
with only the explicit double-Gaussian overlap added. It uses the standard
installed Pythia8 library, not the isolated shifted-PDF diagnostic library.
All 1,500 attempts succeeded. Common hepyy jet analysis selects 2,071 native jets
and 1,357 frozen-reference jets, with yields normalized per original attempt.
No fit, shape rescaling, event removal or tolerance adjustment was applied.

The pT ratios in the populated 10–15, 15–20, 20–30, 30–40, 40–60 and 60–80 GeV
bins are 1.14178, 1.01215, 0.93396, 0.89216, 1.28986 and 0.72222.
All six observable gates remain not-passed; empty higher bins remain undefined.
Configuration parity remains unestablished. The PDF factorization scale,
non-diffractive cross section and legacy saved-grid normalization still differ
or need further investigation. The [saved-state audit](validation-mpi-saved-state.md)
confirms that the reference retains tables built before its screening reduction. This source-derived profile is retained as an
explicit option, not promoted to a default on the strength of jet ratios.

The energy-scaled topology gate passes, with maximum momentum-component
residual 4.25389e-7 GeV. Eleven stricter component diagnostics remain recorded;
they are not erased by the looser energy-scaled acceptance.

Browse the [interactive comparison](mpi-overlap-vacuum-comparison.html), including
historical settings, or inspect the [overlap audit](validation-mpi-overlap.json),
[CLI evidence](validation-mpi-overlap-input.json) and
[topology evidence](validation-mpi-overlap-topology.json).

Reproduce the expression audit with:

```sh
python3 tools/audit_mpi_overlap.py /path/to/pythia6425mod-lhapdf6.f \
  /path/to/pythia8/src/MultipartonInteractions.cc overlap.json
```

The tool requires C++ and `/opt/homebrew/bin/gfortran` on this validation host.
