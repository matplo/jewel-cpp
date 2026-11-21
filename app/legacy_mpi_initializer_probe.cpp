#include "jewel/legacy_mpi_cross_section.hpp"
#include <Pythia8/StandardModel.h>
#include <LHAPDF/LHAPDF.h>
#include <iostream>
#include <fstream>
#include <iomanip>
int main(int argc,char** argv){try{
 if(argc!=3)return 2;LHAPDF::setVerbosity(0);std::cout<<std::setprecision(17);
 std::ifstream file(argv[1]);int n;double fudge;if(!(file>>fudge>>n)||n<0||n>400)return 2;
 std::vector<jewel::LegacyGluonDecay> decays(n);for(auto& d:decays)if(!(file>>d.mass_a>>d.mass_b>>d.secondary_weight>>d.mode))return 2;
 Pythia8::AlphaStrong alpha;alpha.init(std::stod(argv[2]),1,5,false);
 int samples;
 while(std::cin>>samples){
  jewel::LegacyMpiCrossSectionConfig c;double core,radius,sigma_nd,floor;
  if(!(std::cin>>c.ecm>>c.screening_squared>>core>>radius>>sigma_nd>>c.scale_shift>>c.renormalization_multiplier>>c.gev2_to_mb>>c.interference>>c.max_flavour>>c.beam_a>>c.beam_b))return 2;
  for(auto& v:c.masses)if(!(std::cin>>v))return 2;
  for(auto& v:c.enabled){int on;if(!(std::cin>>on)||(on!=0&&on!=1))return 2;v=on;}
  for(auto& v:c.coefficients)if(!(std::cin>>v))return 2;
  if(!(std::cin>>floor))return 2;c.decays=decays;c.width_fudge=fudge;
  int count;if(!(std::cin>>count)||count<0)return 2;std::vector<double> tape(count);for(auto& u:tape)if(!(std::cin>>u))return 2;
  jewel::ReplayRandom random(tape);jewel::LegacyMpiCrossSection provider(c,"CT14nlo",0,floor,[&](double q){return alpha.alphaS(q);});
  auto result=jewel::integrate_legacy_mpi(c.screening_squared,samples,{c.coefficients[0],c.coefficients[2],c.coefficients[3]},random,
   [&](double xt,const auto& phase){double value=provider.evaluate(xt,phase,1,1).channels.total;
    std::cout<<"SAMPLE "<<xt<<' '<<phase.tau<<' '<<phase.rapidity<<' '<<phase.cos_theta<<' '<<value<<'\n';return value;});
  for(int i=0;i<20;++i)std::cout<<"BIN "<<i+1<<' '<<result.counts[i]<<' '<<result.sums[i]<<'\n';
  std::cout<<"INTEGRAL "<<result.integral<<' '<<random.checkpoint()<<'\n';
  if(result.integral>=1.1*sigma_nd){jewel::LegacyMpiOverlap model(core,radius,result.integral,sigma_nd);auto s=model.state();
   std::cout<<"OVERLAP "<<s.integral<<' '<<s.overlap_average<<' '<<s.overlap_normalization<<' '<<s.mean_impact<<' '<<s.dividing_impact<<' '<<s.k_factor<<'\n';}
  if(random.checkpoint()!=tape.size())return 3;std::cout<<"END\n";
 }
 if(!std::cin.eof())return 2;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 2;}}
