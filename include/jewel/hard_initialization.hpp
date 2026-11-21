// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "jewel/branching.hpp"
#include "jewel/shower_state.hpp"
#include <optional>
namespace jewel {
struct HardInitializationConfig {
 double pt_hat{},x{},y{},formation_factor{1},lifetime{20};
 bool first_angle_ordering{};
 bool mpi_pair{false},mpi_fsr{true};
 // Source TYPE1/TYPE2 are global: caller must supply them for an MPI pair.
 std::optional<std::array<Channel,2>> mpi_sampling_channels;
 std::size_t maximum_trials{1000000};
};
struct HardInitializationResult {std::size_t trials{};std::array<double,2> colour_angles{};};
// GETTHETAM, using source scheduling links for the partner's parent slot.
double colour_opening_angle(const ShowerState&,ParticleId);
// PPJJ and MPI-pair branches of FINDVIRTUALITIES. Requires two quarks/gluons;
// PPJJ additionally requires complete colour partners. MPI pair discovery and
// other production channels are not mapped. MPI sampling channels must be supplied explicitly.
// State changes are transactional; random draws are not rewound on failure.
HardInitializationResult initialize_hard_dijet(ShowerState&,ParticleId,ParticleId,
 const HardInitializationConfig&,const BranchingSampler&,Random&);
// Literal MAKEMASSLESS preparation of a surviving emitted parton and hard pair.
// Preserves sequential component rescaling (not a simultaneous vector rescale).
// Updates momenta, emitted record mass and auxiliary generator vertices only.
// Distinct active records are required; invalid outputs leave all three intact.
void make_parton_massless(ShowerParticle& emitted,ShowerParticle& first,ShowerParticle& second);

}
