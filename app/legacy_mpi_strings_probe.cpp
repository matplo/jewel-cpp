#include "jewel/legacy_mpi.hpp"
#include <iostream>
int main(){try{
 int n,k,pdg;double flavour,attach;
 while(std::cin>>n>>k>>flavour>>attach>>pdg){
  if(n<0||n>998||k<0||k>998)return 2;
  std::vector<jewel::Momentum> points(n+2);
  for(auto& p:points)if(!(std::cin>>p.px>>p.py>>p.pz>>p.energy))return 2;
  std::vector<jewel::LegacyMpiStringPiece> pieces(k);
  for(auto& s:pieces){int a,b;if(!(std::cin>>a>>b)||a<=0||b<=0)return 2;s={std::size_t(a-1),std::size_t(b-1)};}
  auto out=jewel::legacy_mpi_attach_strings(points,pieces,flavour,attach,pdg);
  std::cout<<out.size();for(auto s:out)std::cout<<' '<<s.first+1<<' '<<s.second+1;std::cout<<'\n';
 }if(!std::cin.eof())return 2;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 2;}}
