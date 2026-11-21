// SPDX-License-Identifier: GPL-2.0-or-later
// PPJJ branch of FINDVIRTUALITIES and GETTHETAM, JEWEL 2.6.0.
#include "jewel/hard_initialization.hpp"
#include "jewel/kinematics.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace jewel {
namespace {
void require(bool value,const char* message){if(!value)throw std::domain_error(message);}
bool parton(int id){return id==21||(std::abs(id)>=1&&std::abs(id)<=5);}
void boost(ShowerParticle& p,const std::array<double,3>& beta){if(p.particle.status>0)rotate_boost(p.particle.momentum,&p.generator_vertex,0,0,beta);}
}
double colour_opening_angle(const ShowerState& state,ParticleId id){
 const auto& selected=state.at(id);int flavour=selected.particle.pdg;
 if((std::abs(flavour)>6)&&flavour!=21)return -1;
 ParticleId first=0,second=0;
 for(const auto& node:state.particles())if(node.particle.status<10){
  if(flavour==21){if(node.particle.anticolour==selected.particle.colour)first=node.particle.id;if(node.particle.colour==selected.particle.anticolour)second=node.particle.id;}
  else if(flavour<0){if(node.particle.colour==selected.particle.anticolour)first=node.particle.id;}
  else if(node.particle.anticolour==selected.particle.colour)first=node.particle.id;
 }
 auto angle=[&](ParticleId partner){
  require(partner!=0,"missing hard-parton colour partner");
  auto parent=state.scheduling_parent(partner);std::size_t slot=0;
  for(std::size_t i=0;i<state.particles().size();++i)if(state.particles()[i].particle.id==parent)slot=i+1;
  if(slot<=2)return 1e6;
  auto p=selected.particle.momentum,q=state.at(partner).particle.momentum;
  double cosine=(p.px*q.px+p.py*q.py+p.pz*q.pz)/(momentum_magnitude(p)*momentum_magnitude(q));
  double result=std::acos(cosine);require(std::isfinite(result),"undefined hard colour angle");return result;
 };
 double result=angle(first);if(flavour==21)result=std::min(result,angle(second));
 return result>1e5?-1:result;
}
HardInitializationResult initialize_hard_dijet(ShowerState& state,ParticleId first,ParticleId second,
 const HardInitializationConfig& config,const BranchingSampler& sampler,Random& random){
 state.validate();require(first!=second,"identical hard legs");
 for(double x:{config.pt_hat,config.x,config.y,config.formation_factor,config.lifetime})require(std::isfinite(x),"nonfinite hard initialization configuration");
 require(config.pt_hat>=0&&config.formation_factor>0&&config.lifetime>=0&&config.maximum_trials>0,"invalid hard initialization configuration");
 auto working=state;auto& a=working.at(first);auto& b=working.at(second);
 for(auto* leg:{&a,&b})require(parton(leg->particle.pdg)&&(leg->particle.status==1||leg->particle.status==4),"hard dijet initialization requires active quarks/gluons");
 HardInitializationResult result;result.colour_angles={colour_opening_angle(working,first),colour_opening_angle(working,second)};
 const double energy=a.particle.momentum.energy+b.particle.momentum.energy;
 require(energy>0,"nonpositive hard pair energy");
 const std::array<double,3> beta{(a.particle.momentum.px+b.particle.momentum.px)/energy,(a.particle.momentum.py+b.particle.momentum.py)/energy,(a.particle.momentum.pz+b.particle.momentum.pz)/energy};
 const std::array<double,3> inverse{-beta[0],-beta[1],-beta[2]};
 require(beta[0]*beta[0]+beta[1]*beta[1]+beta[2]*beta[2]<1,"non-timelike hard pair");
 boost(a,inverse);boost(b,inverse);const double total=a.particle.momentum.energy+b.particle.momentum.energy;
 boost(a,beta);boost(b,beta);
 auto sample=[&](ShowerParticle& leg,double maximum){return sampler.mass(0,maximum,-1,energy,leg.particle.pdg==21?Channel::GC:Channel::QQ,energy,false,random);};
 auto qa=sample(a,a.particle.status==4?0:config.pt_hat),qb=sample(b,b.particle.status==4?0:config.pt_hat);
 auto draw=[&]{double u=random.uniform();require(std::isfinite(u)&&u>0&&u<1,"invalid initialization random draw");return u;};
 for(;;){
  if(++result.trials>config.maximum_trials)throw std::runtime_error("hard initialization retries exhausted");
  double ea=total/2+(qa.mass*qa.mass-qb.mass*qb.mass)/(2*total),eb=total/2-(qa.mass*qa.mass-qb.mass*qb.mass)/(2*total);
  double pa=ea*ea-qa.mass*qa.mass,pb=eb*eb-qb.mass*qb.mass;
  bool rejected=draw()>1;
  if(rejected||pa<0||pb<0||ea<0||eb<0){if(qa.mass>qb.mass)qa=sample(a,qa.mass);else qb=sample(b,qb.mass);continue;}
  boost(a,inverse);boost(b,inverse);
  auto reshape=[](ShowerParticle& leg,double square,double en,double mass){auto& p=leg.particle.momentum;double old=momentum_magnitude(p);require(old>0,"zero hard momentum in rest frame");p.px=p.px*std::sqrt(square)/old;p.py=p.py*std::sqrt(square)/old;p.pz=p.pz*std::sqrt(square)/old;p.energy=en;leg.record_mass=mass;};
  reshape(a,pa,ea,qa.mass);reshape(b,pb,eb,qb.mass);boost(a,beta);boost(b,beta);
  auto veto=[&](const ShowerParticle& p,const MassSample& q,double maximum_angle){
   if(q.mass<=0)return false;
   double eps=.5-.5*std::sqrt(1-sampler.cutoff()*sampler.cutoff()/(q.mass*q.mass))*std::sqrt(1-q.mass*q.mass/(p.particle.momentum.energy*p.particle.momentum.energy));
   double z=q.fraction.value();
   if(z<eps||z>1-eps)return true;
   return config.first_angle_ordering&&maximum_angle>0&&q.mass/(std::sqrt(z*(1-z))*p.particle.momentum.energy)>maximum_angle;
  };
  if(veto(a,qa,result.colour_angles[0])){qa=sample(a,qa.mass);continue;}
  if(veto(b,qb,result.colour_angles[1])){qb=sample(b,qb.mass);continue;}
  break;
 }
 auto finish=[&](ShowerParticle& p,const MassSample& q){
  double z=q.fraction.value();p.angle=z>0?p.record_mass/(std::sqrt(z*(1-z))*p.particle.momentum.energy):0;
  p.fraction=z;p.quark_pair=q.quark_pair.value();p.angular_required=false;p.particle.production={config.x,config.y,0,0};
  if(p.record_mass>0){double lambda=1./(config.formation_factor*p.particle.momentum.energy*double(.2f)/(q.mass*q.mass));p.decay_time=-std::log(1-draw())/lambda;}
  else p.decay_time=config.lifetime;
 };
 finish(a,qa);finish(b,qb);working.validate();state=std::move(working);return result;
}
}
