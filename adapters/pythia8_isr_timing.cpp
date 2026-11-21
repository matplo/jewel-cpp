// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/pythia8_isr.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace jewel {
namespace {void require(bool b,const char* s){if(!b)throw std::domain_error(s);}}
std::vector<std::array<IsrTimingChain,2>> prepare_subsystem_isr_timing(
 const HardEventSnapshot& input,const std::function<double(int)>& nominal_mass){
 auto chains=map_subsystem_isr_ancestry(input);
 auto frames=reconstruct_subsystem_isr_frames(input);
 auto primordial=reconstruct_subsystem_primordial_frames(input);
 require(chains.size()==frames.size()&&chains.size()==primordial.size(),"incomplete ISR timing frames");
 auto at=[&](ParticleId id)->const GeneratorParticle&{require(id>0&&id<=input.particles.size(),"invalid ISR timing record");return input.particles[id-1];};
 auto staged=[&](ParticleId id){const auto& node=at(id);require(node.energy_before_beam_repair.has_value(),"missing ISR timing energy");auto p=node.momentum;p.energy=*node.energy_before_beam_repair;return p;};
 auto nominal=[&](int pdg){double mass=nominal_mass(pdg);require(std::isfinite(mass)&&mass>=0,"invalid ISR timing nominal mass");return mass;};
 std::vector<std::array<IsrTimingChain,2>> result(chains.size());
 for(std::size_t system=0;system<chains.size();++system){
  auto transport=[&](ParticleId emitted,Momentum p){
   require(std::any_of(frames[system].begin(),frames[system].end(),[&](const auto& f){return f.emitted==emitted;}),"missing ISR branch recoil frame");
   for(const auto& f:frames[system])if(f.emitted>emitted)p=apply_isr_frame(f,p);
   p=apply_primordial_frame(primordial[system],p);
   for(double v:{p.px,p.py,p.pz,p.energy})require(std::isfinite(v),"nonfinite ISR timing transport");return p;
  };
  for(std::size_t side=0;side<2;++side){const auto& chain=chains[system][side];auto& target=result[system][side];
   target.hard_incoming=chain.hard_incoming;target.beam=chain.beam;
   std::vector<IsrKinematics> daughters;
   for(const auto& branch:chain.branches){
    auto q=transport(branch.emitted,branch.spacelike_daughter);
    daughters.push_back({q,-std::sqrt(branch.virtuality_squared),nominal(at(branch.daughter).pdg)});
    auto predicted=transport(branch.emitted,staged(branch.emitted));auto actual=staged(branch.emitted_final);
    std::array<double,4> a{predicted.px,predicted.py,predicted.pz,predicted.energy},b{actual.px,actual.py,actual.pz,actual.energy};
    for(std::size_t i=0;i<4;++i){double error=std::abs(a[i]-b[i]);target.maximum_emission_copy_error=std::max(target.maximum_emission_copy_error,error);
     require(error<=1e-8+1e-10*std::abs(b[i]),"composed ISR emission transport exceeds existing closure gate");}
    target.emitted_final.push_back(branch.emitted_final);
   }
   if(daughters.empty()){
    // No emitted output; the timing projection consumes the source hard draw.
    const auto& hard=at(chain.hard_incoming);target.hard={staged(hard.id),0,nominal(hard.pdg)};
    continue;
   }
   target.hard=daughters.front();
   for(std::size_t i=0;i<chain.branches.size();++i){const auto& branch=chain.branches[i];const auto& final=at(branch.emitted_final);
    IsrTimeStep step;step.emitted={final.momentum,final.mass,nominal(final.pdg)};
    if(i+1<daughters.size())step.incoming=daughters[i+1];
    else{const auto& beam=at(chain.backward_lines.back());step.incoming={beam.momentum,beam.mass,nominal(beam.pdg)};step.beam_side=side==0?1:-1;}
    target.steps.push_back(step);
   }
  }
 }
 return result;
}
}
