// SPDX-License-Identifier: GPL-2.0-or-later
// DOCOHSCATTERING orchestration, JEWEL 2.6.0.
#include "jewel/coherence.hpp"
#include "jewel/kinematics.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace jewel {
namespace {
SpaceTime advance(Momentum p,SpaceTime x,double dt,double time){
 double norm=std::max(momentum_magnitude(p),p.energy);
 if(norm<=0)throw std::domain_error("zero coherent propagation momentum");
 x.x+=dt*p.px/norm;x.y+=dt*p.py/norm;x.z+=dt*p.pz/norm;x.t=time;return x;
}
void fill(CoherentHistogram& h,double x,double weight){
 if(h.bins<1||h.bins>100||!std::isfinite(h.minimum)||!std::isfinite(h.maximum)||h.maximum<=h.minimum)throw std::domain_error("invalid coherent histogram");
 if(x>h.minimum&&x<h.maximum){auto bin=int((x-h.minimum)*h.bins/(h.maximum-h.minimum));
  if(bin>=h.bins)throw std::domain_error("coherent histogram rounding overflow");h.values[bin]+=weight;}
}
}
CoherentResult evolve_coherent(ShowerState& state,ParticleId selected,ParticleId scratch,
 TransferHistory& history,RecoilStorage& ledger,double start,double duration,double angle,double split_time,
 CoherentConfig config,const ScatteringRates& rates,const TransferSampler& transfers,const BranchingSampler& branching,
 const Medium& medium,Random& random,CoherentHistograms* histograms){
 for(double v:{split_time,config.length_factor,config.lifetime,config.cross_section_scale})if(!std::isfinite(v))throw std::domain_error("nonfinite coherent configuration");
 if(history.last>history.records.size())throw std::domain_error("missing coherent transfer history");
 CoherentResult result;result.selected=selected;
 result.initial=begin_coherent(state,selected,config.enabled,start,duration,angle);auto& init=result.initial;
 if(!init.active)return result;
 auto current=init.coherent;double now=start,left=init.remaining;
 Momentum mom1=init.first_before.particle.momentum,mom2=init.second_before.particle.momentum;
 SpaceTime pos1=init.first_before.particle.production,pos2=init.second_before.particle.production;
 unsigned rejected=0;
 for(unsigned iteration=0;;++iteration){
  if(iteration==1000000)throw std::runtime_error("coherent evolution exhausted one million trials");
  const auto leg=state.at(current);
  auto next=rates.next_collision(leg.particle.momentum,leg.record_mass,leg.particle.pdg,leg.particle.production,
   now,left,config.lifetime,config.cross_section_scale,true,random);++result.collision_calls;
  if(!next.collision)break;
  now+=next.elapsed;left-=next.elapsed;
  auto old=history.last;auto slot=history.last++;
  if(slot>=10000){state.discarded=true;return result;}
  for(;;){
   transfers.sample(state,current,scratch,history,slot,now-state.at(current).particle.production.t,1,false,random);
   if(state.discarded)return result;
   if(-history.records[slot].invariant<=std::pow(state.at(selected).particle.momentum.energy,2))break;
   if(rejected<100){++rejected;continue;}
   history.records[slot].invariant=0;history.records[slot].momentum={};--history.last;break;
  }
  if(history.last>old){
   auto& q=history.records[slot];
   if(coherent_resolution(state.at(selected).particle.momentum,q,config.length_factor,angle,now,split_time).resolved){
    q.invariant=0;q.momentum={};--history.last;continue;
   }
   if(histograms){fill(histograms->time,q.position.t,state.weight);fill(histograms->transfer,-q.invariant,state.weight);}
   ++result.kinematic_calls;
   const auto before_count=state.particles().size();
   CollisionSequenceRequest request;request.first=slot;request.last=slot;request.target_mass=state.at(current).record_mass;
   request.time=now;request.fraction=state.at(current).fraction;request.quark_pair=state.at(current).quark_pair;request.coherent=true;
   auto applied=apply_collisions(state,current,scratch,history,ledger,request,config.collision,config.finalization,branching,medium,random);
   state.at(current).fraction=applied.fraction; // ZD(N) is passed by reference at the original row.
   if(state.discarded)return result;
   current=state.particles().back().particle.id;
   auto& outgoing=state.at(current);
   auto pair=reconstruct_coherent(outgoing.particle.momentum,outgoing.generator_vertex,outgoing.record_mass,
    state.at(init.first).record_mass,state.at(init.second).record_mass,init.fraction,init.azimuth);
   outgoing.particle.momentum=pair.parent;outgoing.generator_vertex=pair.generator_vertex;
   if(!pair.possible){
    // Stock removes only the latest two rows, retaining colour/storage side effects.
    if(state.particles().size()!=before_count+2)throw std::domain_error("coherent rollback without one appended collision pair");
    auto saved=state.snapshot();saved.particles.resize(saved.particles.size()-2);
    auto& previous=saved.particles.back();previous.particle.status=1;previous.daughters={};previous.decay_time=0;
    current=previous.particle.id;state.restore(saved);++result.undone_kicks;
   }else{
    pos1=advance(mom1,pos1,next.elapsed,pos1.t+next.elapsed);
    pos2=advance(mom2,pos2,next.elapsed,pos2.t+next.elapsed);
    mom1=pair.first;mom2=pair.second;
   }
   --history.last;
   if(history.last==1){history.sum_squared=history.records[0].invariant;history.sum=history.records[0].momentum;}
   else{history.sum_squared=0;history.sum={};}
  }
  if(left<=0)break;
 }
 const double end=start+init.duration;auto parent=state.at(current);
 auto pair=reconstruct_coherent(parent.particle.momentum,parent.generator_vertex,parent.record_mass,
  state.at(init.first).record_mass,state.at(init.second).record_mass,init.fraction,init.azimuth);
 if(!pair.possible)throw std::domain_error("nonphysical final coherent resplitting");
 if(state.particles().size()+2>22990){state.discarded=true;return result;}
 ShowerParticle a,b;
 for(auto child:{&a,&b}){child->parent=current;child->particle.status=1;child->angular_required=true;}
 auto prepare=[&](ShowerParticle& child,const ShowerParticle& original,Momentum momentum,SpaceTime pos,Momentum old){
  child.particle.pdg=original.particle.pdg;child.record_mass=original.record_mass;child.particle.momentum=momentum;
  child.particle.production=advance(old,pos,end-parent.particle.production.t,end);
  child.decay_time=original.decay_time;child.angle=original.angle;child.fraction=original.fraction;child.quark_pair=original.quark_pair;
 };
 prepare(a,init.first_before,pair.first,pos1,mom1);prepare(b,init.second_before,pair.second,pos2,mom2);
 if(init.quark_pair){a.particle.colour=0;a.particle.anticolour=parent.particle.anticolour;}
 else{
  if(parent.particle.pdg>0&&init.colour_direction>=0){a.particle.colour=parent.particle.colour;a.particle.anticolour=state.colour_max+1;}
  else{a.particle.colour=state.colour_max+1;a.particle.anticolour=parent.particle.anticolour;}
  ++state.colour_max;
 }
 if(init.quark){
  if(b.particle.pdg>0){b.particle.colour=a.particle.anticolour;b.particle.anticolour=0;}
  else{b.particle.colour=0;b.particle.anticolour=a.particle.colour;}
 }else if(init.quark_pair){b.particle.colour=parent.particle.colour;b.particle.anticolour=0;}
 else if(init.colour_direction==1){b.particle.colour=a.particle.anticolour;b.particle.anticolour=parent.particle.anticolour;}
 else{b.particle.colour=parent.particle.colour;b.particle.anticolour=a.particle.colour;}
 a.evolution_time=selected==init.first?init.first_before.evolution_time:end;
 b.evolution_time=selected==init.second?init.second_before.evolution_time:end;
 parent.particle.status=21;parent.decay_time=end;parent.particle.momentum=pair.parent;parent.generator_vertex=pair.generator_vertex;
 state.at(current)=parent;
 auto first=state.append(a),second=state.append(b);state.connect(current,first,second);
 result.selected=selected==init.first?first:second;return result;
}
}
