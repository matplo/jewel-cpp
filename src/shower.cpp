// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/shower.hpp"
#include "jewel/kinematics.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace jewel {
namespace {
void rotate(ShowerParticle& p,double theta,double phi){
 if(p.particle.status>0)rotate_boost(p.particle.momentum,&p.generator_vertex,theta,phi,{0,0,0});
}
}
void reject_splitting(ShowerState& state,ParticleId mother,const BranchingSampler& sampler,Random& random){
 auto& m=state.at(mother);auto first=m.daughters[0],second=m.daughters[1];
 if(!first||!second)throw std::domain_error("rejected splitting requires two siblings");
 for(auto sibling:{first,second}){
  auto id=sibling;
  for(std::size_t visited=0;state.at(id).particle.status==12;++visited){
   if(visited>=state.particles().size())throw std::domain_error("cyclic rejected-scattering chain");
   auto& p=state.at(id);
   for(auto child:p.daughters)state.at(child).particle.status=-1;
   id=p.parent;
  }
  if(state.at(sibling).particle.status==12)state.at(sibling).particle.status=1;
 }
 auto& c=state.at(first);auto& b=state.at(second);
 double theta=polar_angle(m.particle.momentum),phi=azimuth(m.particle.momentum);
 // PYROBO rotates the inclusive event-record range, not just the endpoints.
 std::vector<ParticleId> range;bool inside=false;
 for(const auto& p:state.particles()){
  if(p.particle.id==first)inside=true;
  if(inside)range.push_back(p.particle.id);
  if(p.particle.id==second)break;
 }
 if(range.empty()||range.back()!=second)throw std::domain_error("reversed sibling record range");
 rotate(m,0,-phi);rotate(m,-theta,0);
 for(auto id:range)rotate(state.at(id),0,-phi);
 for(auto id:range)rotate(state.at(id),-theta,0);
 auto& pm=m.particle.momentum;auto& pb=b.particle.momentum;auto& pc=c.particle.momentum;
 double pts=pc.px*pc.px+pc.py*pc.py;
 double mb=b.record_mass,mc=c.record_mass,mbnew=mb,mcnew=mc;
 bool newb=b.angle>m.angle,newc=c.angle>m.angle;
 double bmax=std::min(mb,pb.energy),cmax=std::min(mc,pc.energy),pznew=0,ptsnew=0;
 auto sample=[&](ShowerParticle& p,double previous,double maximum){
  auto value=sampler.mass(0,previous,m.angle,p.particle.momentum.energy,
    p.particle.pdg==21?Channel::GC:Channel::QQ,maximum,false,random);
  p.record_mass=value.mass;p.fraction=value.fraction.value();p.quark_pair=value.quark_pair.value();
  p.angle=p.fraction>0?value.mass/(std::sqrt(p.fraction*(1-p.fraction))*p.particle.momentum.energy):0.;
  return value.mass;
 };
 for(unsigned trial=0;;++trial){
  if(trial==1000000)throw std::runtime_error("rejected splitting failed to converge");
  if(newc)mcnew=sample(c,mc,cmax);
  if(newb)mbnew=sample(b,mb,bmax);
  if(pm.pz==0)throw std::domain_error("undefined parent splitting axis");
  pznew=(2*m.fraction*pm.energy*pm.energy-m.record_mass*m.record_mass-mbnew*mbnew+mcnew*mcnew)/(2*pm.pz);
  ptsnew=m.fraction*m.fraction*pm.energy*pm.energy-pznew*pznew-mbnew*mbnew;
  if(ptsnew>=0)break;
  if(mbnew==0&&mcnew==0){state.discarded=true;return;}
  if(mbnew==0){cmax=mcnew;newb=false;newc=true;}
  else if(mcnew==0){bmax=mbnew;newc=false;newb=true;}
  else if(random.uniform()>.5){newb=false;newc=true;cmax=mcnew;}
  else {newb=true;newc=false;bmax=mbnew;}
 }
 if(pts<=0)throw std::domain_error("undefined zero transverse momentum rescaling");
 pb.px*=std::sqrt(ptsnew/pts);pb.py*=std::sqrt(ptsnew/pts);pb.pz=pznew;
 pc.px=pm.px-pb.px;pc.py=pm.py-pb.py;pc.pz=pm.pz-pb.pz;
 if(newc)c.evolution_time=c.decay_time;
 if(newb)b.evolution_time=b.decay_time;
 if(mcnew==0)c.decay_time=0;
 else c.decay_time=c.particle.production.t+(c.decay_time-c.particle.production.t)*mc*mc/(mcnew*mcnew);
 if(mbnew==0)b.decay_time=0;
 else b.decay_time=b.particle.production.t+(b.decay_time-b.particle.production.t)*mb*mb/(mbnew*mbnew);
 rotate(m,theta,0);rotate(m,0,phi);
 for(auto id:range)rotate(state.at(id),theta,0);
 for(auto id:range)rotate(state.at(id),0,phi);
}
}
