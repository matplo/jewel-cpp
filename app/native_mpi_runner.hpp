#pragma once
#include "jewel/legacy_mpi_cross_section.hpp"
#include "jewel/kinematics.hpp"
#include "jewel/pythia8_shower_input.hpp"
#include <Pythia8/StandardModel.h>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace jewel {
class NativeMpiRunner {
 Pythia8::AlphaStrong alpha_;
 LegacyMpiScreeningConfig screening_;
 std::optional<LegacyMpiInitialization> initialized_;
 std::unique_ptr<LegacyMpiCrossSection> cross_section_;
public:
 explicit NativeMpiRunner(unsigned long seed){
  screening_={5020,2,1800,.25,45.733890062687706,100,{.36399305120539549,.37538934770218202,.37538934770218202}};
  LegacyMpiCrossSectionConfig c;
  c.coefficients={screening_.phase_coefficients[0],.63600694879460462,screening_.phase_coefficients[1],screening_.phase_coefficients[2],.24922130459563596};
  c.ecm=5020;c.gev2_to_mb=.38938;c.interference=1;
  c.masses={.33,.33,.5,1.5,4.8};c.enabled.fill(true);
  c.decays={{.33,.33},{.33,.33},{.5,.5},{1.5,1.5},{4.8,4.8},{175,175,.9999999999999998,0},{400,400,1,-1},{400,400,1,-1},{0,0}};
  alpha_.init(.11754434294459773,1,5,false);
  auto coupling=[this](double q2){return alpha_.alphaS(q2);};
  SplitMixRandom random(seed+700000);
  initialized_=initialize_legacy_mpi_with_pdfs(screening_,.5,.4,random,c,"CT14nlo",0,0.,coupling);
  c.screening_squared=initialized_->secondary.back().screening_squared;
  c.scale_shift=initialized_->secondary.back().scale_shift;
  cross_section_=std::make_unique<LegacyMpiCrossSection>(c,"CT14nlo",0,0.,coupling);
 }
 const LegacyMpiInitialization& initialization() const {return *initialized_;}
 unsigned add(ImportedPythiaRecord& event,const HardEventSnapshot& hard,SpaceTime vertex,double lifetime,Random& random,bool longitudinal_recoil=false){
  LegacyMpiEvolutionState state;
  std::vector<ParticleId> ids;
  std::vector<std::size_t> remnants;
  std::map<int,std::size_t> colours,anticolours;
  Momentum total;
  auto accumulate=[](Momentum& a,Momentum b,double sign=1.){
   a.px+=sign*b.px;a.py+=sign*b.py;a.pz+=sign*b.pz;a.energy+=sign*b.energy;
  };
  for(const auto& node:event.state.particles()){
   const auto& p=node.particle;
   const auto role=event.roles.find(p.id);
   if(role==event.roles.end())continue;
   const auto slot=ids.size();
   ids.push_back(p.id);state.momenta.push_back(p.momentum);state.pdgs.push_back(p.pdg);
   if(p.colour&&!colours.emplace(p.colour,slot).second)throw std::domain_error("duplicate MPI colour");
   if(p.anticolour&&!anticolours.emplace(p.anticolour,slot).second)throw std::domain_error("duplicate MPI anticolour");
   if(role->second==ImportedRole::remnant){remnants.push_back(slot);accumulate(total,p.momentum);}
  }
  std::vector<int> piece_tags;
  for(auto [tag,i]:colours)if(auto it=anticolours.find(tag);it!=anticolours.end()){
   state.strings.push_back({i,it->second});piece_tags.push_back(tag);
  }
  state.remaining_a=std::clamp((total.energy+total.pz)/screening_.ecm,0.,1.);
  state.remaining_b=std::clamp((total.energy-total.pz)/screening_.ecm,0.,1.);
  state.upper_xt=4*hard.pt_hat*hard.pt_hat/(screening_.ecm*screening_.ecm);
  const auto impact=initialized_->overlap.sample_hard(random);
  LegacyMpiEvolutionConfig config{screening_.ecm,initialized_->secondary.back().screening_squared,
   initialized_->overlap.state().overlap_average,impact.enhancement,1971.0931412839443,screening_.sigma_nd,
   screening_.phase_coefficients,.9,.95,double(.2f),2*std::acos(-1.)};
  LegacyMpiEvolution evolution(config,state);
  while(!evolution.state().stopped)evolution.step(random,std::cref(*cross_section_));
  const auto& result=evolution.state();
  if(!result.additional_pairs)return 0;

  // Recoil the outgoing remnant system as a whole, preserving each invariant
  // mass and the directions in its rest frame.
  Momentum target=total;
  for(std::size_t i=ids.size();i<result.momenta.size();++i)accumulate(target,result.momenta[i],-1);
  auto mass=[](Momentum p){return std::sqrt(std::max(0.,p.energy*p.energy-p.px*p.px-p.py*p.py-p.pz*p.pz));};
  auto beta=[](Momentum p,double sign){return std::array<double,3>{sign*p.px/p.energy,sign*p.py/p.energy,sign*p.pz/p.energy};};
  const double available=mass(target);
  if(remnants.size()<2||target.energy<=0||available<=0||mass(total)<=0)throw std::domain_error("insufficient MPI remnant recoil");
  std::vector<Momentum> rest;
  std::vector<double> masses;
  for(auto i:remnants){
   auto p=state.momenta[i];masses.push_back(mass(p));
   rotate_boost(p,nullptr,0,0,beta(total,-1));rest.push_back(p);
  }
  auto energy=[&](double scale){
   double sum=0;
   for(std::size_t i=0;i<rest.size();++i)sum+=longitudinal_recoil?std::hypot(std::hypot(masses[i],std::hypot(rest[i].px,rest[i].py)),scale*rest[i].pz):std::hypot(masses[i],scale*momentum_magnitude(rest[i]));
   return sum;
  };
  if(energy(0)>available||energy(1)<available)throw std::domain_error("MPI recoil outside remnant mass range");
  double low=0,high=1;
  for(int i=0;i<60;++i){double mid=(low+high)/2;if(energy(mid)>available)high=mid;else low=mid;}
  Momentum recoiled;
  for(std::size_t j=0;j<rest.size();++j){
   auto p=rest[j];if(!longitudinal_recoil){p.px*=low;p.py*=low;}p.pz*=low;p.energy=std::hypot(masses[j],momentum_magnitude(p));
   rotate_boost(p,nullptr,0,0,beta(target,1));accumulate(recoiled,p);
   event.state.at(ids[remnants[j]]).particle.momentum=p;
  }
  accumulate(recoiled,target,-1);
  if(std::max({std::abs(recoiled.energy),std::abs(recoiled.px),std::abs(recoiled.py),std::abs(recoiled.pz)})>1e-8*total.energy)
   throw std::domain_error("MPI remnant momentum does not close");
  const auto initial_size=ids.size();
  for(std::size_t i=initial_size;i<result.momenta.size();i+=2){
   ImportedSubsystem subsystem;subsystem.index=int(event.subsystems.size());
   subsystem.pt_hat=std::hypot(result.momenta[i].px,result.momenta[i].py);
   for(std::size_t side=0;side<2;++side){
    ShowerParticle leg;leg.particle.pdg=result.pdgs[i+side];leg.particle.status=4;
    leg.particle.momentum=result.momenta[i+side];leg.particle.production=vertex;leg.decay_time=lifetime;
    leg.scheduling_slots=std::array<std::size_t,3>{0,0,0};
    const auto id=event.state.append(leg,true,true);
    ids.push_back(id);subsystem.hard[side]=id;
    event.roles.emplace(id,ImportedRole::mpi_hard);
    event.input_sources.emplace(id,hard.particles.size()+i+side-initial_size+1);
   }
   event.subsystems.push_back(subsystem);
  }
  for(std::size_t i=0;i<result.strings.size();++i){
   const auto piece=result.strings[i];
   int tag=i<piece_tags.size()?piece_tags[i]:++event.state.colour_max;
   event.state.at(ids[piece.first]).particle.colour=tag;
   event.state.at(ids[piece.second]).particle.anticolour=tag;
  }
  event.state.validate();
  return result.additional_pairs;
 }
};
}
