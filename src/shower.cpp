// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/shower.hpp"
#include "jewel/kinematics.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace jewel {
namespace {
void rotate(ShowerParticle& p,double theta,double phi){
 if(p.particle.status>0)rotate_boost(p.particle.momentum,&p.generator_vertex,theta,phi,{0,0,0});
}
}
void reject_splitting(ShowerState& state,ParticleId mother,const BranchingSampler& sampler,Random& random){
 auto& m=state.at(mother);auto children=state.scheduling_daughters(mother);auto first=children[0],second=children[1];
 if(!first||!second)throw std::domain_error("rejected splitting requires two siblings");
 for(auto sibling:{first,second}){
  auto id=sibling;
  for(std::size_t visited=0;state.at(id).particle.status==12;++visited){
   if(visited>=state.particles().size())throw std::domain_error("cyclic rejected-scattering chain");
   for(auto child:state.scheduling_daughters(id))state.at(child).particle.status=-1;
   id=state.scheduling_parent(id);
  }
  if(state.at(sibling).particle.status==12)state.at(sibling).particle.status=1;
 }
 auto& c=state.at(first);auto& b=state.at(second);
 double theta=polar_angle(m.particle.momentum),phi=azimuth(m.particle.momentum);
 // PYROBO rotates the inclusive event-record range, not just the endpoints.
 std::vector<ParticleId> range;bool inside=false;
 for(const auto& p:state.particles()){
  if(p.particle.id==first)inside=true;
  if(inside)range.push_back(p.particle.id);
  if(p.particle.id==second)break;
 }
 if(range.empty()||range.back()!=second)throw std::domain_error("reversed sibling record range");
 rotate(m,0,-phi);rotate(m,-theta,0);
 for(auto id:range)rotate(state.at(id),0,-phi);
 for(auto id:range)rotate(state.at(id),-theta,0);
 auto& pm=m.particle.momentum;auto& pb=b.particle.momentum;auto& pc=c.particle.momentum;
 double pts=pc.px*pc.px+pc.py*pc.py;
 double mb=b.record_mass,mc=c.record_mass,mbnew=mb,mcnew=mc;
 bool newb=b.angle>m.angle,newc=c.angle>m.angle;
 double bmax=std::min(mb,pb.energy),cmax=std::min(mc,pc.energy),pznew=0,ptsnew=0;
 auto sample=[&](ShowerParticle& p,double previous,double maximum){
  auto value=sampler.mass(0,previous,m.angle,p.particle.momentum.energy,
    p.particle.pdg==21?Channel::GC:Channel::QQ,maximum,false,random);
  p.record_mass=value.mass;p.fraction=value.fraction.value();p.quark_pair=value.quark_pair.value();
  p.angle=p.fraction>0?value.mass/(std::sqrt(p.fraction*(1-p.fraction))*p.particle.momentum.energy):0.;
  return value.mass;
 };
 for(unsigned trial=0;;++trial){
  if(trial==1000000)throw std::runtime_error("rejected splitting failed to converge");
  if(newc)mcnew=sample(c,mc,cmax);
  if(newb)mbnew=sample(b,mb,bmax);
  if(pm.pz==0)throw std::domain_error("undefined parent splitting axis");
  pznew=(2*m.fraction*pm.energy*pm.energy-m.record_mass*m.record_mass-mbnew*mbnew+mcnew*mcnew)/(2*pm.pz);
  ptsnew=m.fraction*m.fraction*pm.energy*pm.energy-pznew*pznew-mbnew*mbnew;
  if(ptsnew>=0)break;
  if(mbnew==0&&mcnew==0){state.discarded=true;return;}
  if(mbnew==0){cmax=mcnew;newb=false;newc=true;}
  else if(mcnew==0){bmax=mbnew;newc=false;newb=true;}
  else if(random.uniform()>.5){newb=false;newc=true;cmax=mcnew;}
  else {newb=true;newc=false;bmax=mbnew;}
 }
 if(pts<=0)throw std::domain_error("undefined zero transverse momentum rescaling");
 pb.px*=std::sqrt(ptsnew/pts);pb.py*=std::sqrt(ptsnew/pts);pb.pz=pznew;
 pc.px=pm.px-pb.px;pc.py=pm.py-pb.py;pc.pz=pm.pz-pb.pz;
 if(newc)c.evolution_time=c.decay_time;
 if(newb)b.evolution_time=b.decay_time;
 if(mcnew==0)c.decay_time=0;
 else c.decay_time=c.particle.production.t+(c.decay_time-c.particle.production.t)*mc*mc/(mcnew*mcnew);
 if(mbnew==0)b.decay_time=0;
 else b.decay_time=b.particle.production.t+(b.decay_time-b.particle.production.t)*mb*mb/(mbnew*mbnew);
 rotate(m,theta,0);rotate(m,0,phi);
 for(auto id:range)rotate(state.at(id),theta,0);
 for(auto id:range)rotate(state.at(id),0,phi);
}
void make_splitting(ShowerState& state,ParticleId id,const BranchingSampler& sampler,
 const Medium& medium,double formation_factor,Random& random){
 auto& parent=state.at(id);auto& p=parent.particle.momentum;
 int status=parent.particle.status,mode=sampler.angular_ordering();
 if(parent.record_mass==0||status==11||status==12||status==13||status==14||status==3||parent.fraction<0)return;
 if(state.particles().size()+2>22990){state.discarded=true;return;}
 if(!std::isfinite(formation_factor)||formation_factor<=0||p.energy<=0)
  throw std::domain_error("invalid splitting formation scale or energy");
 double theta_in=polar_angle(p),phi_in=azimuth(p);
 rotate(parent,0,-phi_in);rotate(parent,-theta_in,0);
 double theta=0;
 if(mode==3){
  auto v=parent.particle.production;double dt=parent.decay_time-v.t;
  v={v.x+dt*p.px/p.energy,v.y+dt*p.py/p.energy,v.z+dt*p.pz/p.energy,parent.decay_time};
  theta=medium.sample(v).temperature>0?-1:parent.angle;
 }else if(mode==4)theta=parent.angle;
 bool quark=parent.particle.pdg!=21,qqbar=parent.quark_pair;
 int direction=(quark||qqbar)?0:(random.uniform()<.5?1:-1);
 double z=parent.fraction;
 if(z==0){state.discarded=true;return;}
 if(z<=0||z>=1||p.pz==0)throw std::domain_error("undefined splitting kinematics");
 double bmax=std::min(parent.record_mass,z*p.energy),cmax=std::min(parent.record_mass,(1-z)*p.energy);
 bool newb=true,newc=true;MassSample bmass,cmass;double pz=0,pts=0,thetab=0,thetac=0;
 auto restore=[&]{rotate(state.at(id),theta_in,0);rotate(state.at(id),0,phi_in);};
 for(unsigned trial=0;;++trial){
  if(trial==1000000)throw std::runtime_error("splitting kinematics failed to converge");
  if(newb)bmass=sampler.mass(0,bmax,theta,z*p.energy,(quark||qqbar)?Channel::QQ:Channel::GC,bmax,false,random);
  if(newc)cmass=sampler.mass(0,cmax,theta,(1-z)*p.energy,(quark||!qqbar)?Channel::GC:Channel::QQ,cmax,false,random);
  double mb=bmass.mass,mc=cmass.mass;
  // MAKESPLITTING squares E before multiplying by z. Preserve the frozen
  // GNU Fortran subtraction order: fuse the parent-mass and pz squares,
  // while the reused daughter-mass squares are rounded independently.
  double e2=p.energy*p.energy,mb2=mb*mb,mc2=mc*mc;
  double numerator=std::fma(-parent.record_mass,parent.record_mass,(2*z)*e2);
  numerator-=mb2;
  numerator+=mc2;
  pz=numerator/(2*p.pz);
  pts=std::fma(-pz,pz,(z*z)*e2)-mb2;
  if(pts<0||mb+mc>parent.record_mass){
   if(mb==0&&mc==0){
    state.discarded=true;restore();
    // Stock label 31 also rotates the last two existing records on this exit.
    auto n=state.particles().size();
    if(n<2)throw std::domain_error("splitting requires an initialized event record");
    auto a=state.particles()[n-2].particle.id,b=state.particles()[n-1].particle.id;
    rotate(state.at(a),theta_in,0);rotate(state.at(b),theta_in,0);
    rotate(state.at(a),0,phi_in);rotate(state.at(b),0,phi_in);return;
   }
   if(mb==0){newb=false;newc=true;cmax=mc;}
   else if(mc==0){newb=true;newc=false;bmax=mb;}
   else if(random.uniform()>.5){newb=false;newc=true;cmax=mc;}
   else {newb=true;newc=false;bmax=mb;}
   continue;
  }
  if(mode==5)parent.angle=std::asin(std::sqrt(pts/(z*z*p.energy*p.energy-mb*mb)))+
    std::asin(std::sqrt(pts/((1-z)*(1-z)*p.energy*p.energy-mc*mc)));
  if(mode==1||mode==5){
   auto mother=state.scheduling_parent(id);
   if(!mother)throw std::domain_error("angular ordering requires a parent record");
   bool scattered=false,first=state.scheduling_daughters(mother)[0]==id;
   for(std::size_t count=0;state.at(mother).particle.status==12;++count){
    if(count>=state.particles().size())throw std::domain_error("cyclic scattering ancestry");
    if(state.at(state.scheduling_daughters(mother)[0]).particle.status!=11)scattered=true;
    first=state.scheduling_daughters(state.scheduling_parent(mother))[0]==mother;mother=state.scheduling_parent(mother);
   }
   auto& m=state.at(mother);bool reject=false;
   if(!scattered&&parent.angular_required&&m.angle>=0){
    auto daughter=state.scheduling_daughters(mother)[first?1:0];scattered=false;
    for(std::size_t count=0;state.at(daughter).particle.status==12;++count){
     if(count>=state.particles().size())throw std::domain_error("cyclic sibling scattering history");
     if(state.at(state.scheduling_daughters(state.scheduling_parent(daughter))[0]).particle.status!=11)scattered=true;
     daughter=state.scheduling_daughters(daughter)[1];
    }
    if(!scattered&&state.at(daughter).particle.status!=11)
     reject=state.at(state.scheduling_daughters(mother)[0]).angle>m.angle||state.at(state.scheduling_daughters(mother)[1]).angle>m.angle;
   }
   if(reject){restore();reject_splitting(state,mother,sampler,random);return;}
  }
  auto zb=bmass.fraction.value(),zc=cmass.fraction.value();
  thetab=zb>0?mb/(std::sqrt(zb*(1-zb))*z*p.energy):0;
  thetac=zc>0?mc/(std::sqrt(zc*(1-zc))*(1-z)*p.energy):0;
  if(mode==2&&parent.angle>0){
   if(thetab>parent.angle){newb=true;newc=false;bmax=mb;continue;}
   if(thetac>parent.angle){newc=true;newb=false;cmax=mc;continue;}
  }
  break;
 }
 ShowerParticle c,b;
 for(auto* child:{&c,&b}){child->particle.status=1;child->parent=id;child->angular_required=true;child->particle.production.t=parent.decay_time;}
 c.record_mass=cmass.mass;c.fraction=cmass.fraction.value();c.quark_pair=cmass.quark_pair.value();c.angle=thetac;
 b.record_mass=bmass.mass;b.fraction=bmass.fraction.value();b.quark_pair=bmass.quark_pair.value();b.angle=thetab;
 if(qqbar){c.particle.pdg=-1;c.particle.anticolour=parent.particle.anticolour;}
 else{
  c.particle.pdg=21;
  if(parent.particle.pdg>0&&direction>=0){c.particle.colour=parent.particle.colour;c.particle.anticolour=state.colour_max+1;}
  else {c.particle.colour=state.colour_max+1;c.particle.anticolour=parent.particle.anticolour;}
  ++state.colour_max;
 }
 if(quark){b.particle.pdg=parent.particle.pdg;
  if(b.particle.pdg>0)b.particle.colour=c.particle.anticolour;else b.particle.anticolour=c.particle.colour;
 }else if(qqbar){b.particle.pdg=1;b.particle.colour=parent.particle.colour;}
 else {b.particle.pdg=21;
  if(direction==1){b.particle.colour=c.particle.anticolour;b.particle.anticolour=parent.particle.anticolour;}
  else {b.particle.colour=parent.particle.colour;b.particle.anticolour=c.particle.colour;}
 }
 double phi=2*3.141592653589793*random.uniform();
 b.particle.momentum={std::sqrt(pts)*std::cos(phi),std::sqrt(pts)*std::sin(phi),pz,z*p.energy};
 auto pb=b.particle.momentum;
 c.particle.momentum={p.px-pb.px,p.py-pb.py,p.pz-pb.pz,(1-z)*p.energy};
 for(auto* child:{&c,&b})if(child->record_mass>0){
  double lambda=1/(formation_factor*child->particle.momentum.energy*double(.2f)/(child->record_mass*child->record_mass));
  child->decay_time=parent.decay_time-std::log(1-random.uniform())/lambda;
 }
 parent.particle.status=status==2?13:11;
 // append can reallocate: do not retain references into state beyond this point.
 auto cid=state.append(c,true,true),bid=state.append(b,true,true);state.connect(id,cid,bid);
 restore();rotate(state.at(cid),theta_in,0);rotate(state.at(bid),theta_in,0);
 rotate(state.at(cid),0,phi_in);rotate(state.at(bid),0,phi_in);
}

}
