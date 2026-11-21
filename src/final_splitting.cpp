// SPDX-License-Identifier: GPL-2.0-or-later
// JEWEL 2.6.0 SPLITFNCINT, GETSPLITI and final-state GETSUDAKOV translation.
#include "jewel/radiation.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace jewel {
namespace {
constexpr double pi=3.141592653589793;
void check(bool b,const char* why){if(!b)throw std::domain_error(why);}
}
FinalSplittingTable::FinalSplittingTable(Kernels k,double q0,double emax,double scale,const Scatterer& maximum):kernels_(k),q0_(q0),values_(1000000) {
 check(std::isfinite(q0)&&q0>2*k.lambda_shower&&std::isfinite(emax)&&emax>2*q0&&std::isfinite(scale)&&scale>0,"invalid final splitting configuration");
 k.alphas(q0*q0,k.lambda_shower);
 auto p=maximum.momentum;double m=maximum.mass;
 check(std::isfinite(m)&&m>=0&&std::isfinite(p.energy)&&p.energy>0&&std::isfinite(p.px)&&std::isfinite(p.py)&&std::isfinite(p.pz),"invalid maximum scatterer");
 double shat=m*m+2*emax*(p.energy+std::sqrt(p.px*p.px+p.py*p.py+p.pz*p.pz));
 double pcms2=std::pow(shat-m*m,2)/(4*shat),qmax=std::sqrt(scale*4*pcms2);
 check(std::isfinite(qmax)&&qmax>q0,"invalid final splitting upper scale");
 double logmin=std::log(q0/emax),logmax=std::log(.5);
 for(int i=0;i<1000;++i) {
  double q=i*(qmax-q0)/999+q0;q_[i]=q;double qquad=q*q;
  for(int j=0;j<1000;++j) {
   double z=std::exp(j*(logmax-logmin)/999+logmin);z_[j]=z;
   double cutoff=qquad<q0*q0?.5:.5-.5*std::sqrt(1-q0*q0/qquad);z=std::max(z,cutoff);
   if(z==.5){values_[i*1000+j]={0,0,0};continue;}
   for(int c=0;c<3;++c) {
    auto f=[&](double x){double kernel=c==0?k.pqq(x):c==1?k.pgg(x):k.pqg(x);return k.alphas(x*(1-x)*qquad,k.lambda_shower)*kernel/(2*pi);};
    values_[i*1000+j][c]=integrate(f,z,1-z,1e-5,double(.01f));
   }
  }
 }
}
double FinalSplittingTable::splitting(double parent,double q,double zeta,double energy,Channel channel,int angular) const {
 check(std::isfinite(parent)&&parent>0&&std::isfinite(q)&&q>=q0_&&std::isfinite(energy)&&energy>=q&&std::isfinite(zeta)&&zeta>0&&zeta<=1&&angular>=0&&angular<=5,"invalid final splitting kinematics");
 check(channel!=Channel::GQ,"GQ has no final-state splitting table");
 double low=.5-.5*std::sqrt(1-q0_*q0_/(q*q))*std::sqrt(1-q*q/(energy*energy));
 if(angular>0&&zeta!=1) {
  double discriminant=1-4*q*q*(1-zeta)/(zeta*parent*parent);
  check(discriminant>=0,"angular-ordering square root outside domain");
  low=std::max(low,.5-.5*std::sqrt(discriminant));
 }
 check(low>0&&std::isfinite(low),"invalid final splitting fraction");
 int iq=int(std::clamp((q-q_[0])*999/(q_[999]-q_[0])+1,1.,999.))-1;
 int iz=int(std::clamp((std::log(low)-std::log(z_[0]))*999/(std::log(z_[999])-std::log(z_[0]))+1,1.,999.))-1;
 auto lookup=[&](int c){
  double y[2];
  for(int i=0;i<2;++i){double a=values_[(iq+i)*1000+iz][c],b=values_[(iq+i)*1000+iz+1][c];double slope=(b-a)/(z_[iz+1]-z_[iz]),intercept=a-slope*z_[iz];y[i]=slope*low+intercept;}
  double result;
  if(q_[iq]==q_[iq+1])result=(y[0]+y[1])/2;
  else {double slope=(y[1]-y[0])/(q_[iq+1]-q_[iq]),intercept=y[0]-slope*q_[iq];result=slope*q+intercept;}
  return std::min(result,10.);
 };
 switch(channel){case Channel::QQ:return lookup(0);case Channel::GG:return lookup(1);case Channel::QG:return kernels_.nf*lookup(2);case Channel::GC:{
 // Preserve the reference assignment/rounding of SPLITINTQG before adding GG.
 // Fusing NF*QG + GG changes adaptive Sudakov integration paths.
 double gg=lookup(1); volatile double qg=kernels_.nf*lookup(2); return gg+qg;
 }default:throw std::domain_error("unknown final splitting channel");}
}
double FinalSplittingTable::sudakov(double high,double parent,double low,double zeta,double energy,Channel c,int angular) const {
 check(std::isfinite(low)&&low>=q0_&&std::isfinite(high)&&high>0,"invalid final Sudakov interval");
 double adjusted=low;if(adjusted<q0_+1e-10)adjusted+=1e-10;
 if(adjusted>=high-1e-10)return 1;
 return std::exp(-integrate([&](double x){return 2*splitting(parent,x,zeta,energy,c,angular)/x;},adjusted,high,1e-4,double(.01f)*(high-low)));
}
}
