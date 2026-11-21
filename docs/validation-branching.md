# Branch-candidate sampling validation

Recorded 2026-11-20 (America/Los_Angeles), macOS arm64, AppleClang 21.0.0
and GNU Fortran 16.2.0, Release.

GETMASS and GENERATEZ are compared with unchanged stock routine bodies using
identical finite random tapes. Every candidate mass, assigned fraction, flavour
branch and random-draw count is checked. The reference initializes output
arguments to sentinels so unchanged initial-state outputs are observable.

| Sampler | Cases | Maximum absolute difference | Maximum draws |
|---|---:|---:|---:|
| final-mass | 1728 | 0 | 135 |
| initial-mass | 576 | 0 | 46 |
| fraction | 864 | 0 | 21 |

All 3,168 cases match exactly, including draw counts. The suite spans NF=3/5,
Q0=1/1.5, shower Lambda=0.2/0.25, PTFAC=1/2, ANGORD=0–5, final- and
initial-state sampling, positive and disabled final-state angular vetoes,
kinematic caps, narrow intervals, empty phase space and QQ/GG/QG fraction
sampling. Floating comparison tolerances remain relative 2e-12 / absolute 2e-13;
draw counts and flavour choices require exact equality.

## Explicit stock-domain handling

Accepted INS=true GETMASS leaves ZDEC and QQBARDEC untouched. The C++ result
uses optional values for these unassigned outputs. Zero-candidate and narrow
interval branches retain the source's explicit assignments.

For ANGORD=3/4, the source computes an angular expression using local Z even
when INS=true, where Z was never set. GETNEWMASS's stock calls pass theta=-1,
so the result of that expression does not affect observable output. C++ skips
this unused expression for initial-state sampling; all six modes are replay
validated with theta=-1. Positive-theta initial-state calls in these modes are
rejected explicitly and excluded from the parity claim because the reference
behavior is undefined. This does not remove a stock call-site feature.

The extraction tool now recognizes GENERATEZ's labelled END and validates that
each selected span contains exactly one routine, preventing accidental inclusion
of a following routine. C++ contracts additionally check optional outputs,
no-draw early returns, exhausted tapes and invalid kinematic domains.

[Detailed replay results and source/executable hashes](validation-branching.json).
This is candidate-sampling parity, not event-tree construction or full shower
validation. Event state, kinematics, transport scheduling, colour/recoils and
Pythia8 integration remain outstanding.
