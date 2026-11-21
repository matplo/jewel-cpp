// SPDX-License-Identifier: GPL-2.0-or-later
// Two-body section of DOKINEMATICS, JEWEL 2.6.0.
#include "jewel/collision.hpp"
#include "jewel/kinematics.hpp"
#include "kinematics_internal.hpp"
#include "collision_internal.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <limits>
#include <cstdlib>
#include <iostream>
#include <iomanip>
namespace jewel {
namespace {
void transform(ShowerParticle& a,double theta,double phi,std::array<double,3> beta={0,0,0}){
 if(a.particle.status>0)rotate_boost(a.particle.momentum,&a.generator_vertex,theta,phi,beta);
}
double squared(double x){return x*x;}
}
CollisionPair detail::scatter_pair_candidate(ShowerParticle projectile,ShowerParticle scratch,const TransferRecord& transfer,
 double total,double remaining,double target,double delta,CollisionConfig config,
 const BranchingSampler& sampler,const Medium& medium,Random& random,const ShowerParticle* original_projectile,std::optional<double> requested_mass){
 for(double value:{config.cutoff,config.mass_scale,total,remaining,target,delta,transfer.invariant})
  if(!std::isfinite(value))throw std::domain_error("nonfinite collision input");
 if(config.kinematic_mode<0||config.kinematic_mode>3||config.cutoff<=0||config.mass_scale<=0)
  throw std::domain_error("invalid collision configuration");
 CollisionPair result;result.incoming=projectile;result.centre=scratch;result.target_mass=target;
 result.remaining_transfer=total;
 auto& a=result.incoming;auto& b=result.centre;auto& c=result.outgoing;auto& d=result.recoil;
 auto& p=a.particle.momentum;auto& q=b.particle.momentum;
 auto& pc=c.particle.momentum;auto& pd=d.particle.momentum;
 const auto& origin=original_projectile?*original_projectile:projectile;
 bool is_recoil=origin.particle.status==2;
 Channel channel=projectile.particle.pdg==21?Channel::GC:Channel::QQ;
 if(projectile.particle.pdg==21)result.colour_direction=random.uniform()<.5?1:-1;
 b.particle.status=6;b.particle.pdg=transfer.scatterer.type;q=transfer.scatterer.momentum;
 b.record_mass=transfer.scatterer.mass;b.particle.production=transfer.position;
 double t=transfer.invariant;
 if(t==0){result.rejected=true;return result;}
 if(t>0)throw std::domain_error("timelike collision transfer");
 double dm=total==0?0:remaining<0?std::max(remaining*t/total*1.5,remaining):std::min(remaining*t/total*1.5,remaining);
 result.remaining_transfer=total-t;result.mass_change=dm;
 std::array<double,3> beta{(q.px+p.px)/(q.energy+p.energy),(q.py+p.py)/(q.energy+p.energy),(q.pz+p.pz)/(q.energy+p.energy)};
 bool reshuffle=!(beta[0]>1||beta[1]>1||beta[2]>1||std::sqrt(squared(beta[0])+squared(beta[1])+squared(beta[2]))>1);
 double theta=0,phi=0,m42=0,m32=0,newmass=0,shat=0,p3old=0;
 auto inverse=[&]{return std::array<double,3>{-beta[0],-beta[1],-beta[2]};};
 auto to_axis=[&]{theta=polar_angle(p);phi=azimuth(p);transform(a,0,-phi);transform(b,0,-phi);transform(a,-theta,0);transform(b,-theta,0);};
 auto from_axis=[&](ShowerParticle& particle){transform(particle,theta,0);transform(particle,0,phi);transform(particle,0,0,beta);};
 auto sample_recoil=[&](double maximum){
  auto value=sampler.mass(0,maximum,-1,q.energy,channel,q.energy,false,random);
  result.recoil_fraction=value.fraction.value();result.recoil_pair=value.quark_pair.value();
  result.split_recoil=value.mass>0;return squared(value.mass);
 };
 auto choose_mass=[&]{
  if(config.kinematic_mode==0)return 0.;
  if(config.kinematic_mode==1)return squared(b.record_mass);
  if(config.mass_scale*std::sqrt(-t)>config.cutoff)return sample_recoil(config.mass_scale*std::sqrt(-t));
  return squared(b.record_mass);
 };
 auto projectile_mass=[&](double original){
  newmass=original+dm;double value=std::copysign(squared(newmass),newmass);
  if(delta==0&&is_recoil&&config.kinematic_mode==3){
   double local=medium.sample(b.particle.production).temperature;
   double old=medium.sample(origin.particle.production).temperature;
   if(local>0){if(old==0)throw std::domain_error("zero original recoil temperature");value=squared(origin.record_mass*local/old);result.target_mass=std::sqrt(value);}
  }
  return value;
 };
 if(reshuffle){
  transform(a,0,0,inverse());transform(b,0,0,inverse());
  if(q.energy<0||p.energy<0){transform(b,0,0,beta);transform(a,0,0,beta);reshuffle=false;}
 }
 result.reshuffled=reshuffle;
 if(!reshuffle){
  beta={q.px/q.energy,q.py/q.energy,q.pz/q.energy};transform(a,0,0,inverse());transform(b,0,0,inverse());to_axis();
  m42=choose_mass();
  double maximum=-2.*b.record_mass*p.energy-squared(b.record_mass)+m42;
  if(t<maximum){result.rejected=true;result.mass_change=0;from_axis(a);from_axis(b);return result;}
  double m12=-squared(a.record_mass),enew=0,pl=0,pt2=0;
  for(unsigned retry=0;;++retry){
   if(retry==1000000)throw std::runtime_error("collision transverse retry exhausted");
   m32=projectile_mass(a.record_mass);
   enew=p.energy+(t+squared(b.record_mass)-m42)/(2.*b.record_mass);
   pl=(t+2.*p.energy*enew-m12-m32)/(2.*p.pz);pt2=std::fma(enew,enew,-squared(pl))-m32;
   if(std::abs(pt2)<1e-8)pt2=0;
   if(pt2>=0)break;
   if(result.split_recoil){m42=sample_recoil(std::sqrt(m42));continue;}
   if(dm!=0){dm=0;continue;}
   result.rejected=true;from_axis(a);from_axis(b);return result;
  }
  double pt=std::sqrt(pt2),phi2=random.uniform()*2.*3.141592653589793;
  pc={pt*std::cos(phi2),pt*std::sin(phi2),pl,enew};c.record_mass=std::copysign(std::sqrt(std::abs(m32)),newmass);
 }else{
  to_axis();shat=squared(q.energy+p.energy);p3old=p.pz;
  if(t< -4.*squared(p.pz)){result.rejected=true;from_axis(a);from_axis(b);return result;}
  double theta2=std::acos(1+t/(2.*squared(p.pz))),phi2=random.uniform()*2.*3.141592653589793;
  pc={p.pz*std::sin(theta2)*std::cos(phi2),p.pz*std::sin(theta2)*std::sin(phi2),p.pz*std::cos(theta2),p.energy};c.record_mass=a.record_mass;
 }
 pd={q.px+p.px-pc.px,q.py+p.py-pc.py,q.pz+p.pz-pc.pz,q.energy+p.energy-pc.energy};
 // Preserve fused cancellation used by the frozen Fortran reference.
 double mass2=std::fma(pd.energy,pd.energy,-squared(pd.px));
 mass2=std::fma(-pd.py,pd.py,mass2);mass2=std::fma(-pd.pz,pd.pz,mass2);
 // Source literal is -1.-6 (= -7), not -1.e-6.
 if(mass2<0&&mass2> -7.)mass2=0;
 if(mass2<0)throw std::domain_error("negative collision recoil mass");
 d.record_mass=std::sqrt(mass2);
 c.particle.status=a.particle.status;
 if(config.last_transfer&&is_recoil&&requested_mass.value_or(target)>origin.record_mass&&c.record_mass>config.cutoff)c.particle.status=1;
 c.particle.pdg=a.particle.pdg;a.particle.status=a.particle.status==1?12:14;
 d.particle.status=3;d.particle.pdg=21;
 if(reshuffle){
  m42=choose_mass();double e3=0,e4=0,p32=0,p42=0;
  for(unsigned retry=0;;++retry){
   if(retry==1000000)throw std::runtime_error("collision mass retry exhausted");
   m32=projectile_mass(c.record_mass);
   e3=(shat+m32-m42)/(2.*std::sqrt(shat));e4=(shat-m32+m42)/(2.*std::sqrt(shat));
   p32=squared(e3)-m32;p42=squared(e4)-m42;
   if(p32<0||p42<0||e3<0||e4<0){
    p42=m42==0?1e-4:0.;e4=std::sqrt(p42+m42);e3=std::sqrt(shat)-e4;
    p32=std::fma(e4,e4,-m42);m32=squared(e3)-squared(e4)+m42;
    // The fallback explicitly sets p42=0 and e4=sqrt(m42). Its exact
    // p32=e4^2-m42 is zero; rounded sqrt followed by fused subtraction can
    // instead give a tiny negative value and poison all subsequent boosts.
    // Repair only this algebraically zero branch, not general negative p^2.
    if(p42==0&&p32<0){
     if(std::getenv("JEWEL_COLLISION_TRACE"))std::cerr<<std::setprecision(17)<<"COLLISION_ZERO_P2 "<<p32<<' '<<m42<<' '<<e4<<'\n';
     p32=0;
    }
    if(e3<0||e4<0){
     if(result.split_recoil){m42=sample_recoil(std::sqrt(m42));continue;}
     if(dm!=0){dm=0;continue;}
     m42=squared(b.record_mass);e3=pc.energy;e4=pd.energy;p32=squared(p3old);p42=p32;m32=std::copysign(squared(c.record_mass),c.record_mass);
    }
   }
   break;
  }
  if((p32<0||p42<0)&&std::getenv("JEWEL_COLLISION_TRACE")){
   std::cerr<<std::setprecision(17)<<"COLLISION_NEGATIVE_P2 "<<shat<<' '<<m32<<' '<<m42<<' '<<e3<<' '<<e4<<' '<<p32<<' '<<p42<<' '<<p3old<<'\n';
  }
  pc={std::sqrt(p32)*pc.px/p3old,std::sqrt(p32)*pc.py/p3old,std::sqrt(p32)*pc.pz/p3old,e3};
  pd={std::sqrt(p42)*pd.px/p3old,std::sqrt(p42)*pd.py/p3old,std::sqrt(p42)*pd.pz/p3old,e4};
  c.record_mass=std::copysign(std::sqrt(std::abs(m32)),newmass);d.record_mass=std::sqrt(m42);
 }
 // Keep temporary invalid outgoing candidates until the sequence retry decision.
 auto candidate_from_axis=[&](ShowerParticle& particle){
  if(particle.particle.status<=0)return;
  detail::rotate_boost_candidate(particle.particle.momentum,&particle.generator_vertex,theta,0,{0,0,0});
  detail::rotate_boost_candidate(particle.particle.momentum,&particle.generator_vertex,0,phi,{0,0,0});
  detail::rotate_boost_candidate(particle.particle.momentum,&particle.generator_vertex,0,0,beta);
 };
 candidate_from_axis(c);candidate_from_axis(d);from_axis(a);from_axis(b);result.mass_change=dm;return result;
}
}

namespace jewel {
void detail::finish_collision_candidate(CollisionPair& result,int& colour_max,RecoilStorage& storage,
 CollisionFinalizationConfig config,CollisionStepTiming timing,const Medium& medium,Random& random){
 for(double v:{config.formation_factor,config.soft_recoil_cut,config.hard_recoil_cut,timing.total_mass_change,
  timing.fraction,timing.first_decay_time,timing.coherent_end_time,timing.next_transfer_time,timing.transfer_time})
  if(!std::isfinite(v))throw std::domain_error("nonfinite collision finalization input");
 if(config.formation_factor<=0||colour_max<0)
  throw std::domain_error("invalid collision finalization input");
 if(result.rejected)return;
 auto& a=result.incoming;auto& b=result.centre;auto& c=result.outgoing;auto& d=result.recoil;
 bool soft=false;
 if(!config.hadronize_recoils)d.particle.status=13;
 else{
  auto point=b.particle.production;
  if(point.t<=std::abs(point.z))throw std::domain_error("collision outside fluid rapidity domain");
  double ys=.5*std::log((point.t+point.z)/(point.t-point.z));auto p=d.particle.momentum;
  double longitudinal=std::sinh(-ys)*p.energy+std::cosh(-ys)*p.pz;
  double magnitude=std::sqrt(longitudinal*longitudinal+p.px*p.px+p.py*p.py);
  double temperature=medium.sample(point).temperature;
  soft=magnitude<config.soft_recoil_cut*3.*temperature;
  d.particle.status=soft?13:config.scatter_recoils&&magnitude>config.hard_recoil_cut*3.*temperature?2:3;
 }
 bool coloured=config.hadronize_recoils&&!soft;
 if(coloured){
  if(colour_max==std::numeric_limits<int>::max())throw std::domain_error("collision colour index exhausted");
  if(c.particle.pdg==21){
   c.particle.colour=result.colour_direction==1?colour_max+1:a.particle.colour;
   c.particle.anticolour=result.colour_direction==1?a.particle.anticolour:colour_max+1;
  }else if(c.particle.pdg>0){c.particle.colour=colour_max+1;c.particle.anticolour=0;}
  else{c.particle.colour=0;c.particle.anticolour=colour_max+1;}
  ++colour_max;
  if(c.particle.pdg>0&&result.colour_direction>=0){d.particle.colour=a.particle.colour;d.particle.anticolour=c.particle.colour;}
  else{d.particle.colour=c.particle.anticolour;d.particle.anticolour=a.particle.anticolour;}
 }else{
  c.particle.colour=a.particle.colour;c.particle.anticolour=a.particle.anticolour;
  d.particle.colour=0;d.particle.anticolour=0;
 }
 if(c.particle.status==2&&timing.fraction==0){c.fraction=-1;c.angle=-1;}
 else{c.fraction=timing.fraction;c.angle=timing.fraction==0?0:c.record_mass/(std::sqrt(timing.fraction*(1-timing.fraction))*c.particle.momentum.energy);}
 c.quark_pair=timing.quark_pair;c.angular_required=false;
 if(result.split_recoil){
  if(d.particle.status==2)d.particle.status=1;
  d.angle=d.record_mass/(std::sqrt(result.recoil_fraction*(1-result.recoil_fraction))*d.particle.momentum.energy);
  d.fraction=result.recoil_fraction;d.quark_pair=result.recoil_pair;
 }else{d.angle=-1;d.fraction=-1;d.quark_pair=false;}
 d.angular_required=false;
 c.particle.production=position_at(a.particle.momentum,a.particle.production,b.particle.production.t);
 d.particle.production=c.particle.production;
 if(d.record_mass>b.record_mass){
  double lambda=1./(config.formation_factor*double(.2f)*d.particle.momentum.energy/(d.record_mass*d.record_mass));
  d.decay_time=d.particle.production.t-std::log(1-random.uniform())/lambda;
 }else d.decay_time=0;
 if(!timing.last_transfer)c.decay_time=timing.next_transfer_time;
 else if(c.record_mass>0){
  if(timing.total_mass_change==0)c.decay_time=timing.coherent?timing.coherent_end_time:timing.first_decay_time;
  else{
   double lambda=1./(config.formation_factor*c.particle.momentum.energy*double(.2f)/(c.record_mass*c.record_mass));
   c.decay_time=b.particle.production.t-std::log(1-random.uniform())/lambda;
  }
 }else c.decay_time=0;
 a.decay_time=timing.transfer_time;
 store_recoil(storage,config.storage,b.particle.pdg,b.particle.momentum,b.record_mass,d.particle.momentum,soft,false);
}
}

namespace jewel {
// The public single-step API retains its strict physical-fraction contract.
// Full sequences use the private candidate path so source retries can discard
// temporary invalid records before apply_collisions validates publication.
void finish_collision(CollisionPair& result,int& colour_max,RecoilStorage& storage,
 CollisionFinalizationConfig config,CollisionStepTiming timing,const Medium& medium,Random& random){
 if(timing.fraction<0||timing.fraction>=1)throw std::domain_error("invalid collision finalization input");
 detail::finish_collision_candidate(result,colour_max,storage,config,timing,medium,random);
}
CollisionPair scatter_pair(ShowerParticle projectile,ShowerParticle scratch,const TransferRecord& transfer,
 double total,double remaining,double target,double delta,CollisionConfig config,
 const BranchingSampler& sampler,const Medium& medium,Random& random,const ShowerParticle* original,std::optional<double> requested){
 auto result=detail::scatter_pair_candidate(projectile,scratch,transfer,total,remaining,target,delta,config,sampler,medium,random,original,requested);
 if(!result.rejected)for(const auto* particle:{&result.outgoing,&result.recoil}){
  const auto& p=particle->particle.momentum;
  for(double v:{p.px,p.py,p.pz,p.energy})if(!std::isfinite(v))throw std::domain_error("nonfinite collision candidate");
 }
 return result;
}
}
