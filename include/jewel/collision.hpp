// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "jewel/branching.hpp"
#include "jewel/medium.hpp"
#include "jewel/shower_state.hpp"
#include "jewel/transfer.hpp"
#include "jewel/recoil.hpp"
namespace jewel {
struct CollisionConfig {double cutoff{1},mass_scale{1};int kinematic_mode{1};bool last_transfer{true};};
struct CollisionPair {
 ShowerParticle incoming,centre,outgoing,recoil;
 bool rejected{},reshuffled{},split_recoil{},recoil_pair{};
 int colour_direction{};
 double mass_change{},remaining_transfer{},target_mass{},recoil_fraction{};
};
// One transfer's kinematics in DOKINEMATICS, before colour/recoil-storage updates.
// total_transfer and remaining_mass_change belong to the enclosing sequence.
CollisionPair scatter_pair(ShowerParticle projectile,ShowerParticle scratch,
 const TransferRecord&,double total_transfer,double remaining_mass_change,
 double target_mass,double total_mass_change,CollisionConfig,
 const BranchingSampler&,const Medium&,Random&,const ShowerParticle* original_projectile=nullptr,std::optional<double> requested_mass=std::nullopt);

struct CollisionFinalizationConfig {
 double formation_factor{1},soft_recoil_cut{},hard_recoil_cut{};
 bool hadronize_recoils{true},scatter_recoils{true};
 RecoilStorageConfig storage;
};
struct CollisionStepTiming {
 double total_mass_change{},fraction{},first_decay_time{},coherent_end_time{},next_transfer_time{},transfer_time{};
 bool quark_pair{},coherent{},last_transfer{true};
};
// Complete one accepted transfer's colours, branching metadata, positions,
// lifetimes and recoil storage. Sequence-level final-time overwrite is separate.
// Rejected pairs are unchanged; no event IDs or genealogy are assigned here.
void finish_collision(CollisionPair&,int& colour_max,RecoilStorage&,
 CollisionFinalizationConfig,CollisionStepTiming,const Medium&,Random&);

struct CollisionSequenceRequest {
 std::size_t first{},last{}; // zero-based inclusive transfer range; last can shrink
 double target_mass{},time{},energy_fraction{1},fraction{};
 bool quark_pair{},coherent{},retry_splitting{};
};
struct CollisionSequenceResult {
 ParticleId outgoing{};
 std::size_t last{},retries{};
 double fraction{};
 bool retry_splitting{};
};
CollisionSequenceResult apply_collisions(ShowerState&,ParticleId projectile,ParticleId scratch,
 const TransferHistory&,RecoilStorage&,CollisionSequenceRequest,CollisionConfig,
 CollisionFinalizationConfig,const BranchingSampler&,const Medium&,Random&);
}
