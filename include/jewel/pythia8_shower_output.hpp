// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "jewel/pythia8_input.hpp"
#include "jewel/shower_state.hpp"
namespace jewel {
// HADROTYPE=0 boundary. Does not mutate the native event or repair colour.
// Rejects unfinished showers, discarded string builds and unconsumed partons.
// Stable IDs/colour/vertices come from native sources, not CLEANUP's stale slots.
std::vector<GeneratorParticle> prepare_vacuum_hadronization(const ShowerState&,
 bool hadronize_recoils=true,bool compress=true);
}
