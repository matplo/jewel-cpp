// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "jewel/branching.hpp"
#include "jewel/shower_state.hpp"
#include <optional>
namespace jewel {
// Pythia8 colour-octet onium precursors (ParticleDataEntry::isOctetHadron).
// Require a genuine two-tag octet, not arbitrary coloured nonpartons.
inline bool inactive_octet_onium(const Particle& p) {
 return p.pdg>=9940000&&p.pdg<9960000&&p.status==4&&
  p.colour>0&&p.anticolour>0&&p.colour!=p.anticolour;
}
struct HardInitializationConfig {
 double pt_hat{},x{},y{},formation_factor{1},lifetime{20};
 bool first_angle_ordering{};
 // Optional diagnostic only when the first-splitting angular veto is disabled.
 bool evaluate_colour_angles{true};
 bool mpi_pair{false},mpi_fsr{true};
 // Native convention: finite unused time for inactive nonpartons (colourless or octet onium).
 bool inactive_nonparton_lifetime{false};
 // Source TYPE1/TYPE2 are global: caller must supply them for an MPI pair.
 std::optional<std::array<Channel,2>> mpi_sampling_channels;
 std::size_t maximum_trials{1000000};
};
struct HardInitializationResult {std::size_t trials{};std::array<double,2> colour_angles{};bool colour_angles_evaluated{true};};
// GETTHETAM, using source scheduling links for the partner's parent slot.
double colour_opening_angle(const ShowerState&,ParticleId);
// PPJJ and MPI-pair branches of FINDVIRTUALITIES. Allows quarks/gluons and
// inactive colourless MPI photons; other nonpartons need the explicit time convention.
// PPJJ requires complete colour partners when angles are evaluated. MPI pair discovery and
// other production channels are not mapped. MPI sampling channels must be supplied explicitly.
// State changes are transactional; random draws are not rewound on failure.
HardInitializationResult initialize_hard_dijet(ShowerState&,ParticleId,ParticleId,
 const HardInitializationConfig&,const BranchingSampler&,Random&);
// Literal MAKEMASSLESS preparation of a surviving emitted parton and hard pair.
// Preserves sequential component rescaling (not a simultaneous vector rescale).
// Updates momenta, emitted record mass and auxiliary generator vertices only.
// Distinct active records are required; invalid outputs leave all three intact.
void make_parton_massless(ShowerParticle& emitted,ShowerParticle& first,ShowerParticle& second);

// GENEVENT status-selection block on surviving positive-status records, in
// source slot order. Nominal masses are supplied by the generator adapter.
// The first/second indices identify the hard pair within this record range.
// Changes are transactional; no random numbers are consumed.
void prepare_surviving_partons(std::vector<ShowerParticle>&,std::size_t first,
 std::size_t second,const std::vector<double>& nominal_masses,double max_pseudorapidity);

}
