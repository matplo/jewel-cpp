#include "jewel/legacy_mpi_cross_section.hpp"
#include <Pythia8/StandardModel.h>
#include <LHAPDF/LHAPDF.h>
#include <iostream>
#include <fstream>
#include <iomanip>
int main(int argc,char** argv){try{
 if(argc!=3)return 2;LHAPDF::setVerbosity(0);std::cout<<std::setprecision(17);
 std::ifstream file(argv[1]);int count;double fudge;if(!(file>>fudge>>count)||count<0||count>400)return 2;
 std::vector<jewel::LegacyGluonDecay> decays(count);for(auto& d:decays)if(!(file>>d.mass_a>>d.mass_b>>d.secondary_weight>>d.mode))return 2;
 Pythia8::AlphaStrong alpha;alpha.init(std::stod(argv[2]),1,5,false);
 jewel::LegacyMpiEvolutionConfig e;
 while(std::cin>>e.ecm>>e.screening>>e.overlap_average>>e.enhancement>>e.bound>>e.sigma_nd>>e.attach_probability>>e.gluon_probability>>e.strange_weight>>e.two_pi){
  for(auto& c:e.phase_coefficients)if(!(std::cin>>c))return 2;
  jewel::LegacyMpiCrossSectionConfig c;c.ecm=e.ecm;c.screening_squared=e.screening;c.decays=decays;c.width_fudge=fudge;
  if(!(std::cin>>c.scale_shift>>c.renormalization_multiplier>>c.gev2_to_mb>>c.interference>>c.max_flavour>>c.beam_a>>c.beam_b))return 2;
  for(auto& m:c.masses)if(!(std::cin>>m))return 2;
  for(auto& on:c.enabled){int v;if(!(std::cin>>v)||(v!=0&&v!=1))return 2;on=v;}
  for(auto& v:c.coefficients)if(!(std::cin>>v))return 2;
  double floor;if(!(std::cin>>floor))return 2;
  jewel::LegacyMpiEvolutionState s;int n,nstr,done;
  if(!(std::cin>>s.upper_xt>>s.remaining_a>>s.remaining_b>>s.used_a>>s.used_b>>s.interactions>>s.additional_pairs>>s.sum_pt>>s.first_pt>>n>>nstr>>done)||n<0||nstr<0)return 2;
  s.stopped=done;s.momenta.resize(n);s.pdgs.resize(n);
  for(int i=0;i<n;++i){auto& p=s.momenta[i];if(!(std::cin>>s.pdgs[i]>>p.px>>p.py>>p.pz>>p.energy))return 2;}
  s.strings.resize(nstr);for(auto& piece:s.strings){int a,b;if(!(std::cin>>a>>b)||a<=0||b<=0)return 2;piece={std::size_t(a-1),std::size_t(b-1)};}
  int ntape;if(!(std::cin>>ntape)||ntape<0)return 2;std::vector<double> tape(ntape);for(auto& u:tape)if(!(std::cin>>u))return 2;
  jewel::ReplayRandom random(tape);jewel::LegacyMpiEvolution evolution(e,s);
  jewel::LegacyMpiCrossSection provider(c,"CT14nlo",0,floor,[&](double q){return alpha.alphaS(q);});
  auto emit=[&](){const auto& v=evolution.state();std::cout<<"STATE "<<random.checkpoint()<<' '<<v.upper_xt<<' '<<v.remaining_a<<' '<<v.remaining_b<<' '<<v.used_a<<' '<<v.used_b<<' '<<v.interactions<<' '<<v.additional_pairs<<' '<<v.sum_pt<<' '<<v.first_pt<<' '<<v.momenta.size()<<' '<<v.strings.size()<<' '<<v.stopped;
   for(std::size_t i=0;i<v.momenta.size();++i){auto p=v.momenta[i];std::cout<<' '<<v.pdgs[i]<<' '<<p.px<<' '<<p.py<<' '<<p.pz<<' '<<p.energy;}
   for(auto piece:v.strings)std::cout<<' '<<piece.first+1<<' '<<piece.second+1;std::cout<<'\n';};
  emit();int iterations=0;
  while(!evolution.state().stopped){
   if(++iterations>100000)return 3;
   evolution.step(random,[&](double xt,const auto& phase,const auto& state){double sigma=provider(xt,phase,state);std::cout<<"SIGMA "<<sigma<<'\n';return sigma;});emit();
  }
  if(random.checkpoint()!=tape.size())return 4;std::cout<<"END\n";
 }
 if(!std::cin.eof())return 2;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 2;}}
