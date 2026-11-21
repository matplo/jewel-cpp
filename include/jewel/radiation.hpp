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
class FinalSplittingTable {
 Kernels kernels_; double q0_;
 std::array<double,1000> q_{},z_{};
 std::vector<std::array<double,3>> values_; // QQ, GG, QG; q-major then z.
public:
 FinalSplittingTable(Kernels kernels,double q0,double emax,double scale,const Scatterer& maximum);
 const auto& scales() const {return q_;}
 const auto& fractions() const {return z_;}
 const auto& values() const {return values_;}
 double splitting(double parent_q,double q,double zeta,double energy,Channel,int angular_ordering) const;
 double sudakov(double high,double parent_q,double low,double zeta,double energy,Channel,int angular_ordering) const;
};

}
