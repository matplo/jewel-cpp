// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "jewel/radiation.hpp"
namespace jewel {
enum class RateChannel { QA, QB, GA, GB };
struct CollisionTime {
 bool collision{}; double elapsed{}; unsigned bound_violations{};
};
class ScatteringRates {
 Kernels kernels_; double q0_,scale_; bool boost_;
 ExpandingMedium medium_;
 InitialRadiation radiation_; InitialSudakovTable sudakov_; PartonicPdf pdf_;
 ExponentialIntegral ei_;
 std::array<double,1000> transfer_{}; std::array<double,100> debye_{};
 std::vector<std::array<double,4>> values_;
public:
 ScatteringRates(Kernels,double q0,double emax,double scale,ExpandingMedium,bool boost=true);
 double no_initial_radiation(Momentum,double mass,int pdg,SpaceTime) const;
 CollisionTime next_collision(Momentum,double mass,int pdg,SpaceTime production,
  double start,double maximum_interval,double lifetime,double cross_section_scale,
  bool elastic_only,Random&) const;
 const auto& transfers() const{return transfer_;}
 const auto& debye_masses() const{return debye_;}
 const auto& values() const{return values_;}
 double integral(double transfer,double debye,RateChannel) const;
 double cross_section(Momentum projectile,double mass,double lower_transfer,
  char incoming,char outgoing,SpaceTime position,int mode=0) const;
};
}
