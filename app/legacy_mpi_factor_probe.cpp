#include "jewel/legacy_mpi.hpp"
#include <iostream>
#include <iomanip>
int main(){try{
 std::cout<<std::setprecision(17);double s,xt,tau,y,screen,units,k;std::array<double,5>c;
 while(std::cin>>s>>xt>>tau>>y>>screen>>units>>k){
  for(double& x:c)if(!(std::cin>>x))return 2;
  std::cout<<jewel::legacy_mpi_common_factor(s,xt,tau,y,screen,c,units,k)<<'\n';
 }
 if(!std::cin.eof())return 2;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 2;}}
