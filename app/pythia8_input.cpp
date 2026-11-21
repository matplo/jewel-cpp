// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/pythia8_input.hpp"
#include "jewel/pythia8_isr.hpp"
#include <filesystem>
#include "input_options.hpp"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
namespace {
template<class T>void list(std::ostream& out,const T& values){out<<'[';bool first=true;for(const auto& value:values){if(!first)out<<',';first=false;out<<value;}out<<']';}
unsigned long argument(const char* value){std::string text=value;std::size_t used=0;auto n=std::stoul(text,&used);if(used!=text.size()||text.empty()||text[0]=='-')throw std::invalid_argument("invalid integer argument");return n;}
}
int main(int argc,char** argv){
 try{
  if(argc<4){std::cerr<<"Usage: jewel-pythia8-input output.jsonl attempts seed [--isr on|off] [--mpi on|off] [--lhapdf SET/MEMBER --hard-alpha-s VALUE --hard-alpha-s-order ORDER]\nControlled Pythia8 hard input only; no JEWEL evolution or hadronization.\n";return 2;}
  auto count=argument(argv[2]),seed=argument(argv[3]);
  if(count==0||count>10000000||seed==0||seed>900000000)throw std::invalid_argument("invalid attempts or seed");
  if(std::filesystem::exists(argv[1]))throw std::runtime_error("refusing to overwrite output");
  jewel::PythiaInputConfig config;config.seed=static_cast<int>(seed);bool isr_seen=false,mpi_seen=false;
  for(int i=4;i<argc;++i){
   if(std::string(argv[i])=="--isr"){
    if(isr_seen||i+1>=argc)throw std::invalid_argument("--isr requires one on/off value");
    isr_seen=true;std::string value=argv[++i];
    if(value!="on"&&value!="off")throw std::invalid_argument("--isr requires on/off");
    config.initial_state_radiation=value=="on";
   }else if(std::string(argv[i])=="--mpi"){
    if(mpi_seen||i+1>=argc)throw std::invalid_argument("--mpi requires one on/off value");
    mpi_seen=true;std::string value=argv[++i];if(value!="on"&&value!="off")throw std::invalid_argument("--mpi requires on/off");config.multiple_parton_interactions=value=="on";
   }else if(!input_option(argc,argv,i,config))throw std::invalid_argument("unknown hard-input option");
  }
  jewel::Pythia8Input input(config);
  std::ofstream out(argv[1]);if(!out)throw std::runtime_error("cannot create output");out<<std::setprecision(17)<<std::boolalpha;
  out<<"{\"type\":\"run\",\"schema\":\"jewel-hard-input-v1\",\"scope\":\"adapter-only; no JEWEL shower\",\"pythia_version\":"<<input.version()<<",\"momentum_unit\":\"GeV\",\"vertex_unit\":\"mm\",\"attempts_requested\":"<<count<<",\"seed\":"<<seed<<",\"settings\":[";
  bool first=true;for(const auto& setting:input.settings()){if(!first)out<<',';first=false;out<<std::quoted(setting);}out<<"]}\n";
  for(unsigned long i=0;i<count;++i){auto event=input.next();
   if(!event){out<<"{\"type\":\"generation_failure\",\"attempt\":"<<i+1<<"}\n";continue;}
   const auto& e=*event;
   out<<"{\"type\":\"event\",\"attempt\":"<<e.attempt<<",\"process_code\":"<<e.process_code<<",\"weight\":"<<e.weight<<",\"pt_hat\":"<<e.pt_hat<<",\"x1\":"<<e.x1<<",\"x2\":"<<e.x2<<",\"incoming_id1\":"<<e.incoming_id1<<",\"incoming_id2\":"<<e.incoming_id2<<",\"q_factorization\":"<<e.q_factorization<<",\"q_renormalization\":"<<e.q_renormalization<<",\"hard_incoming\":";list(out,e.hard_incoming);out<<",\"hard_outgoing\":";list(out,e.hard_outgoing);out<<",\"hard_final\":";list(out,e.hard_final);
   out<<",\"initial_state_radiation\":"<<e.initial_state_radiation<<",\"isr_emissions\":";list(out,e.isr_emissions);out<<",\"isr_final\":";list(out,e.isr_final);
   out<<",\"multiple_parton_interactions\":"<<e.multiple_parton_interactions<<",\"subsystems\":[";bool first_system=true;
   for(const auto& system:e.subsystems){if(!first_system)out<<',';first_system=false;
    out<<"{\"index\":"<<system.index<<",\"shat\":"<<system.shat<<",\"pt_hat\":"<<system.pt_hat<<",\"incoming\":";list(out,system.incoming);
    out<<",\"outgoing\":";list(out,system.outgoing);out<<",\"outgoing_final\":";list(out,system.outgoing_final);out<<",\"process_outgoing\":";list(out,system.process_outgoing);out<<",\"primordial_targets_px_py_pz_energy\":";
    if(!system.primordial_targets)out<<"null";else{out<<'[';bool first_target=true;for(const auto& target:*system.primordial_targets){if(!first_target)out<<',';first_target=false;list(out,std::array<double,4>{target.px,target.py,target.pz,target.energy});}out<<']';}out<<'}';}
   out<<']';
   if(e.multiple_parton_interactions){out<<",\"isr_chains\":null,\"isr_recoil_frames\":null,\"primordial_recoil_frame\":null";}
   else {
   out<<",\"isr_chains\":[";
   if(e.initial_state_radiation){auto chains=jewel::map_pythia_isr_ancestry(e);bool first_chain=true;
    for(const auto& chain:chains){if(!first_chain)out<<',';first_chain=false;
     out<<"{\"hard_incoming\":"<<chain.hard_incoming<<",\"beam\":"<<chain.beam<<",\"backward_lines\":";list(out,chain.backward_lines);out<<",\"branches\":[";bool first_branch=true;
     for(const auto& b:chain.branches){if(!first_branch)out<<',';first_branch=false;
      out<<"{\"mother\":"<<b.mother<<",\"daughter\":"<<b.daughter<<",\"emitted\":"<<b.emitted<<",\"emitted_final\":"<<b.emitted_final<<",\"virtuality_squared\":"<<b.virtuality_squared<<",\"spacelike_daughter\":";
      list(out,std::array<double,4>{b.spacelike_daughter.px,b.spacelike_daughter.py,b.spacelike_daughter.pz,b.spacelike_daughter.energy});out<<'}';}
     out<<"]}";}
   }out<<']';
   out<<",\"isr_recoil_frames\":[";
   if(e.initial_state_radiation){bool first_frame=true;for(const auto& f:jewel::reconstruct_isr_recoil_frames(e)){
    if(!first_frame)out<<',';first_frame=false;
    out<<"{\"emitted\":"<<f.emitted<<",\"mother\":"<<f.mother<<",\"recoiler\":"<<f.recoiler<<",\"old_recoiler\":"<<f.old_recoiler<<",\"maximum_copy_error_gev\":"<<f.maximum_copy_error<<",\"matrix_e_xyz\":";list(out,f.matrix);
    out<<",\"copies\":[";bool first_copy=true;for(const auto& pair:f.copies){if(!first_copy)out<<',';first_copy=false;list(out,pair);}out<<"]}";
   }}out<<']';
   {auto f=jewel::reconstruct_primordial_recoil_frame(e);
    out<<",\"primordial_recoil_frame\":{\"incoming_before\":";list(out,f.incoming_before);out<<",\"incoming_after\":";list(out,f.incoming_after);
    out<<",\"maximum_copy_error_gev\":"<<f.maximum_copy_error<<",\"matrix_e_xyz\":";list(out,f.matrix);out<<",\"copies\":[";
    bool first_copy=true;for(const auto& pair:f.copies){if(!first_copy)out<<',';first_copy=false;list(out,pair);}out<<"]}";}
   }
   out<<",\"subsystem_isr_chains\":[";
   {bool first_system=true;for(const auto& chains:jewel::map_subsystem_isr_ancestry(e)){
    if(!first_system)out<<',';first_system=false;out<<'[';bool first_chain=true;
    for(const auto& chain:chains){if(!first_chain)out<<',';first_chain=false;
     out<<"{\"momentum_stage\":\"before_beam_energy_repair\",\"hard_incoming\":"<<chain.hard_incoming<<",\"beam\":"<<chain.beam<<",\"backward_lines\":";list(out,chain.backward_lines);out<<",\"branches\":[";bool first_branch=true;
     for(const auto& b:chain.branches){if(!first_branch)out<<',';first_branch=false;
      out<<"{\"mother\":"<<b.mother<<",\"daughter\":"<<b.daughter<<",\"emitted\":"<<b.emitted<<",\"emitted_final\":"<<b.emitted_final<<",\"virtuality_squared\":"<<b.virtuality_squared<<",\"spacelike_daughter\":";
      list(out,std::array<double,4>{b.spacelike_daughter.px,b.spacelike_daughter.py,b.spacelike_daughter.pz,b.spacelike_daughter.energy});out<<'}';}
     out<<"]}";
    }out<<']';
   }}out<<']';
   std::vector<jewel::PrimordialRecoilFrame> subsystem_frames;std::string subsystem_error;
   try{subsystem_frames=jewel::reconstruct_subsystem_primordial_frames(e);}catch(const std::domain_error& error){subsystem_error=error.what();}
   out<<",\"primordial_subsystem_error\":";if(subsystem_error.empty())out<<"null";else out<<std::quoted(subsystem_error);
   out<<",\"primordial_subsystem_frames\":";
   if(!subsystem_error.empty())out<<"null";else {out<<'[';bool first_frame=true;for(const auto& f:subsystem_frames){
    if(!first_frame)out<<',';first_frame=false;
    out<<"{\"momentum_stage\":\"before_beam_energy_repair\",\"incoming_before\":";list(out,f.incoming_before);out<<",\"incoming_after\":";list(out,f.incoming_after);
    out<<",\"maximum_copy_error_gev\":"<<f.maximum_copy_error<<",\"matrix_e_xyz\":";list(out,f.matrix);out<<",\"copies\":[";
    bool first_copy=true;for(const auto& pair:f.copies){if(!first_copy)out<<',';first_copy=false;list(out,pair);}out<<"]}";
   }out<<']';}
   out<<",\"particles\":[";first=true;
   for(const auto& p:e.particles){if(!first)out<<',';first=false;
    out<<"{\"id\":"<<p.id<<",\"source_index\":"<<p.source_index<<",\"pdg\":"<<p.pdg<<",\"status\":"<<p.status<<",\"final\":"<<p.final<<",\"colour\":"<<p.colour<<",\"anticolour\":"<<p.anticolour<<",\"momentum\":";list(out,std::array<double,4>{p.momentum.px,p.momentum.py,p.momentum.pz,p.momentum.energy});
    out<<",\"energy_before_beam_repair\":";if(p.energy_before_beam_repair)out<<*p.energy_before_beam_repair;else out<<"null";
    out<<",\"mass\":"<<p.mass<<",\"scale\":"<<p.scale<<",\"vertex_mm\":";list(out,p.vertex_mm);out<<",\"mothers\":";list(out,p.mothers);out<<",\"daughters\":";list(out,p.daughters);out<<'}';
   }
   out<<"],\"junctions\":[";first=true;for(const auto& j:e.junctions){if(!first)out<<',';first=false;out<<"{\"kind\":"<<j.kind<<",\"colours\":";list(out,j.colours);out<<'}';}out<<"]}\n";
  }
  auto s=input.statistics();out<<"{\"type\":\"summary\",\"attempts\":"<<s.attempts<<",\"returned\":"<<s.returned<<",\"failed\":"<<s.failed<<",\"pythia_accepted\":"<<s.pythia_accepted<<",\"sigma_mb\":"<<s.sigma_mb<<",\"sigma_error_mb\":"<<s.sigma_error_mb<<",\"weight_sum\":"<<s.weight_sum<<"}\n";
  out.close();if(!out)throw std::runtime_error("failed to finish output");
  std::cout<<"Saved "<<s.returned<<" hard snapshots from "<<s.attempts<<" attempts to "<<argv[1]<<'\n';return 0;
 }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
