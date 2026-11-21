// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "jewel/pythia8_input.hpp"
#include "jewel/shower_state.hpp"
#include "jewel/branching.hpp"
namespace jewel {
// HADROTYPE=0 boundary. Does not mutate the native event or repair colour.
// Rejects unfinished showers, discarded string builds and unconsumed partons.
// Stable IDs/colour/vertices come from native sources, not CLEANUP's stale slots.
std::vector<GeneratorParticle> prepare_vacuum_hadronization(const ShowerState&,
 bool hadronize_recoils=true,bool compress=true);
// Native Pythia8 topology boundary: preserve all final colour tags and explicit
// outgoing junctions (kinds 1/2). No Pythia6 string ordering or colour repair.
std::vector<GeneratorParticle> prepare_native_hadronization(const ShowerState&,
 const std::vector<GeneratorJunction>&,bool hadronize_recoils=true);
// HADROTYPE=1: IDs are local to the ordered output; source_index identifies the
// native input slot, or -1 for artificial endpoints. Does not mutate state.
std::vector<GeneratorParticle> prepare_minimum_hadronization(const ShowerState&,
 const BranchingSampler&,Random&,double max_rapidity,bool hadronize_recoils=true,bool compress=true);
}
