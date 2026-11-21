// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/pythia8_isr.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <unordered_set>
namespace jewel {
namespace {void require(bool b,const char* message){if(!b)throw std::domain_error(message);}}
namespace {
std::array<IsrIncomingChain,2> map_chains(const HardEventSnapshot& input,
 const std::array<ParticleId,2>& incoming,bool staged,
 std::unordered_set<ParticleId>& used,std::unordered_set<ParticleId>& emissions){
 auto at=[&](ParticleId id)->const GeneratorParticle&{
  require(id>0&&id<=input.particles.size(),"missing ISR ancestry line");return input.particles[id-1];};
 std::array<IsrIncomingChain,2> result;
 for(std::size_t side=0;side<2;++side){auto& chain=result[side];chain.hard_incoming=incoming[side];auto current=chain.hard_incoming;
  for(;;){
   require(used.insert(current).second,"cyclic or shared ISR incoming ancestry");
   const auto& child=at(current);chain.backward_lines.push_back(current);
   require(child.mothers.size()==1,"ISR incoming line requires one mother");
   const auto& parent=at(child.mothers[0]);
   require(std::find(parent.daughters.begin(),parent.daughters.end(),current)!=parent.daughters.end(),"nonreciprocal ISR incoming ancestry");
   int status=std::abs(parent.status);
   if(status==12){require(std::abs(child.status)==61,"ISR beam lacks primordial incoming copy");chain.beam=parent.id;break;}
   require(status==41||status==42||status==61,"unsupported ISR incoming status");
   if(status==41){
    require(parent.daughters.size()==2,"ISR radiator requires a daughter and emission");
    auto id=parent.daughters[0]==current?parent.daughters[1]:parent.daughters[0];const auto& emitted=at(id);
    require(std::abs(emitted.status)==43&&emitted.mothers.size()==1&&emitted.mothers[0]==parent.id,"invalid ISR emitted sibling");
    require(emissions.insert(id).second,"duplicated ISR emitted sibling");
    auto found=std::find(input.isr_emissions.begin(),input.isr_emissions.end(),id);
    require(found!=input.isr_emissions.end(),"ISR sibling absent from emission ledger");
    auto p=parent.momentum;auto s=emitted.momentum;
    if(staged){require(parent.energy_before_beam_repair.has_value()&&emitted.energy_before_beam_repair.has_value(),"missing source-stage ISR energy");
     p.energy=*parent.energy_before_beam_repair;s.energy=*emitted.energy_before_beam_repair;}
    Momentum q{p.px-s.px,p.py-s.py,p.pz-s.pz,p.energy-s.energy};
    double q2=q.px*q.px+q.py*q.py+q.pz*q.pz-q.energy*q.energy;
    require(std::isfinite(q2)&&q2>0,"unsupported non-spacelike ISR branch reconstruction");
    chain.branches.push_back({parent.id,current,id,input.isr_final[found-input.isr_emissions.begin()],q,q2});
   }else{
    require(parent.daughters.size()==1&&parent.pdg==child.pdg,"invalid ISR incoming recoil copy");
   }
   current=parent.id;
  }
 }
 require(result[0].beam!=result[1].beam,"ISR incoming chains share a beam");

 return result;
}
} // namespace
std::array<IsrIncomingChain,2> map_pythia_isr_ancestry(const HardEventSnapshot& input){
 validate_hard_snapshot(input);require(!input.multiple_parton_interactions,"single-system recoil mapping does not support MPI snapshots");
 require(input.initial_state_radiation,"ISR ancestry mapping requires an ISR-enabled snapshot");
 std::unordered_set<ParticleId> used,emissions;
 auto result=map_chains(input,input.hard_incoming,false,used,emissions);
 require(emissions.size()==input.isr_emissions.size(),"unmapped ISR emission outside incoming chains");return result;
}
std::vector<std::array<IsrIncomingChain,2>> map_subsystem_isr_ancestry(const HardEventSnapshot& input){
 validate_hard_snapshot(input);
 std::vector<std::array<IsrIncomingChain,2>> result;
 std::unordered_set<ParticleId> used,emissions;
 for(const auto& system:input.subsystems){
  const auto& mothers=input.particles[system.process_outgoing[0]-1].mothers;
  require(mothers.size()==2,"subsystem lacks two incoming process roots");
  std::array<ParticleId,2> roots{mothers[0],mothers[1]};
  auto chains=map_chains(input,roots,true,used,emissions);
  for(std::size_t side=0;side<2;++side){const auto& chain=chains[side];
   require(chain.backward_lines.back()==system.incoming[side],"ISR chain terminates in a different subsystem");
   for(const auto& branch:chain.branches)require(std::find(system.outgoing_final.begin(),system.outgoing_final.end(),branch.emitted_final)!=system.outgoing_final.end(),"ISR emission belongs to a different subsystem");
  }
  result.push_back(std::move(chains));
 }
 require(emissions.size()==input.isr_emissions.size(),"unmapped ISR emission outside subsystem chains");
 return result;
}

}
