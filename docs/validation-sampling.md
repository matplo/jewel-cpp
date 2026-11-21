# Medium sampling validation

Recorded on 2026-11-20 (America/Los_Angeles), macOS arm64, AppleClang 21.0.0
and GNU Fortran 16.2.0, Release builds. Six CTest suites pass.

181 replay cases compare C++ with unchanged stock MEDNEXTEVT, PICKVTX and
GETSCATTERER. Every case requires exact random-draw counts and numerical
agreement with relative tolerance 2e-12 / absolute tolerance 2e-13.
Scatterer mass-shell consistency is checked separately.

| Component | Cases | Maximum absolute difference | Maximum draws |
|---|---:|---:|---:|
| centrality | 20 | 0 | 1 |
| vertex | 50 | 0 | 96 |
| scatterer | 108 | 1.7764e-15 | 341 |
| cold-fallback | 3 | 0 | 1 |

The tests cover centrality ranges 0–10%, 10–50%, 50–90%, 0–100%; production
vertices at b=0,3,7,12,18 fm with forced first-candidate rejection; NF=0,3,5;
both species selection branches; boost on/off; positive/negative rapidity;
three proper times; and the stock cold-medium fallback. Recorded source and
executable hashes are in [validation-sampling.json](validation-sampling.json).

The existing medium differential test now also covers GETTEMPMAX, GETMDMAX,
GETMDMIN, GETMSMAX, GETNATMDMIN, GETLTIMEMAX, GETNEFFMAX, AVSCATCEN and
MAXSCATCEN. All earlier kernel/table tests still pass. C++ contracts cover
invalid centrality ranges, absent overlap, finite replay exhaustion, and
observable cold-medium fallback status.

This establishes medium-sampling replay, not shower/event or jet agreement.
