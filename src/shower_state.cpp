// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/shower_state.hpp"
#include <cmath>
#include <stdexcept>
#include <utility>
#include <limits>
namespace jewel {
namespace {
void require(bool b,const char* why){if(!b)throw std::domain_error(why);}
void fields(const ShowerParticle& s){
 auto p=s.particle.momentum;auto x=s.particle.production;auto v=s.generator_vertex;
 for(double value:{p.px,p.py,p.pz,p.energy,x.x,x.y,x.z,x.t,v.x,v.y,v.z,v.t,s.record_mass,s.decay_time,s.evolution_time,s.fraction,s.angle})require(std::isfinite(value),"nonfinite shower record");
 require(s.particle.colour>=0&&s.particle.anticolour>=0,"negative colour index");
}
}
ParticleId ShowerState::append(ShowerParticle s){
 fields(s);require(next_!=std::numeric_limits<ParticleId>::max(),"particle ID space exhausted");
 require(s.daughters[0]==0&&s.daughters[1]==0,"append requires unattached daughters");
 if(s.parent)at(s.parent);
 auto id=next_;s.particle.id=id;
 s.particle.parents=s.parent?std::vector<ParticleId>{s.parent}:std::vector<ParticleId>{};
 auto inserted=slots_.emplace(id,particles_.size());
 try{particles_.push_back(std::move(s));}catch(...){slots_.erase(inserted.first);throw;}
 ++next_;return id;
}
const ShowerParticle& ShowerState::at(ParticleId id) const {
 auto it=slots_.find(id);if(it==slots_.end())throw std::out_of_range("unknown shower particle ID");return particles_[it->second];
}
ShowerParticle& ShowerState::at(ParticleId id){return const_cast<ShowerParticle&>(std::as_const(*this).at(id));}
void ShowerState::connect(ParticleId parent,ParticleId first,ParticleId second){
 require(parent!=first&&parent!=second&&first!=second,"invalid daughter IDs");
 auto& a=at(parent);auto& b=at(first);auto& c=at(second);
 require(a.daughters[0]==0&&a.daughters[1]==0,"parent already connected");
 require(b.parent==parent&&c.parent==parent,"daughter parent mismatch");
 a.daughters={first,second};
}
void ShowerState::validate() const {
 require(std::isfinite(weight),"nonfinite event weight");require(colour_max>=0,"negative maximum colour");
 require(slots_.size()==particles_.size(),"shower index mismatch");
 // ISR can insert a new ancestor after its child; IDs do not encode genealogy order.
 std::unordered_map<ParticleId,int> visited;
 for(const auto& s:particles_){
  std::vector<ParticleId> path;auto id=s.particle.id;
  while(id&&visited[id]!=2){require(visited[id]!=1,"cyclic shower ancestry");visited[id]=1;path.push_back(id);id=at(id).parent;}
  for(auto entry:path)visited[entry]=2;
 }
 for(const auto& s:particles_){
  fields(s);auto id=s.particle.id;require(&at(id)==&s,"particle ID changed");
  require(s.particle.colour<=colour_max&&s.particle.anticolour<=colour_max,"colour exceeds allocated maximum");
  if(s.parent)at(s.parent);
  require(s.particle.parents==(s.parent?std::vector<ParticleId>{s.parent}:std::vector<ParticleId>{}),"parent metadata mismatch");
  require((s.daughters[0]==0)==(s.daughters[1]==0),"incomplete daughter pair");
  if(s.daughters[0]){
   require(s.daughters[0]!=s.daughters[1],"duplicate daughters");
   for(auto daughter:s.daughters)require(at(daughter).parent==id,"nonreciprocal daughter link");
  }
 }
}
ShowerSnapshot ShowerState::snapshot() const {validate();return {particles_,colour_max,weight,discarded};}
void ShowerState::restore(const ShowerSnapshot& snapshot){
 ShowerState candidate;candidate.particles_=snapshot.particles;candidate.colour_max=snapshot.colour_max;candidate.weight=snapshot.weight;candidate.discarded=snapshot.discarded;
 ParticleId maximum=0;
 for(std::size_t i=0;i<candidate.particles_.size();++i){auto id=candidate.particles_[i].particle.id;require(id>0&&candidate.slots_.emplace(id,i).second,"invalid snapshot IDs");maximum=std::max(maximum,id);}
 require(maximum<std::numeric_limits<ParticleId>::max(),"snapshot ID space exhausted");candidate.next_=std::max(next_,maximum+1);candidate.validate();
 *this=std::move(candidate);
}
}
