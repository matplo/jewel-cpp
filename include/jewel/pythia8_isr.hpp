// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "jewel/pythia8_input.hpp"
namespace jewel {
struct IsrBranchMapping {
 ParticleId mother{},daughter{},emitted{},emitted_final{};
 // Mother minus emitted momentum in this branching's recorded recoil frame.
 // This is NOT the on-shell bookkeeping daughter's stored momentum.
 // A spacelike vector can have negative energy in this frame; retain its sign.
 Momentum spacelike_daughter;
 double virtuality_squared{};
};
struct IsrIncomingChain {
 ParticleId hard_incoming{},beam{};
 std::vector<ParticleId> backward_lines; // hard root through last status-61 line
 std::vector<IsrBranchMapping> branches; // hard-to-beam order, skipping recoil copies
};
// Preserve stage-local frames. Connecting these stages to one formation-time
// chain requires further recoil/frame mapping; do not feed the result directly
// into assign_isr_times as if all momenta belonged to one shower frame.
std::array<IsrIncomingChain,2> map_pythia_isr_ancestry(const HardEventSnapshot&);
}
