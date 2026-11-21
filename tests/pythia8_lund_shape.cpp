#include "jewel/pythia8_hadronizer.hpp"
#include <Pythia8/Pythia.h>
#include <iostream>
#include <iomanip>
#include <stdexcept>
struct Probe: Pythia8::StringZ {
 void load(Pythia8::Settings& s){aLund=s.parm("StringZ:aLund");bLund=s.parm("StringZ:bLund");aExtraDiquark=s.parm("StringZ:aExtraDiquark");aExtraSQuark=s.parm("StringZ:aExtraSQuark");useOldAExtra=s.flag("StringZ:useOldAExtra");}
 void show(int old,int next,double mt){initFlav(old,next);initShape(mt);bool oldD=old>1000,newD=next>1000;
 if(aShape!=(newD?1.:.5)||bShape!=double(.6f)*mt||cShape!=1.-.5*oldD+.5*newD)throw std::runtime_error("Lund shape differs from frozen Pythia6 light-flavour coefficients");
 std::cout<<"SHAPE "<<old<<' '<<next<<' '<<mt<<' '<<aShape<<' '<<bShape<<' '<<cShape<<'\n';}
};
int main(int argc,char** argv){
 if(argc!=2)throw std::runtime_error("XML directory required");
 Pythia8::Pythia p(argv[1],false);p.readString("Tune:pp = 14");
 std::cout<<std::setprecision(17)<<"BEFORE "<<p.settings.parm("StringZ:aExtraDiquark")<<' '<<p.settings.flag("StringZ:useOldAExtra")<<'\n';
 for(const auto& s:jewel::jewel260_fragmentation_settings())if(!p.readString(s))throw std::runtime_error("setting rejected");
 if(p.settings.parm("StringZ:aExtraDiquark")!=.5||p.settings.parm("StringZ:aExtraSQuark")!=0||p.settings.flag("StringZ:useOldAExtra"))throw std::runtime_error("mapped Lund settings not effective");
 Probe probe;probe.load(p.settings);
 for(int old:{1,3,2103,3103})for(int next:{1,3,2103,3103})for(double mt:{.1,1.,4.})probe.show(old,next,mt);
}
