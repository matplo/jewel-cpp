// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/pythia8_isr.hpp"
#include <Pythia8/Basics.h>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <sstream>
#include <iomanip>
namespace jewel {
namespace {
void require(bool b,const char* message){if(!b)throw std::domain_error(message);}
std::array<double,4> apply(const IsrRecoilFrame& frame,std::array<double,4> input){
 std::array<double,4> out{};
 for(int i=0;i<4;++i)for(int j=0;j<4;++j){require(std::isfinite(frame.matrix[4*i+j])&&std::isfinite(input[j]),"nonfinite ISR frame input");out[i]+=frame.matrix[4*i+j]*input[j];}
 for(double value:out)require(std::isfinite(value),"nonfinite ISR frame output");return out;
}
bool collinear(const GeneratorParticle& p){return p.momentum.energy>0&&p.mass==0&&std::hypot(p.momentum.px,p.momentum.py)<=1e-10*p.momentum.energy&&std::abs(std::abs(p.momentum.pz)-p.momentum.energy)<=1e-10*p.momentum.energy;}
}
Momentum apply_isr_frame(const IsrRecoilFrame& frame,Momentum p){auto q=apply(frame,{p.energy,p.px,p.py,p.pz});return {q[1],q[2],q[3],q[0]};}
SpaceTime apply_isr_frame_displacement(const IsrRecoilFrame& frame,SpaceTime v){auto q=apply(frame,{v.t,v.x,v.y,v.z});return {q[1],q[2],q[3],q[0]};}
namespace {
std::vector<IsrRecoilFrame> reconstruct_frames(const HardEventSnapshot& input,const std::array<IsrIncomingChain,2>& chains,const std::vector<GeneratorIsrFractions>* fractions=nullptr){
 std::vector<IsrRecoilFrame> frames;
 auto at=[&](ParticleId id)->const GeneratorParticle&{require(id>0&&id<=input.particles.size(),"missing ISR recoil line");return input.particles[id-1];};
 const double pi=std::acos(-1.);
 for(int side=0;side<2;++side)for(const auto& branch:chains[side].branches){
  const auto& mother=at(branch.mother);const auto& daughter=at(branch.daughter);const auto& sister=at(branch.emitted);
  // SimpleSpaceShower appends the incoming radiator/recoiler pair consecutively.
  std::vector<ParticleId> candidates;
  for(auto id:{mother.id-1,mother.id+1})if(id>0&&id<=input.particles.size()&&std::abs(at(id).status)==42)candidates.push_back(id);
  require(candidates.size()==1,"ambiguous or unsupported ISR recoil pair");const auto& recoiler=at(candidates[0]);
  require(recoiler.daughters.size()==1,"invalid ISR recoiler daughter");const auto& old=at(recoiler.daughters[0]);
  require(std::find(chains[1-side].backward_lines.begin(),chains[1-side].backward_lines.end(),recoiler.id)!=chains[1-side].backward_lines.end(),"ISR recoiler is not on opposite beam chain");
  const GeneratorIsrFractions* captured=nullptr;
  if(fractions){auto found=std::find_if(fractions->begin(),fractions->end(),[&](const auto& f){return f.emitted==sister.id;});
   require(found!=fractions->end()&&found->mother==mother.id&&found->recoiler==recoiler.id,"missing or mismatched ISR source fractions");captured=&*found;
   // Canonical beam kinematics come from source x values. Historical momenta
   // can drift off their mass shell after highly boosted source operations.
   for(const auto* p:{&mother,&daughter,&recoiler,&old})require(p->mass==0&&std::hypot(p->momentum.px,p->momentum.py)<=1e-10*std::abs(p->momentum.pz),"ISR source fractions require massless longitudinal incoming records");
  }else require(collinear(mother)&&collinear(daughter)&&collinear(recoiler)&&collinear(old),"ISR frame requires massless collinear II incoming records");
  double beam_sign=side==0?1.:-1.;require(mother.momentum.pz*beam_sign>0&&daughter.momentum.pz*beam_sign>0&&recoiler.momentum.pz*beam_sign<0&&old.momentum.pz*beam_sign<0,"ISR frame beam directions disagree");
  double ea=side==0?daughter.momentum.energy:old.momentum.energy,eb=side==0?old.momentum.energy:daughter.momentum.energy;
  double ena=side==0?mother.momentum.energy:recoiler.momentum.energy,enb=side==0?recoiler.momentum.energy:mother.momentum.energy;
  double m2=4*ea*eb,q2=branch.virtuality_squared,z=daughter.momentum.energy/mother.momentum.energy;
  if(captured){ea=captured->before[0];eb=captured->before[1];ena=captured->after[0];enb=captured->after[1];
   m2=ea*eb*captured->collision_s;z=captured->before[side]/captured->after[side];}
  double m=std::sqrt(m2);
  require(z>0&&z<1,"invalid reconstructed ISR momentum fraction");double ms2=sister.mass*sister.mass;
  // For light-flavour/massless sisters the source uses Q2=pT2/(1-z). Recover pT from
  // the stored evolution scale instead of subtracting nearly equal squares.
  // Massive threshold branches have a different prescription; retain their
  // independently checked spacelike reconstruction until that mode is mapped.
  if(captured&&(sister.mass==0||std::abs(sister.pdg)<=3)){require(std::isfinite(sister.scale)&&sister.scale>0,"missing ISR evolution scale");q2=sister.scale*sister.scale/(1-z);}
  auto build_frame=[&](double q2,bool recoil_azimuth){
  double pt2=q2-z*(m2+q2)*(q2+ms2)/m2;require(pt2>0&&std::isfinite(pt2),"invalid reconstructed ISR transverse scale");
  double sign=side==0?1.:-1.,er=.5*(m2+q2)/m,pzr=-sign*er;
  double pt=std::sqrt(pt2)*m2/(z*(m2+q2)),pzm=sign*.5*m*((m2-q2)/(z*(m2+q2))+(q2+ms2)/m2);
  Pythia8::Vec4 pm(pt,0,pzm,std::sqrt(pt*pt+pzm*pzm)),pr(0,0,pzr,er),ps(pt,0,pzm+pzr,std::sqrt(pt*pt+(pzm+pzr)*(pzm+pzr)+ms2));
  auto total=pm+pr;
  auto from_rest=[&](double phi){Pythia8::RotBstMatrix matrix;matrix.bst(-total.px()/total.e(),0,-total.pz()/total.e());
   auto transformed=pm;transformed.rotbst(matrix);double theta=transformed.theta();if(transformed.px()<0)theta=-theta;if(side==1)theta+=pi;
   matrix.rot(-theta,phi);matrix.bst(0,0,(ena-enb)/(ena+enb));return matrix;};
  auto base=ps;base.rotbst(from_rest(0));
  double phi=std::atan2(sister.momentum.py,sister.momentum.px)-base.phi();
  if(recoil_azimuth){
   // An II recoil changes transverse momenta along the emission azimuth.
   // Use the largest displacement to avoid subtracting nearly equal vectors.
   double best=0,dx=0,dy=0;
   for(auto id=std::max(mother.id,recoiler.id)+1;id<sister.id;++id){
    const auto& after=at(id);
    require(std::abs(after.status)==44&&after.mothers.size()==1,"unsupported ISR final-state recoil record");
    const auto& before=at(after.mothers[0]);
    require(before.id<std::min(mother.id,recoiler.id)&&before.pdg==after.pdg,"invalid ISR recoil-copy provenance");
    double x=after.momentum.px-before.momentum.px,y=after.momentum.py-before.momentum.py;
    if(std::hypot(x,y)>best){best=std::hypot(x,y);dx=x;dy=y;}
   }
   require(best>0,"cannot resolve ISR recoil azimuth");
   if(dx*sister.momentum.px+dy*sister.momentum.py<0){dx=-dx;dy=-dy;}
   phi=std::atan2(dy,dx)-base.phi();
  }
  if(captured){phi=std::fmod(phi,2*pi);if(phi<0)phi+=2*pi;} // Source samples phi in [0,2pi).
  Pythia8::RotBstMatrix matrix;matrix.bst(0,0,(eb-ea)/(ea+eb));matrix.rot(0,-phi);matrix.rotbst(from_rest(phi));
  IsrRecoilFrame frame;frame.emitted=sister.id;frame.mother=mother.id;frame.recoiler=recoiler.id;frame.old_recoiler=old.id;
  for(int i=0;i<4;++i)for(int j=0;j<4;++j){frame.matrix[4*i+j]=matrix.value(i,j);require(std::isfinite(frame.matrix[4*i+j]),"nonfinite reconstructed ISR matrix");}
  for(auto id=std::max(mother.id,recoiler.id)+1;id<sister.id;++id){const auto& after=at(id);
   require(std::abs(after.status)==44&&after.mothers.size()==1,"unsupported ISR final-state recoil record");const auto& before=at(after.mothers[0]);
   require(before.id<std::min(mother.id,recoiler.id)&&before.pdg==after.pdg,"invalid ISR recoil-copy provenance");
   auto predicted=apply_isr_frame(frame,before.momentum);const auto& actual=after.momentum;
   for(auto pair:{std::array<double,2>{predicted.px,actual.px},{predicted.py,actual.py},{predicted.pz,actual.pz},{predicted.energy,actual.energy}}){
    double error=std::abs(pair[0]-pair[1]);frame.maximum_copy_error=std::max(frame.maximum_copy_error,error);
    if(error>2e-9+2e-11*std::abs(pair[1])){std::ostringstream message;message<<std::setprecision(17)
     <<"reconstructed ISR frame disagrees with recorded recoil copy: emitted "<<sister.id<<" before "<<before.id<<" after "<<after.id<<" predicted "<<pair[0]<<" actual "<<pair[1]<<" error "<<error;throw std::domain_error(message.str());}}
   frame.copies.push_back({before.id,after.id});
  }
  require(frame.copies.size()>=2,"missing ISR final-state recoil copies");return frame;
  };
  try{frames.push_back(build_frame(q2,false));}
  catch(const std::domain_error&){
   // sqrt(pT2) in the event record is not a lossless encoding of source pT2.
   // A bounded inverse-rounding search is allowed only for the mapped light
   // II branch, after ordinary reconstruction fails. Every candidate must
   // reproduce ALL recoil copies with the unchanged component tolerance.
   if(!captured||!(sister.mass==0||std::abs(sister.pdg)<=3))throw;
   double squared=sister.scale*sister.scale;
   std::array<double,5> candidates{squared,std::nextafter(squared,INFINITY),std::nextafter(squared,0.),
    std::nextafter(std::nextafter(squared,INFINITY),INFINITY),std::nextafter(std::nextafter(squared,0.),0.)};
   bool recovered=false;
   for(double candidate:candidates){
    if(!(candidate>0)||!std::isfinite(candidate)||std::sqrt(candidate)!=sister.scale)continue;
    try{auto frame=build_frame(candidate/(1-z),true);frame.source_rounding_recovered=true;frame.recovered_scale_squared=candidate;
     frames.push_back(frame);recovered=true;break;
    }catch(const std::domain_error&){}
   }
   if(!recovered)throw;
  }
 }
 std::sort(frames.begin(),frames.end(),[](const auto& a,const auto& b){return a.emitted<b.emitted;});return frames;
}
} // namespace
std::vector<IsrRecoilFrame> reconstruct_isr_recoil_frames(const HardEventSnapshot& input){
 return reconstruct_frames(input,map_pythia_isr_ancestry(input));
}
std::vector<std::vector<IsrRecoilFrame>> reconstruct_subsystem_isr_frames(const HardEventSnapshot& input){
 auto chains=map_subsystem_isr_ancestry(input);
 // Work with an owned source-stage view; retain the exported final record.
 auto staged=input;
 for(auto& node:staged.particles){require(node.energy_before_beam_repair.has_value(),"missing source-stage ISR recoil energy");node.momentum.energy=*node.energy_before_beam_repair;}
 std::vector<std::vector<IsrRecoilFrame>> result;
 for(const auto& pair:chains)result.push_back(reconstruct_frames(staged,pair,&input.isr_fractions));
 return result;
}

}
