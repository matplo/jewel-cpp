// SPDX-License-Identifier: GPL-2.0-or-later
// MAKEINSPLIT, JEWEL 2.6.0.
#include "jewel/shower.hpp"
#include "jewel/kinematics.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
namespace jewel {
namespace {
void rotate_initial(ShowerParticle& s,double theta,double phi){
 if(s.particle.status>0)rotate_boost(s.particle.momentum,&s.generator_vertex,theta,phi,{0,0,0});
}
}
InitialSplitResult make_initial_splitting(ShowerState& state,ParticleId id,const InitialSplitRequest& input,
 const BranchingSampler& sampler,Random& random){
 for(double v:{input.fraction,input.transfer_sum,input.virtuality,input.time,input.radiation_time,input.formation_factor,input.mass_scale})
  if(!std::isfinite(v))throw std::domain_error("nonfinite initial splitting input");
 if(input.fraction<=0||input.fraction>=1||input.transfer_sum>0||input.virtuality<0||input.formation_factor<=0||input.mass_scale<=0)
  throw std::domain_error("invalid initial splitting input");
 auto mother=state.at(id);InitialSplitResult result{input.fraction,0,0};
 if(state.particles().size()+2>22990){state.discarded=true;return result;}
 enum class Type {GG,QG,GQ,QQ};
 bool gluon=mother.particle.pdg==21,newgluon=input.incoming_flavour==21;
 auto type=gluon?(newgluon?Type::GG:Type::QG):(newgluon?Type::GQ:Type::QQ);
 auto channel=(type==Type::GG||type==Type::QQ)?Channel::GC:Channel::QQ;
 int direction=type==Type::GG?(random.uniform()<.5?1:-1):0;
 double x=input.fraction,energy=mother.particle.momentum.energy;
 double mb2=mother.record_mass*mother.record_mass-input.virtuality*input.virtuality;
 auto emitted=sampler.mass(0,input.mass_scale*std::sqrt(-input.transfer_sum),-1,(1-x)*energy,channel,(1-x)*energy,false,random);
 double mc2=emitted.mass*emitted.mass,zdec=emitted.fraction.value();bool pair=emitted.quark_pair.value();
 double theta=polar_angle(mother.particle.momentum),phi=azimuth(mother.particle.momentum);
 rotate_initial(mother,0,-phi);rotate_initial(mother,-theta,0);
 auto& p=mother.particle.momentum;
 if(p.pz==0)throw std::domain_error("zero initial-splitting longitudinal momentum");
 auto longitudinal=[&]{return (2*x*(p.energy*p.energy)-mother.record_mass*mother.record_mass-mb2+mc2)/(2*p.pz);};
 // Preserve the reference fused subtraction, including the collinear limit.
 double pz=longitudinal(),kt2=std::fma(-pz,pz,x*x*(p.energy*p.energy))-mb2;
 if(kt2<0){
  mc2=0;if(mother.particle.status==2)zdec=-1;
  pz=longitudinal();kt2=std::fma(-pz,pz,x*x*(p.energy*p.energy))-mb2;
  if(kt2<0){
   rotate_initial(mother,theta,0);rotate_initial(mother,0,phi);state.at(id)=mother;result.fraction=1;return result;
  }
 }
 ShowerParticle c,b;for(auto* child:{&c,&b}){child->particle.status=1;child->parent=id;}
 if(type==Type::QG){
  c.particle.pdg=-input.incoming_flavour;
  if(c.particle.pdg>0)c.particle.colour=mother.particle.colour;else c.particle.anticolour=mother.particle.anticolour;
 }else{
  if(state.colour_max==std::numeric_limits<int>::max())throw std::domain_error("initial splitting colour index exhausted");
  if(type==Type::GQ){
   c.particle.pdg=mother.particle.pdg;
   if(c.particle.pdg>0)c.particle.colour=state.colour_max+1;else c.particle.anticolour=state.colour_max+1;
  }else{
   c.particle.pdg=21;
   if(mother.particle.pdg>0&&direction>=0){c.particle.colour=mother.particle.colour;c.particle.anticolour=state.colour_max+1;}
   else{c.particle.colour=state.colour_max+1;c.particle.anticolour=mother.particle.anticolour;}
  }
  ++state.colour_max;
 }
 c.particle.momentum.energy=(1-x)*p.energy;c.record_mass=std::sqrt(mc2);
 if(type==Type::QG){
  b.particle.pdg=input.incoming_flavour;
  if(b.particle.pdg>0)b.particle.colour=mother.particle.colour;else b.particle.anticolour=mother.particle.anticolour;
 }else if(!newgluon){
  b.particle.pdg=mother.particle.pdg;
  if(b.particle.pdg>0)b.particle.colour=c.particle.anticolour;else b.particle.anticolour=c.particle.colour;
 }else{
  b.particle.pdg=21;
  if(c.particle.pdg==21){
   b.particle.colour=direction==1?c.particle.anticolour:mother.particle.colour;
   b.particle.anticolour=direction==1?mother.particle.anticolour:c.particle.colour;
  }else if(c.particle.pdg>0){b.particle.colour=mother.particle.colour;b.particle.anticolour=c.particle.colour;}
  else{b.particle.colour=c.particle.anticolour;b.particle.anticolour=mother.particle.anticolour;}
 }
 b.record_mass=mb2<0?-std::sqrt(-mb2):std::sqrt(mb2);
 double phi_q=2*3.141592653589793*random.uniform();
 b.particle.momentum={std::sqrt(kt2)*std::cos(phi_q),std::sqrt(kt2)*std::sin(phi_q),pz,x*p.energy};
 auto pb=b.particle.momentum;
 c.particle.momentum={p.px-pb.px,p.py-pb.py,p.pz-pb.pz,(1-x)*p.energy};
 mother.decay_time=input.time-input.radiation_time;c.particle.production.t=mother.decay_time;b.particle.production.t=mother.decay_time;
 if(c.record_mass>0){
  double lambda=1./(input.formation_factor*c.particle.momentum.energy*double(.2f)/(c.record_mass*c.record_mass));
  c.decay_time=mother.decay_time-std::log(1-random.uniform())/lambda;
 }
 if(b.record_mass>0)b.decay_time=input.time;
 c.fraction=zdec;c.quark_pair=pair;c.angle=zdec==0?-1:c.record_mass/(std::sqrt(zdec*(1-zdec))*c.particle.momentum.energy);
 b.angle=-1;b.fraction=0;b.quark_pair=false;
 mother.particle.status=mother.particle.status==2?13:11;
 rotate_initial(mother,theta,0);rotate_initial(c,theta,0);rotate_initial(b,theta,0);
 rotate_initial(mother,0,phi);rotate_initial(c,0,phi);rotate_initial(b,0,phi);
 c.particle.production=position_at(mother.particle.momentum,mother.particle.production,c.particle.production.t);
 b.particle.production=position_at(mother.particle.momentum,mother.particle.production,b.particle.production.t);
 state.at(id)=mother;result.radiated=state.append(c,true,true);result.continuing=state.append(b,true,true);
 state.connect(id,result.radiated,result.continuing,true);return result;
}
}
