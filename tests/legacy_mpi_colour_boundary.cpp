#include "jewel/pythia8_hadronizer.hpp"
#include <cmath>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <limits>
#include <stdexcept>
void check(bool v){if(!v)throw std::runtime_error("MPI colour boundary contract failed");}
int main(int argc,char** argv){try{
 using namespace jewel;
 unsigned invalid=0;
 auto bad=[&](std::vector<LegacyMpiStringPiece> pieces,std::vector<int> types,int first=101){
  bool rejected=false;try{legacy_mpi_colour_tags(pieces,types,first);}catch(const std::domain_error&){rejected=true;}
  check(rejected);++invalid;
 };
 bad({{0,1}},{1,-1},0);bad({{0,1},{1,0}},{2,2},std::numeric_limits<int>::max());
 bad({{0,0}},{2});bad({{0,2}},{1,-1});bad({{0,1},{0,2}},{1,-1,-1});
 bad({{0,2},{1,2}},{1,1,-1});bad({},{1,-1});bad({{0,1}},{-1,1});
 bad({{0,1}},{0,0});bad({{0,1}},{3,-3});
 check(legacy_mpi_colour_tags({{0,1}},{1,-1},std::numeric_limits<int>::max())[0][0]==std::numeric_limits<int>::max());
 HadronizerConfig c;c.seed=2600571;c.fragmentation=FragmentationProfile::Jewel260Mapped;
 Pythia8Hadronizer h(c);unsigned successes=0,final_count=0;double max_residual=0;
 for(int mode=0;mode<4;++mode)for(int trial=0;trial<20;++trial){
  double mass=h.nominal_mass(mode==3?2101:1),energy=std::sqrt(10000+mass*mass);
  std::vector<Momentum> momenta={{0,0,100,energy},{0,0,-100,energy}};
  std::vector<int> ids=mode==3?std::vector<int>{2101,-2101}:std::vector<int>{1,-1};
  std::vector<LegacyMpiStringPiece> pieces=mode==3?std::vector<LegacyMpiStringPiece>{{1,0}}:std::vector<LegacyMpiStringPiece>{{0,1}};
  // Two sequential accepted pairs exercise attachment to previously added pieces.
  for(int pair_index=0;pair_index<2;++pair_index){
   double flavour=(mode==0||mode==3)?.1:mode==1?.92:.99;
   ReplayRandom random({.1,flavour,(trial+.3+pair_index*.2)/21.,.2});
   auto pair=sample_legacy_mpi_pair(1,1,200,.04,.2,.2,0,.9,.95,.2,2*std::acos(-1.),random);
   check(bool(pair));momenta.push_back(pair->a);momenta.push_back(pair->b);ids.push_back(pair->pdg_a);ids.push_back(pair->pdg_b);
   pieces=legacy_mpi_attach_strings(momenta,pieces,pair->flavour_draw,.9,pair->pdg_a);
  }
  std::vector<GeneratorParticle> particles;
  for(std::size_t i=0;i<momenta.size();++i){GeneratorParticle p;p.id=i+1;p.pdg=ids[i];p.final=true;p.status=23;p.momentum=momenta[i];p.mass=i<2?mass:0;particles.push_back(p);}
  auto result=h.hadronize_string_pieces(particles,pieces);check(bool(result));++successes;
  check(result->input_sources.size()==particles.size());
  std::array<double,4> before{},after{};
  for(const auto& p:particles){before[0]+=p.momentum.px;before[1]+=p.momentum.py;before[2]+=p.momentum.pz;before[3]+=p.momentum.energy;check(!p.colour&&!p.anticolour);}
  for(const auto& p:result->particles)if(p.final){++final_count;check(!p.colour&&!p.anticolour);after[0]+=p.momentum.px;after[1]+=p.momentum.py;after[2]+=p.momentum.pz;after[3]+=p.momentum.energy;}
  for(int j=0;j<4;++j){max_residual=std::max(max_residual,std::abs(after[j]-before[j]));check(std::abs(after[j]-before[j])<1e-7*before[3]);}
  if(trial==0&&mode==0){particles[0].final=false;bool rejected=false;try{h.hadronize_string_pieces(particles,pieces);}catch(const std::domain_error&){rejected=true;}check(rejected);++invalid;}
 }
 check(h.statistics().failed==0&&successes==80);
 std::ofstream file;if(argc>1){file.open(argv[1]);check(bool(file));}std::ostream& out=argc>1?file:std::cout;
 out<<std::setprecision(17)<<"{\"scope\":\"complete outgoing MPI string graph to Pythia8 tags and hadronization; no incoming legs, remnants or Fortran hadron comparison\",\"cases\":"<<successes<<",\"pairs_per_case\":2,\"final_particles\":"<<final_count<<",\"invalid_cases\":"<<invalid<<",\"max_four_momentum_residual_GeV\":"<<max_residual<<"}\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
