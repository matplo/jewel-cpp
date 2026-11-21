// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "jewel/branching.hpp"
#include "jewel/shower_state.hpp"
namespace jewel {
// REJECTSPLITTING: resample sibling masses and repair their kinematics/lifetimes.
// Mutates state; caller owns event discard/exception handling.
void reject_splitting(ShowerState&,ParticleId mother,const BranchingSampler&,Random&);
void make_splitting(ShowerState&,ParticleId,const BranchingSampler&,
 const Medium&,double formation_factor,Random&);

struct InitialSplitRequest {
 double fraction{},transfer_sum{},virtuality{},time{},radiation_time{};
 int incoming_flavour{};
 double formation_factor{1},mass_scale{1};
};
struct InitialSplitResult {double fraction{};ParticleId radiated{},continuing{};};
InitialSplitResult make_initial_splitting(ShowerState&,ParticleId,const InitialSplitRequest&,
 const BranchingSampler&,Random&);
}
