// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "jewel/branching.hpp"
#include "jewel/scattering.hpp"
#include "jewel/transfer.hpp"
namespace jewel {
struct EvolutionConfig {
 double cutoff{1},mass_scale{1},formation_factor{1},lifetime{20},cross_section_scale{1};
};
struct FinalScatteringResult {
 double elapsed{},mass{},fraction{};
 bool quark_pair{},above_cutoff{};
};
// The referenced models must outlive this object and share its run configuration.
class ScatteringEvolution {
 EvolutionConfig config_;
 const BranchingSampler& branching_; const InitialSudakovTable& initial_;
 const FinalSplittingTable& final_; const ScatteringRates& rates_;
 const TransferSampler& transfers_;
public:
 ScatteringEvolution(EvolutionConfig,const BranchingSampler&,const InitialSudakovTable&,
  const FinalSplittingTable&,const ScatteringRates&,const TransferSampler&);
 FinalScatteringResult final_state(ShowerState&,ParticleId projectile,ParticleId scratch,
  TransferHistory&,double now,double time_left,FinalScatteringResult initial,Random&) const;
};
}
