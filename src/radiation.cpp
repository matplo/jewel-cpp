// SPDX-License-Identifier: GPL-2.0-or-later
// Translations of JEWEL 2.6.0, Copyright Korinna C. Zapp.
#include "jewel/radiation.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace jewel {
namespace {
constexpr double pi=3.141592653589793;
void check(bool b,const char* s){if(!b)throw std::domain_error(s);}
int column(Channel c) {
 switch(c){case Channel::QQ:return 1;case Channel::QG:return 2;case Channel::GG:return 3;case Channel::GC:return 4;default:throw std::domain_error("channel has no Sudakov table");}
}
}
InitialRadiation::InitialRadiation(Kernels k,double q0):kernels_(k),q0_(q0) {
 check(std::isfinite(q0)&&q0>2*k.lambda_shower,"Q0 must exceed 2 Lambda_shower");k.alphas(q0*q0,k.lambda_shower);
}
double InitialRadiation::splitting(double q,Channel c) const {
 check(std::isfinite(q)&&q>0,"invalid splitting scale");
 double up=1-q0_*q0_/(4*q*q),low=0,l=kernels_.lambda_shower;
 // Preserve gfortran integer powers: x^6 = (x^2*x)^2. Adaptive paths depend on this.
 double l2=l*l,l4=l2*l2,l6=(l2*l)*(l2*l),q2=q*q,q4=q2*q2,q6=(q2*q)*(q2*q);
 double gg=0,qg=0;
 if(c==Channel::GG||c==Channel::GC) {
  low=1-up;if(up<=low)return 0;
  double a=std::log((1-low)*q2/l2),b=std::log((1-up)*q2/l2);
  gg=2*(std::log(a)-l2*ei_.extrapolated(a)/q2+l4*ei_.extrapolated(2*a)/q4-l6*ei_.extrapolated(3*a)/q6
    -std::log(b)+l2*ei_.extrapolated(b)/q2-l4*ei_.extrapolated(2*b)/q4+l6*ei_.extrapolated(3*b)/q6
    +low-std::log(low)-up+std::log(up))*3*12*pi/(2*pi*(33-2*kernels_.nf));
  if(c==Channel::GG)return gg;
 }
 if(c==Channel::QG||c==Channel::GC) {
  low=0;if(up<=low)return 0;
  double a=std::log((1-low)*q2/l2),b=std::log((1-up)*q2/l2);
  qg=(2*l6*ei_.extrapolated(3*a)/q6-2*l4*ei_.extrapolated(2*a)/q4+2*l2*ei_.extrapolated(a)/q2
    -2*l6*ei_.extrapolated(3*b)/q6+2*l4*ei_.extrapolated(2*b)/q4-2*l2*ei_.extrapolated(b)/q2)*12*pi/(2*2*pi*(33-2*kernels_.nf));
  qg=kernels_.nf*qg;if(c==Channel::QG)return qg;
 }
 if(c==Channel::QQ) {
  low=0;if(up<=low)return 0;
  double a=std::log((1-low)*q2/l2),b=std::log((1-up)*q2/l2);
  return (2*std::log(a)-2*l2*ei_.extrapolated(a)/q2+l4*ei_.extrapolated(2*a)/q4
    -2*std::log(b)+2*l2*ei_.extrapolated(b)/q2-l4*ei_.extrapolated(2*b)/q4)*4*12*pi/(3*2*pi*(33-2*kernels_.nf));
 }
 if(c==Channel::GQ) {
  low=1-up;if(up<=low)return 0;
  return (up*up/2-2*up+2*std::log(up)-low*low/2+2*low-2*std::log(low))*4*12*pi/(3*2*pi*(33-2*kernels_.nf)*std::log(q2/l2));
 }
 check(c==Channel::GC,"invalid splitting channel");return gg+qg;
}
double InitialRadiation::sudakov(double low,double high,Channel c) const {
 check(std::isfinite(low)&&std::isfinite(high)&&low>=q0_&&high>0,"invalid Sudakov range");
 if(low<q0_+1e-12)low+=1e-12;
 if(low>=high-1e-12)return 1;
 auto f=[&](double x){return 2*splitting(x,c)/x;};
 return std::exp(-integrate(f,low,high,1e-4,double(.01f)*(high-low)));
}
InitialSudakovTable::InitialSudakovTable(Kernels k,double q0,double emax,double scale,const Scatterer& maximum):radiation_(k,q0),q0_(q0) {
 check(std::isfinite(emax)&&emax>0&&std::isfinite(scale)&&scale>0,"invalid tabulation scale");
 auto p=maximum.momentum;double mass=maximum.mass;
 check(std::isfinite(mass)&&mass>=0&&std::isfinite(p.energy)&&p.energy>0,"invalid maximum scatterer");
 double shat=mass*mass+2*emax*(p.energy+std::sqrt(p.px*p.px+p.py*p.py+p.pz*p.pz));
 double pcms2=std::pow(shat-mass*mass,2)/(4*shat),qmax=std::sqrt(scale*4*pcms2);
 double first=q0*q0*(1+1e-6),delta=(std::log(3*qmax)-std::log(first))/999;
 check(std::isfinite(delta)&&delta>0,"invalid Sudakov table range");
 for(int i=0;i<1000;++i) {
  double q=std::exp(i*delta+std::log(first));
  table_[i]={q,radiation_.sudakov(q0,q,Channel::QQ),radiation_.sudakov(q0,q,Channel::QG),radiation_.sudakov(q0,q,Channel::GG),radiation_.sudakov(q0,q,Channel::GC)};
 }
}
double InitialSudakovTable::reduced(double q,Channel c) const {
 check(std::isfinite(q)&&q>0,"invalid Sudakov lookup scale");int col=column(c);
 int i=std::clamp(int((std::log(q)-std::log(table_[0][0]))*999/(std::log(table_[999][0])-std::log(table_[0][0]))+1),1,999)-1;
 auto a=table_[i],b=table_[i+1];double slope=(b[col]-a[col])/(b[0]-a[0]),intercept=a[col]-slope*a[0];
 return std::max(0.,slope*q+intercept);
}
double InitialSudakovTable::ratio(double low,double high,Channel c) const {
 check(std::isfinite(low)&&low>0&&std::isfinite(high)&&high>0,"invalid Sudakov lookup interval");
 if(high<=low)return 1;
 double value;
 if(low<=q0_)value=reduced(high,c);
 else {double denominator=reduced(low,c);check(denominator>0,"zero Sudakov denominator");value=reduced(high,c)/denominator;}
 return std::clamp(value,0.,1.);
}
}
