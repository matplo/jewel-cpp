// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "jewel/pythia8_input.hpp"
#include "jewel/pythia8_isr.hpp"
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
struct IsrTimingAttachment {ParticleId native{},source{};std::size_t subsystem{},side{};};
// Attach candidate ISR timing before any final-record kinematic preparation.
// Requires a complete one-to-one map of source final particles to live native
// records. Only production and decay_time change; all other metadata survive.
// Records are transactional; random draws are not rewound on a timing failure.
std::vector<IsrTimingAttachment> attach_pythia_isr_times(ShowerState&,
 const std::map<ParticleId,ParticleId>& input_sources,const HardEventSnapshot&,
 const std::vector<std::array<IsrTimingChain,2>>&,const IsrTimeConfig&,Random&);

}
