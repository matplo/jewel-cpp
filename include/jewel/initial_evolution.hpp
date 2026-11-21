// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "jewel/evolution.hpp"
#include "jewel/coherence.hpp"
#include <optional>
namespace jewel {
struct InitialEvolutionConfig {
 EvolutionConfig evolution;
 CoherentConfig coherent;
 double coherence_rejection{1};
};
struct InitialScatteringResult {
 ParticleId selected{};double fraction{1},virtuality{},elapsed{};int incoming_flavour{};
 bool above_cutoff{};
 std::optional<CoherentStart> coherent;
 unsigned trials{},unresolved_rejections{},coherence_reversions{};
};
// Models must outlive this object and share its run settings. This routine
// samples the emission; MAKEINSPLIT/collision application belong to its caller.
class InitialScatteringEvolution {
 InitialEvolutionConfig config_;Kernels kernels_;
 const BranchingSampler& branching_;const InitialSudakovTable& initial_;
 const FinalSplittingTable& final_;const PartonicPdf& pdf_;const ScatteringRates& rates_;
 const TransferSampler& transfers_;const Medium& medium_;
public:
 InitialScatteringEvolution(InitialEvolutionConfig,Kernels,const BranchingSampler&,
  const InitialSudakovTable&,const FinalSplittingTable&,const PartonicPdf&,
  const ScatteringRates&,const TransferSampler&,const Medium&);
 // On exceptions restore/discard caller snapshots of all mutable state. Source
 // retry paths preserve colour/recoil/histogram side effects; RNG is not rewound.
 InitialScatteringResult sample(ShowerState&,ParticleId selected,ParticleId scratch,
  TransferHistory&,RecoilStorage&,double start,double duration,bool retry_splitting,
  Random&,CoherentHistograms* = nullptr) const;
};
}
