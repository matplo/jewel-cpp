// SPDX-License-Identifier: GPL-2.0-or-later
// Adapted from JEWEL 2.6.0 subtraction.C; see NOTICE.
#include "jewel/subtraction.hpp"
#include <cmath>
#include <cstdlib>
#include <stdexcept>
namespace jewel {
namespace {
struct Cylindrical {double pt,mdelta,phi,y;};
struct Pair {double distance;std::size_t particle,thermal;};
int compare(const void* a,const void* b){
 double x=static_cast<const Pair*>(a)->distance,y=static_cast<const Pair*>(b)->distance;
 return x<y?-1:x>y?1:0;
}
double square(double x){return x*x;}
bool convert(Momentum p,Cylindrical& c){
 for(double x:{p.px,p.py,p.pz,p.energy})if(!std::isfinite(x))throw std::domain_error("nonfinite subtraction input");
 if(p.energy-std::abs(p.pz)<=0)return false;
 double pt=std::sqrt(square(p.px)+square(p.py));
 double mass2=square(p.energy)-square(pt)-square(p.pz);
 c={pt,std::sqrt(mass2+square(pt))-pt,std::atan2(p.py,p.px),.5*std::log((p.energy+p.pz)/(p.energy-p.pz))};
 for(double x:{c.pt,c.mdelta,c.phi,c.y})if(!std::isfinite(x))throw std::domain_error("nonfinite subtraction coordinates");
 return true;
}
SubtractionParticle restore(const Cylindrical& c,ParticleId source,int pdg,bool thermal){
 double mt=c.pt+c.mdelta;
 Momentum p{c.pt*std::cos(c.phi),c.pt*std::sin(c.phi),mt*std::sinh(c.y),mt*std::cosh(c.y)};
 double mass=std::sqrt(square(mt)-square(c.pt));
 for(double x:{p.px,p.py,p.pz,p.energy,mass})if(!std::isfinite(x))throw std::domain_error("nonfinite subtraction result");
 return {source,pdg,p,mass,thermal};
}
}
SubtractionResult subtract_thermal_momenta(const std::vector<SubtractionParticle>& input,
 const std::vector<Momentum>& thermals,double radius,Random& random){
 if(!std::isfinite(radius)||radius<0)throw std::domain_error("invalid subtraction radius");
 SubtractionResult result;std::vector<Cylindrical> particles,thermal;
 std::vector<std::size_t> sources;
 for(std::size_t i=0;i<input.size();++i){Cylindrical p;if(convert(input[i].momentum,p)){particles.push_back(p);sources.push_back(i);}else ++result.skipped_particles;}
 for(auto p:thermals){Cylindrical t;if(convert(p,t))thermal.push_back(t);else ++result.skipped_thermals;}
 std::vector<Pair> pairs;
 if(!thermal.empty()&&particles.size()>pairs.max_size()/thermal.size())throw std::length_error("too many subtraction pairs");
 pairs.reserve(particles.size()*thermal.size());
 constexpr double pi=3.14159265358979323846;
 for(std::size_t i=0;i<particles.size();++i)for(std::size_t j=0;j<thermal.size();++j){
  double dphi=std::abs(particles[i].phi-thermal[j].phi);
  if(dphi>pi)dphi=2*pi-dphi;
  pairs.push_back({std::sqrt(square(dphi)+square(particles[i].y-thermal[j].y)),i,j});
 }
 // Upstream takes &vector[0] even when empty. Avoid that undefined access.
 if(!pairs.empty())std::qsort(pairs.data(),pairs.size(),sizeof(Pair),compare);
 for(const auto& pair:pairs){
  if(pair.distance>radius)break;
  auto& p=particles[pair.particle];auto& t=thermal[pair.thermal];
  if(p.pt>t.pt){p.pt-=t.pt;t.pt=0;}else{t.pt-=p.pt;p.pt=0;}
  if(p.mdelta>t.mdelta){p.mdelta-=t.mdelta;t.mdelta=0;}else{t.mdelta-=p.mdelta;p.mdelta=0;}
 }
 for(std::size_t i=0;i<particles.size();++i)if(particles[i].pt>0){
  const auto& source=input[sources[i]];
  result.particles.push_back(restore(particles[i],source.source,source.pdg,false));
 }
 for(const auto& t:thermal)if(t.pt>0){
  double draw=random.uniform();
  if(!std::isfinite(draw)||draw<0||draw>1)throw std::domain_error("invalid subtraction random draw");
  int pdg=draw<1./3?211:draw<2./3?-211:111;
  if(result.particles.size()+1>=23000){result.particles.clear();result.capacity_exceeded=true;return result;}
  result.particles.push_back(restore(t,0,pdg,true));
 }
 return result;
}
}
