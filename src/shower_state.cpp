// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/shower_state.hpp"
#include <cmath>
#include <stdexcept>
#include <utility>
#include <limits>
namespace jewel {
namespace {
void require(bool b,const char* why){if(!b)throw std::domain_error(why);}
std::vector<ParticleId> origins(const ShowerParticle& s){
 require(!(s.parent&&s.collision_source),"ambiguous primary/collision ancestry");
 require(!s.coherent_partner||(s.parent&&s.coherent_partner!=s.parent&&!s.collision_source),"invalid coherent ancestry");
 std::vector<ParticleId> result;
 if(auto origin=s.parent?s.parent:s.collision_source)result.push_back(origin);
 if(s.coherent_partner)result.push_back(s.coherent_partner);
 return result;
}
void fields(const ShowerParticle& s){
 auto p=s.particle.momentum;auto x=s.particle.production;auto v=s.generator_vertex;
 for(double value:{p.px,p.py,p.pz,p.energy,x.x,x.y,x.z,x.t,v.x,v.y,v.z,v.t,s.record_mass,s.decay_time,s.evolution_time,s.fraction,s.angle})require(std::isfinite(value),"nonfinite shower record");
 require(s.particle.colour>=0&&s.particle.anticolour>=0,"negative colour index");
}
}
ShowerState::ShowerState(double unused):unused_evolution_time_(unused){require(std::isfinite(unused),"nonfinite unused evolution time");}
ParticleId ShowerState::append(ShowerParticle s,bool inherit_evolution_time,bool inherit_medium_induced){
 return append_impl(std::move(s),inherit_evolution_time,inherit_medium_induced,true);
}
ParticleId ShowerState::append_impl(ShowerParticle s,bool inherit_evolution_time,bool inherit_medium_induced,bool check_fields){
 if(inherit_evolution_time)s.evolution_time=particles_.size()<retired_evolution_.size()?retired_evolution_[particles_.size()]:unused_evolution_time_;
 if(inherit_medium_induced)s.medium_induced=particles_.size()<retired_medium_induced_.size()?retired_medium_induced_[particles_.size()]:false;
 if(check_fields)fields(s);require(next_!=std::numeric_limits<ParticleId>::max(),"particle ID space exhausted");
 require(s.daughters[0]==0&&s.daughters[1]==0,"append requires unattached daughters");
 auto ancestry=origins(s);for(auto origin:ancestry)at(origin);
 auto id=next_;s.particle.id=id;
 s.particle.parents=std::move(ancestry);
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
void ShowerState::connect_collision(ParticleId incoming,ParticleId recoil,ParticleId outgoing){
 require(incoming!=recoil&&incoming!=outgoing&&recoil!=outgoing,"invalid collision IDs");
 auto& a=at(incoming);const auto& b=at(recoil);const auto& c=at(outgoing);
 require(b.parent==0&&b.collision_source==incoming&&c.parent==incoming&&c.collision_source==0,"collision ancestry mismatch");
 a.daughters={recoil,outgoing};
}
void ShowerState::connect_coherent(ParticleId first,ParticleId second,ParticleId coherent){
 require(first!=second&&first!=coherent&&second!=coherent,"invalid coherent IDs");
 auto& a=at(first);auto& b=at(second);const auto& c=at(coherent);
 require(a.parent&&a.parent==b.parent,"coherent inputs are not siblings");
 require(c.parent==first&&c.coherent_partner==second&&!c.collision_source,"coherent ancestry mismatch");
 a.particle.status=20;b.particle.status=20;
 a.daughters={coherent,coherent};b.daughters={coherent,coherent};
}
void ShowerState::validate() const {
 require(std::isfinite(weight),"nonfinite event weight");require(colour_max>=0,"negative maximum colour");
 require(slots_.size()==particles_.size(),"shower index mismatch");
 for(double value:retired_evolution_)require(std::isfinite(value),"nonfinite retired evolution time");
 // ISR can insert a new ancestor after its child; IDs do not encode genealogy order.
 std::unordered_map<ParticleId,int> visited;
 for(const auto& s:particles_){
  // Iterative DFS visits BOTH merge inputs, including ancestors inserted by ISR.
  std::vector<std::pair<ParticleId,bool>> pending{{s.particle.id,false}};
  while(!pending.empty()){
   auto [id,leaving]=pending.back();pending.pop_back();
   if(leaving){visited[id]=2;continue;}
   if(visited[id]==2)continue;
   require(visited[id]!=1,"cyclic shower ancestry");visited[id]=1;
   pending.emplace_back(id,true);
   for(auto origin:origins(at(id)))pending.emplace_back(origin,false);
  }
 }
 for(const auto& s:particles_){
  fields(s);auto id=s.particle.id;require(&at(id)==&s,"particle ID changed");
  require(s.particle.colour<=colour_max&&s.particle.anticolour<=colour_max,"colour exceeds allocated maximum");
  auto ancestry=origins(s);for(auto origin:ancestry)at(origin);
  require(s.particle.parents==ancestry,"parent metadata mismatch");
  if(s.coherent_partner){
   const auto& a=at(s.parent);const auto& b=at(s.coherent_partner);
   require(a.parent&&a.parent==b.parent,"coherent inputs are not siblings");
   for(const auto* incoming:{&a,&b})require(incoming->particle.status==20&&incoming->daughters==std::array<ParticleId,2>{id,id},"incomplete coherent merge");
  }
  require((s.daughters[0]==0)==(s.daughters[1]==0),"incomplete daughter pair");
  if(s.daughters[0]){
   if(s.daughters[0]==s.daughters[1]){
    const auto& child=at(s.daughters[0]);
    require(s.particle.status==20&&child.coherent_partner&&(child.parent==id||child.coherent_partner==id),"invalid shared coherent daughter");
   }
   for(auto daughter:s.daughters){const auto& child=at(daughter);require(child.parent==id||child.collision_source==id||child.coherent_partner==id,"nonreciprocal daughter link");}

  }
 }
}
ShowerSnapshot ShowerState::snapshot() const {validate();return {particles_,colour_max,weight,discarded,unused_evolution_time_,retired_evolution_,retired_medium_induced_};}
ShowerSnapshot ShowerState::snapshot_truncated(std::size_t count) const {
 return snapshot_truncated_impl(count,true);
}
ShowerSnapshot ShowerState::snapshot_truncated_impl(std::size_t count,bool check_fields) const {
 require(count<=particles_.size(),"invalid truncated shower size");
 if(check_fields)validate();
 ShowerSnapshot saved{particles_,colour_max,weight,discarded,unused_evolution_time_,retired_evolution_,retired_medium_induced_};saved.retired_evolution.resize(std::max(saved.retired_evolution.size(),particles_.size()));
 saved.retired_medium_induced.resize(std::max(saved.retired_medium_induced.size(),particles_.size()));
 for(std::size_t i=count;i<particles_.size();++i){saved.retired_evolution[i]=particles_[i].evolution_time;saved.retired_medium_induced[i]=particles_[i].medium_induced;}
 saved.particles.resize(count);return saved;
}
void ShowerState::restore(const ShowerSnapshot& snapshot){
 ShowerState candidate(snapshot.unused_evolution_time);candidate.particles_=snapshot.particles;candidate.colour_max=snapshot.colour_max;candidate.weight=snapshot.weight;candidate.discarded=snapshot.discarded;candidate.retired_evolution_=snapshot.retired_evolution;candidate.retired_medium_induced_=snapshot.retired_medium_induced;
 ParticleId maximum=0;
 for(std::size_t i=0;i<candidate.particles_.size();++i){auto id=candidate.particles_[i].particle.id;require(id>0&&candidate.slots_.emplace(id,i).second,"invalid snapshot IDs");maximum=std::max(maximum,id);}
 require(maximum<std::numeric_limits<ParticleId>::max(),"snapshot ID space exhausted");candidate.next_=std::max(next_,maximum+1);candidate.validate();
 *this=std::move(candidate);
}
}
