// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "jewel/radiation.hpp"
namespace jewel {
enum class RateChannel { QA, QB, GA, GB };
class ScatteringRates {
 Kernels kernels_; double q0_,scale_; bool boost_;
 ExpandingMedium medium_;
 InitialRadiation radiation_; InitialSudakovTable sudakov_; PartonicPdf pdf_;
 ExponentialIntegral ei_;
 std::array<double,1000> transfer_{}; std::array<double,100> debye_{};
 std::vector<std::array<double,4>> values_;
public:
 ScatteringRates(Kernels,double q0,double emax,double scale,ExpandingMedium,bool boost=true);
 const auto& transfers() const{return transfer_;}
 const auto& debye_masses() const{return debye_;}
 const auto& values() const{return values_;}
 double integral(double transfer,double debye,RateChannel) const;
 double cross_section(Momentum projectile,double mass,double lower_transfer,
  char incoming,char outgoing,SpaceTime position,int mode=0) const;
};
}
