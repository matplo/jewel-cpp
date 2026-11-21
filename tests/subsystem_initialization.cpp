// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/pythia8_shower_input.hpp"
#include <iostream>
#include <stdexcept>
void check(bool ok){if(!ok)throw std::runtime_error("subsystem initialization contract failed");}
int main(){
 jewel::PythiaInputConfig cfg;cfg.seed=2600417;cfg.initial_state_radiation=true;cfg.multiple_parton_interactions=true;
 jewel::Pythia8Input generator(cfg);jewel::BranchingSampler sampler({},1,1,0);
 unsigned completed=0,mpi_pairs=0,mismatched=0;
 for(int attempt=0;attempt<20;++attempt){auto event=generator.next();check(bool(event));
  auto imported=jewel::import_pythia_final_record(*event,1.2,-.7,20);
  jewel::ReplayRandom unused({});bool rejected=false;jewel::HardInitializationConfig config;config.x=1.2;config.y=-.7;
  try{jewel::initialize_imported_subsystems(imported,config,sampler,unused);}catch(const std::domain_error&){rejected=true;}
  check(rejected&&unused.checkpoint()==0);
  jewel::prepare_imported_partons(imported,[&](int pdg){return generator.nominal_mass(pdg);},2);
  auto before=imported.state;auto native=imported;
  // Exhaust after a complete primary initialization, at the first MPI draw.
  const auto& primary=imported.subsystems[0];auto primary_state=before;
  auto primary_config=config;primary_config.pt_hat=primary.pt_hat;
  jewel::SplitMixRandom primary_random(2607000+attempt);
  jewel::initialize_hard_dijet(primary_state,primary.hard[0],primary.hard[1],primary_config,sampler,primary_random);
  if(imported.subsystems.size()>1){
   jewel::SplitMixRandom tape_source(2607000+attempt);std::vector<double> tape;
   for(std::size_t i=0;i<primary_random.checkpoint();++i)tape.push_back(tape_source.uniform());
   jewel::ReplayRandom exhausted(tape);rejected=false;
   try{jewel::initialize_imported_subsystems(imported,config,sampler,exhausted);}catch(const std::out_of_range&){rejected=true;}
   check(rejected&&exhausted.checkpoint()==tape.size());
   for(const auto& old:before.particles()){const auto& now=imported.state.at(old.particle.id);check(now.record_mass==old.record_mass&&now.particle.momentum.energy==old.particle.momentum.energy&&now.decay_time==old.decay_time);}
   auto unsupported=imported;unsupported.state.at(unsupported.subsystems[1].hard[0]).particle.pdg=22;
   rejected=false;
   try{jewel::initialize_imported_subsystems(unsupported,config,sampler,unused);}catch(const std::domain_error&){rejected=true;}
   check(rejected&&unused.checkpoint()==0);
  }
  auto manual=before;std::array<jewel::Channel,2> channels;
  for(int side=0;side<2;++side)channels[side]=manual.at(imported.subsystems[0].hard[side]).particle.pdg==21?jewel::Channel::GC:jewel::Channel::QQ;
  jewel::SplitMixRandom rng(2607000+attempt),replay(2607000+attempt);
  auto result=jewel::initialize_imported_subsystems(imported,config,sampler,rng);
  check(result.primary_sampling_channels==channels&&result.pairs.size()==imported.subsystems.size());
  config.pt_hat=imported.subsystems[0].pt_hat;config.mpi_sampling_channels=channels;
  for(std::size_t i=0;i<imported.subsystems.size();++i){const auto& pair=imported.subsystems[i];config.mpi_pair=i>0;
   auto expected=jewel::initialize_hard_dijet(manual,pair.hard[0],pair.hard[1],config,sampler,replay);
   check(expected.trials==result.pairs[i].trials&&expected.colour_angles==result.pairs[i].colour_angles);
   if(i){++mpi_pairs;for(int side=0;side<2;++side)mismatched+=(manual.at(pair.hard[side]).particle.pdg==21)!=(channels[side]==jewel::Channel::GC);}
  }
  check(rng.checkpoint()==replay.checkpoint());
  for(const auto& expected:manual.particles()){const auto& actual=imported.state.at(expected.particle.id);
   check(actual.record_mass==expected.record_mass&&actual.fraction==expected.fraction&&actual.angle==expected.angle&&actual.decay_time==expected.decay_time);
   auto p=actual.particle.momentum,q=expected.particle.momentum;check(p.px==q.px&&p.py==q.py&&p.pz==q.pz&&p.energy==q.energy);
   check(actual.scheduling_slots==expected.scheduling_slots&&actual.parent==expected.parent);
  }
  jewel::SplitMixRandom native_rng(2607000+attempt),native_replay(2607000+attempt);
  auto native_result=jewel::initialize_imported_subsystems(native,config,sampler,native_rng,jewel::MpiChannelPolicy::outgoing_flavour);
  auto native_manual=before;
  check(native_result.pairs.size()==native.subsystems.size());
  for(std::size_t i=0;i<native.subsystems.size();++i){const auto& pair=native.subsystems[i];
   config.mpi_pair=i>0;
   for(int side=0;side<2;++side)channels[side]=native_manual.at(pair.hard[side]).particle.pdg==21?jewel::Channel::GC:jewel::Channel::QQ;
   config.mpi_sampling_channels=channels;
   jewel::initialize_hard_dijet(native_manual,pair.hard[0],pair.hard[1],config,sampler,native_replay);
   for(auto id:pair.hard){const auto& actual=native.state.at(id);const auto& expected=native_manual.at(id);
    check(actual.particle.pdg==21||!actual.quark_pair);
    check(actual.record_mass==expected.record_mass&&actual.fraction==expected.fraction&&actual.quark_pair==expected.quark_pair&&actual.decay_time==expected.decay_time);
    auto p=actual.particle.momentum,q=expected.particle.momentum;
    check(p.px==q.px&&p.py==q.py&&p.pz==q.pz&&p.energy==q.energy);
   }
  }
  check(native_rng.checkpoint()==native_replay.checkpoint());
  ++completed;
 }
 check(completed==20&&mpi_pairs>0&&mismatched>0);
 std::cout<<"events "<<completed<<" MPI pairs "<<mpi_pairs<<" inherited channels differing from MPI flavour "<<mismatched<<'\n';
}
