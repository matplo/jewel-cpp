// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "jewel/medium.hpp"
#include "jewel/shower_state.hpp"
namespace jewel {
struct TransferRecord {
 double invariant{}; Momentum momentum; double time{};
 Scatterer scatterer; SpaceTime position;
};
struct TransferHistory {
 std::vector<TransferRecord> records;
 Momentum sum; double sum_squared{};
};
class TransferSampler {
 Kernels kernels_; ExpandingMedium medium_; bool boost_;
public:
 TransferSampler(Kernels,ExpandingMedium,bool longitudinal_boost=true);
 // slot is zero-based and corresponds to stock J=NEND. scratch is the
 // dedicated temporary event record (stock line 1), never the projectile.
 void sample(ShowerState&,ParticleId projectile,ParticleId scratch,
  TransferHistory&,std::size_t slot,double elapsed_since_production,
  double energy_fraction,bool recalculate,Random&) const;
};
}
