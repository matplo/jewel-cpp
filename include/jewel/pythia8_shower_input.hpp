// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "jewel/pythia8_input.hpp"
#include "jewel/hard_initialization.hpp"
#include <map>
namespace jewel {
struct PreparedDijet {
 ShowerState state{-1e6};
 ParticleId scratch{},hard_process{};
 std::array<ParticleId,2> hard{};
 std::map<ParticleId,ParticleId> input_sources;
 HardInitializationResult initialization;
 double weight{};
};
// Reduced ISR/MPI-off adapter: only final hard copies evolve; remnants remain
// in the record with status 4. An inert process anchor distinguishes the hard
// scattering from a shower splitting while permitting coherence of later daughters.
// pt_hat comes from the snapshot. First-angle ordering needs a full mapped
// hard ancestry and is rejected here; it remains supported by the core routine.
PreparedDijet prepare_pythia_dijet(const HardEventSnapshot&,HardInitializationConfig,
 const BranchingSampler&,Random&);
}
