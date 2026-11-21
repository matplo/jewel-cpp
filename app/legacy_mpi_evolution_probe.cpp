#include "jewel/legacy_mpi.hpp"
#include <iostream>
#include <iomanip>
int main(){try{std::cout<<std::setprecision(17);
 jewel::LegacyMpiEvolutionConfig c;jewel::LegacyMpiEvolutionState s;int steps;
 while(std::cin>>c.ecm>>s.upper_xt>>c.screening>>c.overlap_average>>c.enhancement>>c.bound>>c.sigma_nd
  >>s.remaining_a>>s.remaining_b>>c.attach_probability>>c.gluon_probability>>c.strange_weight>>c.two_pi>>s.interactions>>steps){
  for(auto& v:c.phase_coefficients)if(!(std::cin>>v))return 2;
  s.used_a=s.used_b=s.sum_pt=s.first_pt=0;s.additional_pairs=0;s.stopped=false;
  s.momenta={{0,0,10,10},{0,0,-10,10}};s.pdgs={1,-1};s.strings={{0,1}};
  jewel::LegacyMpiEvolution evolution(c,s);
  for(int i=0;i<steps;++i){double sigma;if(!(std::cin>>sigma))return 2;
   std::vector<double> tape(10);for(auto& u:tape)if(!(std::cin>>u))return 2;
   jewel::ReplayRandom random(tape);int calls=0;
   auto status=evolution.step(random,[&](double,const auto&,const auto&){++calls;return sigma;});
   const auto& v=evolution.state();
   std::cout<<int(status)<<' '<<random.checkpoint()<<' '<<calls<<' '<<v.upper_xt<<' '<<v.remaining_a<<' '<<v.remaining_b<<' '
    <<v.used_a<<' '<<v.used_b<<' '<<v.interactions<<' '<<v.additional_pairs<<' '<<v.sum_pt<<' '<<v.first_pt<<' '
    <<v.momenta.size()<<' '<<v.strings.size()<<' '<<v.stopped;
   for(std::size_t j=0;j<v.momenta.size();++j){auto p=v.momenta[j];std::cout<<' '<<v.pdgs[j]<<' '<<p.px<<' '<<p.py<<' '<<p.pz<<' '<<p.energy;}
   for(auto piece:v.strings)std::cout<<' '<<piece.first+1<<' '<<piece.second+1;
   std::cout<<'\n';
  }
 }if(!std::cin.eof())return 2;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 2;}}
