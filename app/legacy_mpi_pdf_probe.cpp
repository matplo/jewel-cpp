#include "jewel/legacy_mpi_pdf.hpp"
#include <LHAPDF/LHAPDF.h>
#include <iostream>
#include <iomanip>
int main(int argc,char** argv){try{if(argc!=3&&argc!=4)return 2;LHAPDF::setVerbosity(0);
 std::optional<double> floor;if(argc==4)floor=std::stod(argv[3]);
 jewel::LegacyMpiPdf pdf(argv[1],std::stoi(argv[2]),floor);std::cout<<std::setprecision(17);
 int beam,nf;double x,remaining,q2,charm,bottom;
 while(std::cin>>beam>>x>>remaining>>q2>>nf>>charm>>bottom){
  auto values=pdf.evaluate(beam,x,remaining,q2,nf,charm,bottom);
  for(double v:values)std::cout<<v<<' ';std::cout<<'\n';
 }if(!std::cin.eof())return 2;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 2;}}
