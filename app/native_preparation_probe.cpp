// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/pythia8_shower_input.hpp"
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
int main(int argc,char** argv){try{
 if(argc!=5)throw std::runtime_error("usage: jewel-native-preparation OUTPUT FIXTURES COUNT SEED");
 int count=std::stoi(argv[3]),seed=std::stoi(argv[4]);if(count<=0)throw std::runtime_error("invalid count");
 if(std::filesystem::exists(argv[1])||std::filesystem::exists(argv[2]))throw std::runtime_error("outputs already exist");
 std::ofstream out(argv[1]),fixtures(argv[2]);if(!out||!fixtures)throw std::runtime_error("cannot open outputs");
 out<<std::setprecision(17);
 jewel::PythiaInputConfig config;config.seed=seed;config.initial_state_radiation=true;config.multiple_parton_interactions=true;
 jewel::Pythia8Input generator(config);unsigned completed=0;
 out<<"{\"type\":\"run\",\"seed\":"<<seed<<",\"attempts\":"<<count<<",\"eta_max\":2,\"order\":\"Pythia8 final source order\",\"settings\":[";
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
   out<<"]}\n";
  }catch(const std::domain_error& error){out<<"{\"type\":\"failure\",\"attempt\":"<<attempt<<",\"stage\":"<<std::quoted(stage)<<",\"reason\":"<<std::quoted(error.what())<<"}\n";}
 }
 out<<"{\"type\":\"summary\",\"completed\":"<<completed<<"}\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 2;}}
