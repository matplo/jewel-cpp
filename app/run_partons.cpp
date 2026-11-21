// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/pythia8_shower_input.hpp"
#include "jewel/cascade.hpp"
#include "input_options.hpp"
#include "jewel/pythia8_shower_output.hpp"
#include "jewel/pythia8_hadronizer.hpp"
#include <filesystem>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
namespace {
#ifdef JEWEL_HADRONIZE_RUN
constexpr bool hadronize_run=true;
#else
constexpr bool hadronize_run=false;
#endif
void momentum(std::ostream& out,jewel::Momentum p){out<<'['<<p.px<<','<<p.py<<','<<p.pz<<','<<p.energy<<']';}
void particle(std::ostream& out,const jewel::ShowerParticle& s){const auto& p=s.particle;
 out<<"{\"id\":"<<p.id<<",\"pdg\":"<<p.pdg<<",\"status\":"<<p.status<<",\"parent\":"<<s.parent<<",\"coherent_partner\":"<<s.coherent_partner<<",\"collision_source\":"<<s.collision_source<<",\"colour\":"<<p.colour<<",\"anticolour\":"<<p.anticolour<<",\"momentum\":";momentum(out,p.momentum);
 out<<",\"mass\":"<<s.record_mass<<",\"production_fm\":["<<p.production.x<<','<<p.production.y<<','<<p.production.z<<','<<p.production.t<<"],\"decay_time_fm\":"<<s.decay_time<<",\"evolution_time_fm\":"<<s.evolution_time<<",\"daughters\":["<<s.daughters[0]<<','<<s.daughters[1]<<"]}";
}
void generator_particle(std::ostream& out,const jewel::GeneratorParticle& p){
 out<<"{\"id\":"<<p.id<<",\"pdg\":"<<p.pdg<<",\"status\":"<<p.status<<",\"final\":"<<(p.final?"true":"false")
    <<",\"colour\":"<<p.colour<<",\"anticolour\":"<<p.anticolour<<",\"mass\":"<<p.mass<<",\"momentum\":";momentum(out,p.momentum);
 out<<",\"vertex_mm\":["<<p.vertex_mm[0]<<','<<p.vertex_mm[1]<<','<<p.vertex_mm[2]<<','<<p.vertex_mm[3]<<"],\"mothers\":[";
 for(std::size_t i=0;i<p.mothers.size();++i){if(i)out<<',';out<<p.mothers[i];}out<<"],\"daughters\":[";
 for(std::size_t i=0;i<p.daughters.size();++i){if(i)out<<',';out<<p.daughters[i];}out<<"]}";
}
// A probe fixture captures the exact initialized state and RNG cursor. Its
// source-index links are valid before any native compression or rollback.
void replay_fixture(const std::filesystem::path& path,const jewel::PreparedDijet& prepared,
 double lifetime,double cross,std::uint64_t seed,std::uint64_t cursor,double table_b,double event_b,double table_energy){
 std::ofstream out(path);if(!out)throw std::runtime_error("cannot create replay fixture");out<<std::setprecision(17);
 out<<"CONFIG\n208 3 1 .4 .59 .17 .17 .54 6.199999809265137 "<<table_b<<" 2 .45 1\n"
    <<"FCONFIG\n3 1 .2 "<<table_energy<<" 1 0\nRCONFIG\n3 1 .2 "<<table_energy<<" 1 0\nXCONFIG\n3 1 .2 .2 "<<table_energy<<" 1 0\n"
    <<"PCONFIG\n3 1 .2 "<<table_energy<<" 1 0\nQCONFIG\n3 .2 .2 0\nBCONFIG\n3 1 .2 1 0\n"
    <<"EVENTB\n"<<event_b<<"\nECONFIG\n1 1 1 "<<lifetime<<' '<<cross<<"\nNCASCADE\n"
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
  if(argc<5){std::cerr<<"Usage: "<<(hadronize_run?"jewel-run-events":"jewel-run-partons")<<" output.jsonl attempts seed vacuum|medium [new-replay-directory] [--centrality MIN MAX] [--table-energy GEV|jewel] [--jewel-fragmentation] [--lhapdf SET/MEMBER --hard-alpha-s VALUE --hard-alpha-s-order ORDER]\nDevelopment PPJJ runner: ISR/MPI off.\n";return 2;}
  auto attempts=integer(argv[2]),seed=integer(argv[3]);std::string mode=argv[4];
  if(!attempts||attempts>1000000||!seed||seed>900000000||(mode!="vacuum"&&mode!="medium"))throw std::invalid_argument("invalid runner arguments");
  if(std::filesystem::exists(argv[1]))throw std::runtime_error("refusing to overwrite output");
  std::filesystem::path replay_directory;
  jewel::PythiaInputConfig input_config;input_config.seed=static_cast<int>(seed);
  double table_energy=100;std::optional<std::string> table_energy_option;
  bool jewel_fragmentation=false;std::optional<std::array<double,2>> centrality;
  for(int i=5;i<argc;++i){std::string option=argv[i];
   if(input_option(argc,argv,i,input_config))continue;
   if(option=="--table-energy"){
    if(table_energy_option||i+1>=argc)throw std::invalid_argument("table energy requires one GEV or jewel value");
    table_energy_option=argv[++i];
    if(*table_energy_option!="jewel"){std::size_t used=0;table_energy=std::stod(*table_energy_option,&used);
     if(used!=table_energy_option->size()||!std::isfinite(table_energy)||table_energy<=0)throw std::invalid_argument("table energy must be finite and positive");}
   }
   else if(option=="--centrality"){
    if(centrality||i+2>=argc)throw std::invalid_argument("centrality requires one MIN MAX pair");
    std::array<double,2> range;
    for(auto& value:range){std::string token=argv[++i];std::size_t used=0;value=std::stod(token,&used);if(used!=token.size()||!std::isfinite(value))throw std::invalid_argument("invalid centrality");}
    if(range[0]<0||range[1]>100||range[0]>=range[1])throw std::invalid_argument("centrality requires 0 <= MIN < MAX <= 100");centrality=range;
   }
   else if(option=="--jewel-fragmentation"){if(!hadronize_run)throw std::invalid_argument("fragmentation option requires jewel-run-events");jewel_fragmentation=true;}
   else if(option.rfind("--",0)==0||!replay_directory.empty())throw std::invalid_argument("unknown or repeated runner option");
   else replay_directory=option;
  }
  if(!replay_directory.empty()&&!std::filesystem::create_directory(replay_directory))throw std::runtime_error("replay directory must be new");
  jewel::Pythia8Input input(input_config);
  std::unique_ptr<jewel::Pythia8Hadronizer> hadronizer;
  if(hadronize_run)hadronizer=std::make_unique<jewel::Pythia8Hadronizer>(jewel::HadronizerConfig{"",static_cast<int>(seed),jewel_fragmentation?jewel::FragmentationProfile::Jewel260Mapped:jewel::FragmentationProfile::Monash14});
  jewel::Kernels kernels;jewel::MediumConfig medium_config;medium_config.max_rapidity=2;jewel::ExpandingMedium medium(medium_config);
  std::optional<jewel::GeometrySampler> geometry;double table_b=0;
  const auto geometry_seed=static_cast<std::uint64_t>(seed)+500000;
  if(centrality){geometry.emplace(medium.cross_section_table());jewel::SplitMixRandom stream(geometry_seed);
   table_b=geometry->sample((*centrality)[0],(*centrality)[1],stream).impact_parameter;medium=medium.with_impact_parameter(table_b);}
  // INIT: EOVEST uses the hard upper pT cut and ETAMAX (here max_rapidity).
  if(table_energy_option=="jewel")table_energy=input_config.pt_max>0?std::min(1.5*(input_config.pt_max+50.)*std::cosh(medium_config.max_rapidity),input_config.ecm/2.):input_config.ecm/2.;
  auto maximum=medium.maximum_scatterer(false);double lifetime=medium.bounds().lifetime_max;
  jewel::InitialSudakovTable sudakov(kernels,1,table_energy,1,maximum);jewel::FinalSplittingTable splitting(kernels,1,table_energy,1,maximum);
  jewel::PartonicPdf pdf(kernels,1,table_energy,1,maximum);jewel::ScatteringRates rates(kernels,1,table_energy,1,medium,false);
  jewel::BranchingSampler branching(kernels,1,1,0);
  jewel::InitialEvolutionConfig config;config.evolution.lifetime=lifetime;config.evolution.cross_section_scale=mode=="medium"?1:0;
  config.coherent.lifetime=lifetime;config.coherent.cross_section_scale=config.evolution.cross_section_scale;
  config.coherent.finalization.scatter_recoils=false;config.coherent.finalization.storage={0,true,false};
  std::ofstream out(argv[1]);if(!out)throw std::runtime_error("cannot create output");out<<std::setprecision(17);
  out<<"{\"type\":\"run\",\"schema\":"<<std::quoted(hadronize_run?"jewel-hadron-development-v1":"jewel-parton-development-v1")<<",\"scope\":\"reduced PPJJ; no ISR/MPI\",\"mode\":"<<std::quoted(mode)<<",\"attempts_requested\":"<<attempts<<",\"seed\":"<<seed<<",\"pythia_version\":"<<input.version()<<",\"table_lifetime_fm\":"<<lifetime<<",\"table_energy_gev\":"<<table_energy<<",\"table_energy_prescription\":"<<std::quoted(table_energy_option=="jewel"?"jewel-eovest":table_energy_option?"explicit":"legacy-100")<<",\"table_impact_parameter_fm\":"<<table_b<<",\"geometry_seed\":"<<geometry_seed<<",\"angular_ordering\":0,\"coherence\":true,\"recoil_rescattering\":false,\"longitudinal_scatterer_boost\":false,\"compression\":true,\"pythia_settings\":[";
  bool comma=false;for(const auto& s:input.settings()){if(comma)out<<',';comma=true;out<<std::quoted(s);}out<<"],\"hadronization\":"<<(hadronize_run?"true":"false")<<",\"hadrotype\":0,\"hadronizer_settings\":[";
  if(hadronizer){comma=false;for(const auto& setting:hadronizer->settings()){if(comma)out<<',';comma=true;out<<std::quoted(setting);}}out<<"],\"centrality_percent\":";
  if(centrality)out<<'['<<(*centrality)[0]<<','<<(*centrality)[1]<<']';else out<<"null";out<<"}\n";
  std::uint64_t accepted=0,failed=0,generated_failed=0,final_count=0;double accepted_weight=0,failed_weight=0;
  for(unsigned long attempt=1;attempt<=attempts;++attempt){auto hard=input.next();
   if(!hard){++generated_failed;out<<"{\"type\":\"generation_failure\",\"attempt\":"<<attempt<<"}\n";continue;}
   const auto shower_seed=static_cast<std::uint64_t>(seed)+1000000+100*attempt;
   jewel::SplitMixRandom random(shower_seed);std::string stage="geometry";double event_b=0;std::optional<double> event_centrality;
   try{
    if(geometry){auto sampled=geometry->sample((*centrality)[0],(*centrality)[1],random);event_b=sampled.impact_parameter;event_centrality=sampled.centrality_fraction;}
    auto event_medium=medium.with_impact_parameter(event_b);double event_lifetime=event_medium.bounds().lifetime_max;
    std::optional<jewel::ScatteringRates> rebound;if(geometry)rebound.emplace(rates.with_event_impact_parameter(event_b));
    const auto& event_rates=rebound?*rebound:rates;
    jewel::TransferSampler transfers(kernels,event_medium,false);
    auto event_config=config;event_config.evolution.lifetime=event_lifetime;event_config.coherent.lifetime=event_lifetime;
    jewel::InitialScatteringEvolution initial(event_config,kernels,branching,sudakov,splitting,pdf,event_rates,transfers,event_medium);
    jewel::ScatteringEvolution final(event_config.evolution,branching,sudakov,splitting,event_rates,transfers);
    jewel::Cascade cascade({event_config.evolution,event_config.coherent.collision,event_config.coherent.finalization,1000000,true},initial,final,branching,event_medium);
    auto vertex=event_medium.sample_vertex(random);stage="hard initialization";
    jewel::HardInitializationConfig hard_config;hard_config.x=vertex.x;hard_config.y=vertex.y;hard_config.lifetime=event_lifetime;
    auto prepared=jewel::prepare_pythia_dijet(*hard,hard_config,branching,random);auto initial_hard=std::array<jewel::ShowerParticle,2>{prepared.state.at(prepared.hard[0]),prepared.state.at(prepared.hard[1])};
    if(!replay_directory.empty()){stage="replay export";replay_fixture(replay_directory/(std::to_string(attempt)+".input"),prepared,event_lifetime,config.evolution.cross_section_scale,shower_seed,random.checkpoint(),table_b,event_b,table_energy);}
    jewel::TransferHistory history;history.records.resize(100);jewel::RecoilStorage ledger;stage="cascade";
    auto evolved=cascade.evolve(prepared.state,prepared.scratch,history,ledger,random);prepared.state.validate();
    if(prepared.state.discarded)throw std::runtime_error("cascade discarded event");
    for(const auto& s:prepared.state.particles())if(s.particle.status==1||s.particle.status==2)throw std::runtime_error("unfinished cascade record");
    std::vector<jewel::GeneratorParticle> hadron_input;std::optional<jewel::HadronizedSnapshot> hadrons;
    if(hadronizer){stage="string construction";hadron_input=jewel::prepare_vacuum_hadronization(prepared.state);
     stage="hadronization";hadrons=hadronizer->hadronize(hadron_input);if(!hadrons)throw std::runtime_error("Pythia8 hadronization failed");}
    ++accepted;accepted_weight+=hard->weight;
    out<<"{\"type\":\"event\",\"attempt\":"<<attempt<<",\"impact_parameter_fm\":"<<event_b<<",\"centrality_fraction\":";if(event_centrality)out<<*event_centrality;else out<<"null";out<<",\"event_lifetime_fm\":"<<event_lifetime<<",\"weight\":"<<hard->weight<<",\"pt_hat\":"<<hard->pt_hat<<",\"shower_seed\":"<<shower_seed<<",\"draws\":"<<random.checkpoint()<<",\"branch_calls\":"<<evolved.branch_calls<<",\"initial_hard\":[";
    particle(out,initial_hard[0]);out<<',';particle(out,initial_hard[1]);out<<"],\"input_sources\":[";comma=false;
    for(const auto& entry:prepared.input_sources){if(comma)out<<',';comma=true;out<<'['<<entry.first<<','<<entry.second<<']';}
    out<<"],\"particles\":[";comma=false;
    for(const auto& p:prepared.state.particles()){if(comma)out<<',';comma=true;particle(out,p);if(p.particle.status==3||p.particle.status==4||p.particle.status==5)++final_count;}
    out<<"],\"archived\":[";comma=false;for(const auto& entry:prepared.state.archived()){if(comma)out<<',';comma=true;particle(out,entry.second);}
    out<<"],\"thermal_references\":[";
    for(std::size_t i=0;i<ledger.used;++i){if(i)out<<',';const auto& r=ledger.records.at(i);out<<"{\"pdg\":"<<r.flavour<<",\"momentum\":";momentum(out,r.centre);out<<",\"dummy\":";momentum(out,r.dummy);out<<'}';}out<<']';
    if(hadrons){out<<",\"hadronization_input\":[";comma=false;for(const auto& p:hadron_input){if(comma)out<<',';comma=true;generator_particle(out,p);}
     out<<"],\"hadronized_particles\":[";comma=false;for(const auto& p:hadrons->particles){if(comma)out<<',';comma=true;generator_particle(out,p);}
     out<<"],\"hadronization_sources\":[";comma=false;for(auto id:hadrons->input_sources){if(comma)out<<',';comma=true;out<<id;}out<<']';}out<<"}\n";
   }catch(const std::domain_error& error){++failed;failed_weight+=hard->weight;out<<"{\"type\":\"evolution_failure\",\"attempt\":"<<attempt<<",\"impact_parameter_fm\":"<<event_b<<",\"centrality_fraction\":";if(event_centrality)out<<*event_centrality;else out<<"null";out<<",\"weight\":"<<hard->weight<<",\"stage\":"<<std::quoted(stage)<<",\"reason\":"<<std::quoted(error.what())<<",\"draws\":"<<random.checkpoint()<<"}\n";
   }catch(const std::runtime_error& error){if(stage=="replay export")throw;++failed;failed_weight+=hard->weight;out<<"{\"type\":\"evolution_failure\",\"attempt\":"<<attempt<<",\"impact_parameter_fm\":"<<event_b<<",\"centrality_fraction\":";if(event_centrality)out<<*event_centrality;else out<<"null";out<<",\"weight\":"<<hard->weight<<",\"stage\":"<<std::quoted(stage)<<",\"reason\":"<<std::quoted(error.what())<<",\"draws\":"<<random.checkpoint()<<"}\n";}
  }
  auto stats=input.statistics();out<<"{\"type\":\"summary\",\"attempts\":"<<attempts<<",\"accepted\":"<<accepted<<",\"evolution_failures\":"<<failed<<",\"generation_failures\":"<<generated_failed<<",\"accepted_weight\":"<<accepted_weight<<",\"failed_weight\":"<<failed_weight<<",\"generator_weight_sum\":"<<stats.weight_sum<<",\"generator_sigma_mb\":"<<stats.sigma_mb<<",\"generator_sigma_error_mb\":"<<stats.sigma_error_mb<<",\"final_records\":"<<final_count<<"}\n";
  out.close();if(!out)throw std::runtime_error("failed to finish output");std::cout<<"Accepted "<<accepted<<" / "<<attempts<<" attempts; evolution failures "<<failed<<'\n';return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
