// SPDX-License-Identifier: GPL-2.0-or-later
// MAKEBRANCH and MAKECASCADE, JEWEL 2.6.0.
#include "jewel/cascade.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace jewel {
namespace {
bool radiates(const ShowerParticle& p){return (p.particle.status==1&&p.record_mass>0)||(p.particle.status==2&&p.fraction>0);}
void initialize_time(ShowerParticle& p){
 if(p.evolution_time < -1e5)p.evolution_time=(p.particle.production.t<0&&(p.decay_time>0||p.record_mass==0))?0:p.particle.production.t;
}
void daughter_time(ShowerParticle& p){p.evolution_time=p.particle.production.t<0&&p.decay_time>0?0:p.particle.production.t;}
}
Cascade::Cascade(CascadeConfig config,const InitialScatteringEvolution& initial,const ScatteringEvolution& final,
 const BranchingSampler& branching,const Medium& medium):config_(config),initial_(initial),final_(final),branching_(branching),medium_(medium){
 if(config.maximum_branch_calls==0||!std::isfinite(config.evolution.lifetime)||config.evolution.lifetime<0)
  throw std::domain_error("invalid cascade configuration");
 config_.collision.cutoff=config.evolution.cutoff;config_.collision.mass_scale=config.evolution.mass_scale;
 config_.finalization.formation_factor=config.evolution.formation_factor;
}
void Cascade::branch(ShowerState& state,ParticleId selected,ParticleId scratch,TransferHistory& history,
 RecoilStorage& ledger,Random& random,CoherentHistograms* histograms) const {
 if(selected==scratch)throw std::domain_error("cascade selected scratch");
 state.at(scratch);auto line=selected;initialize_time(state.at(line));
 history.first=history.last=0;history.sum_squared=0;history.sum={};
 if(state.discarded)return;
 const auto& e=config_.evolution;
 auto leg=state.at(line);bool radiation=radiates(leg);
 double formation=radiation?(leg.medium_induced?leg.evolution_time:std::min(leg.decay_time,e.lifetime)):e.lifetime;
 double left=formation-leg.evolution_time,total=0;state.at(line).medium_induced=false;
 if(left<=1e-6||leg.evolution_time<0){
  if(radiation){
   auto size=state.particles().size();make_splitting(state,line,branching_,medium_,e.formation_factor,random);
   if(state.discarded)return;
   if(state.particles().size()>size){
    const auto mother=state.at(line);const auto& p=mother.particle.momentum;
    double denominator=std::max(std::sqrt(p.px*p.px+p.py*p.py+p.pz*p.pz),p.energy);
    if(denominator<=0)throw std::domain_error("invalid split propagation energy");
    for(auto id:mother.daughters){auto& daughter=state.at(id);auto& x=daughter.particle.production;auto birth=mother.particle.production;double dt=x.t-birth.t;
     x.x=birth.x+dt*p.px/denominator;x.y=birth.y+dt*p.py/denominator;x.z=birth.z+dt*p.pz/denominator;daughter_time(daughter);}
    line=mother.daughters[1];
   }
  }else state.at(line).evolution_time=formation;
 }else{
  auto initial=initial_.sample(state,line,scratch,history,ledger,leg.evolution_time,left,false,random,histograms);
  if(state.discarded)return;
  line=initial.selected;double fraction=initial.fraction,dt=initial.elapsed;
  total+=dt;left-=dt;
  const auto original=state.at(line);const auto size=state.particles().size(),stored=ledger.used;
  const auto baseline=state.snapshot();double z=original.fraction;bool pair=original.quark_pair,above=initial.above_cutoff,induced=false;
  for(unsigned retry=0;;++retry){
   if(retry>=100000)throw std::runtime_error("cascade initial-split retries exhausted");
   ParticleId kinematic=line;double new_mass=0;
   auto collapse=[&]{
    if(history.first==0||history.first>history.records.size())throw std::domain_error("missing cascade fallback transfer");
    history.last=history.first;const auto& q=history.records[history.last-1];history.sum_squared=q.invariant;history.sum=q.momentum;
    above=-q.invariant>e.cutoff*e.cutoff/(e.mass_scale*e.mass_scale);
    left=state.at(line).evolution_time+total+left-history.records.front().time;
    total=history.records.front().time-state.at(line).evolution_time;
   };
   if(fraction<1){
    auto split=make_initial_splitting(state,line,{fraction,history.sum_squared,initial.virtuality,
     state.at(line).evolution_time+total,dt,initial.incoming_flavour,e.formation_factor,e.mass_scale},branching_,random);
    if(state.discarded)return;fraction=split.fraction;
    if(fraction<1){
     state.at(split.radiated).evolution_time=state.at(line).evolution_time;
     state.at(split.continuing).evolution_time=state.at(line).evolution_time;
     line=kinematic=split.continuing;auto& p=state.at(line);
     auto mass=branching_.mass(0,e.mass_scale*std::sqrt(-history.sum_squared),-1,p.particle.momentum.energy,
      p.particle.pdg==21?Channel::GC:Channel::QQ,std::sqrt(-history.sum_squared),false,random);
     new_mass=mass.mass;z=mass.fraction.value();pair=mass.quark_pair.value();
     p.angle=z>0?new_mass/(std::sqrt(z*(1-z))*p.particle.momentum.energy):0;p.fraction=z;p.quark_pair=pair;
    }else collapse();
   }
   if(fraction==1&&history.last>0){
    auto value=final_.final_state(state,line,scratch,history,state.at(line).evolution_time+total,left,{0,0,z,pair,above},random);
    new_mass=value.mass;z=value.fraction;pair=value.quark_pair;above=value.above_cutoff;dt=value.elapsed;
    induced=new_mass>state.at(line).record_mass*(1+1e-6);
    if(!induced){z=state.at(line).fraction;pair=state.at(line).quark_pair;}
    total+=dt;left-=dt;kinematic=line;
   }
   if(history.last>0){
    auto before=state.particles().size();
    auto collision=apply_collisions(state,kinematic,scratch,history,ledger,
     {history.first-1,history.last-1,new_mass,state.at(kinematic).evolution_time+total,fraction,z,pair,false,false},
     config_.collision,config_.finalization,branching_,medium_,random);
    if(state.discarded)return;
    history.last=collision.last+1;z=collision.fraction;
    auto last=state.particles().back().particle.id;
    if(induced&&state.at(last).fraction>0){
     auto& p=state.at(last);double eps=.5-.5*std::sqrt(1-e.cutoff*e.cutoff/(p.record_mass*p.record_mass))*std::sqrt(1-p.record_mass*p.record_mass/(p.particle.momentum.energy*p.particle.momentum.energy));
     if(p.fraction<eps||p.fraction>1-eps)p.fraction=branching_.fraction(0,0,eps,p.particle.pdg<21?Channel::QQ:p.quark_pair?Channel::GQ:Channel::GG,random);
     p.angle=p.record_mass/(std::sqrt(p.fraction*(1-p.fraction))*p.particle.momentum.energy);
    }
    if(collision.retry_splitting){
     left=state.at(line).evolution_time+total+left-history.records.front().time;
     total=history.records.front().time-state.at(line).evolution_time;
     if(fraction<1){
      history.last=history.first;const auto& q=history.records[history.last-1];history.sum_squared=q.invariant;history.sum=q.momentum;
      above=-q.invariant>e.cutoff*e.cutoff/(e.mass_scale*e.mass_scale);
      auto rollback=state.snapshot_truncated(size);
      for(std::size_t i=0;i<size;++i)rollback.particles[i].daughters=baseline.particles[i].daughters;
      for(auto& p:rollback.particles)if(p.particle.id==original.particle.id){p.particle.status=original.particle.status;p.decay_time=original.decay_time;}
      state.restore(rollback);line=original.particle.id;fraction=1;ledger.used=stored;continue;
     }
     state.at(last).evolution_time=state.at(line).evolution_time+total;
     if(state.particles().size()>before)line=last;
    }else{
     double time=state.at(line).evolution_time+total;
     auto target=state.particles().size()>before?last:line;
     if(new_mass!=state.at(kinematic).record_mass&&state.at(target).decay_time>0)time=std::min(state.at(target).decay_time,time);
     state.at(target).evolution_time=time;line=target;
    }
    state.at(line).medium_induced=induced;
   }else state.at(line).evolution_time+=total;
   break;
  }
 }
 auto& end=state.at(line);
 if(radiates(end)||end.evolution_time<e.lifetime)return;
 if(end.particle.status==1&&end.record_mass==0)end.particle.status=4;
 if(end.particle.status==2&&end.fraction<0)end.particle.status=5;
}
CascadeResult Cascade::evolve(ShowerState& state,ParticleId scratch,TransferHistory& history,RecoilStorage& ledger,
 Random& random,CoherentHistograms* histograms) const {
 if(state.particles().empty()||state.particles().front().particle.id!=scratch)throw std::domain_error("cascade scratch must be first record");
 CascadeResult result;
 while(!state.discarded){
  ++result.passes;bool active=false;auto count=state.particles().size();
  for(std::size_t i=1;i<count;++i){
   if(i>=state.particles().size())break;
   auto id=state.particles()[i].particle.id;auto status=state.at(id).particle.status;
   if(status==1||status==2){
    active=true;if(result.branch_calls>=config_.maximum_branch_calls)throw std::runtime_error("cascade branch limit exhausted");
    ++result.branch_calls;branch(state,id,scratch,history,ledger,random,histograms);if(state.discarded)return result;
   }
  }
  if(!active)break;
 }
 return result;
}
}
