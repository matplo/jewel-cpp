// SPDX-License-Identifier: GPL-2.0-or-later
// Port of DOCOHSCATTERING sections, JEWEL 2.6.0; see reference/coherence-source.json.
#include "jewel/coherence.hpp"
#include "jewel/kinematics.hpp"
#include <cmath>
#include <algorithm>
#include <stdexcept>
namespace jewel {
namespace {
void finite(double x) { if(!std::isfinite(x))throw std::domain_error("nonfinite coherent input/result"); }
void finite(Momentum p) { for(double x:{p.px,p.py,p.pz,p.energy})finite(x); }
}
CoherentStart begin_coherent(ShowerState& state,ParticleId selected,bool enabled,double start,double duration,double angle){
 for(double x:{start,duration,angle})finite(x);
 CoherentStart r;r.selected=selected;r.duration=duration;r.original_size=state.particles().size();
 if(!enabled)return r;
 r.remaining=duration;
 const auto original=state.at(selected);
 if(!original.parent)return r; // Stock assumes a valid mother index.
 const auto mother=state.at(original.parent);r.mother=original.parent;
 if(!mother.parent)return r;
 auto sibling=mother.daughters[mother.daughters[0]==selected?1:0];
 if(!sibling)return r;
 // Stock orders by record slot; stable IDs need not follow slot order after restore.
 for(const auto& node:state.particles())if(node.particle.id==selected||node.particle.id==sibling){r.first=node.particle.id;break;}
 r.second=r.first==selected?sibling:selected;
 const auto other=state.at(sibling);
 r.quark=mother.particle.pdg!=21;
 r.quark_pair=!r.quark&&state.at(r.first).particle.pdg!=21;
 r.colour_direction=r.quark||r.quark_pair?0:(state.at(r.first).particle.colour==mother.particle.colour?1:-1);
 if(mother.particle.status!=11||other.particle.status!=1||other.decay_time<start||angle<0)return r;
 if(other.decay_time>0&&other.decay_time<start+duration)r.duration=other.decay_time-start;
 if(r.duration<0)return r;
 if(state.particles().size()>=22990){state.discarded=true;return r;}
 r.first_before=state.at(r.first);r.second_before=state.at(r.second);
 if(mother.particle.momentum.energy==0)throw std::domain_error("zero coherent mother energy");
 r.fraction=r.second_before.particle.momentum.energy/mother.particle.momentum.energy;finite(r.fraction);
 double theta=polar_angle(mother.particle.momentum),phi=azimuth(mother.particle.momentum);
 auto second=r.second_before;
 auto rotate=[&](double t,double p){if(second.particle.status>0)rotate_boost(second.particle.momentum,&second.generator_vertex,t,p,{0,0,0});};
 rotate(0,-phi);rotate(-theta,0);r.azimuth=azimuth(second.particle.momentum);rotate(theta,0);rotate(0,phi);
 ShowerParticle merged;
 merged.parent=r.first;merged.coherent_partner=r.second;
 merged.particle.status=1;merged.particle.pdg=mother.particle.pdg;
 merged.particle.momentum=mother.particle.momentum;merged.record_mass=mother.record_mass;
 merged.particle.production=original.particle.production;
 merged.particle.colour=mother.particle.colour;merged.particle.anticolour=mother.particle.anticolour;
 merged.angle=mother.angle;merged.fraction=mother.fraction;merged.quark_pair=mother.quark_pair;
 r.coherent=state.append(merged,true);
 state.at(r.second)=second;
 state.at(r.first).decay_time=state.at(r.first).particle.production.t;
 state.at(r.second).decay_time=state.at(r.second).particle.production.t;
 state.connect_coherent(r.first,r.second,r.coherent);r.active=true;return r;
}
CoherentResolution coherent_resolution(Momentum p,const TransferRecord& q,double factor,double angle,double time,double split_time) {
 finite(p);for(double x:{q.invariant,q.momentum.px,q.momentum.py,factor,angle,time,split_time})finite(x);
 double norm=p.px*p.px+p.py*p.py+p.pz*p.pz;
 if(norm<=0)throw std::domain_error("zero coherent projection momentum");
 double dot=q.invariant*p.px+q.momentum.px*p.py+q.momentum.py*p.pz;
 double q2=q.invariant*q.invariant+q.momentum.px*q.momentum.px+q.momentum.py*q.momentum.py;
 CoherentResolution r;
 r.transverse_transfer=std::sqrt(q2-dot*dot/norm);finite(r.transverse_transfer);
 r.separation=factor*angle*(time-split_time);finite(r.separation);
 r.threshold=double(.2f)/r.separation;
 r.resolved=r.transverse_transfer>r.threshold;
 return r;
}
CoherentMomenta reconstruct_coherent(Momentum p,SpaceTime v,double mass,double ma,double mb,double z,double phi) {
 finite(p);for(double x:{v.x,v.y,v.z,v.t,mass,ma,mb,z,phi})finite(x);
 if(mass<0||ma<0||mb<0||z<=0||z>=1||p.energy<=0)throw std::domain_error("invalid coherent reconstruction");
 double theta=polar_angle(p),az=azimuth(p);
 rotate_boost(p,&v,0,-az,{0,0,0});rotate_boost(p,&v,-theta,0,{0,0,0});
 if(p.pz==0)throw std::domain_error("zero axial coherent momentum");
 CoherentMomenta r;r.parent=p;r.generator_vertex=v;
 r.longitudinal=(2*z*(p.energy*p.energy)-mass*mass-mb*mb+ma*ma)/(2*p.pz);
 r.transverse_squared=std::fma(-r.longitudinal,r.longitudinal,z*z*(p.energy*p.energy))-mb*mb;
 finite(r.longitudinal);finite(r.transverse_squared);
 if(r.transverse_squared<0)return r;
 double pt=std::sqrt(r.transverse_squared);
 r.first={-pt*std::cos(phi),-pt*std::sin(phi),p.pz-r.longitudinal,(1-z)*p.energy};
 r.second={pt*std::cos(phi),pt*std::sin(phi),r.longitudinal,z*p.energy};
 for(auto m:{&r.parent,&r.first,&r.second}) {
  auto vertex=m==&r.parent?&r.generator_vertex:nullptr;
  rotate_boost(*m,vertex,theta,0,{0,0,0});rotate_boost(*m,vertex,0,az,{0,0,0});finite(*m);
 }
 r.possible=true;return r;
}
}
