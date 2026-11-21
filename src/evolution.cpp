// SPDX-License-Identifier: GPL-2.0-or-later
// DOFISTATESCAT from JEWEL 2.6.0.
#include "jewel/evolution.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace jewel {
namespace {void require(bool b,const char* s){if(!b)throw std::domain_error(s);}}
ScatteringEvolution::ScatteringEvolution(EvolutionConfig config,const BranchingSampler& branching,
 const InitialSudakovTable& initial,const FinalSplittingTable& final,const ScatteringRates& rates,
 const TransferSampler& transfers):config_(config),branching_(branching),initial_(initial),final_(final),rates_(rates),transfers_(transfers){
 for(double value:{config.cutoff,config.mass_scale,config.formation_factor,config.lifetime,config.cross_section_scale})
  require(std::isfinite(value),"nonfinite evolution configuration");
 require(config.cutoff>0&&config.mass_scale>0&&config.formation_factor>0&&config.lifetime>=0&&config.cross_section_scale>=0,"invalid evolution configuration");
}
FinalScatteringResult ScatteringEvolution::final_state(ShowerState& state,ParticleId id,ParticleId scratch,
 TransferHistory& history,double now,double time_left,FinalScatteringResult result,Random& random) const {
 require(id!=scratch,"projectile aliases evolution scratch record");
 state.at(scratch);
 require(history.first<=history.last,"invalid active transfer range");
 require(history.last>=1&&history.last<=100&&history.records.size()>=history.last,"final scattering requires initial transfers");
 for(double value:{now,time_left,result.mass,result.fraction,history.sum_squared})require(std::isfinite(value),"nonfinite final scattering input");
 require(time_left>=0,"negative final scattering interval");
 const auto& leg=state.at(id);auto p=leg.particle.momentum;double mass=leg.record_mass;
 require(p.energy>0&&std::isfinite(p.energy),"invalid final scattering energy");
 double scale2=config_.mass_scale*config_.mass_scale,low=config_.cutoff*config_.cutoff/scale2;
 double total=0,phase=0,left=time_left,formation=0;
 result.elapsed=0;
 auto update=[&](double old){
  VirtualityUpdate request{scale2*history.sum_squared,scale2*old,result.mass,mass,p.energy,
   leg.particle.pdg,false,1,result.fraction,result.quark_pair};
  auto value=branching_.updated_mass(request,initial_,final_,random);
  result.mass=value.mass;result.fraction=value.fraction.value();result.quark_pair=value.quark_pair.value();
 };
 if(result.above_cutoff||-history.sum_squared>low){update(0);result.above_cutoff=true;}
 else result.mass=mass;
 double lifetime_draw=random.uniform();
 for(unsigned iteration=0;;++iteration){
  require(iteration<1000000,"final scattering loop exhausted");
  formation=(result.mass==0||result.mass==mass)?left:
   std::max(config_.formation_factor*(1-phase)*double(.2f)*p.energy/(result.mass*result.mass),0.);
  if(formation>0){double lambda=1/formation;formation=-std::log(1-lifetime_draw)/lambda;}
  auto collision=rates_.next_collision(p,mass,leg.particle.pdg,leg.particle.production,
    now+total,std::min(formation,left),config_.lifetime,config_.cross_section_scale,false,random);
  if(!collision.collision)break;
  ++history.last;
  if(history.last>100){--history.last;break;}
  if(history.first==0)history.first=1;
  total+=collision.elapsed;left-=collision.elapsed;
  if(result.mass!=0&&result.mass!=mass)phase+=5*collision.elapsed*(result.mass*result.mass)/p.energy;
  if(history.records.size()<history.last)history.records.resize(history.last);
  auto slot=history.last-1;history.records[slot].time=now+total;
  Momentum old_sum=history.sum;double old_squared=history.sum_squared;
  for(unsigned attempt=0;;++attempt){
   transfers_.sample(state,id,scratch,history,slot,now+total-leg.particle.production.t,1,false,random);
   if(-history.sum_squared<=p.energy*p.energy)break;
   history.sum=old_sum;history.sum_squared=old_squared;
   if(attempt<100)continue;
   history.records[slot].invariant=0;history.records[slot].momentum={};--history.last;break;
  }
  if(result.above_cutoff||-history.sum_squared>low){update(old_squared);result.above_cutoff=true;}
 }
 if(result.mass*result.mass>low&&result.mass!=mass){
  if(left<formation||random.uniform()>1./double(history.last)){
   result.elapsed=history.last==100?total:left<formation?total+left:total+formation;
   result.mass=mass;
  }else result.elapsed=total+formation;
 }else{
  result.elapsed=0;history.first=1;history.last=1;
  const auto& first=history.records.front();history.sum_squared=first.invariant;history.sum=first.momentum;
  result.above_cutoff=-first.invariant>low;result.mass=mass;
 }
 return result;
}
}
