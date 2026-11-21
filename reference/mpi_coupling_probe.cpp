#include <Pythia8/Pythia.h>
#include <iostream>
#include <iomanip>
int main(){Pythia8::Pythia p("/Users/ploskon/devel/omni-sbi/jest-a/.venv/hepyy_packages/pythia8/8.317/share/Pythia8/xmldoc",false);
if(!p.readString("Tune:pp = 14"))return 2;
if(!p.readString("Beams:idA = 2212"))return 2;
if(!p.readString("Beams:idB = 2212"))return 2;
if(!p.readString("Beams:eCM = 5020"))return 2;
if(!p.readString("HardQCD:all = on"))return 2;
if(!p.readString("PhaseSpace:pTHatMin = 20"))return 2;
if(!p.readString("PhaseSpace:pTHatMax = 100"))return 2;
if(!p.readString("PDF:pSet = LHAPDF6:CT14nlo/0"))return 2;
if(!p.readString("HadronLevel:all = off"))return 2;
if(!p.readString("PartonLevel:FSR = off"))return 2;
if(!p.readString("PartonLevel:ISR = on"))return 2;
if(!p.readString("PartonLevel:MPI = on"))return 2;
if(!p.readString("Check:event = on"))return 2;
if(!p.readString("Random:setSeed = on"))return 2;
if(!p.readString("Random:seed = 2600421"))return 2;
if(!p.readString("Init:showChangedSettings = off"))return 2;
if(!p.readString("Init:showChangedParticleData = off"))return 2;
if(!p.readString("Init:showProcesses = off"))return 2;
if(!p.readString("Next:numberShowInfo = 0"))return 2;
if(!p.readString("Next:numberShowProcess = 0"))return 2;
if(!p.readString("Next:numberShowEvent = 0"))return 2;
if(!p.readString("SigmaProcess:alphaSvalue = 0.11754434294459773"))return 2;
if(!p.readString("SigmaProcess:alphaSorder = 1"))return 2;
if(!p.readString("SpaceShower:alphaSvalue = 0.11754434294459773"))return 2;
if(!p.readString("SpaceShower:alphaSorder = 1"))return 2;
if(!p.readString("StandardModel:alphaSnfmax = 5"))return 2;
if(!p.readString("SpaceShower:alphaSuseCMW = off"))return 2;
if(!p.init())return 3;
std::cout<<std::setprecision(17);
for(auto name:{"MultipartonInteractions:alphaSvalue","MultipartonInteractions:pT0Ref","MultipartonInteractions:ecmRef","MultipartonInteractions:ecmPow","MultipartonInteractions:expPow","MultipartonInteractions:coreFraction","MultipartonInteractions:coreRadius"})std::cout<<"MPI_NATIVE "<<name<<" "<<p.settings.parm(name)<<"\n";
for(auto name:{"MultipartonInteractions:alphaSorder","MultipartonInteractions:bProfile","MultipartonInteractions:pT0parametrization","StandardModel:alphaSnfmax"})std::cout<<"MPI_NATIVE "<<name<<" "<<p.settings.mode(name)<<"\n";
Pythia8::AlphaStrong translated,current;translated.init(.11754434294459773,1,5,false);current.init(p.settings.parm("MultipartonInteractions:alphaSvalue"),p.settings.mode("MultipartonInteractions:alphaSorder"),5,false);
double q;while(std::cin>>q)std::cout<<"MPI_VALUE "<<q<<" "<<translated.alphaS(q)<<" "<<current.alphaS(q)<<"\n";
}
