// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "jewel/branching.hpp"
#include "jewel/shower_state.hpp"
namespace jewel {
// REJECTSPLITTING: resample sibling masses and repair their kinematics/lifetimes.
// Mutates state; caller owns event discard/exception handling.
void reject_splitting(ShowerState&,ParticleId mother,const BranchingSampler&,Random&);
}
