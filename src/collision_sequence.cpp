// SPDX-License-Identifier: GPL-2.0-or-later
// DOKINEMATICS sequence orchestration, JEWEL 2.6.0.
#include "jewel/collision.hpp"
#include "collision_internal.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace jewel {
static CollisionSequenceResult apply_candidates(ShowerState& state,ParticleId projectile,ParticleId scratch,
 const TransferHistory& history,RecoilStorage& ledger,CollisionSequenceRequest request,CollisionConfig config,
 CollisionFinalizationConfig finish,const BranchingSampler& sampler,const Medium& medium,Random& random){
 if(projectile==scratch||request.first>request.last||request.last>=history.records.size())throw std::domain_error("invalid collision sequence range/IDs");
 for(double v:{request.target_mass,request.time,request.energy_fraction,request.fraction})
  if(!std::isfinite(v))throw std::domain_error("nonfinite collision sequence input");
 state.at(scratch);const auto baseline=state.snapshot();const auto size=state.particles().size();
 CollisionSequenceResult result{projectile,request.last,0,request.fraction,request.retry_splitting};
 if(size+2*(request.last-request.first+1)>22990){state.discarded=true;return result;}
 const int original_status=state.at(projectile).particle.status;
 const double first_decay=state.at(projectile).decay_time;
 const auto stored=ledger.used;double target=request.target_mass,last_mass=state.at(projectile).record_mass;
 for(unsigned retry=0;;++retry){
  if(retry==100000)throw std::runtime_error("collision sequence retries exhausted");
  result.retries=retry;double delta=target-state.at(projectile).record_mass,remaining=delta,total=0;
  for(auto j=request.first;j<=result.last;++j)total+=history.records[j].invariant;
  auto line=projectile;result.outgoing=line;
  for(auto j=request.first;j<=result.last;++j){
   auto origin=state.at(projectile);origin.particle.status=original_status;config.last_transfer=j==result.last;
   auto pair=detail::scatter_pair_candidate(state.at(line),state.at(scratch),history.records[j],total,remaining,target,delta,config,sampler,medium,random,&origin,request.target_mass);
   total=pair.remaining_transfer;target=pair.target_mass;
   state.at(line)=pair.incoming;state.at(scratch)=pair.centre;
   if(pair.rejected)continue;
   CollisionStepTiming timing{delta,result.fraction,first_decay,request.time,
    j<result.last?history.records[j+1].position.t:0,history.records[j].time,
    request.quark_pair,request.coherent,j==result.last};
   finish_collision(pair,state.colour_max,ledger,finish,timing,medium,random);
   state.at(line)=pair.incoming;state.at(scratch)=pair.centre;
   pair.recoil.coherent_partner=0;pair.recoil.parent=0;pair.recoil.collision_source=line;
   pair.outgoing.coherent_partner=0;pair.outgoing.parent=line;pair.outgoing.collision_source=0;
   auto recoil=detail::CollisionWorkspace::append(state,pair.recoil),outgoing=detail::CollisionWorkspace::append(state,pair.outgoing);
   state.connect_collision(line,recoil,outgoing);
   remaining-=state.at(outgoing).record_mass-state.at(line).record_mass;
   line=outgoing;result.outgoing=outgoing;last_mass=state.at(outgoing).record_mass;
  }
  // Stock deliberately overwrites the last decay time, potentially consuming
  // another lifetime draw even when the last transfer already assigned it.
  if(state.particles().size()>size){
   auto& last=state.at(state.particles().back().particle.id);
   if(last.record_mass>0){
    if(delta==0)last.decay_time=request.coherent?request.time:first_decay;
    else{
     double lambda=1./(finish.formation_factor*last.particle.momentum.energy*double(.2f)/(last.record_mass*last.record_mass));
     last.decay_time=last.particle.production.t-std::log(1-random.uniform())/lambda;
    }
   }else last.decay_time=0;
  }
  if(last_mass<0){result.retry_splitting=true;return result;}
  if(!(std::abs(last_mass/target-1)>1e-6))return result; // stock 0/0 comparison is false
  if(request.energy_fraction==1&&last_mass>config.cutoff)return result;
  if(request.energy_fraction!=1)result.retry_splitting=true;
  // Keep round-trip incoming/scratch arithmetic and the allocated recoil tail,
  // while removing rejected event nodes. Stable IDs are never reused.
  auto rollback=detail::CollisionWorkspace::truncate(state,size);rollback.colour_max=baseline.colour_max;
  for(std::size_t i=0;i<size;++i){rollback.particles[i].daughters=baseline.particles[i].daughters;rollback.particles[i].scheduling_slots=baseline.particles[i].scheduling_slots;}
  for(auto& p:rollback.particles)if(p.particle.id==projectile)p.particle.status=original_status;
  state.restore(rollback);ledger.used=stored;result.outgoing=projectile;
  auto& original=state.at(projectile);
  if(original.record_mass<0){target=0;continue;}
  if(original.record_mass<config.cutoff&&original.particle.status!=14){
   if(target==0){result.retry_splitting=true;return result;}
   target=target==request.target_mass&&request.target_mass!=config.cutoff+1e-6?config.cutoff+1e-6:0;
  }else{
   target=original.record_mass;
   if(original.particle.status==14)result.fraction=0;
  }
  for(auto i=request.first;i<=result.last;++i)if(history.records[i].invariant<0){result.last=i;break;}
 }
}
}

namespace jewel {
CollisionSequenceResult apply_collisions(ShowerState& state,ParticleId projectile,ParticleId scratch,
 const TransferHistory& history,RecoilStorage& ledger,CollisionSequenceRequest request,CollisionConfig config,
 CollisionFinalizationConfig finish,const BranchingSampler& sampler,const Medium& medium,Random& random){
 state.validate();
 auto candidate=state;auto candidate_ledger=ledger;
 auto result=apply_candidates(candidate,projectile,scratch,history,candidate_ledger,request,config,finish,sampler,medium,random);
 candidate.validate();
 state=std::move(candidate);ledger=std::move(candidate_ledger);
 return result;
}
}
