// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "jewel/medium.hpp"
namespace jewel {
enum class Channel { QQ, QG, GG, GC, GQ };
class InitialRadiation {
 Kernels kernels_;
 double q0_;
 ExponentialIntegral ei_;
public:
 InitialRadiation(Kernels kernels,double q0);
 double splitting(double q,Channel) const;
 double sudakov(double low,double high,Channel) const;
};
class InitialSudakovTable {
 InitialRadiation radiation_;
 double q0_;
 std::array<std::array<double,5>,1000> table_{};
public:
 InitialSudakovTable(Kernels kernels,double q0,double emax,double scale,const Scatterer& maximum);
 const auto& table() const {return table_;}
 double reduced(double q,Channel) const;
 double ratio(double low,double high,Channel) const;
};
}
