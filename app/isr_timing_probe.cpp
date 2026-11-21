// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/pythia8_isr.hpp"
#include "jewel/pythia8_shower_input.hpp"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <filesystem>
namespace {
void record(std::ostream& out,int parent,const jewel::IsrKinematics& k){auto p=k.momentum;out<<parent<<' '<<p.px<<' '<<p.py<<' '<<p.pz<<' '<<p.energy<<' '<<k.record_mass<<' '<<k.nominal_mass<<'\n';}
}
int main(int argc,char** argv){try{
 if(argc!=6)throw std::runtime_error("usage: jewel-isr-timing OUTPUT FIXTURES COUNT SEED MPI(0|1)");
 int count=std::stoi(argv[3]),seed=std::stoi(argv[4]),mpi=std::stoi(argv[5]);
 if(count<=0||(mpi!=0&&mpi!=1))throw std::runtime_error("invalid probe arguments");
 if(std::filesystem::exists(argv[1])||std::filesystem::exists(argv[2]))throw std::runtime_error("refusing existing probe outputs");
 std::ofstream out(argv[1]),fixtures(argv[2]);if(!out||!fixtures)throw std::runtime_error("cannot open probe outputs");
 out<<std::setprecision(17);fixtures<<std::setprecision(17);
 jewel::PythiaInputConfig config;config.seed=seed;config.initial_state_radiation=true;config.multiple_parton_interactions=mpi;
 jewel::Pythia8Input generator(config);unsigned fixture=0;
 out<<"{\"type\":\"header\",\"scope\":\"candidate ISR timing only; no native evolution\",\"seed\":"<<seed<<",\"mpi\":"<<mpi<<",\"attempts\":"<<count<<",\"nominal_mass_policy\":\"Pythia8 particleData.m0\",\"settings\":[";
 bool separator=false;for(const auto& s:generator.settings()){if(separator)out<<',';separator=true;out<<std::quoted(s);}out<<"]}\n";
 for(int attempt=1;attempt<=count;++attempt){auto e=generator.next();
  if(!e){out<<"{\"type\":\"generation_failure\",\"attempt\":"<<attempt<<"}\n";continue;}
  std::vector<std::array<jewel::IsrTimingChain,2>> systems;
  try{systems=jewel::prepare_subsystem_isr_timing(*e,[&](int pdg){return generator.nominal_mass(pdg);});}
  catch(const std::domain_error& error){out<<"{\"type\":\"mapping_failure\",\"attempt\":"<<attempt<<",\"reason\":"<<std::quoted(error.what())<<"}\n";continue;}
  std::vector<jewel::ParticleId> import_order;
  for(auto it=e->particles.rbegin();it!=e->particles.rend();++it)if(it->final)import_order.push_back(it->id);
  auto imported=jewel::import_pythia_final_record(*e,1.2,-.7,20,import_order);
  auto& native=imported.state;const auto& sources=imported.input_sources;
  std::map<jewel::ParticleId,jewel::ParticleId> native_for;
  for(const auto& [id,source]:sources){native_for.emplace(source,id);auto& node=native.at(id);node.particle.production={7,8,9,10};node.decay_time=42;}
  std::array<std::size_t,4> role_counts{};for(const auto& entry:imported.roles)++role_counts[static_cast<std::size_t>(entry.second)];
  std::size_t count_draws=2*systems.size();for(const auto& pair:systems)for(const auto& c:pair)count_draws+=2*c.steps.size();
  jewel::ReplayRandom attach_random(std::vector<double>(count_draws,.5));jewel::IsrTimeConfig attach_config;attach_config.x=1.2;attach_config.y=-.7;attach_config.lifetime=20;
  auto bindings=jewel::attach_pythia_isr_times(native,sources,*e,systems,attach_config,attach_random);
  std::map<jewel::ParticleId,bool> attached;for(const auto& binding:bindings)attached.emplace(binding.source,true);
  if(bindings.size()!=e->isr_final.size())throw std::runtime_error("incomplete native ISR attachment");
  for(const auto& [id,source]:sources){const auto& node=native.at(id);const auto& original=e->particles[source-1];
   if(node.particle.status!=4||node.record_mass!=original.mass||node.evolution_time!=-1e6)throw std::runtime_error("attachment changed shower metadata");
   if(!attached.count(source)&&(node.particle.production.x!=7||node.particle.production.y!=8||node.particle.production.z!=9||node.particle.production.t!=10||node.decay_time!=42))throw std::runtime_error("attachment changed an unrelated record");
  }
  for(std::size_t system=0;system<systems.size();++system){const auto& chains=systems[system];
   jewel::IsrTimeConfig timing;timing.x=1.2;timing.y=-.7;timing.lifetime=20;
   std::size_t branches=chains[0].steps.size()+chains[1].steps.size();std::vector<double> tape(2+2*branches,.5);jewel::ReplayRandom rng(tape);
   std::array<std::vector<jewel::IsrTimedLine>,2> times;std::string failure;
   try{for(int side=0;side<2;++side)times[side]=jewel::assign_isr_emission_times(chains[side].hard,chains[side].steps,timing,rng);}
   catch(const std::domain_error& error){failure=error.what();}
   unsigned negative=0,zero_mass_offshell=0;double closure=0;
   for(const auto& chain:chains){negative+=chain.hard.momentum.energy<0;closure=std::max(closure,chain.maximum_emission_copy_error);
    for(const auto& step:chain.steps){negative+=step.beam_side==0&&step.incoming.momentum.energy<0;zero_mass_offshell+=step.emitted.record_mass==0&&step.emitted.nominal_mass!=0;}}
   out<<"{\"type\":\"system\",\"attempt\":"<<attempt<<",\"subsystem\":"<<system<<",\"branches\":"<<branches<<",\"negative_incoming_energies\":"<<negative<<",\"zero_mass_offshell_emissions\":"<<zero_mass_offshell<<",\"maximum_emission_copy_error_gev\":"<<closure<<",\"draws\":"<<rng.checkpoint()<<",\"timing_error\":";
   if(failure.empty())out<<"null";else out<<std::quoted(failure);
   out<<",\"import_role_counts\":["<<role_counts[0]<<','<<role_counts[1]<<','<<role_counts[2]<<','<<role_counts[3]<<"],\"native_weight\":"<<native.weight<<",\"native_anchor_count\":"<<1+imported.subsystems.size()<<",\"source_hard_pair\":["<<imported.subsystems[system].source_hard[0]<<','<<imported.subsystems[system].source_hard[1]<<"],\"native_hard_pair\":["<<imported.subsystems[system].hard[0]<<','<<imported.subsystems[system].hard[1]<<"],\"fixture\":";
   if(!failure.empty()){out<<"null}\n";continue;}
   out<<fixture++<<",\"emissions\":[";bool first=true;
   for(int side=0;side<2;++side)for(std::size_t j=0;j<times[side].size();++j){const auto& t=times[side][j];if(!first)out<<',';first=false;
    auto native_id=native_for.at(chains[side].emitted_final[j]);const auto& node=native.at(native_id);
    if(node.particle.production.x!=t.production.x||node.particle.production.y!=t.production.y||node.particle.production.z!=t.production.z||node.particle.production.t!=t.production.t||node.decay_time!=t.end_time)throw std::runtime_error("native ISR timing differs from replay");
    out<<"{\"final_id\":"<<chains[side].emitted_final[j]<<",\"native_id\":"<<native_id<<",\"production\":["<<t.production.x<<','<<t.production.y<<','<<t.production.z<<','<<t.production.t<<"],\"end_time\":"<<t.end_time<<'}';}
   out<<"]}\n";
   int next=107;std::array<int,2> roots{};
   for(int side=0;side<2;++side){roots[side]=chains[side].steps.empty()?3+side:next;next+=2*chains[side].steps.size();}
   fixtures<<next-1<<" 1 1.2 -.7 20 1 "<<tape.size()<<'\n';
   for(int side=0;side<2;++side)record(fixtures,roots[side],chains[side].hard);
   next=107;
   for(int side=0;side<2;++side)for(const auto& step:chains[side].steps){record(fixtures,step.beam_side?3+side:next+2,step.incoming);record(fixtures,next,step.emitted);next+=2;}
   for(double u:tape)fixtures<<u<<' ';fixtures<<'\n';
  }
 }
 out<<"{\"type\":\"summary\",\"fixtures\":"<<fixture<<"}\n";
 out.close();fixtures.close();if(!out||!fixtures)throw std::runtime_error("failed to finish probe output");
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
