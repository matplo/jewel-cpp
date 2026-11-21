// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/pythia8_hadronizer.hpp"
#include <cmath>
#include <map>
#include <stdexcept>
void check(bool x){if(!x)throw std::runtime_error("fragmentation profile contract failed");}
int main(){
 std::map<std::string,double> values;
 for(const auto& line:jewel::jewel260_fragmentation_settings()){
  if(line=="StringZ:useOldAExtra = off")continue;
  auto split=line.find(" = ");values[line.substr(0,split)]=std::stod(line.substr(split+3));
 }
 for(auto entry:{std::pair<const char*,double>{"StringFlav:mesonUDvector",double(.31f)},
   {"StringFlav:mesonSvector",double(.4f)},{"StringFlav:mesonCvector",double(.54f)},
   {"StringFlav:mesonBvector",double(.54f)}}){auto r=values.at(entry.first);check(std::abs(r/(1+r)-entry.second)<1e-15);}
 check(values.at("StringZ:rFactC")==1&&values.at("StringZ:rFactB")==double(.67f));
 jewel::HadronizerConfig config;config.fragmentation=jewel::FragmentationProfile::Jewel260Mapped;
 jewel::Pythia8Hadronizer hadronizer(config);
 for(int flavour:{1,3,4,5})for(int i=0;i<5;++i){
  double mass=flavour==1?.33:flavour==3?.5:flavour==4?1.5:4.8;
  jewel::GeneratorParticle a;a.id=1;a.pdg=flavour;a.final=true;a.colour=101;a.mass=mass;
  a.momentum={20.+i,0,0,std::sqrt((20.+i)*(20.+i)+mass*mass)};
  auto b=a;b.id=2;b.pdg=-flavour;b.colour=0;b.anticolour=101;b.momentum.px=-a.momentum.px;
  auto result=hadronizer.hadronize({a,b});check(result.has_value());
 }
 check(hadronizer.statistics().attempts==20&&hadronizer.statistics().returned==20);
 // JEWEL PI0DEC changes the neutral pion's final-state survival.
 for(bool decay:{false,true}){
  auto c=config;c.pi0_decay=decay;jewel::Pythia8Hadronizer h(c);
  jewel::GeneratorParticle p;p.id=1;p.pdg=111;p.final=true;p.mass=h.nominal_mass(111);
  p.momentum={0,0,0,p.mass};auto out=h.hadronize({p});check(out.has_value());
  int final_pions=0,final_particles=0;double energy=0;
  for(const auto& q:out->particles)if(q.final){final_pions+=q.pdg==111;++final_particles;energy+=q.momentum.energy;}
  check(final_pions==(decay?0:1)&&final_particles>=(decay?2:1));
  check(std::abs(energy-p.mass)<1e-12);
 }

}
