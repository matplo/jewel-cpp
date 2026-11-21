// SPDX-License-Identifier: GPL-2.0-or-later
// GETDELTAT from JEWEL 2.6.0: collision-time veto sampling.
#include "jewel/scattering.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace jewel {
CollisionTime ScatteringRates::next_collision(Momentum p,double mass,int pdg,
 SpaceTime production,double start,double maximum_interval,double lifetime,
 double scale,bool elastic,Random& random) const {
 for(double x:{p.px,p.py,p.pz,p.energy,mass,production.x,production.y,production.z,
 production.t,start,maximum_interval,lifetime,scale})
  if(!std::isfinite(x))throw std::domain_error("nonfinite collision-time input");
 if(p.energy<=0||maximum_interval<0||scale<0)
  throw std::domain_error("invalid collision-time domain");
 CollisionTime result;
 if(medium_.is_vacuum())return result;
 double limit=maximum_interval;
 if(start+limit>=lifetime)limit=lifetime-start;
 if(limit<0)return result;
 char incoming=pdg==21?'G':'Q',outgoing=elastic?incoming:'C';
 double offset=start-production.t;
 SpaceTime initial{production.x+offset*p.px/p.energy,
  production.y+offset*p.py/p.energy,production.z+offset*p.pz/p.energy,start};
 double maximum=cross_section(p,mass,0,incoming,outgoing,initial,1);
 double minimum=cross_section(p,mass,0,incoming,outgoing,initial,2);
 auto bounds=medium_.bounds();
 double inverse=scale*5.*std::max(bounds.density_at_debye_min*maximum,bounds.density_max*minimum);
 if(inverse==0)return result;
 if(!std::isfinite(inverse)||inverse<0)throw std::domain_error("invalid collision-rate bound");
 int empty=0;
 for(int i=0;i<1000000;++i){
  result.elapsed-=std::log(random.uniform())/inverse;
  SpaceTime position{initial.x+result.elapsed*p.px/p.energy,
   initial.y+result.elapsed*p.py/p.energy,initial.z+result.elapsed*p.pz/p.energy,
   start+result.elapsed};
  // Stock uses TS < ZS here. The medium API separately defines empty density
  // outside the forward light cone; no Fortran parity is claimed there.
  double tau=position.t<position.z?-1.:std::sqrt(position.t*position.t-position.z*position.z);
  double density=medium_.sample(position).density;
  bool stop=false;
  if(tau>1&&density==0){if(empty>4)stop=true;else ++empty;}else empty=0;
  if(result.elapsed>limit||stop){result.elapsed=limit;return result;}
  double sigma=density>0?cross_section(p,mass,0,incoming,outgoing,position,0)*scale:0.;
  double weight=5.*density*sigma/inverse;
  if(weight>1.+1e-6)++result.bound_violations;
  if(random.uniform()<weight){result.collision=true;return result;}
 }
 throw std::runtime_error("collision-time veto exhausted one million trials");
}
}
