// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/pythia8_shower_input.hpp"
#include <filesystem>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
namespace {
void initialized_record(std::ostream& out,const jewel::ShowerState& state,const jewel::ShowerParticle& n,const char* separator){
 auto p=n.particle.momentum;auto v=n.generator_vertex;auto x=n.particle.production;
 auto daughters=state.scheduling_daughters(n.particle.id);
 const double values[]={double(n.particle.status),double(n.particle.pdg),double(state.scheduling_parent(n.particle.id)),double(daughters[0]),double(daughters[1]),p.px,p.py,p.pz,p.energy,n.record_mass,v.x,v.y,v.z,v.t,x.x,x.y,x.z,x.t,n.decay_time,n.fraction,n.angle,double(n.quark_pair),double(n.angular_required),double(n.particle.colour),double(n.particle.anticolour),n.evolution_time};
 bool sep=false;for(auto value:values){if(sep)out<<separator;sep=true;out<<value;}
}
}
int main(int argc,char** argv){try{
 if(argc!=5&&argc!=6)throw std::runtime_error("usage: jewel-native-preparation OUTPUT FIXTURES COUNT SEED [--initialize|--initialize-native]");
 bool initialize=argc==6;bool native_channels=initialize&&std::string(argv[5])=="--initialize-native";if(initialize&&!native_channels&&std::string(argv[5])!="--initialize")throw std::runtime_error("unknown option");
 int count=std::stoi(argv[3]),seed=std::stoi(argv[4]);if(count<=0)throw std::runtime_error("invalid count");
 if(std::filesystem::exists(argv[1])||std::filesystem::exists(argv[2]))throw std::runtime_error("outputs already exist");
 std::ofstream out(argv[1]),fixtures(argv[2]);if(!out||!fixtures)throw std::runtime_error("cannot open outputs");
 std::ofstream initialization_fixtures;
 if(initialize){auto path=std::string(argv[2])+".initialization";
  if(std::filesystem::exists(path))throw std::runtime_error("initialization fixtures already exist");
  initialization_fixtures.open(path);if(!initialization_fixtures)throw std::runtime_error("cannot open initialization fixtures");
  initialization_fixtures<<std::setprecision(17)<<"CONFIG\n208 3 1 .4 .59 .17 .17 .54 6.199999809265137 0 2 .45 1\nBCONFIG\n3 1 .2 1 0\n";
 }
 out<<std::setprecision(17);
 jewel::PythiaInputConfig config;config.seed=seed;config.initial_state_radiation=true;config.multiple_parton_interactions=true;
 jewel::Pythia8Input generator(config);unsigned completed=0;
 jewel::BranchingSampler sampler({},1,1,0);
 out<<"{\"type\":\"run\",\"seed\":"<<seed<<",\"attempts\":"<<count<<",\"initialize_subsystems\":"<<(initialize?"true":"false")<<",\"mpi_channel_policy\":"<<std::quoted(native_channels?"outgoing-flavour":"source-primary")<<",\"eta_max\":2,\"order\":\"Pythia8 final source order\",\"settings\":[";
 bool sep=false;for(const auto& setting:generator.settings()){if(sep)out<<',';sep=true;out<<std::quoted(setting);}out<<"]}\n";
 for(int attempt=1;attempt<=count;++attempt){auto event=generator.next();
  if(!event){out<<"{\"type\":\"failure\",\"attempt\":"<<attempt<<",\"stage\":\"generation\"}\n";continue;}
  std::string stage="mapping";
  try{
   auto chains=jewel::prepare_subsystem_isr_timing(*event,[&](int pdg){return generator.nominal_mass(pdg);});
   auto imported=jewel::import_pythia_final_record(*event,1.2,-.7,20);
   std::size_t draws=2*chains.size();for(const auto& pair:chains)for(const auto& chain:pair)draws+=2*chain.steps.size();
   jewel::ReplayRandom random(std::vector<double>(draws,.5));jewel::IsrTimeConfig timing;timing.x=1.2;timing.y=-.7;timing.lifetime=20;
   jewel::attach_pythia_isr_times(imported.state,imported.input_sources,*event,chains,timing,random);
   stage="preparation";auto before=imported.state;
   std::vector<jewel::ParticleId> ids;std::array<std::size_t,2> hard{};
   for(const auto& node:before.particles())if(imported.input_sources.count(node.particle.id)){
    for(int side=0;side<2;++side)if(node.particle.id==imported.subsystems.front().hard[side])hard[side]=ids.size()+1;
    ids.push_back(node.particle.id);
   }
   std::ostringstream fixture;fixture<<std::setprecision(17)<<"PREPARE\n"<<ids.size()<<' '<<hard[0]<<' '<<hard[1]<<" 2\n";
   for(auto id:ids){const auto& n=before.at(id);auto p=n.particle.momentum;auto v=n.generator_vertex;
    fixture<<n.particle.status<<' '<<n.particle.pdg<<' '<<p.px<<' '<<p.py<<' '<<p.pz<<' '<<p.energy<<' '<<n.record_mass<<' '<<v.x<<' '<<v.y<<' '<<v.z<<' '<<v.t<<' '<<generator.nominal_mass(n.particle.pdg)<<'\n';}
   jewel::prepare_imported_partons(imported,[&](int pdg){return generator.nominal_mass(pdg);},2);
   for(const auto& old:before.particles()){
    const auto& n=imported.state.at(old.particle.id);auto a=n.particle.production,b=old.particle.production;
    if(n.scheduling_slots!=old.scheduling_slots||n.parent!=old.parent||n.daughters!=old.daughters||n.particle.parents!=old.particle.parents||n.particle.colour!=old.particle.colour||n.particle.anticolour!=old.particle.anticolour||a.x!=b.x||a.y!=b.y||a.z!=b.z||a.t!=b.t||n.decay_time!=old.decay_time||n.evolution_time!=old.evolution_time)throw std::runtime_error("preparation altered timing or ancestry");
    if(!imported.input_sources.count(old.particle.id)&&(n.particle.status!=old.particle.status||n.record_mass!=old.record_mass||n.particle.momentum.energy!=old.particle.momentum.energy))throw std::runtime_error("preparation altered anchor");
   }
   fixtures<<fixture.str();++completed;
   out<<"{\"type\":\"event\",\"attempt\":"<<attempt<<",\"values\":[";sep=false;
   for(auto id:ids){const auto& n=imported.state.at(id);auto p=n.particle.momentum;auto v=n.generator_vertex;
    for(double value:{double(n.particle.status),p.px,p.py,p.pz,p.energy,n.record_mass,v.x,v.y,v.z,v.t}){if(sep)out<<',';sep=true;out<<value;}}
   out<<']';
   if(initialize){
    jewel::HardInitializationConfig init;init.x=1.2;init.y=-.7;
    jewel::SplitMixRandom init_random(std::uint64_t(seed)*100000+attempt);
    out<<",\"initialization\":{\"seed\":"<<(std::uint64_t(seed)*100000+attempt);
    try{
     auto prepared=imported.state;
     auto result=jewel::initialize_imported_subsystems(imported,init,sampler,init_random,native_channels?jewel::MpiChannelPolicy::outgoing_flavour:jewel::MpiChannelPolicy::source_primary);
     initialization_fixtures<<(native_channels?"SEQFLAV\n":"SEQINIT\n")<<imported.subsystems[0].pt_hat<<" 1.2 -.7 1 20 0\n"<<prepared.particles().size()<<' '<<imported.subsystems.size()<<" 1\n";
     for(const auto& pair:imported.subsystems)initialization_fixtures<<pair.hard[0]<<' '<<pair.hard[1]<<'\n';
     for(const auto& node:prepared.particles()){initialized_record(initialization_fixtures,prepared,node," ");initialization_fixtures<<'\n';}
     initialization_fixtures<<"-1\n"<<(std::uint64_t(seed)*100000+attempt)<<'\n';
     out<<",\"records\":[";bool comma=false;
     for(const auto& node:imported.state.particles()){if(comma)out<<',';comma=true;out<<'[';initialized_record(out,imported.state,node,",");out<<']';}out<<']';
     double maximum_pair_residual=0;std::size_t active=0;
     for(const auto& pair:imported.subsystems){auto a=prepared.at(pair.hard[0]).particle.momentum,b=prepared.at(pair.hard[1]).particle.momentum;
      auto c=imported.state.at(pair.hard[0]).particle.momentum,d=imported.state.at(pair.hard[1]).particle.momentum;
      for(double residual:{a.px+b.px-c.px-d.px,a.py+b.py-c.py-d.py,a.pz+b.pz-c.pz-d.pz,a.energy+b.energy-c.energy-d.energy})maximum_pair_residual=std::max(maximum_pair_residual,std::abs(residual));
     }
     for(const auto& node:imported.state.particles())active+=node.particle.status==1;
     out<<",\"error\":null,\"pairs\":"<<result.pairs.size()<<",\"active_records\":"<<active<<",\"maximum_pair_p4_residual\":"<<maximum_pair_residual;
    }catch(const std::exception& error){out<<",\"error\":"<<std::quoted(error.what());}
    out<<",\"draws\":"<<init_random.checkpoint()<<'}';
   }
   out<<"}\n";
  }catch(const std::domain_error& error){out<<"{\"type\":\"failure\",\"attempt\":"<<attempt<<",\"stage\":"<<std::quoted(stage)<<",\"reason\":"<<std::quoted(error.what())<<"}\n";}
 }
 out<<"{\"type\":\"summary\",\"completed\":"<<completed<<"}\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 2;}}
