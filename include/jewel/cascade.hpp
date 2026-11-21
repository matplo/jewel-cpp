// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "jewel/initial_evolution.hpp"
#include "jewel/shower.hpp"
#include <functional>
namespace jewel {
struct CascadeConfig {
 EvolutionConfig evolution;
 CollisionConfig collision;
 CollisionFinalizationConfig finalization;
 std::size_t maximum_branch_calls{1000000};
 bool compress_records{false};
};
// Read-only diagnostic callback: branch count, zero-based scheduling slot, before/after.
using CascadeObserver = std::function<void(std::size_t,std::size_t,bool,const ShowerState&)>;
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
  RecoilStorage&,Random&,CoherentHistograms* = nullptr,const CascadeObserver& = {}) const;
};
}
