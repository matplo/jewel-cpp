// SPDX-License-Identifier: GPL-2.0-or-later
// DOINSTATESCAT, JEWEL 2.6.0. The source forces NOSCAT=.TRUE. after its
// formation-time GETDELTAT call: preserve its RNG consumption, not dead physics.
#include "jewel/initial_evolution.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace jewel {
namespace {
void require(bool value,const char* message){if(!value)throw std::domain_error(message);}
SpaceTime location(const ShowerParticle& leg,double time){
 auto p=leg.particle.momentum;auto x=leg.particle.production;double dt=time-x.t;
 require(p.energy>0,"invalid initial evolution energy");
 return {x.x+dt*p.px/p.energy,x.y+dt*p.py/p.energy,x.z+dt*p.pz/p.energy,time};
}
}
InitialScatteringEvolution::InitialScatteringEvolution(InitialEvolutionConfig config,Kernels kernels,
 const BranchingSampler& branching,const InitialSudakovTable& initial,const FinalSplittingTable& final,
 const PartonicPdf& pdf,const ScatteringRates& rates,const TransferSampler& transfers,const Medium& medium):
 config_(config),kernels_(kernels),branching_(branching),initial_(initial),final_(final),pdf_(pdf),rates_(rates),transfers_(transfers),medium_(medium){
 auto& e=config_.evolution;
 for(double x:{e.cutoff,e.mass_scale,e.formation_factor,e.lifetime,e.cross_section_scale,config_.coherence_rejection})require(std::isfinite(x),"nonfinite initial evolution configuration");
 require(e.cutoff>0&&e.mass_scale>0&&e.formation_factor>0&&e.lifetime>=0&&e.cross_section_scale>=0,"invalid initial evolution configuration");
 auto& c=config_.coherent;c.lifetime=e.lifetime;c.cross_section_scale=e.cross_section_scale;
 c.collision.cutoff=e.cutoff;c.collision.mass_scale=e.mass_scale;c.finalization.formation_factor=e.formation_factor;
}
InitialScatteringResult InitialScatteringEvolution::sample(ShowerState& state,ParticleId selected,ParticleId scratch,
 TransferHistory& history,RecoilStorage& ledger,double start,double duration,bool retry,Random& random,CoherentHistograms* histograms) const {
 require(selected!=scratch,"initial evolution aliases scratch");state.at(scratch);
 require(std::isfinite(start)&&std::isfinite(duration)&&duration>=0,"invalid initial evolution interval");
 InitialScatteringResult result;result.selected=selected;result.elapsed=duration;
 const auto& e=config_.evolution;double scale2=e.mass_scale*e.mass_scale,low=e.cutoff*e.cutoff/scale2;
 double split_angle=-1,split_time=0;
 if(auto mother=state.scheduling_parent(selected)){const auto& p=state.at(mother);if(p.particle.status==11){split_angle=p.angle;split_time=p.decay_time;}}
 auto position=location(state.at(selected),start);
 double first_dt=-1,total=0,now=start,left=duration;
 auto collapse=[&]{require(history.first>0&&history.first<=history.records.size(),"missing fallback initial transfer");
  history.last=history.first;const auto& q=history.records[history.last-1];history.sum_squared=q.invariant;history.sum=q.momentum;
  result.virtuality=0;result.fraction=1;result.incoming_flavour=state.at(result.selected).particle.pdg;
  result.above_cutoff=-q.invariant>low;result.elapsed=q.time-start;
 };
 auto failure=[&]{result.virtuality=0;result.fraction=1;result.incoming_flavour=state.at(result.selected).particle.pdg;
  result.above_cutoff=false;history.first=history.last=0;history.sum={};history.sum_squared=0;return result;};
 auto coherent=[&](double interval){
  auto value=evolve_coherent(state,result.selected,scratch,history,ledger,start,interval,split_angle,split_time,
   config_.coherent,rates_,transfers_,branching_,medium_,random,histograms);
  result.selected=value.selected;
  if(value.initial.active)result.coherent=value.initial;else result.coherent.reset();
 };
 auto revert=[&]{
  if(!result.coherent)return;
  const auto saved=*result.coherent;auto snapshot=state.snapshot_truncated(saved.original_size);
  for(auto& p:snapshot.particles){const ShowerParticle* old=nullptr;
   if(p.particle.id==saved.first)old=&saved.first_before;if(p.particle.id==saved.second)old=&saved.second_before;
   if(old){p.particle.status=old->particle.status;p.daughters={};if(p.scheduling_slots){(*p.scheduling_slots)[1]=0;(*p.scheduling_slots)[2]=0;}p.decay_time=old->decay_time;}
  }
  state.restore(snapshot);result.selected=saved.selected;result.coherent.reset();++result.coherence_reversions;
 };
 for(unsigned collision_trial=0;;++collision_trial){
  require(collision_trial<1000000,"initial unresolved-scattering loop exhausted");
  auto leg=state.at(result.selected);
  auto collision=rates_.next_collision(leg.particle.momentum,leg.record_mass,leg.particle.pdg,leg.particle.production,
   now,left,e.lifetime,e.cross_section_scale,false,random);
  double dt=collision.elapsed;if(first_dt<0)first_dt=dt;
  if(!collision.collision&&!retry){history.first=history.last=0;coherent(duration);return failure();}
  double no_radiation=rates_.no_initial_radiation(leg.particle.momentum,leg.record_mass,leg.particle.pdg,position);
  bool norad=random.uniform()<no_radiation||leg.particle.momentum.energy<double(1.001f)*e.cutoff;
  position=location(leg,start+total+dt);
  bool gluon=leg.particle.pdg==21;char type=gluon?'G':'Q';
  double sigma=rates_.cross_section(leg.particle.momentum,leg.record_mass,e.cutoff,type,'C',position);
  double change=(sigma==0||no_radiation==1)?0:rates_.cross_section(leg.particle.momentum,leg.record_mass,e.cutoff,type,gluon?'Q':'G',position)/sigma;
  sigma=rates_.cross_section(leg.particle.momentum,leg.record_mass,0,type,'C',position);
  bool flavour_change=random.uniform()<change;if(norad)flavour_change=false;
  result.incoming_flavour=flavour_change?(gluon?(random.uniform()<.5?-2:2):21):leg.particle.pdg;
  double tmax=4*(leg.particle.momentum.energy*leg.particle.momentum.energy-leg.record_mass*leg.record_mass);
  require(tmax>0,"invalid initial emission transfer bound");
  double xmax=1-e.cutoff*e.cutoff/(scale2*4*tmax);
  if(sigma==0)return failure();
  double lifetime_draw=random.uniform();bool unresolved=false;
  for(;;){
   ++result.trials;leg=state.at(result.selected);
   Channel channel=gluon?(flavour_change?Channel::QG:Channel::GG):Channel::QQ;
   result.fraction=branching_.fraction(0,0,1-xmax,channel,random);
   if(!gluon&&flavour_change)result.fraction=1-result.fraction;
   if(norad)result.fraction=1;
   history.sum={};history.sum_squared=-1e-10;history.first=history.last=1;
   result.above_cutoff=false;result.virtuality=leg.record_mass;
   now+=dt;total+=dt;left-=dt;
   if(history.records.empty())history.records.resize(1);
   history.records[0].time=now;double old_squared=history.sum_squared;
   for(unsigned attempt=0;;++attempt){
    transfers_.sample(state,result.selected,scratch,history,0,now-leg.particle.production.t,result.fraction,false,random);
    if(state.discarded)return result;
    if(-history.sum_squared<=leg.particle.momentum.energy*leg.particle.momentum.energy)break;
    history.sum={};history.sum_squared=old_squared;
    if(attempt<100)continue;
    history.records[0].invariant=0;history.records[0].momentum={};history.last=0;break;
   }
   if(history.last==1&&split_angle>0){
    auto resolution=coherent_resolution(state.at(result.selected).particle.momentum,history.records[0],config_.coherent.length_factor,split_angle,now,split_time);
    if(resolution.transverse_transfer<resolution.threshold&&random.uniform()<config_.coherence_rejection){
     history.sum={};history.sum_squared=0;history.records[0].invariant=0;history.records[0].momentum={};history.first=history.last=0;
     ++result.unresolved_rejections;unresolved=true;break;
    }
   }
   if(history.last>0)result.above_cutoff=-history.records[history.last-1].invariant>low;
   if(result.above_cutoff&&!norad){
    auto sample=branching_.updated_mass({scale2*history.sum_squared,scale2*old_squared,0,leg.record_mass,leg.particle.momentum.energy,
     leg.particle.pdg,true,result.fraction,0,false},initial_,final_,random);
    result.virtuality=sample.mass;
   }else result.virtuality=0;
   coherent(total);if(state.discarded)return result;
   if(result.coherent){require(history.last>0,"coherent recalculation without an initial transfer");
    transfers_.sample(state,result.selected,scratch,history,history.last-1,now-state.at(result.selected).particle.production.t,result.fraction,true,random);}
   leg=state.at(result.selected);
   double formation=(result.virtuality==0||result.virtuality==leg.record_mass)?duration:
    std::max(e.formation_factor*double(.2f)*result.fraction*leg.particle.momentum.energy/(result.virtuality*result.virtuality),0.);
   if(formation>0){double lambda=1/formation;formation=-std::log(1-lifetime_draw)/lambda;}
   // Preserve the call and its draws even though stock immediately sets NOSCAT true.
   rates_.next_collision(leg.particle.momentum,leg.record_mass,leg.particle.pdg,leg.particle.production,now,std::min(left,formation),e.lifetime,e.cross_section_scale,false,random);
   double weight=0;
   if(norad){weight=1;result.virtuality=0;result.fraction=1;}
   else if(-history.sum_squared>=low&&result.virtuality!=0&&-history.sum_squared<=leg.particle.momentum.energy*leg.particle.momentum.energy){
    constexpr double pi=3.141592653589793;
    double fmax=std::log(-scale2*history.sum_squared/(e.cutoff*e.cutoff))*kernels_.alphas(e.cutoff*e.cutoff/4,kernels_.lambda_shower)/(2*pi);
    if(gluon)fmax=2*std::log(-scale2*history.sum_squared/(e.cutoff*e.cutoff))*kernels_.alphas(e.cutoff*e.cutoff/4,kernels_.lambda_shower)/(2*pi);
    auto pdf_channel=gluon?(flavour_change?Channel::QG:Channel::GG):(flavour_change?Channel::GQ:Channel::QQ);
    double kernel=gluon?(flavour_change?kernels_.pqg(result.fraction):kernels_.pgg(result.fraction)):
      (flavour_change?kernels_.pgq(result.fraction):kernels_.pqq(result.fraction));
    double density=pdf_.density(result.fraction,e.mass_scale*std::sqrt(-history.sum_squared),pdf_channel);
    weight=(gluon&&flavour_change?2*density:density)/(kernel*fmax);
   }
   if(random.uniform()>weight){
    if(result.trials<10000){revert();left=duration;now=start;total=0;dt=first_dt;continue;}
    collapse();return result;
   }
   if(history.last==0)return failure();
   bool reject_phase=left<formation||random.uniform()>1/double(history.last);
   if(reject_phase&&!norad){collapse();return result;}
   if(!norad){left-=formation;now+=formation;total+=formation;}
   result.elapsed=total;return result;
  }
  if(!unresolved)throw std::logic_error("unreachable initial evolution branch");
 }
}
}
