// SPDX-License-Identifier: GPL-2.0-or-later
// JEWEL 2.6.0 GETMASS and GENERATEZ translations.
#include "jewel/branching.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace jewel {
namespace {
constexpr double pi=3.141592653589793;
void check(bool b,const char* why){if(!b)throw std::domain_error(why);}
double draw(Random& r){double x=r.uniform();check(std::isfinite(x)&&x>0&&x<1,"invalid random draw");return x;}
}
BranchingSampler::BranchingSampler(Kernels k,double q0,double pt,int angular):kernels_(k),q0_(q0),pt_factor_(pt),angular_(angular),initial_(k,q0) {
 check(std::isfinite(pt)&&pt>0&&angular>=0&&angular<=5,"invalid branching configuration");
 k.alphas(3*pt*q0*q0/16,k.lambda_shower);
}
MassSample BranchingSampler::mass(double minimum,double maximum,double angle,double energy,Channel channel,double max2,bool ins,Random& random) const {
 for(double v:{minimum,maximum,angle,energy,max2})check(std::isfinite(v),"nonfinite mass-sampling input");
 check(minimum>=0&&maximum>=0&&energy>=0&&max2>=0,"invalid mass-sampling input");
 check(channel==Channel::QQ||channel==Channel::GC,"GETMASS supports QQ or GC");
 // Stock callers use theta=-1 for INS. Positive-angle INS in modes 3/4 reads undefined Z.
 check(!(ins&&angle>0&&(angular_==3||angular_==4)),"initial-state angle veto has no defined stock Z");
 const auto& k=kernels_;double q2min=q0_*q0_,alphmax=k.alphas(3*pt_factor_*q2min/16,k.lambda_shower);
 double pref=channel==Channel::QQ?4*alphmax/(3*2*pi):29*alphmax/(8*2*pi);
 if(maximum<=minimum||energy<minimum)return {0,0,false};
 double q2max=maximum*maximum;
 for(;;) {
  if(q2max-minimum*minimum<double(1e-4f)) {
   bool pair=channel==Channel::QQ?false:draw(random)<k.pqg(.5)/(k.pqg(.5)+k.pgg(.5));
   return {minimum,.5,pair};
  }
  double logmax=std::log(q2min/(4*q2max));double gmax=pref*(logmax*logmax),rmin=0;
  if(minimum>0){double x=std::log(q2min/(4*minimum*minimum));rmin=std::exp(pref*(x*x)-gmax);}
  double r=draw(random)*(1-rmin)+rmin,arg=gmax+std::log(r);
  if(arg<0)return {0,0,false};
  double candidate=q2min*std::exp(std::sqrt(arg/pref))/4,eps=q2min/(4*candidate);
  if(candidate<q2min||candidate<minimum*minimum)return {0,0,false};
  if(candidate>max2*max2||candidate>energy*energy){q2max=candidate;continue;}
  double weight,z=0;
  if(ins)weight=initial_.splitting(std::sqrt(candidate),channel)/(-2*pref*std::log(eps));
  else {
   double trueeps=.5-.5*std::sqrt(1-q2min/candidate)*std::sqrt(1-candidate/(energy*energy));
   z=1-std::pow(eps,draw(random));
   if(z<trueeps||z>1-trueeps)weight=0;
   else if(channel==Channel::QQ) {
    double value=k.alphas(pt_factor_*z*(1-z)*candidate,k.lambda_shower)*k.pqq(z)/(2*pi);
    weight=value/(2*pref/(1-z));
   } else {
    if(draw(random)<double(17.f/29.f))z=1-z;
    double value=k.alphas(pt_factor_*z*(1-z)*candidate,k.lambda_shower)*(k.pgg(z)+k.pqg(z))/(2*pi);
    weight=value/(alphmax*(17/(4*z)+3/(1-z))/(2*pi));
   }
  }
  if(draw(random)>weight){q2max=candidate;continue;}
  if(!ins&&(angular_==3||angular_==4)&&angle>0) {
   double newangle=std::sqrt(candidate/(z*(1-z)))/energy;
   if(newangle>angle){q2max=candidate;continue;}
  }
  if(ins)return {std::sqrt(candidate),std::nullopt,std::nullopt};
  bool pair=channel==Channel::QQ?false:draw(random)<k.pqg(z)/(k.pqg(z)+k.pgg(z));
  return {std::sqrt(candidate),z,pair};
 }
}
double BranchingSampler::fraction(double ti,double energy,double epsilon,Channel c,Random& random) const {
 check(std::isfinite(ti)&&ti>=0&&std::isfinite(energy)&&energy>=0&&std::isfinite(epsilon)&&epsilon>0,"invalid fraction query");
 check(c==Channel::QQ||c==Channel::GG||c==Channel::QG,"invalid GENERATEZ channel");
 double eps=epsilon;
 if(ti!=0) {
  check(ti>=q0_*q0_&&ti<=energy*energy,"fraction kinematics outside square-root domain");
  eps=std::max(.5-.5*std::sqrt(1-q0_*q0_/ti)*std::sqrt(1-ti/(energy*energy)),epsilon);
 }
 if(eps>.5)return .5;
 for(;;) {
  double r=draw(random);
  if(c==Channel::QQ) {
   double x=1-(1-eps)*std::pow(eps/(1-eps),r);r=draw(random);
   if(r<(1+x*x)/2)return x;
  } else if(c==Channel::GG) {
   double x=1/(1+std::pow((1-eps)/eps,1-2*r));r=draw(random);
   double help=((1-x)/x+x/(1-x)+x*(1-x))/(1/(1-x)+1/x);
   if(r<help)return x;
  } else {
   r=draw(random)*(1-2*eps)+eps;double r1=draw(random)/2,help=.5*(r*r+(1-r)*(1-r));
   if(r1<help)return r;
  }
 }
}
}
