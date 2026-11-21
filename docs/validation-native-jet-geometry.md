# Native geometry association with final jets

`tools/analyze_native_events.py` now consumes the native runner's optional
`geometry-v1-native` records. Jet finding still uses hepyy/cppyy FastJet. The
association delegates to `hepyy_utils.jewel.matching.match_hard_legs`, with a
maximum delta R of 0.3 in pseudorapidity and azimuth.

All positive-pT inclusive anti-kt R=0.4 jets are clustered and ranked across the
full phase space. The two hard legs are then assigned to distinct jets,
maximizing the number of matches and minimizing their total distance. Only
after assignment are pT >= 10 GeV and |eta| < 2 applied. The existing visible
final-particle selection, subtraction/projection policy and substructure
calculation remain unchanged.

Each analyzed event with geometry contains `hard_leg_matches`, including
matches outside analysis acceptance and a `jet_passes_analysis` flag. A selected
jet records its full-phase-space `jet_id`/`jet_rank`, `matched_leg_id`, delta R,
ambiguity flag and associated geometry. An unmatched selected jet has null
geometry and null matching fields. An unmatched **leg** retains its own length
estimate in the leg table; that does not assign a length to any jet.

The source PDG, momentum and production snapshot remain in the generated event;
the geometry record identifies the shower-start particle, not a compressed
final particle index. The reported length remains an initial-direction
straight-line estimate, not the actual path traversed by the final jet.
Ambiguous matches are retained and flagged, not silently excluded.

## Integration evidence

A 20-attempt medium sample at seed 2601001 produced 18 accepted events and two
recorded cascade failures. Common analysis found:

| Quantity | Count |
|---|---:|
| Hard legs | 36 |
| Matched legs across full phase space | 34 |
| Matched jets passing analysis cuts | 5 |
| Matched jets outside analysis cuts | 29 |
| Selected jets | 12 |
| Unmatched selected jets | 7 |
| Ambiguous legs | 5 |

The previous analysis was run independently on the same generated file. Every
pre-existing sample field, event field and selected-jet observable agrees
exactly after removing the newly added matching metadata. In particular all six
weighted distributions and their uncertainties are unchanged. The integration
audit checks unique jet assignments, selection flags, every geometry attachment,
and null geometry on unmatched jets.

Three regression tests cover out-of-acceptance candidates, low-pT matched jets,
empty jet collections, invalid geometry and duplicate hard-leg identities.
The out-of-acceptance fixture would assign a different selected jet if matching
were incorrectly delayed until after cuts. Tests and integration audit pass.
[Hashes, counts and all 12 selected-jet records](validation-native-jet-geometry.json).
The analysis provenance now hashes the shared hepyy-utils matcher as well.

```sh
build-pythia8/jewel-run-events medium.jsonl 20 2601001 medium --geometry-step .02
# Run in the environment containing hepyy[cppyy,hepbundle] and hepyy-utils.
python tools/analyze_native_events.py medium.jsonl --output analysis.json
python tests/native_geometry_analysis.py
```

The analysis JSON retains event-level matches; sample-level
`geometry_accounting` remains available when a browser strips event records.
This is native output support in the standalone project's common analysis,
not a published hepyy-utils importer. It does not resolve the recorded cascade
failures or establish matched Fortran/native jet physics.
