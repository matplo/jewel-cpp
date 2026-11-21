// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "jewel/core.hpp"
namespace jewel {
struct PathLengths { double geometric_transverse{}, hot{}, hot_transverse{}, temperature_cubed_integral{}; };
// geometry-v1: straight-line estimate from (x,y,0,0), not shower trajectory.
PathLengths straight_path_lengths(double x,double y,double impact_parameter,
 double support_radius,double lifetime,Momentum,double step,const Medium&);
}
