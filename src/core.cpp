// SPDX-License-Identifier: GPL-2.0-or-later
// Kernel translations: JEWEL 2.6.0, Copyright (C) 2026 Korinna C. Zapp.
// See NOTICE for attribution and the distinction from official JEWEL.
#include "jewel/core.hpp"
#include <cmath>
#include <stdexcept>
#include <algorithm>
#include <utility>
#include <limits>
namespace jewel {
namespace {
void require(bool ok, const char* msg) { if (!ok) throw std::domain_error(msg); }
void zcheck(double z) { require(std::isfinite(z) && z > 0 && z < 1, "z must be in (0,1)"); }
}
ParticleId Event::append(int pdg, Momentum p, SpaceTime v, std::vector<ParticleId> parents) {
 for (double x : {p.px,p.py,p.pz,p.energy,v.x,v.y,v.z,v.t}) require(std::isfinite(x), "nonfinite particle");
 require(p.energy >= 0, "negative energy");
 for (auto id : parents) require(std::any_of(particles_.begin(), particles_.end(),
  [id](const Particle& x) { return x.id == id; }), "unknown parent ID");
 const auto id = next_;
 particles_.push_back({id,pdg,0,0,0,p,v,std::move(parents)}); ++next_; return id;
}
ReplayRandom::ReplayRandom(std::vector<double> tape) : tape_(std::move(tape)) {
 for (double r : tape_) require(std::isfinite(r) && r > 0 && r < 1, "RNG tape values must be in (0,1)");
}
double ReplayRandom::uniform() {
 if (cursor_ == tape_.size()) throw std::out_of_range("RNG tape exhausted");
 return tape_[cursor_++];
}
void ReplayRandom::restore(std::size_t cursor) {
 if(cursor > tape_.size()) throw std::out_of_range("invalid RNG checkpoint");
 cursor_ = cursor;
}
double SplitMixRandom::uniform(){
 if(draws_==std::numeric_limits<std::uint64_t>::max())throw std::overflow_error("RNG counter exhausted");
 auto z=seed_+UINT64_C(0x9e3779b97f4a7c15)*(++draws_);
 z=(z^(z>>30))*UINT64_C(0xbf58476d1ce4e5b9);
 z=(z^(z>>27))*UINT64_C(0x94d049bb133111eb);
 z^=z>>31;
 return (double(z>>12)+0.5)*0x1p-52;
}
void Kernels::validate() const { require(nf >= 0 && nf <= 6, "nf must be in [0,6]"); }
double Kernels::pqq(double z) const { zcheck(z); return 4*(1+z*z)/(3*(1-z)); }
double Kernels::pgq(double z) const { zcheck(z); return 4*(1+(1-z)*(1-z))/(3*z); }
double Kernels::pgg(double z) const { zcheck(z); return 3*((1-z)/z+z/(1-z)+z*(1-z)); }
double Kernels::pqg(double z) const { zcheck(z); return 0.5*(z*z+(1-z)*(1-z)); }
double Kernels::alphas(double t, double lambda) const {
 validate(); require(std::isfinite(t) && std::isfinite(lambda) && lambda > 0 && t > lambda*lambda, "alphas requires t > lambda^2 > 0");
 // Preserve the reference's default REAL evaluation of beta0, including NF=5.
 const float beta0 = 11.0f-2.0f*static_cast<float>(nf)/3.0f;
 return 4*3.141592653589793/(static_cast<double>(beta0)*std::log(t/(lambda*lambda)));
}
double Kernels::log_integral(double z,double q) const {
 validate(); zcheck(z);
 require(std::isfinite(q) && q>0 && std::isfinite(lambda_shower) && lambda_shower>0, "invalid shower scale");
 double x=std::log(q*q/(lambda_shower*lambda_shower))+std::log(z);
 require(std::isfinite(x) && x>0, "integral outside logarithm domain"); return std::log(x);
}
double Kernels::intpqq(double z,double q) const { zcheck(z); return -48*log_integral(1-z,q)/((33-2*nf)*3); }
double Kernels::intpgglow(double z,double q) const { return 18*log_integral(z,q)/(33-2*nf); }
double Kernels::intpgghigh(double z,double q) const { zcheck(z); return -18*log_integral(1-z,q)/(33-2*nf); }
double Kernels::gett(double minimum,double maximum,double debye,Random& random) const {
 require(std::isfinite(minimum)&&std::isfinite(maximum)&&std::isfinite(debye)&&minimum>=0&&maximum>=0&&debye>=0,"invalid transfer bounds");
 const double lo=minimum+debye*debye, hi=maximum+debye*debye;
 if(lo>hi) return 0; // Reference convention: consumes no draws.
 const double norm=alphas(lo,lambda_scattering);
 for (;;) {
  double r1=random.uniform(); zcheck(r1);
  double t=hi*lo/(hi+r1*(lo-hi));
  double r2=random.uniform(); zcheck(r2);
  double a=alphas(t,lambda_scattering);
  if(r2 < a*a/(norm*norm)) return t-debye*debye;
 }
}
}
