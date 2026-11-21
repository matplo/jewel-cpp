// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "jewel/core.hpp"
#include <array>
namespace jewel {
double momentum_magnitude(Momentum);
double polar_angle(Momentum);
double azimuth(Momentum);
double rapidity(Momentum,double record_mass);
double pseudorapidity(Momentum);
// Standard rotation then boost with JEWEL's Pythia6 conventions.
// Optional coordinates are auxiliary generator coordinates, not automatically MV production points.
// Returns true if the near-light-speed boost required stock rescaling.
bool rotate_boost(Momentum&,SpaceTime* coordinates,double theta,double phi,std::array<double,3> beta);
SpaceTime position_at(Momentum,SpaceTime production,double time);
}
