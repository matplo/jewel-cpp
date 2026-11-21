// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "jewel/kinematics.hpp"
namespace jewel::detail {
// Provisional collision candidates may carry source-style nonfinite values.
// Never use this entry point to publish an accepted event.
bool rotate_boost_candidate(Momentum&,SpaceTime*,double,double,std::array<double,3>);
}
