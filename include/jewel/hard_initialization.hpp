// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "jewel/branching.hpp"
#include "jewel/shower_state.hpp"
namespace jewel {
struct HardInitializationConfig {
 double pt_hat{},x{},y{},formation_factor{1},lifetime{20};
 bool first_angle_ordering{};
 std::size_t maximum_trials{1000000};
};
struct HardInitializationResult {std::size_t trials{};std::array<double,2> colour_angles{};};
// GETTHETAM, using source scheduling links for the partner's parent slot.
double colour_opening_angle(const ShowerState&,ParticleId);
// PPJJ branch of FINDVIRTUALITIES. Requires two quarks/gluons and complete
// colour partners in the state. Other production channels/MPI are not mapped.
// State changes are transactional; random draws are not rewound on failure.
HardInitializationResult initialize_hard_dijet(ShowerState&,ParticleId,ParticleId,
 const HardInitializationConfig&,const BranchingSampler&,Random&);
}
