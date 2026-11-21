# Timing surviving ISR emissions

`assign_isr_emission_times` in `include/jewel/isr_times.hpp` returns timing
records for emitted siblings in the supplied backward chain order. For a
nonempty chain it uses the existing literal `assign_isr_times` implementation,
including signed incoming energies, and returns its emitted records.

For an empty chain, it validates the finite inputs, positive formation factor
and nonzero hard energy, consumes exactly one valid random draw, and returns an
empty vector. It accepts zero recorded virtuality in this case. This matches
the surviving output and RNG consumption of stock JEWEL's direct-beam path:
the temporary incoming time is not retained after record rebuilding. It does
not assign a finite substitute time or modify the momentum/virtuality.

The original full-record timing API retains its zero-virtuality rejection.
The new interface is for surviving emitted lines, not for consumers requiring
all intermediate incoming coordinates. Empty chains must be established by
ancestry mapping, never inferred by dropping an invalid or unmapped branch.
The native event runner still rejects ISR-enabled snapshots until the complete
history, virtuality, frame and evolution connection is implemented.

## Evidence

The [actual stock record trace](validation-stock-isr-times.md) confirms that
135 nonfinite direct-beam hard incoming records disappear before the cascade
in a 1,000-event sample. Both traced stages have 18,109 finite records, and
instrumentation preserves the complete event file and final RNG state.

The new `isr-emission-times-differential` suite compares against the unchanged,
hash-verified Fortran timing block across 64 cases: both scattering settings,
chain depths `(0,0)`, `(0,1)`, `(2,0)` and `(1,3)`, signed incoming energies,
and four random values including near-endpoint values. Direct-beam hard lines
have zero mass. Branched lines include both on-shell and off-shell emissions.

- All 560 emitted time/position values agree exactly on this platform.
- All 64 random-count comparisons agree exactly.
- 256 nonfinite reference values belong to discarded incoming records; these
  are explicitly excluded from the emitted-output comparison and counted.
- The C++ projection emits no incoming timing records.
- A contract verifies the direct-beam call leaves the next random draw intact
  and rejects an invalid formation factor before drawing.
- The existing 1,152-case full-record timing comparison and timing contracts
  also pass. All three selected CTest suites pass.

[Machine-readable results](validation-isr-emission-times.json) record executable,
reference-block, test and input hashes. This is timing-interface validation,
not a matched full-event or jet-observable comparison.

```sh
cmake --build build -j 4
ctest --test-dir build --output-on-failure \
  -R '^isr-(emission-times|times)-(contracts|differential)$'
```
