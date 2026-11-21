#include "jewel/legacy_mpi_cross_section.hpp"
#include <Pythia8/StandardModel.h>
#include <LHAPDF/LHAPDF.h>
#include <iostream>
#include <fstream>
#include <iomanip>
int main(int argc,char** argv){try{
 if(argc!=6)return 2;
 LHAPDF::setVerbosity(0);double floor=std::stod(argv[4]);Pythia8::AlphaStrong alpha;alpha.init(std::stod(argv[5]),1,5,false);
 std::ifstream decay_file(argv[3]);int count;double fudge;if(!(decay_file>>fudge>>count)||count<0||count>400)return 2;
 std::vector<jewel::LegacyGluonDecay> decays(count);for(auto& d:decays)if(!(decay_file>>d.mass_a>>d.mass_b>>d.secondary_weight>>d.mode))return 2;
 std::cout<<std::setprecision(17);jewel::LegacyMpiCrossSectionConfig c;double xt,a,b,pt_cut;jewel::LegacyMpiPhaseSpace phase;
 while(std::cin>>c.ecm>>c.screening_squared>>c.scale_shift>>c.renormalization_multiplier>>c.gev2_to_mb>>c.k_factor>>c.asymmetric_factor>>c.interference>>c.max_flavour>>c.beam_a>>c.beam_b
  >>xt>>phase.tau>>phase.rapidity>>phase.cos_theta>>a>>b>>pt_cut){
  c.kinematic_pt_cut=pt_cut<0?std::nullopt:std::optional<double>(pt_cut);
  for(auto& v:c.coefficients)if(!(std::cin>>v))return 2;
  for(auto& v:c.masses)if(!(std::cin>>v))return 2;
  for(auto& v:c.enabled){int value;if(!(std::cin>>value)||(value!=0&&value!=1))return 2;v=value;}
  c.decays=decays;c.width_fudge=fudge;
  jewel::LegacyMpiCrossSection provider(c,argv[1],std::stoi(argv[2]),floor,[&](double q){return alpha.alphaS(q);});
  auto r=provider.evaluate(xt,phase,a,b);
  std::cout<<r.s<<' '<<r.t<<' '<<r.u<<' '<<r.q2<<' '<<r.alpha_s<<' '<<r.common_factor<<' '<<r.annihilation_weight;
  for(auto pdf:{r.pdf_a,r.pdf_b})for(double v:pdf)std::cout<<' '<<v;
  std::cout<<' '<<r.channels.channels.size()<<' '<<r.channels.total<<'\n';
  for(auto channel:r.channels.channels)std::cout<<channel.incoming_a<<' '<<channel.incoming_b<<' '<<channel.code<<' '<<channel.matrix_weight<<' '<<channel.pdf_weighted<<'\n';
 }if(!std::cin.eof())return 2;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 2;}}
