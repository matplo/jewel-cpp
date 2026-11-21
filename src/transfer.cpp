// SPDX-License-Identifier: GPL-2.0-or-later
// GETQVEC from JEWEL 2.6.0.
#include "jewel/transfer.hpp"
#include "jewel/kinematics.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>
namespace jewel {
namespace {
void require(bool value,const char* message){if(!value)throw std::domain_error(message);}
double square(Momentum p){return p.energy*p.energy-p.px*p.px-p.py*p.py-p.pz*p.pz;}
void transform(ShowerParticle& p,double theta,double phi,std::array<double,3> beta){
 if(p.particle.status>0)rotate_boost(p.particle.momentum,&p.generator_vertex,theta,phi,beta);
}
}
TransferSampler::TransferSampler(Kernels k,ExpandingMedium medium,bool boost):
 kernels_(k),medium_(std::move(medium)),boost_(boost){}
void TransferSampler::sample(ShowerState& state,ParticleId projectile,ParticleId scratch,
 TransferHistory& history,std::size_t slot,double dt,double x,bool recalc,Random& random) const {
 if(slot>=10000){state.discarded=true;return;}
 require(projectile!=scratch,"projectile aliases transfer scratch record");
 require(std::isfinite(dt)&&std::isfinite(x)&&x>=0&&x<=1,"invalid transfer sampling input");
 for(double value:{history.sum.px,history.sum.py,history.sum.pz,history.sum.energy,history.sum_squared})
  require(std::isfinite(value),"nonfinite accumulated transfer");
 auto& leg=state.at(projectile);auto& temporary=state.at(scratch);
 auto& p=leg.particle.momentum;auto& q=temporary.particle.momentum;
 double mass=leg.record_mass;
 for(double value:{p.px,p.py,p.pz,p.energy,mass})require(std::isfinite(value),"nonfinite transfer projectile");
 require(p.energy>0&&momentum_magnitude(p)>0,"undefined transfer projectile");
 if(recalc)require(slot<history.records.size(),"missing transfer to recalculate");
 else if(slot>=history.records.size())history.records.resize(slot+1);
 auto& record=history.records[slot];
 if(recalc)require(std::isfinite(record.invariant)&&record.invariant<=0,"invalid stored transfer invariant");
 if(recalc){history.sum={};history.sum_squared=0;}
 auto birth=leg.particle.production;
 SpaceTime position=recalc?record.position:SpaceTime{birth.x+dt*p.px/p.energy,
  birth.y+dt*p.py/p.energy,birth.z+dt*p.pz/p.energy,birth.t+dt};
 double md=medium_.sample(position).debye_mass;
 auto average=medium_.average_scatterer(position,boost_);auto a=average.momentum;
 Momentum saved=p;
 // Stock reuses the rounded mass square across MAX and subtraction. Prevent
 // contraction of the latter into a fused multiply-subtract of the raw mass.
 volatile const double rescale_mass_squared=mass*mass;
 double xi=std::sqrt(std::max(x*x*(p.energy*p.energy),double(rescale_mass_squared))-rescale_mass_squared)/momentum_magnitude(p);
 p.px=xi*p.px;p.py=xi*p.py;p.pz=xi*p.pz;p.energy=std::max(x*p.energy,mass);
 double shat=average.mass*average.mass+mass*mass+2*(a.energy*saved.energy-a.px*saved.px-a.py*saved.py-a.pz*saved.pz);
 require(shat>0,"undefined transfer scattering invariant");
 double pcms2=std::pow(shat+mass*mass-average.mass*average.mass,2)/(4*shat)-mass*mass;
 double maxt=4*pcms2;if(recalc)maxt=std::min(maxt,p.energy*p.energy);
 require(std::isfinite(maxt)&&maxt>=0,"invalid transfer range");
 unsigned counter=0;
 for(;;){
  if(!recalc){record.scatterer=medium_.sample_scatterer(position,random,boost_);record.position=position;}
  auto centre=record.scatterer;
  temporary.particle.pdg=centre.type;q=centre.momentum;temporary.record_mass=centre.mass;
  temporary.particle.production=record.position;temporary.particle.status=1;
  require(q.energy>0&&std::isfinite(centre.mass)&&centre.mass>0,"undefined scatterer rest frame");
  std::array<double,3> beta{q.px/q.energy,q.py/q.energy,q.pz/q.energy};
  std::array<double,3> reverse{-beta[0],-beta[1],-beta[2]};
  transform(leg,0,0,reverse);transform(temporary,0,0,reverse);
  double theta=polar_angle(p),phi=azimuth(p);
  transform(leg,0,-phi,{});transform(temporary,0,-phi,{});
  transform(leg,-theta,0,{});transform(temporary,-theta,0,{});
  double t=recalc?std::max(record.invariant,-maxt):-kernels_.gett(0,maxt,md,random);
  double energy,pz,pt2;
  for(unsigned fallback=0;;++fallback){
   require(p.pz!=0,"undefined transfer longitudinal axis");
   energy=p.energy+t/(2*centre.mass);
   pz=(t-2*(mass*mass)+2*p.energy*energy)/(2*p.pz);
   pt2=energy*energy-pz*pz-mass*mass;
   if(std::abs(pt2)<1e-10||t==0)pt2=0;
   if(pt2>=0)break;
   require(fallback==0,"transfer zero-kick fallback failed");t=0;
  }
  double phi2=random.uniform()*2*3.141592653589793,pt=std::sqrt(pt2);
  q={pt*std::cos(phi2)-p.px,pt*std::sin(phi2)-p.py,pz-p.pz,energy-p.energy};
  temporary.record_mass=0;
  transform(leg,theta,0,{});transform(temporary,theta,0,{});
  transform(leg,0,phi,{});transform(temporary,0,phi,{});
  transform(leg,0,0,beta);transform(temporary,0,0,beta);
  record.invariant=t;record.momentum=q;
  auto& sum=history.sum;
  sum.px+=q.px;sum.py+=q.py;sum.pz+=q.pz;sum.energy+=q.energy;history.sum_squared=square(sum);
  if(history.sum_squared<=0)break;
  sum.px-=q.px;sum.py-=q.py;sum.pz-=q.pz;sum.energy-=q.energy;history.sum_squared=square(sum);
  if(counter>1000){record.invariant=0;record.momentum={};break;}
  ++counter;
 }
 p=saved;temporary.particle.status=13;
}
}
