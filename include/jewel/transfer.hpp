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
 std::size_t first{},last{}; // Active one-based NSTART/NEND; zero denotes none.
 std::vector<TransferRecord> records;
 Momentum sum; double sum_squared{};
};
class TransferSampler {
 Kernels kernels_; MediumModel medium_; bool boost_;
public:
 TransferSampler(Kernels,MediumModel,bool longitudinal_boost=true);
 TransferSampler with_event_impact_parameter(double b) const {
  auto copy=*this;copy.medium_=medium_.with_impact_parameter(b);return copy;
 }
 // slot is zero-based and corresponds to stock J=NEND. scratch is the
 // dedicated temporary event record (stock line 1), never the projectile.
 void sample(ShowerState&,ParticleId projectile,ParticleId scratch,
  TransferHistory&,std::size_t slot,double elapsed_since_production,
  double energy_fraction,bool recalculate,Random&) const;
};
}
