// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "jewel/initial_evolution.hpp"
#include "jewel/shower.hpp"
namespace jewel {
struct CascadeConfig {
 EvolutionConfig evolution;
 CollisionConfig collision;
 CollisionFinalizationConfig finalization;
 std::size_t maximum_branch_calls{1000000};
};
struct CascadeResult {std::size_t branch_calls{},passes{};};
// MAKEBRANCH / MAKECASCADE. Models must share run settings and outlive this object.
// State, recoil storage, transfer history, histograms and RNG belong to the caller.
class Cascade {
 CascadeConfig config_;
 const InitialScatteringEvolution& initial_;
 const ScatteringEvolution& final_;
 const BranchingSampler& branching_;
 const Medium& medium_;
public:
 Cascade(CascadeConfig,const InitialScatteringEvolution&,const ScatteringEvolution&,
  const BranchingSampler&,const Medium&);
 void branch(ShowerState&,ParticleId selected,ParticleId scratch,TransferHistory&,
  RecoilStorage&,Random&,CoherentHistograms* = nullptr) const;
 CascadeResult evolve(ShowerState&,ParticleId scratch,TransferHistory&,
  RecoilStorage&,Random&,CoherentHistograms* = nullptr) const;
};
}
