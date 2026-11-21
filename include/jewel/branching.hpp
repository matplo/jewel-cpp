// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "jewel/radiation.hpp"
#include <optional>
namespace jewel {
struct MassSample {
 double mass{};
 std::optional<double> fraction;
 std::optional<bool> quark_pair;
};
class BranchingSampler {
 Kernels kernels_;double q0_,pt_factor_;int angular_;
 InitialRadiation initial_;
public:
 BranchingSampler(Kernels,double q0,double pt_factor,int angular_ordering);
 MassSample mass(double minimum,double maximum,double angle,double energy,
  Channel,double kinematic_maximum,bool initial_state,Random&) const;
 double fraction(double virtuality_squared,double energy,double epsilon,Channel,Random&) const;
};
}
