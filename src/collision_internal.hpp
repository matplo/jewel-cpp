// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "jewel/collision.hpp"
#include <utility>
namespace jewel::detail {
CollisionPair scatter_pair_candidate(ShowerParticle,ShowerParticle,const TransferRecord&,
 double,double,double,double,CollisionConfig,const BranchingSampler&,const Medium&,Random&,
 const ShowerParticle*,std::optional<double>);
struct CollisionWorkspace {
 static ParticleId append(ShowerState& state,ShowerParticle particle) {
  return state.append_impl(std::move(particle),true,true,false);
 }
 static ShowerSnapshot truncate(const ShowerState& state,std::size_t size) {
  return state.snapshot_truncated_impl(size,false);
 }
};
}
