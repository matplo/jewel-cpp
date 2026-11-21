# Central Pb–Pb geometry pilot with pThat bias 5

This pilot uses the optical Pb–Pb medium at 5.02 TeV, 0–10% centrality,
A=208, a Woods–Saxon profile and the pp hard isospin channel in both
generators. The medium uses tau0=0.4 fm, T0=0.59 GeV and Tc=0.17 GeV.
The hard window is 20–100 GeV with CT14nlo/0. The medium runs enable
scattering at scale 1 and retain recoils without thermal subtraction.
The results use the pp hard channel with free-proton PDFs.

JEWEL uses `WEIGHTED T` and `WEXPO 5`. C++ uses `--pthat-bias 5` and records
Pythia8's compensating event weight directly. Analysis clusters with hepyy
and FastJet anti-kT R=0.4, then selects pT ≥ 10 GeV and |η| < 2.
Each yield is the sum of jet weights divided by the accepted hard-event
weight. The ratios use no nuclear overlap or cross-section scale factor.
Errors use event-level weighted moments and assume independent samples
within each ratio. The old and new Fortran ratios reuse seeds and share
their vacuum denominator, so the displayed errors apply to individual
ratios.

## First nonfinite cascade state

The reproducer uses NJOB 2600421 and the first attempted event. A trace
around candidate kinematics in `DOKINEMATICS` recorded
`p42 = 0`, `m42 = 0.61630762251099258 GeV²` and
`p32 = −1.4510723636004724 × 10⁻¹⁸ GeV²`. The fallback computes
`E4new = sqrt(p42 + m42)` and then recomputes the equal projectile momentum
squared as `E4new**2 - m42` at line 6064. The subtraction after the rounded
square root yields the small negative value. The square root at line 6090
turns it into a NaN in the x momentum of outgoing projectile line 280.
Incoming projectile line 226 was finite. Later coherent transfers inherit the corrupted
momentum. After the call returns to `MAKEBRANCH`, line 290 has nonfinite
momentum, production coordinates and formation time.

The [Fortran patch](../reference/fix-medium-reshuffle.patch) assigns
`p32 = p42` in this fallback, using the common squared momentum of the
two outgoing particles in their centre-of-mass frame. The one-event
reproducer completes with the patch and the cascade probes enabled.

## Same-seed rerun

The patched medium sample repeats both original seed shards, with 5,500
attempts at NJOB 2600421 and 2,000 at NJOB 2604. The run uses the
compiler-corrected JEWEL 2.6.0 core with the reshuffle patch and the old
discard guard disabled. Both HepMC files passed the attempt ledger and
finite-momentum validation. The C++ medium and vacuum and Fortran vacuum
samples are the same as in the previous report. The old guarded Fortran
medium discarded 2,047 events for nonfinite cascade states.

| Sample | Attempts | Accepted | Selected jets | Effective weighted events |
|---|---:|---:|---:|---:|
| C++ medium | 5,500 | 5,493 | 14,431 | 776 |
| C++ vacuum | 5,500 | 5,500 | 11,825 | 777 |
| Fortran medium, reshuffle fix | 7,500 | 7,409 | 19,191 | 1,053 |
| Fortran medium, previous guard | 7,500 | 5,400 | 12,889 | 802 |
| Fortran vacuum | 5,500 | 5,482 | 10,984 | 812 |

## Medium/vacuum ratios

The main Fortran pilot shard used NJOB 2600421. JEWEL computes the Pythia6
seed as NJOB × 1,000 using a signed 32-bit integer, so that value wrapped.
The larger comparison uses verified positive Pythia6 seeds. The table below
records the original pilot ratios.

| Jet pT (GeV) | C++ | Fortran, previous guard | Fortran, reshuffle fix |
|---|---:|---:|---:|
| 10–20 | 1.193 ± 0.062 | 1.111 ± 0.056 | 1.162 ± 0.052 |
| 20–30 | 0.675 ± 0.048 | 0.602 ± 0.043 | 0.623 ± 0.040 |
| 30–40 | 0.653 ± 0.066 | 0.673 ± 0.068 | 0.637 ± 0.056 |
| 40–60 | 0.629 ± 0.052 | 0.658 ± 0.065 | 0.642 ± 0.062 |
| 60–80 | 0.622 ± 0.052 | 0.747 ± 0.081 | 0.791 ± 0.078 |
| 80–100 | 0.623 ± 0.065 | 0.661 ± 0.075 | 0.642 ± 0.071 |

The [interactive report](central-pbpb-bias5.html) shows the larger
comparison, a pilot overlay, and weighted yields for pT, mass, groomed mass,
pTD, girth and zg. Substructure is available inclusively and in the
20–40, 40–60 and 60–100 GeV jet pT slices. Its ratios retain the change
in total yield within each slice. The [JSON](central-pbpb-bias5.json)
contains the extended results, run settings and checksums for each shard.
