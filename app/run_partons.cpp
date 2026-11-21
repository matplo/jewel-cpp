// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/pythia8_shower_input.hpp"
#include "jewel/cascade.hpp"
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
namespace {
void momentum(std::ostream& out,jewel::Momentum p){out<<'['<<p.px<<','<<p.py<<','<<p.pz<<','<<p.energy<<']';}
void particle(std::ostream& out,const jewel::ShowerParticle& s){const auto& p=s.particle;
 out<<"{\"id\":"<<p.id<<",\"pdg\":"<<p.pdg<<",\"status\":"<<p.status<<",\"parent\":"<<s.parent<<",\"coherent_partner\":"<<s.coherent_partner<<",\"collision_source\":"<<s.collision_source<<",\"colour\":"<<p.colour<<",\"anticolour\":"<<p.anticolour<<",\"momentum\":";momentum(out,p.momentum);
 out<<",\"mass\":"<<s.record_mass<<",\"production_fm\":["<<p.production.x<<','<<p.production.y<<','<<p.production.z<<','<<p.production.t<<"],\"decay_time_fm\":"<<s.decay_time<<",\"evolution_time_fm\":"<<s.evolution_time<<",\"daughters\":["<<s.daughters[0]<<','<<s.daughters[1]<<"]}";
}
// A probe fixture captures the exact initialized state and RNG cursor. Its
// source-index links are valid before any native compression or rollback.
void replay_fixture(const std::filesystem::path& path,const jewel::PreparedDijet& prepared,
 double lifetime,double cross,std::uint64_t seed,std::uint64_t cursor){
 std::ofstream out(path);if(!out)throw std::runtime_error("cannot create replay fixture");out<<std::setprecision(17);
 out<<"CONFIG\n208 3 1 .4 .59 .17 .17 .54 6.199999809265137 0 2 .45 1\n"
    <<"FCONFIG\n3 1 .2 100 1 0\nRCONFIG\n3 1 .2 100 1 0\nXCONFIG\n3 1 .2 .2 100 1 0\n"
    <<"PCONFIG\n3 1 .2 100 1 0\nQCONFIG\n3 .2 .2 0\nBCONFIG\n3 1 .2 1 0\n"
    <<"ECONFIG\n1 1 1 "<<lifetime<<' '<<cross<<"\nNCASCADE\n"
    <<prepared.hard[0]<<" 1 0 0 0 0 1 1 "<<prepared.state.colour_max<<' '<<prepared.weight<<" 0\n0 1\n"
    <<prepared.state.particles().size()<<'\n';
 for(const auto& s:prepared.state.particles()){const auto& p=s.particle;
  out<<p.status<<' '<<p.pdg<<' '<<s.parent<<' '<<s.daughters[0]<<' '<<s.daughters[1]<<' '
   <<p.momentum.px<<' '<<p.momentum.py<<' '<<p.momentum.pz<<' '<<p.momentum.energy<<' '<<s.record_mass<<' '
   <<s.generator_vertex.x<<' '<<s.generator_vertex.y<<' '<<s.generator_vertex.z<<' '<<s.generator_vertex.t<<' '
   <<p.production.x<<' '<<p.production.y<<' '<<p.production.z<<' '<<p.production.t<<' '<<s.decay_time<<' '
   <<s.fraction<<' '<<s.angle<<' '<<s.quark_pair<<' '<<s.angular_required<<' '<<p.colour<<' '<<p.anticolour<<' '<<s.evolution_time<<'\n';
 }
 out<<"1 0 0 1 0 0 1 0 23000\n-1\n"<<seed<<'\n'<<cursor<<'\n';
 out.close();if(!out)throw std::runtime_error("cannot finish replay fixture");
}
unsigned long integer(const char* value){std::string s=value;std::size_t end=0;auto n=std::stoul(s,&end);if(s.empty()||s[0]=='-'||end!=s.size())throw std::invalid_argument("invalid integer argument");return n;}
}
int main(int argc,char** argv){
 try{
  if(argc!=5&&argc!=6){std::cerr<<"Usage: jewel-run-partons output.jsonl attempts seed vacuum|medium [new-replay-directory]\nDevelopment PPJJ runner: ISR/MPI off, no hadronization.\n";return 2;}
  auto attempts=integer(argv[2]),seed=integer(argv[3]);std::string mode=argv[4];
  if(!attempts||attempts>1000000||!seed||seed>900000000||(mode!="vacuum"&&mode!="medium"))throw std::invalid_argument("invalid runner arguments");
  if(std::filesystem::exists(argv[1]))throw std::runtime_error("refusing to overwrite output");
  std::filesystem::path replay_directory;
  if(argc==6){replay_directory=argv[5];if(!std::filesystem::create_directory(replay_directory))throw std::runtime_error("replay directory must be new");}
  jewel::PythiaInputConfig input_config;input_config.seed=static_cast<int>(seed);jewel::Pythia8Input input(input_config);
  jewel::Kernels kernels;jewel::MediumConfig medium_config;medium_config.max_rapidity=2;jewel::ExpandingMedium medium(medium_config);
  auto maximum=medium.maximum_scatterer(false);double lifetime=medium.bounds().lifetime_max;
  jewel::InitialSudakovTable sudakov(kernels,1,100,1,maximum);jewel::FinalSplittingTable splitting(kernels,1,100,1,maximum);
  jewel::PartonicPdf pdf(kernels,1,100,1,maximum);jewel::ScatteringRates rates(kernels,1,100,1,medium,false);
  jewel::TransferSampler transfers(kernels,medium,false);jewel::BranchingSampler branching(kernels,1,1,0);
  jewel::InitialEvolutionConfig config;config.evolution.lifetime=lifetime;config.evolution.cross_section_scale=mode=="medium"?1:0;
  config.coherent.lifetime=lifetime;config.coherent.cross_section_scale=config.evolution.cross_section_scale;
  config.coherent.finalization.scatter_recoils=false;config.coherent.finalization.storage={0,true,false};
  jewel::InitialScatteringEvolution initial(config,kernels,branching,sudakov,splitting,pdf,rates,transfers,medium);
  jewel::ScatteringEvolution final(config.evolution,branching,sudakov,splitting,rates,transfers);
  jewel::CascadeConfig cascade_config{config.evolution,config.coherent.collision,config.coherent.finalization,1000000,true};
  jewel::Cascade cascade(cascade_config,initial,final,branching,medium);
  std::ofstream out(argv[1]);if(!out)throw std::runtime_error("cannot create output");out<<std::setprecision(17);
  out<<"{\"type\":\"run\",\"schema\":\"jewel-parton-development-v1\",\"scope\":\"reduced PPJJ; no ISR/MPI or hadronization\",\"mode\":"<<std::quoted(mode)<<",\"attempts_requested\":"<<attempts<<",\"seed\":"<<seed<<",\"pythia_version\":"<<input.version()<<",\"lifetime_fm\":"<<lifetime<<",\"impact_parameter_fm\":0,\"angular_ordering\":0,\"coherence\":true,\"recoil_rescattering\":false,\"longitudinal_scatterer_boost\":false,\"compression\":true,\"pythia_settings\":[";
  bool comma=false;for(const auto& s:input.settings()){if(comma)out<<',';comma=true;out<<std::quoted(s);}out<<"]}\n";
  std::uint64_t accepted=0,failed=0,generated_failed=0,final_count=0;double accepted_weight=0,failed_weight=0;
  for(unsigned long attempt=1;attempt<=attempts;++attempt){auto hard=input.next();
   if(!hard){++generated_failed;out<<"{\"type\":\"generation_failure\",\"attempt\":"<<attempt<<"}\n";continue;}
   const auto shower_seed=static_cast<std::uint64_t>(seed)+1000000+100*attempt;
   jewel::SplitMixRandom random(shower_seed);std::string stage="geometry";
   try{
    auto vertex=medium.sample_vertex(random);stage="hard initialization";
    jewel::HardInitializationConfig hard_config;hard_config.x=vertex.x;hard_config.y=vertex.y;hard_config.lifetime=lifetime;
    auto prepared=jewel::prepare_pythia_dijet(*hard,hard_config,branching,random);auto initial_hard=std::array<jewel::ShowerParticle,2>{prepared.state.at(prepared.hard[0]),prepared.state.at(prepared.hard[1])};
    if(!replay_directory.empty()){stage="replay export";replay_fixture(replay_directory/(std::to_string(attempt)+".input"),prepared,lifetime,config.evolution.cross_section_scale,shower_seed,random.checkpoint());}
    jewel::TransferHistory history;history.records.resize(100);jewel::RecoilStorage ledger;stage="cascade";
    auto evolved=cascade.evolve(prepared.state,prepared.scratch,history,ledger,random);prepared.state.validate();
    if(prepared.state.discarded)throw std::runtime_error("cascade discarded event");
    for(const auto& s:prepared.state.particles())if(s.particle.status==1||s.particle.status==2)throw std::runtime_error("unfinished cascade record");
    ++accepted;accepted_weight+=hard->weight;
    out<<"{\"type\":\"event\",\"attempt\":"<<attempt<<",\"weight\":"<<hard->weight<<",\"pt_hat\":"<<hard->pt_hat<<",\"shower_seed\":"<<shower_seed<<",\"draws\":"<<random.checkpoint()<<",\"branch_calls\":"<<evolved.branch_calls<<",\"initial_hard\":[";
    particle(out,initial_hard[0]);out<<',';particle(out,initial_hard[1]);out<<"],\"input_sources\":[";comma=false;
    for(const auto& entry:prepared.input_sources){if(comma)out<<',';comma=true;out<<'['<<entry.first<<','<<entry.second<<']';}
    out<<"],\"particles\":[";comma=false;
    for(const auto& p:prepared.state.particles()){if(comma)out<<',';comma=true;particle(out,p);if(p.particle.status==3||p.particle.status==4||p.particle.status==5)++final_count;}
    out<<"],\"archived\":[";comma=false;for(const auto& entry:prepared.state.archived()){if(comma)out<<',';comma=true;particle(out,entry.second);}
    out<<"],\"thermal_references\":[";
    for(std::size_t i=0;i<ledger.used;++i){if(i)out<<',';const auto& r=ledger.records.at(i);out<<"{\"pdg\":"<<r.flavour<<",\"momentum\":";momentum(out,r.centre);out<<",\"dummy\":";momentum(out,r.dummy);out<<'}';}out<<"]}\n";
   }catch(const std::domain_error& error){++failed;failed_weight+=hard->weight;out<<"{\"type\":\"evolution_failure\",\"attempt\":"<<attempt<<",\"weight\":"<<hard->weight<<",\"stage\":"<<std::quoted(stage)<<",\"reason\":"<<std::quoted(error.what())<<",\"draws\":"<<random.checkpoint()<<"}\n";
   }catch(const std::runtime_error& error){if(stage=="replay export")throw;++failed;failed_weight+=hard->weight;out<<"{\"type\":\"evolution_failure\",\"attempt\":"<<attempt<<",\"weight\":"<<hard->weight<<",\"stage\":"<<std::quoted(stage)<<",\"reason\":"<<std::quoted(error.what())<<",\"draws\":"<<random.checkpoint()<<"}\n";}
  }
  auto stats=input.statistics();out<<"{\"type\":\"summary\",\"attempts\":"<<attempts<<",\"accepted\":"<<accepted<<",\"evolution_failures\":"<<failed<<",\"generation_failures\":"<<generated_failed<<",\"accepted_weight\":"<<accepted_weight<<",\"failed_weight\":"<<failed_weight<<",\"generator_weight_sum\":"<<stats.weight_sum<<",\"generator_sigma_mb\":"<<stats.sigma_mb<<",\"generator_sigma_error_mb\":"<<stats.sigma_error_mb<<",\"final_records\":"<<final_count<<"}\n";
  out.close();if(!out)throw std::runtime_error("failed to finish output");std::cout<<"Accepted "<<accepted<<" / "<<attempts<<" attempts; evolution failures "<<failed<<'\n';return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
