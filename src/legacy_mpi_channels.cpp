// SPDX-License-Identifier: GPL-2.0-or-later
// PYSGQC ISUB=96 translation; T. Sjostrand, S. Mrenna, P. Skands.
#include "jewel/legacy_mpi_channels.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace jewel {
namespace {void check(bool ok){if(!ok)throw std::domain_error("invalid legacy MPI channel input");}
double square(double x){return x*x;}}
LegacyGluonWidth legacy_gluon_width(double sh,const std::vector<LegacyGluonDecay>& decays,double fudge){
 check(std::isfinite(sh)&&sh>0&&std::isfinite(fudge)&&fudge>=0&&decays.size()<=400);
 LegacyGluonWidth out;out.partial.resize(decays.size());
 for(std::size_t i=0;i<decays.size();++i){
  const auto& d=decays[i];
  check(std::isfinite(d.mass_a)&&d.mass_a>=0&&std::isfinite(d.mass_b)&&d.mass_b>=0);
  check(std::isfinite(d.secondary_weight)&&d.secondary_weight>=0&&d.mode<=5);
  if(d.mode<0)continue;
  double rm1=d.mass_a*d.mass_a/sh,rm2=d.mass_b*d.mass_b/sh;
  check(std::isfinite(rm1)&&std::isfinite(rm2));
  if(std::sqrt(rm1)+std::sqrt(rm2)>1)continue;
  double secondary=1;
  if(i<8){out.partial[i]=(1+2*rm1)*std::sqrt(std::max(0.,1-4*rm1));
   if(i>=5)secondary=d.secondary_weight;}
  out.partial[i]=fudge*out.partial[i];out.partial_total=out.partial_total+out.partial[i];
  if(d.mode>0){double enabled=out.partial[i]*secondary;
   out.enabled[d.mode]=out.enabled[d.mode]+enabled;out.enabled[0]=out.enabled[0]+enabled;}
 }
 out.annihilation_weight=out.enabled[1]+out.enabled[2]+out.enabled[3]+out.enabled[4];
 check(std::isfinite(out.partial_total)&&std::isfinite(out.enabled[0]));return out;
}
LegacyMpiChannelResult legacy_mpi_channels(const LegacyMpiChannelInput& p,
 const std::array<double,11>& pa,const std::array<double,11>& pb){
 for(double x:{p.s,p.t,p.u,p.cos_theta,p.common_factor,p.alpha_s,p.asymmetric_factor,p.annihilation_weight})check(std::isfinite(x));
 check(p.s>0&&p.t<0&&p.u<0&&std::abs(p.cos_theta)<=1&&p.common_factor>0&&p.alpha_s>0&&p.asymmetric_factor>=0&&p.annihilation_weight>=0);
 check(p.interference==0||p.interference==1);
 for(double x:p.masses)check(std::isfinite(x)&&x>=0);
 for(double x:pa)check(std::isfinite(x));for(double x:pb)check(std::isfinite(x));
 const double sh=p.s,th=p.t,uh=p.u,sh2=sh*sh,th2=th*th,uh2=uh*uh;
 const double com=p.common_factor,as2=p.alpha_s*p.alpha_s,faca=p.asymmetric_factor;
 const int m=p.interference;
 LegacyMpiChannelResult out;
 auto add=[&](int a,int b,int code,double weight){
  check(std::isfinite(weight));double weighted=weight;
  weighted=weighted*pa[a==21?5:a+5];weighted=weighted*pb[b==21?5:b+5];
  check(std::isfinite(weighted));out.channels.push_back({a,b,code,weight,weighted});out.total=out.total+weighted;
 };
 double qq1=com*as2*4/9*(sh2+uh2)/th2;
 double qqb=com*as2*4/9*((sh2+uh2)/th2*faca-m*2./3*uh2/(sh*th));
 double qq2=com*as2*4/9*(sh2+th2)/uh2;
 double qqi=-com*as2*4/9*m*2./3*sh2/(th*uh);
 double ratio=(qq1+qq2+qqi)/(qq1+qq2);
 for(int i=-5;i<=5;++i)if(i)for(int j=-5;j<=5;++j)if(j){
  double w=i==-j?qqb:qq1;if(i==j)w=.5*qq1*ratio;
  add(i,j,111,w);if(i==j)add(i,j,112,.5*qq2*ratio);
 }
 qqb=com*as2*4/9*(th2+uh2)/sh2*p.annihilation_weight;
 double gg1=com*as2*32/27*(uh/th-(2+m*1./4)*uh2/sh2);
 double gg2=com*as2*32/27*(th/uh-(2+m*1./4)*th2/sh2);
 for(int i=-5;i<=5;++i)if(i){add(i,-i,121,qqb);add(i,-i,131,.5*gg1);add(i,-i,132,.5*gg2);}
 double qg1=com*as2*4/9*((2+m*1./4)*uh2/th2-uh/sh)*faca;
 double qg2=com*as2*4/9*((2+m*1./4)*sh2/th2-sh/uh);
 for(int i=-5;i<=5;++i)if(i)for(int side=1;side<=2;++side){
  add(side==1?i:21,side==1?21:i,281,qg1);add(side==1?i:21,side==1?21:i,282,qg2);
 }
 double flavour=0;
 for(int i=0;i<3;++i)if(p.enabled[i])flavour=flavour+std::sqrt(std::max(0.,1-4*square(p.masses[i])/sh));
 qq1=com*as2*1/6*(uh/th-(2+m*1./4)*uh2/sh2)*flavour*faca;
 qq2=com*as2*1/6*(th/uh-(2+m*1./4)*th2/sh2)*flavour*faca;
 add(21,21,531,qq1);add(21,21,532,qq2);
 for(int flavour_index=3;flavour_index<5;++flavour_index){
  double mass2=square(p.masses[flavour_index]);
  if(p.enabled[flavour_index]&&sh>4.04*mass2){
   double beta=std::sqrt(1-4*mass2/sh);
   double tq=-.5*sh*(1-beta*p.cos_theta),uq=-.5*sh*(1+beta*p.cos_theta);
   double tu=tq*uq-mass2*sh;
   if(m==0){
    qq1=uq/tq-2*square(uq)/sh2+4*(mass2/sh)*tu/square(tq);
    qq2=tq/uq-2*square(tq)/sh2+4*(mass2/sh)*tu/square(uq);
   }else{
    qq1=uq/tq-2.25*square(uq)/sh2+4.5*(mass2/sh)*tu/square(tq)+.5*mass2*(tq+mass2)/square(tq)-square(mass2)/(sh*tq);
    qq2=tq/uq-2.25*square(tq)/sh2+4.5*(mass2/sh)*tu/square(uq)+.5*mass2*(uq+mass2)/square(uq)-square(mass2)/(sh*uq);
   }
   qq1=com*faca*as2*(1./6)*qq1*beta;qq2=com*faca*as2*(1./6)*qq2*beta;
   add(21,21,531+2*(flavour_index-2),qq1);add(21,21,532+2*(flavour_index-2),qq2);
  }
 }
 gg1=com*as2*9/4*(sh2/th2+2*sh/th+3+2*th/sh+th2/sh2)*faca;
 gg2=com*as2*9/4*(uh2/sh2+2*uh/sh+3+2*sh/uh+sh2/uh2)*faca;
 double gg3=com*as2*9/4*(th2/uh2+2*th/uh+3+2*uh/th+uh2/th2);
 add(21,21,681,.5*gg1);add(21,21,682,.5*gg2);add(21,21,683,.5*gg3);
 check(std::isfinite(out.total));return out;
}
}
