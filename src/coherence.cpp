// SPDX-License-Identifier: GPL-2.0-or-later
// Port of DOCOHSCATTERING sections, JEWEL 2.6.0; see reference/coherence-source.json.
#include "jewel/coherence.hpp"
#include "jewel/kinematics.hpp"
#include <cmath>
#include <stdexcept>
namespace jewel {
namespace {
void finite(double x) { if(!std::isfinite(x))throw std::domain_error("nonfinite coherent input/result"); }
void finite(Momentum p) { for(double x:{p.px,p.py,p.pz,p.energy})finite(x); }
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
