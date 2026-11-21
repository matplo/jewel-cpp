// SPDX-License-Identifier: GPL-2.0-or-later
// PPJJ and MPI-pair branches of FINDVIRTUALITIES and GETTHETAM, JEWEL 2.6.0.
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
 if(config.mpi_pair){require(config.mpi_sampling_channels.has_value(),"MPI initialization requires explicit TYPE1/TYPE2 channels");
  for(auto channel:*config.mpi_sampling_channels)require(channel==Channel::QQ||channel==Channel::GC,"unsupported MPI sampling channel");}
 auto working=state;auto& a=working.at(first);auto& b=working.at(second);
 for(auto* leg:{&a,&b}){
  const auto& p=leg->particle;
  bool photon=config.mpi_pair&&p.pdg==22&&p.status==4&&leg->record_mass==0&&p.colour==0&&p.anticolour==0;
  require((parton(p.pdg)&&(p.status==1||p.status==4))||photon,"hard pair requires quarks/gluons or an inactive massless MPI photon");
 }
 require(config.mpi_pair||config.evaluate_colour_angles||!config.first_angle_ordering,"first-angle ordering requires evaluated colour angles");
 HardInitializationResult result;result.colour_angles_evaluated=!config.mpi_pair&&config.evaluate_colour_angles;
 result.colour_angles=result.colour_angles_evaluated?std::array<double,2>{colour_opening_angle(working,first),colour_opening_angle(working,second)}:std::array<double,2>{-1,-1};
 // Source computes these scales before the pair rest-frame round trip.
 auto maximum=[&](const ShowerParticle& leg){
  if(leg.particle.status==4||(config.mpi_pair&&!config.mpi_fsr))return 0.;
  if(!config.mpi_pair)return config.pt_hat;
  const auto& p=leg.particle.momentum;
  double delta=std::abs(rapidity(a.particle.momentum,a.record_mass)-rapidity(b.particle.momentum,b.record_mass));
  double value=std::sqrt(p.px*p.px+p.py*p.py)*std::exp(double(.3f)*delta/2.)/2.;
  require(std::isfinite(value),"nonfinite MPI initialization scale");return value;
 };
 const double maximum_a=maximum(a),maximum_b=maximum(b);
 const double energy=a.particle.momentum.energy+b.particle.momentum.energy;
 require(energy>0,"nonpositive hard pair energy");
 const std::array<double,3> beta{(a.particle.momentum.px+b.particle.momentum.px)/energy,(a.particle.momentum.py+b.particle.momentum.py)/energy,(a.particle.momentum.pz+b.particle.momentum.pz)/energy};
 const std::array<double,3> inverse{-beta[0],-beta[1],-beta[2]};
 require(beta[0]*beta[0]+beta[1]*beta[1]+beta[2]*beta[2]<1,"non-timelike hard pair");
 boost(a,inverse);boost(b,inverse);const double total=a.particle.momentum.energy+b.particle.momentum.energy;
 boost(a,beta);boost(b,beta);
 auto sample=[&](ShowerParticle& leg,double maximum){return sampler.mass(0,maximum,-1,energy,config.mpi_pair?(*config.mpi_sampling_channels)[&leg==&a?0:1]:(leg.particle.pdg==21?Channel::GC:Channel::QQ),energy,false,random);};
 auto qa=sample(a,maximum_a),qb=sample(b,maximum_b);
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
void make_parton_massless(ShowerParticle& emitted,ShowerParticle& first,ShowerParticle& second){
 require(&emitted!=&first && &emitted!=&second && &first!=&second,"massless preparation requires distinct records");
 auto a=first,b=second,c=emitted;
 for(const auto* node:{&a,&b,&c}){
  require(node->particle.status>0,"massless preparation requires active records");
  require(std::isfinite(node->record_mass),"nonfinite preparation mass");
  momentum_magnitude(node->particle.momentum);
  require(std::isfinite(node->particle.momentum.energy),"nonfinite preparation energy");
 }
 auto& p=a.particle.momentum;auto& q=b.particle.momentum;auto& k=c.particle.momentum;
 double energy=p.energy+q.energy;require(energy>0,"invalid hard pair energy");
 std::array<double,3> beta{(p.px+q.px)/energy,(p.py+q.py)/energy,(p.pz+q.pz)/energy};
 auto inverse=beta;for(auto& x:inverse)x=-x;
 for(auto* node:{&a,&b,&c})rotate_boost(node->particle.momentum,&node->generator_vertex,0,0,inverse);
 double total=p.energy+q.energy+k.energy,third=momentum_magnitude(k);
 double remaining=total-third,m1=a.record_mass,m2=b.record_mass;
 require(remaining!=0,"zero remaining hard pair energy");
 double square=remaining*remaining-m1*m1-m2*m2;
 double new_square=(square*square-4.*m1*m1*m2*m2)/(4.*remaining*remaining);
 double new_magnitude=std::sqrt(std::abs(new_square));
 double first_energy=std::sqrt(new_square+m1*m1),second_energy=std::sqrt(new_square+m2*m2);
 // PYP(...,8) is evaluated afresh after each assignment in the source.
 for(auto* momentum:{&p,&q})for(double* component:{&momentum->px,&momentum->py,&momentum->pz}){
  double denominator=momentum_magnitude(*momentum);
  require(denominator>0,"undefined massless preparation direction");
  *component=*component*new_magnitude/denominator;
 }
 p.energy=first_energy;q.energy=second_energy;k.energy=third;c.record_mass=0;
 for(auto* node:{&a,&b,&c}){
  rotate_boost(node->particle.momentum,&node->generator_vertex,0,0,beta);
  const auto& v=node->particle.momentum;
  for(double x:{v.px,v.py,v.pz,v.energy,node->generator_vertex.x,node->generator_vertex.y,node->generator_vertex.z,node->generator_vertex.t})require(std::isfinite(x),"nonfinite massless preparation output");
 }
 first=std::move(a);second=std::move(b);emitted=std::move(c);
}

void prepare_surviving_partons(std::vector<ShowerParticle>& records,std::size_t first,
 std::size_t second,const std::vector<double>& nominal_masses,double eta_max){
 require(first<records.size()&&second<records.size()&&first!=second,"invalid preparation hard pair");
 require(nominal_masses.size()==records.size(),"missing preparation nominal masses");
 require(std::isfinite(eta_max)&&eta_max>=0,"invalid preparation pseudorapidity limit");
 for(std::size_t i=0;i<records.size();++i){
  require(records[i].particle.status>0,"preparation requires surviving positive-status records");
  require(std::isfinite(records[i].record_mass)&&std::isfinite(nominal_masses[i])&&nominal_masses[i]>=0,"invalid preparation mass");
 }
 auto working=records;
 for(auto i:{first,second})if(working[i].particle.status<11)working[i].particle.status=1;
 for(std::size_t i=0;i<working.size();++i){
  auto& node=working[i];auto& status=node.particle.status;int pdg=node.particle.pdg;
  // Literal ISQORG includes PDG zero and excludes top; the adapter must not
  // insert unused padding into the surviving record range.
  bool quark_or_gluon=(pdg>-6&&pdg<6)||pdg==21;
  if(!quark_or_gluon&&status<11)status=4;
  else if(i!=first&&i!=second&&status<11){
   if(node.record_mass>nominal_masses[i]*double(1.00001f))status=1;
   else if(quark_or_gluon&&std::abs(pseudorapidity(node.particle.momentum))<eta_max){
    if(node.record_mass>0)make_parton_massless(node,working[first],working[second]);
    status=1;
   }else status=4;
  }
 }
 records=std::move(working);
}

}
