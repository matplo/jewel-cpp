// SPDX-License-Identifier: GPL-2.0-or-later
// JEWEL 2.6.0 XSECINT, GETXSECINT and GETSSCAT translations.
#include "jewel/scattering.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>
namespace jewel {
namespace {
constexpr double pi=3.141592653589793;
void check(bool b,const char* message){if(!b)throw std::domain_error(message);}
}
ScatteringRates::ScatteringRates(Kernels k,double q0,double emax,double scale,ExpandingMedium medium,bool boost):
 kernels_(k),q0_(q0),scale_(scale),boost_(boost),medium_(std::move(medium)),radiation_(k,q0),
 sudakov_(k,q0,emax,scale,medium_.maximum_scatterer(boost)),pdf_(k,q0,emax,scale,medium_.maximum_scatterer(boost)),values_(100000) {
 auto maximum=medium_.maximum_scatterer(boost);auto p=maximum.momentum;double m=maximum.mass;
 double shat=m*m+2*emax*(p.energy+std::sqrt(p.px*p.px+p.py*p.py+p.pz*p.pz));
 double pcms2=std::pow(shat-m*m,2)/(4*shat),tmaxmax=scale*4*pcms2;
 double lower=q0*q0/(scale*scale),first=q0*q0*(1+1e-6)/(scale*scale),delta=(std::log(tmaxmax)-std::log(first))/999;
 check(std::isfinite(delta)&&delta>0,"invalid scattering grid scale");
 auto bounds=medium_.bounds();double mdmin=bounds.debye_min,mdmax=std::max(mdmin,bounds.debye_max),dmd=(mdmax-mdmin)/99;
 for(int j=0;j<1000;++j) {
  double t=std::exp(j*delta+std::log(first));transfer_[j]=t;
  for(int i=0;i<100;++i) {
   double md=mdmin+i*dmd;debye_[i]=md;
   if(t<lower){values_[j*100+i]={0,0,0,0};continue;}
   for(int c=0;c<4;++c) {
    auto f=[&](double x){
     double q=scale*std::sqrt(x);
     if(c==0)return 3*radiation_.splitting(q,Channel::GQ)*ei_.scatprimfunc(x,md,k)/(2*x);
     if(c==2)return 2*radiation_.splitting(q,Channel::QG)*ei_.scatprimfunc(x,md,k)/(3*x);
     double s=x+md*md,a=k.alphas(s,k.lambda_shower);
     if(c==3)return sudakov_.ratio(q0,q,Channel::GC)*3*2*pi*(a*a)/(2*(s*s));
     return sudakov_.ratio(q0,q,Channel::QQ)*2*2*pi*(a*a)/(3*(s*s));
    };
    values_[j*100+i][c]=integrate(f,lower,t,1e-4,double(.01f)*(t-lower));
   }
  }
 }
}
double ScatteringRates::integral(double t,double md,RateChannel channel) const {
 check(std::isfinite(t)&&t>0&&std::isfinite(md)&&md>=0,"invalid scattering table query");
 int c=static_cast<int>(channel);check(c>=0&&c<4,"invalid rate channel");
 int it=int(std::clamp((std::log(t)-std::log(transfer_[0]))*999/(std::log(transfer_[999])-std::log(transfer_[0]))+1,1.,999.))-1;
 int im=debye_[99]==debye_[0]?0:int(std::clamp((md-debye_[0])*99/(debye_[99]-debye_[0])+1,1.,99.))-1;
 double y[2];
 for(int i=0;i<2;++i){double a=values_[it*100+im+i][c],b=values_[(it+1)*100+im+i][c];double slope=(b-a)/(transfer_[it+1]-transfer_[it]),intercept=a-slope*transfer_[it];y[i]=slope*t+intercept;}
 if(debye_[im]==debye_[im+1])return y[0];
 double slope=(y[1]-y[0])/(debye_[im+1]-debye_[im]),intercept=y[0]-slope*debye_[im];return slope*md+intercept;
}
double ScatteringRates::cross_section(Momentum p,double mass,double lower,char incoming,char outgoing,SpaceTime v,int mode) const {
 for(double x:{p.px,p.py,p.pz,p.energy,mass,lower})check(std::isfinite(x),"nonfinite projectile");
 check(p.energy>=0&&lower>=0&&(incoming=='Q'||incoming=='G')&&(outgoing=='Q'||outgoing=='G'||outgoing=='C')&&mode>=0&&mode<=2,"invalid cross section query");
 double colour=incoming=='Q'?double(2.f/3.f):double(3.f/2.f),md,shat,upper;
 Scatterer average;
 if(mode==0) {
  md=medium_.sample(v).debye_mass;average=medium_.average_scatterer(v,boost_);auto s=average.momentum;double m=average.mass;
  shat=m*m+mass*mass+2*(s.energy*p.energy-s.px*p.px-s.py*p.py-s.pz*p.pz);
  if(shat==0)return 0;
  double pcms2=std::pow(shat+mass*mass-m*m,2)/(4*shat)-mass*mass;upper=4*pcms2;
 } else {
  auto bounds=medium_.bounds();md=mode==1?bounds.debye_min:bounds.debye_max;
  average=medium_.maximum_scatterer(boost_);auto s=average.momentum;double m=average.mass;
  double ps=std::sqrt(s.px*s.px+s.py*s.py+s.pz*s.pz),pp=std::sqrt(p.px*p.px+p.py*p.py+p.pz*p.pz);
  shat=m*m+mass*mass+2*(p.energy*s.energy+pp*ps);
  double pcms2=std::pow(shat+mass*mass-m*m,2)/(4*shat)-mass*mass;upper=4*pcms2;
 }
 double low=lower*lower,result=0;if(low>upper)return 0;
 if(outgoing=='C'||incoming==outgoing)result=colour*(ei_.scatprimfunc(upper,md,kernels_)-ei_.scatprimfunc(low,md,kernels_));
 low=q0_*q0_/(scale_*scale_);
 if(upper>low) {
  if(incoming=='Q') {
   if(outgoing=='C'||outgoing=='G') {
    result=result+pdf_.integrated(scale_*std::sqrt(upper),Channel::GQ)*3*ei_.scatprimfunc(upper,md,kernels_)/2;
    result=result-integral(upper,md,RateChannel::QA);
   }
  } else {
   if(outgoing=='C'||outgoing=='G')result=result+colour*(ei_.scatprimfunc(upper,md,kernels_)-ei_.scatprimfunc(low,md,kernels_))-integral(upper,md,RateChannel::GB);
   if(outgoing=='C'||outgoing=='Q') {
    result=result+2*pdf_.integrated(scale_*std::sqrt(upper),Channel::QG)*2*ei_.scatprimfunc(upper,md,kernels_)/3;
    result=result-2*integral(upper,md,RateChannel::GA);
   }
  }
 }
 return std::max(result,0.);
}
}
