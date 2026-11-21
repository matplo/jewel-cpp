// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "jewel/transfer.hpp"
namespace jewel {
struct CoherentResolution {
 double transverse_transfer{}, separation{}, threshold{};
 bool resolved{};
};
// Literal DOCOHSCATTERING convention: (invariant,qx,qy), NOT (qx,qy,qz).
// Zero separation yields an infinite threshold. Undefined projections throw.
CoherentResolution coherent_resolution(Momentum,const TransferRecord&,
 double length_factor,double split_angle,double time,double split_time);
struct CoherentMomenta {
 Momentum parent,first,second;
 SpaceTime generator_vertex;
 double longitudinal{},transverse_squared{};
 bool possible{};
};
// The fraction belongs to the SECOND daughter. On negative transverse_squared,
// parent/vertex remain in the axial frame as in stock's failed attempt; daughters
// are absent. This helper does not perform the enclosing event rollback.
CoherentMomenta reconstruct_coherent(Momentum,SpaceTime generator_vertex,
 double parent_mass,double first_mass,double second_mass,double fraction,double phi);
}
