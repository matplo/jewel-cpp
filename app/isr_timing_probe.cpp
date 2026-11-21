// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/pythia8_isr.hpp"
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
   out<<",\"fixture\":";
   if(!failure.empty()){out<<"null}\n";continue;}
   out<<fixture++<<",\"emissions\":[";bool first=true;
   for(int side=0;side<2;++side)for(std::size_t j=0;j<times[side].size();++j){const auto& t=times[side][j];if(!first)out<<',';first=false;
    out<<"{\"final_id\":"<<chains[side].emitted_final[j]<<",\"production\":["<<t.production.x<<','<<t.production.y<<','<<t.production.z<<','<<t.production.t<<"],\"end_time\":"<<t.end_time<<'}';}
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
