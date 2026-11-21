// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/pythia8_shower_input.hpp"
#include "jewel/cascade.hpp"
#include "jewel/subtraction.hpp"
#include "jewel/projection.hpp"
#include "input_options.hpp"
#include "jewel/geometry.hpp"
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
 double lifetime,double cross,std::uint64_t seed,std::uint64_t cursor,double table_b,double event_b,double table_energy,bool longitudinal_boost,bool keep_recoils,bool scatter_recoils,double soft_cut,double hard_cut,int angular_ordering,bool vacuum){
 std::ofstream out(path);if(!out)throw std::runtime_error("cannot create replay fixture");out<<std::setprecision(17);
 if(vacuum)out<<"VACUUM\n";
 else out<<"CONFIG\n208 3 1 .4 .59 .17 .17 .54 6.199999809265137 "<<table_b<<" 2 .45 1\n";
 out<<"FCONFIG\n3 1 .2 "<<table_energy<<" 1 "<<longitudinal_boost<<"\nRCONFIG\n3 1 .2 "<<table_energy<<" 1 "<<longitudinal_boost<<"\nXCONFIG\n3 1 .2 .2 "<<table_energy<<" 1 "<<longitudinal_boost<<"\n"
    <<"PCONFIG\n3 1 .2 "<<table_energy<<" 1 "<<longitudinal_boost<<"\nQCONFIG\n3 .2 .2 "<<longitudinal_boost<<"\nBCONFIG\n3 1 .2 1 "<<angular_ordering<<"\n"
    <<"EVENTB\n"<<event_b<<"\nECONFIG\n1 1 1 "<<lifetime<<' '<<cross<<"\nNCASCADE\n"
    <<prepared.hard[0]<<" 1 0 0 0 0 1 1 "<<prepared.state.colour_max<<' '<<prepared.weight<<" 0\n0 1\n"
    <<prepared.state.particles().size()<<'\n';
 for(const auto& s:prepared.state.particles()){const auto& p=s.particle;
  auto daughters=prepared.state.scheduling_daughters(p.id);
  out<<p.status<<' '<<p.pdg<<' '<<prepared.state.scheduling_parent(p.id)<<' '<<daughters[0]<<' '<<daughters[1]<<' '
   <<p.momentum.px<<' '<<p.momentum.py<<' '<<p.momentum.pz<<' '<<p.momentum.energy<<' '<<s.record_mass<<' '
   <<s.generator_vertex.x<<' '<<s.generator_vertex.y<<' '<<s.generator_vertex.z<<' '<<s.generator_vertex.t<<' '
   <<p.production.x<<' '<<p.production.y<<' '<<p.production.z<<' '<<p.production.t<<' '<<s.decay_time<<' '
   <<s.fraction<<' '<<s.angle<<' '<<s.quark_pair<<' '<<s.angular_required<<' '<<p.colour<<' '<<p.anticolour<<' '<<s.evolution_time<<'\n';
 }
 out<<"1 "<<soft_cut<<' '<<hard_cut<<' '<<keep_recoils<<' '<<scatter_recoils<<" 0 1 0 23000\n-1\n"<<seed<<'\n'<<cursor<<'\n';
 out.close();if(!out)throw std::runtime_error("cannot finish replay fixture");
}
unsigned long integer(const char* value){std::string s=value;std::size_t end=0;auto n=std::stoul(s,&end);if(s.empty()||s[0]=='-'||end!=s.size())throw std::invalid_argument("invalid integer argument");return n;}
}
int main(int argc,char** argv){
 try{
  if(argc<5){std::cerr<<"Usage: "<<(hadronize_run?"jewel-run-events":"jewel-run-partons")<<" output.jsonl attempts seed vacuum|medium|zero-interaction [new-replay-directory] [--experimental-isr-mpi] [--hadrotype 0|1] [--geometry-step FM] [--centrality MIN MAX] [--table-energy GEV|jewel] [--longitudinal-boost on|off] [--keep-recoils on|off] [--recoil-rescattering on|off] [--recoil-cuts SOFT HARD] [--angular-ordering 0..5] [--subtraction-radius R] [--put-on-shell 0|1|2] [--jewel-fragmentation] [--ecm GEV --pt-min GEV --pt-max GEV] [--lhapdf SET/MEMBER --hard-alpha-s VALUE --hard-alpha-s-order ORDER]\nDevelopment PPJJ runner: ISR/MPI off by default; experimental mode is not validated full physics.\n";return 2;}
  auto attempts=integer(argv[2]),seed=integer(argv[3]);std::string mode=argv[4];
  if(!attempts||attempts>1000000||!seed||seed>900000000||(mode!="vacuum"&&mode!="medium"&&mode!="zero-interaction"))throw std::invalid_argument("invalid runner arguments");
  if(std::filesystem::exists(argv[1]))throw std::runtime_error("refusing to overwrite output");
  std::filesystem::path replay_directory;
  InputOptionState input_options;jewel::PythiaInputConfig input_config;input_config.seed=static_cast<int>(seed);
  std::optional<double> geometry_step;double table_energy=100;std::optional<std::string> table_energy_option;
  bool experimental_isr_mpi=false;
  bool longitudinal_boost=false,boost_option_seen=false;
  int hadrotype=0;bool hadrotype_seen=false;int angular_ordering=0;bool angular_option_seen=false;
  bool keep_recoils=true,recoil_option_seen=false;
  bool scatter_recoils=false,scatter_option_seen=false,cuts_option_seen=false;double soft_cut=0,hard_cut=0;
  std::optional<double> subtraction_radius;int projection_mode=0;bool projection_seen=false;
  bool jewel_fragmentation=false;std::optional<std::array<double,2>> centrality;
  for(int i=5;i<argc;++i){std::string option=argv[i];
   if(input_option(argc,argv,i,input_config,input_options))continue;
   if(option=="--experimental-isr-mpi"){
    if(experimental_isr_mpi)throw std::invalid_argument("experimental ISR/MPI option repeated");
    experimental_isr_mpi=true;input_config.initial_state_radiation=true;input_config.multiple_parton_interactions=true;continue;
   }
   if(option=="--hadrotype"){
    if(hadrotype_seen||i+1>=argc||!hadronize_run)throw std::invalid_argument("hadrotype requires jewel-run-events and one mode");
    std::string value=argv[++i];if(value!="0"&&value!="1")throw std::invalid_argument("hadrotype must be 0 or 1");
    hadrotype=value=="1"?1:0;hadrotype_seen=true;continue;
   }
   if(option=="--geometry-step"){
    if(geometry_step||i+1>=argc)throw std::invalid_argument("geometry step requires one positive value");
    std::string token=argv[++i];std::size_t used=0;double value=std::stod(token,&used);
    if(used!=token.size()||!std::isfinite(value)||value<=0)throw std::invalid_argument("geometry step must be finite and positive");
    geometry_step=value;continue;
   }
   if(option=="--table-energy"){
    if(table_energy_option||i+1>=argc)throw std::invalid_argument("table energy requires one GEV or jewel value");
    table_energy_option=argv[++i];
    if(*table_energy_option!="jewel"){std::size_t used=0;table_energy=std::stod(*table_energy_option,&used);
     if(used!=table_energy_option->size()||!std::isfinite(table_energy)||table_energy<=0)throw std::invalid_argument("table energy must be finite and positive");}
   }
   else if(option=="--longitudinal-boost"){
    if(boost_option_seen||i+1>=argc)throw std::invalid_argument("longitudinal boost requires one on or off value");
    const std::string value=argv[++i];if(value!="on"&&value!="off")throw std::invalid_argument("longitudinal boost requires on or off");
    longitudinal_boost=value=="on";boost_option_seen=true;
   }
   else if(option=="--keep-recoils"){
    if(recoil_option_seen||i+1>=argc)throw std::invalid_argument("keep recoils requires one on or off value");
    const std::string value=argv[++i];if(value!="on"&&value!="off")throw std::invalid_argument("keep recoils requires on or off");
    keep_recoils=value=="on";recoil_option_seen=true;
   }
   else if(option=="--recoil-rescattering"){
    if(scatter_option_seen||i+1>=argc)throw std::invalid_argument("recoil rescattering requires one on or off value");
    const std::string value=argv[++i];if(value!="on"&&value!="off")throw std::invalid_argument("recoil rescattering requires on or off");
    scatter_recoils=value=="on";scatter_option_seen=true;
   }
   else if(option=="--recoil-cuts"){
    if(cuts_option_seen||i+2>=argc)throw std::invalid_argument("recoil cuts require one SOFT HARD pair");
    for(auto* value:{&soft_cut,&hard_cut}){const std::string token=argv[++i];std::size_t used=0;*value=std::stod(token,&used);
     if(used!=token.size()||!std::isfinite(*value)||*value<0)throw std::invalid_argument("recoil cuts must be finite and nonnegative");}
    cuts_option_seen=true;
   }
   else if(option=="--angular-ordering"){
    if(angular_option_seen||i+1>=argc)throw std::invalid_argument("angular ordering requires one integer from 0 to 5");
    const std::string value=argv[++i];if(value.size()!=1||value[0]<'0'||value[0]>'5')throw std::invalid_argument("angular ordering requires one integer from 0 to 5");
    angular_ordering=value[0]-'0';angular_option_seen=true;
   }
   else if(option=="--subtraction-radius"){
    if(subtraction_radius||i+1>=argc||!hadronize_run)throw std::invalid_argument("subtraction requires jewel-run-events and one radius");
    const std::string token=argv[++i];std::size_t used=0;double value=std::stod(token,&used);
    if(used!=token.size()||!std::isfinite(value)||value<0)throw std::invalid_argument("subtraction radius must be finite and nonnegative");
    subtraction_radius=value;
   }
   else if(option=="--put-on-shell"){
    if(projection_seen||i+1>=argc||!hadronize_run)throw std::invalid_argument("projection requires jewel-run-events and one mode");
    const std::string value=argv[++i];if(value.size()!=1||value[0]<'0'||value[0]>'2')throw std::invalid_argument("projection mode must be 0, 1 or 2");
    projection_mode=value[0]-'0';projection_seen=true;
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
  if(mode=="vacuum"&&centrality)throw std::invalid_argument("vacuum has no centrality; use zero-interaction for a noninteracting expanding background");
  if(experimental_isr_mpi&&hadronize_run&&hadrotype!=0)throw std::invalid_argument("experimental ISR/MPI hadronization currently requires HADROTYPE 0");
  if(projection_seen&&!subtraction_radius)throw std::invalid_argument("projection requires --subtraction-radius");
  if(subtraction_radius&&!keep_recoils)throw std::invalid_argument("thermal subtraction requires retained recoils");
  if(!replay_directory.empty()&&!std::filesystem::create_directory(replay_directory))throw std::runtime_error("replay directory must be new");
  jewel::Pythia8Input input(input_config);
  std::unique_ptr<jewel::Pythia8Hadronizer> hadronizer;
  if(hadronize_run)hadronizer=std::make_unique<jewel::Pythia8Hadronizer>(jewel::HadronizerConfig{"",static_cast<int>(seed),jewel_fragmentation?jewel::FragmentationProfile::Jewel260Mapped:jewel::FragmentationProfile::Monash14});
  jewel::Kernels kernels;jewel::MediumConfig medium_config;medium_config.max_rapidity=2;jewel::MediumModel medium=mode=="vacuum"?jewel::MediumModel{}:jewel::MediumModel(jewel::ExpandingMedium(medium_config));
  std::optional<jewel::GeometrySampler> geometry;double table_b=0;
  const auto geometry_seed=static_cast<std::uint64_t>(seed)+500000;
  if(centrality){geometry.emplace(medium.cross_section_table());jewel::SplitMixRandom stream(geometry_seed);
   table_b=geometry->sample((*centrality)[0],(*centrality)[1],stream).impact_parameter;medium=medium.with_impact_parameter(table_b);}
  // INIT: EOVEST uses the hard upper pT cut and ETAMAX (here max_rapidity).
  if(table_energy_option=="jewel")table_energy=input_config.pt_max>0?std::min(1.5*(input_config.pt_max+50.)*std::cosh(medium_config.max_rapidity),input_config.ecm/2.):input_config.ecm/2.;
  auto maximum=medium.maximum_scatterer(longitudinal_boost);double lifetime=medium.bounds().lifetime_max;
  jewel::InitialSudakovTable sudakov(kernels,1,table_energy,1,maximum);jewel::FinalSplittingTable splitting(kernels,1,table_energy,1,maximum);
  jewel::PartonicPdf pdf(kernels,1,table_energy,1,maximum);jewel::ScatteringRates rates(kernels,1,table_energy,1,medium,longitudinal_boost);
  jewel::BranchingSampler branching(kernels,1,1,angular_ordering);
  jewel::InitialEvolutionConfig config;config.evolution.lifetime=lifetime;config.evolution.cross_section_scale=mode=="medium"?1:0;
  config.coherent.lifetime=lifetime;config.coherent.cross_section_scale=config.evolution.cross_section_scale;
  config.coherent.finalization.hadronize_recoils=keep_recoils;
  config.coherent.finalization.scatter_recoils=scatter_recoils;
  config.coherent.finalization.soft_recoil_cut=soft_cut;config.coherent.finalization.hard_recoil_cut=hard_cut;config.coherent.finalization.storage={0,true,false};
  std::ofstream out(argv[1]);if(!out)throw std::runtime_error("cannot create output");out<<std::setprecision(17);
  out<<"{\"type\":\"run\",\"schema\":"<<std::quoted(hadronize_run?"jewel-hadron-development-v1":"jewel-parton-development-v1")<<",\"scope\":"<<std::quoted(experimental_isr_mpi?"experimental PPJJ ISR+MPI; incomplete topology and cascade validation":"reduced PPJJ; no ISR/MPI")<<",\"experimental_isr_mpi\":"<<(experimental_isr_mpi?"true":"false")<<",\"hard_colour_angle_policy\":"<<std::quoted(experimental_isr_mpi?"not-evaluated-veto-disabled":"source-diagnostic")<<",\"medium_backend\":"<<std::quoted(mode=="vacuum"?"jewel-vacuum":"expanding")<<",\"mpi_channel_policy\":"<<std::quoted(experimental_isr_mpi?"outgoing-flavour":"not-applicable")<<",\"mode\":"<<std::quoted(mode)<<",\"attempts_requested\":"<<attempts<<",\"seed\":"<<seed<<",\"pythia_version\":"<<input.version()<<",\"table_lifetime_fm\":"<<lifetime<<",\"table_energy_gev\":"<<table_energy<<",\"table_energy_prescription\":"<<std::quoted(table_energy_option=="jewel"?"jewel-eovest":table_energy_option?"explicit":"legacy-100")<<",\"table_impact_parameter_fm\":"<<table_b<<",\"geometry_seed\":"<<geometry_seed<<",\"angular_ordering\":"<<angular_ordering<<",\"first_angle_ordering\":false,\"coherence\":true,\"keep_recoils\":"<<(keep_recoils?"true":"false")<<",\"recoil_rescattering\":"<<(scatter_recoils?"true":"false")<<",\"recoil_soft_cut_3T\":"<<soft_cut<<",\"recoil_hard_cut_3T\":"<<hard_cut<<",\"longitudinal_scatterer_boost\":"<<(longitudinal_boost?"true":"false")<<",\"compression\":true,\"pythia_settings\":[";
  bool comma=false;for(const auto& s:input.settings()){if(comma)out<<',';comma=true;out<<std::quoted(s);}out<<"],\"hadronization\":"<<(hadronize_run?"true":"false")<<",\"hadrotype\":"<<hadrotype<<",\"hadronizer_settings\":[";
  if(hadronizer){comma=false;for(const auto& setting:hadronizer->settings()){if(comma)out<<',';comma=true;out<<std::quoted(setting);}}out<<"],\"centrality_percent\":";
  if(centrality)out<<'['<<(*centrality)[0]<<','<<(*centrality)[1]<<']';else out<<"null";
  if(hadrotype==1)out<<",\"minimum_string_policy\":{\"input_ids\":\"ordered-string-local\",\"native_sources\":\"parallel stable IDs; null for synthetic endpoints\",\"generated_quark_mass\":\"parent mass scaled with daughter four-momentum\",\"endpoint_energy_gev\":10,\"max_rapidity\":"<<medium_config.max_rapidity<<",\"rapidity_margin\":1}";
  if(geometry_step)out<<",\"geometry_estimator\":{\"schema\":\"geometry-v1-native\",\"step_fm\":"<<*geometry_step<<",\"radius_fm\":"<<(mode=="medium"?medium.thickness_table().back()[0]:0)<<",\"trajectory\":\"straight-line from shower-start momentum; not traversed shower history\"}";
  if(subtraction_radius)out<<",\"subtraction\":{\"algorithm\":\"jewel260-kernel-zero-offset\",\"radius\":"<<*subtraction_radius<<",\"put_on_shell\":"<<projection_mode<<",\"mass_policy\":\"signed-invariant-mass\",\"rng\":\"splitmix64-open52-v1\",\"seed_base\":"<<static_cast<std::uint64_t>(seed)+2000000<<",\"seed_stride\":100,\"input\":\"all-final-hadronized\",\"thermal_input\":\"stored-centres\",\"positive_thermal_residuals\":true}";
  if(projection_mode)out<<",\"projection_mass_source\":\"Pythia8 ParticleData::m0\",\"projection_range\":\"all-subtracted-PPJJ\"";
  out<<"}\n";
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
    jewel::TransferSampler transfers(kernels,event_medium,longitudinal_boost);
    auto event_config=config;event_config.evolution.lifetime=event_lifetime;event_config.coherent.lifetime=event_lifetime;
    jewel::InitialScatteringEvolution initial(event_config,kernels,branching,sudakov,splitting,pdf,event_rates,transfers,event_medium);
    jewel::ScatteringEvolution final(event_config.evolution,branching,sudakov,splitting,event_rates,transfers);
    jewel::Cascade cascade({event_config.evolution,event_config.coherent.collision,event_config.coherent.finalization,1000000,true},initial,final,branching,event_medium);
    auto vertex=event_medium.sample_vertex(random);stage="hard initialization";
    jewel::HardInitializationConfig hard_config;hard_config.x=vertex.x;hard_config.y=vertex.y;hard_config.lifetime=event_lifetime;
    jewel::PreparedDijet prepared;
    if(experimental_isr_mpi){
     stage="ISR mapping";
     auto chains=jewel::prepare_subsystem_isr_timing(*hard,[&](int pdg){return input.nominal_mass(pdg);});
     auto imported=jewel::import_pythia_final_record(*hard,vertex.x,vertex.y,event_lifetime);
     stage="ISR timing";jewel::IsrTimeConfig timing;timing.x=vertex.x;timing.y=vertex.y;timing.lifetime=event_lifetime;
     jewel::attach_pythia_isr_times(imported.state,imported.input_sources,*hard,chains,timing,random);
     stage="survivor preparation";
     jewel::prepare_imported_partons(imported,[&](int pdg){return input.nominal_mass(pdg);},medium_config.max_rapidity);
     stage="subsystem initialization";
     auto initialized=jewel::initialize_imported_subsystems(imported,hard_config,branching,random,jewel::MpiChannelPolicy::outgoing_flavour);
     prepared.hard=imported.subsystems.front().hard;prepared.hard_process=imported.subsystems.front().hard_origins[0];
     prepared.scratch=imported.scratch;prepared.weight=imported.state.weight;prepared.initialization=initialized.pairs.front();
     prepared.input_sources=std::move(imported.input_sources);prepared.state=std::move(imported.state);
    }else prepared=jewel::prepare_pythia_dijet(*hard,hard_config,branching,random);
    auto initial_hard=std::array<jewel::ShowerParticle,2>{prepared.state.at(prepared.hard[0]),prepared.state.at(prepared.hard[1])};
    std::array<jewel::PathLengths,2> path_lengths;
    if(geometry_step){stage="geometry estimate";for(int leg=0;leg<2;++leg)
     path_lengths[leg]=jewel::straight_path_lengths(vertex.x,vertex.y,event_b,
      mode=="medium"?event_medium.thickness_table().back()[0]:0,event_lifetime,
      initial_hard[leg].particle.momentum,*geometry_step,event_medium);}
    if(!replay_directory.empty()){stage="replay export";replay_fixture(replay_directory/(std::to_string(attempt)+".input"),prepared,event_lifetime,config.evolution.cross_section_scale,shower_seed,random.checkpoint(),table_b,event_b,table_energy,longitudinal_boost,keep_recoils,scatter_recoils,soft_cut,hard_cut,angular_ordering,mode=="vacuum");}
    jewel::TransferHistory history;history.records.resize(100);jewel::RecoilStorage ledger;stage="cascade";
    auto evolved=cascade.evolve(prepared.state,prepared.scratch,history,ledger,random);prepared.state.validate();
    if(prepared.state.discarded)throw std::runtime_error("cascade discarded event");
    for(const auto& s:prepared.state.particles())if(s.particle.status==1||s.particle.status==2)throw std::runtime_error("unfinished cascade record");
    // GENEVENT excludes any residual status-3 centres before string construction.
    if(!keep_recoils)for(const auto& node:prepared.state.particles())if(node.particle.status==3)prepared.state.at(node.particle.id).particle.status=22;
    const auto cascade_draws=random.checkpoint();
    std::vector<jewel::GeneratorParticle> hadron_input;std::optional<jewel::HadronizedSnapshot> hadrons;
    if(hadronizer){stage="string construction";hadron_input=experimental_isr_mpi?jewel::prepare_native_hadronization(prepared.state,hard->junctions,keep_recoils):hadrotype==0?jewel::prepare_vacuum_hadronization(prepared.state,keep_recoils):jewel::prepare_minimum_hadronization(prepared.state,branching,random,medium_config.max_rapidity,keep_recoils);
     stage="hadronization";hadrons=hadronizer->hadronize(hadron_input,experimental_isr_mpi?hard->junctions:std::vector<jewel::GeneratorJunction>{});if(!hadrons)throw std::runtime_error("Pythia8 hadronization failed");}
    std::optional<jewel::SubtractionResult> subtracted;
    const auto subtraction_seed=static_cast<std::uint64_t>(seed)+2000000+100*attempt;
    jewel::SplitMixRandom subtraction_random(subtraction_seed);
    if(subtraction_radius){
     stage="thermal subtraction";std::vector<jewel::SubtractionParticle> selected;std::vector<jewel::Momentum> thermals;
     for(const auto& p:hadrons->particles)if(p.final)selected.push_back({p.id,p.pdg,p.momentum,p.mass,false});
     for(std::size_t i=0;i<ledger.used;++i)thermals.push_back(ledger.records.at(i).centre);
     subtracted=jewel::subtract_thermal_momenta(selected,thermals,*subtraction_radius,subtraction_random,jewel::SubtractionMassPolicy::SignedMass);
     if(subtracted->capacity_exceeded)throw std::runtime_error("subtracted event exceeds capacity");
    }
    std::optional<jewel::ProjectionResult> projected;jewel::Momentum projection_before{},projection_after{};
    if(projection_mode){
     stage="on-shell projection";std::vector<jewel::ProjectionParticle> selected;
     for(const auto& p:subtracted->particles){selected.push_back({p.momentum,p.mass,hadronizer->nominal_mass(p.pdg)});
      projection_before.px+=p.momentum.px;projection_before.py+=p.momentum.py;projection_before.pz+=p.momentum.pz;projection_before.energy+=p.momentum.energy;}
     projected=jewel::project_on_shell(selected,projection_mode,0);
     for(const auto& p:projected->particles){projection_after.px+=p.momentum.px;projection_after.py+=p.momentum.py;projection_after.pz+=p.momentum.pz;projection_after.energy+=p.momentum.energy;}
    }
    ++accepted;accepted_weight+=hard->weight;
    out<<"{\"type\":\"event\",\"attempt\":"<<attempt<<",\"impact_parameter_fm\":"<<event_b<<",\"centrality_fraction\":";if(event_centrality)out<<*event_centrality;else out<<"null";out<<",\"event_lifetime_fm\":"<<event_lifetime<<",\"weight\":"<<hard->weight<<",\"pt_hat\":"<<hard->pt_hat<<",\"shower_seed\":"<<shower_seed<<",\"draws\":"<<cascade_draws<<",\"branch_calls\":"<<evolved.branch_calls<<",\"initial_hard\":[";
    particle(out,initial_hard[0]);out<<',';particle(out,initial_hard[1]);out<<"],\"input_sources\":[";comma=false;
    for(const auto& entry:prepared.input_sources){if(comma)out<<',';comma=true;out<<'['<<entry.first<<','<<entry.second<<']';}
    out<<']';
    if(geometry_step){out<<",\"geometry\":[";
     for(int leg=0;leg<2;++leg){if(leg)out<<',';const auto& g=path_lengths[leg];
      out<<"{\"leg_id\":"<<leg+1<<",\"shower_start_id\":"<<initial_hard[leg].particle.id
       <<",\"production_fm\":["<<vertex.x<<','<<vertex.y<<",0,0],\"L_geom_T_fm\":"<<g.geometric_transverse
       <<",\"L_hot_fm\":"<<g.hot<<",\"L_hot_T_fm\":"<<g.hot_transverse<<",\"int_T3_dl\":"<<g.temperature_cubed_integral<<'}';}out<<']';}
    out<<",\"particles\":[";comma=false;
    for(const auto& p:prepared.state.particles()){if(comma)out<<',';comma=true;particle(out,p);if(p.particle.status==3||p.particle.status==4||p.particle.status==5)++final_count;}
    out<<"],\"archived\":[";comma=false;for(const auto& entry:prepared.state.archived()){if(comma)out<<',';comma=true;particle(out,entry.second);}
    out<<"],\"thermal_references\":[";
    for(std::size_t i=0;i<ledger.used;++i){if(i)out<<',';const auto& r=ledger.records.at(i);out<<"{\"pdg\":"<<r.flavour<<",\"momentum\":";momentum(out,r.centre);out<<",\"dummy\":";momentum(out,r.dummy);out<<'}';}out<<']';
    if(subtracted){
     out<<",\"subtraction_seed\":"<<subtraction_seed<<",\"subtraction_draws\":"<<subtraction_random.checkpoint()
        <<",\"subtraction_spacelike_mass_fields\":"<<subtracted->spacelike_mass_fields<<",\"subtraction_skipped_particles\":"<<subtracted->skipped_particles<<",\"subtraction_skipped_thermals\":"<<subtracted->skipped_thermals<<",\"subtracted_particles\":[";
     std::size_t id=0;for(const auto& p:subtracted->particles){if(id)out<<',';
      out<<"{\"id\":"<<++id<<",\"source_id\":"<<p.source<<",\"pdg\":"<<p.pdg<<",\"final\":true,\"thermal_residual\":"<<(p.thermal_residual?"true":"false")<<",\"mass\":"<<p.mass<<",\"momentum\":";momentum(out,p.momentum);out<<'}';}
     out<<']';
    }
    if(projected){
     out<<",\"projection\":{\"mode\":"<<projection_mode<<",\"massless_iterations\":"<<projected->massless_iterations<<",\"massive_iterations\":"<<projected->massive_iterations
        <<",\"massless_converged\":"<<(projected->massless_converged?"true":"false")<<",\"massive_converged\":"<<(projected->massive_converged?"true":"false")
        <<",\"insufficient_energy\":"<<(projected->insufficient_energy?"true":"false")<<",\"total_before\":";momentum(out,projection_before);out<<",\"total_after\":";momentum(out,projection_after);out<<"},\"projected_particles\":[";
     for(std::size_t i=0;i<projected->particles.size();++i){if(i)out<<',';const auto& p=projected->particles[i];const auto& source=subtracted->particles[i];
      out<<"{\"id\":"<<i+1<<",\"source_id\":"<<source.source<<",\"pdg\":"<<source.pdg<<",\"final\":true,\"thermal_residual\":"<<(source.thermal_residual?"true":"false")<<",\"mass\":"<<p.mass<<",\"nominal_mass\":"<<p.nominal_mass<<",\"momentum\":";momentum(out,p.momentum);out<<'}';}out<<']';
    }
    if(hadrotype==1){out<<",\"string_draws\":"<<random.checkpoint()-cascade_draws<<",\"hadronization_native_sources\":[";
     for(std::size_t i=0;i<hadron_input.size();++i){if(i)out<<',';int source=hadron_input[i].source_index;
      if(source<0)out<<"null";else out<<prepared.state.particles().at(source).particle.id;}out<<']';}
    if(hadrons){out<<",\"hadronization_input\":[";comma=false;for(const auto& p:hadron_input){if(comma)out<<',';comma=true;generator_particle(out,p);}
     out<<"],\"hadronization_junctions\":[";comma=false;
     if(experimental_isr_mpi)for(const auto& j:hard->junctions){if(comma)out<<',';comma=true;out<<"{\"kind\":"<<j.kind<<",\"colours\":["<<j.colours[0]<<','<<j.colours[1]<<','<<j.colours[2]<<"]}";}
     out<<"],\"hadronized_particles\":[";comma=false;for(const auto& p:hadrons->particles){if(comma)out<<',';comma=true;generator_particle(out,p);}
     out<<"],\"hadronization_sources\":[";comma=false;for(auto id:hadrons->input_sources){if(comma)out<<',';comma=true;out<<id;}out<<']';}out<<"}\n";
   }catch(const std::domain_error& error){++failed;failed_weight+=hard->weight;out<<"{\"type\":\"evolution_failure\",\"attempt\":"<<attempt<<",\"impact_parameter_fm\":"<<event_b<<",\"centrality_fraction\":";if(event_centrality)out<<*event_centrality;else out<<"null";out<<",\"weight\":"<<hard->weight<<",\"stage\":"<<std::quoted(stage)<<",\"reason\":"<<std::quoted(error.what())<<",\"draws\":"<<random.checkpoint()<<"}\n";
   }catch(const std::runtime_error& error){if(stage=="replay export")throw;++failed;failed_weight+=hard->weight;out<<"{\"type\":\"evolution_failure\",\"attempt\":"<<attempt<<",\"impact_parameter_fm\":"<<event_b<<",\"centrality_fraction\":";if(event_centrality)out<<*event_centrality;else out<<"null";out<<",\"weight\":"<<hard->weight<<",\"stage\":"<<std::quoted(stage)<<",\"reason\":"<<std::quoted(error.what())<<",\"draws\":"<<random.checkpoint()<<"}\n";}
  }
  auto stats=input.statistics();out<<"{\"type\":\"summary\",\"attempts\":"<<attempts<<",\"accepted\":"<<accepted<<",\"evolution_failures\":"<<failed<<",\"generation_failures\":"<<generated_failed<<",\"accepted_weight\":"<<accepted_weight<<",\"failed_weight\":"<<failed_weight<<",\"generator_weight_sum\":"<<stats.weight_sum<<",\"generator_sigma_mb\":"<<stats.sigma_mb<<",\"generator_sigma_error_mb\":"<<stats.sigma_error_mb<<",\"final_records\":"<<final_count<<"}\n";
  out.close();if(!out)throw std::runtime_error("failed to finish output");std::cout<<"Accepted "<<accepted<<" / "<<attempts<<" attempts; evolution failures "<<failed<<'\n';return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
