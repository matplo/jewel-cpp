#include <Pythia8/Pythia.h>
#include <iostream>
#include <iomanip>
int main(int argc,char** argv){Pythia8::Pythia p(argv[1],false);p.readString("Tune:pp = 14");p.readString("SigmaProcess:alphaSvalue = .118");p.readString("SigmaProcess:alphaSorder = 2");std::cout<<std::setprecision(17);
for(auto scope:{"SigmaProcess","SpaceShower","MultipartonInteractions"}){
 std::string s=scope;double value=p.settings.parm(s+":alphaSvalue");int order=p.settings.mode(s+":alphaSorder"),nf=p.settings.mode("StandardModel:alphaSnfmax");bool cmw=s=="SpaceShower"&&p.settings.flag(s+":alphaSuseCMW");
 std::cout<<"CONFIG "<<s<<' '<<value<<' '<<order<<' '<<nf<<' '<<cmw<<'\n';
 Pythia8::AlphaStrong a;a.init(value,order,nf,cmw);
 for(double q:{1.,2.,5.,10.,20.,50.,91.1876,100.,200.,1000.})std::cout<<"COUPLING "<<s<<' '<<q<<' '<<a.alphaS(q*q)<<'\n';
}
Pythia8::AlphaStrong matched;matched.init(0.11754434294459773,1,5,false);
for(double q:{1.,2.,5.,10.,20.,50.,91.1876,100.,200.,1000.})std::cout<<"MATCHED "<<q<<' '<<matched.alphaS(q*q)<<'\n';
}
