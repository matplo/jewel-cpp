// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/pythia8_hadronizer.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
void check(bool value){if(!value)throw std::runtime_error("Pythia8 boundary contract failed");}
int main(int argc,char** argv){
 try{
  unsigned attempts=argc>1?static_cast<unsigned>(std::stoul(argv[1])):20;
  if(attempts==0||attempts>100000)throw std::invalid_argument("invalid attempt count");
  jewel::PythiaInputConfig config;config.seed=2600317;jewel::Pythia8Input input(config);
  jewel::HadronizerConfig hconfig;hconfig.seed=2600318;
  jewel::Pythia8Hadronizer hadronizer(hconfig),repeat(hconfig);
  unsigned hadronized=0;std::uint64_t final_particles=0;double max_residual=0;
  for(unsigned i=0;i<attempts;++i){auto snapshot=input.next();if(!snapshot)continue;
   std::vector<jewel::GeneratorParticle> partons;
   for(const auto& p:snapshot->particles)if(p.final)partons.push_back(p);
   auto a=hadronizer.hadronize(partons,snapshot->junctions),b=repeat.hadronize(partons,snapshot->junctions);
   check(a.has_value()==b.has_value());if(!a)continue;++hadronized;
   check(a->input_sources==b->input_sources&&a->particles.size()==b->particles.size());
   check(a->input_sources.size()==partons.size());
   for(std::size_t j=0;j<partons.size();++j)check(a->input_sources[j]==partons[j].id);
   std::array<double,4> total{},out{};
   for(const auto& p:partons){total[0]+=p.momentum.px;total[1]+=p.momentum.py;total[2]+=p.momentum.pz;total[3]+=p.momentum.energy;}
   for(std::size_t j=0;j<a->particles.size();++j){const auto& p=a->particles[j];const auto& q=b->particles[j];
    check(p.pdg==q.pdg&&p.status==q.status&&p.mothers==q.mothers);
    check(p.momentum.px==q.momentum.px&&p.momentum.py==q.momentum.py&&p.momentum.pz==q.momentum.pz&&p.momentum.energy==q.momentum.energy);
    if(!p.final)continue;++final_particles;check(!p.colour&&!p.anticolour);
    out[0]+=p.momentum.px;out[1]+=p.momentum.py;out[2]+=p.momentum.pz;out[3]+=p.momentum.energy;
   }
   for(int j=0;j<4;++j)max_residual=std::max(max_residual,std::abs(total[j]-out[j]));
  }
  auto s=input.statistics();auto h=hadronizer.statistics();check(s.attempts==attempts&&h.attempts==s.returned&&h.returned==hadronized&&hadronized>0);
  // Malformed kinematics must throw; an open colour line must fail without
  // inventing an endpoint or hiding the failed hadronization attempt.
  jewel::Pythia8Hadronizer negative(hconfig);
  jewel::GeneratorParticle orphan;orphan.id=1;orphan.pdg=1;orphan.colour=101;orphan.momentum={0,0,10,10};
  orphan.mass=1;bool rejected=false;
  try{negative.hadronize({orphan});}catch(const std::domain_error&){rejected=true;}check(rejected);
  orphan.mass=0;check(!negative.hadronize({orphan}));
  check(negative.statistics().attempts==1&&negative.statistics().failed==1);
  std::ostream* output=&std::cout;std::ofstream file;
  if(argc>2){file.open(argv[2]);if(!file)throw std::runtime_error("cannot write report");output=&file;}
  *output<<std::setprecision(17)<<"{\"scope\":\"Pythia8 adapter closure only; no JEWEL evolution or Fortran physics comparison\",\"pythia_version\":"<<input.version()<<",\"attempts\":"<<attempts<<",\"hard_snapshots\":"<<s.returned<<",\"hard_failures\":"<<s.failed<<",\"hadronized\":"<<hadronized<<",\"hadronization_failures\":"<<h.failed<<",\"final_particles\":"<<final_particles<<",\"max_four_momentum_residual_GeV\":"<<max_residual<<",\"sigma_mb\":"<<s.sigma_mb<<",\"sigma_error_mb\":"<<s.sigma_error_mb<<",\"weight_sum\":"<<s.weight_sum<<",\"hadronizer_settings\":[";
  bool first=true;for(const auto& setting:hadronizer.settings()){if(!first)*output<<',';first=false;*output<<std::quoted(setting);}*output<<"]}\n";
  return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
