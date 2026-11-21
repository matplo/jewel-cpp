// SPDX-License-Identifier: GPL-2.0-or-later
#include "Pythia8/TraceSpaceShower.h"
#include "Pythia8/SimpleTimeShower.h"
#include "pythia_isr_trace.hpp"
#include <fstream>
#include <iomanip>
std::vector<RecordedIsr> captured;
class TraceModel:public Pythia8::SimpleShowerModel{
public:bool init(Pythia8::MergingPtr m,Pythia8::MergingHooksPtr h,Pythia8::PartonVertexPtr,Pythia8::WeightContainer*)override{
subObjects.clear();mergingPtr=m;if(m)registerSubObject(*m);mergingHooksPtr=h;if(h)registerSubObject(*h);
timesPtr=timesDecPtr=std::make_shared<Pythia8::SimpleTimeShower>();registerSubObject(*timesPtr);
spacePtr=std::make_shared<Pythia8::TraceSpaceShower>();registerSubObject(*spacePtr);return true;}};
int main(int argc,char**argv){if(argc!=5)return 2;Pythia8::Pythia p(argv[4],false);
std::ifstream settings(argv[1]);std::string line;while(std::getline(settings,line))if(!p.readString(line))return 3;
p.setShowerModelPtr(std::make_shared<TraceModel>());if(!p.init())return 4;
std::ofstream out(argv[2]);out<<std::setprecision(17);for(int attempt=1;attempt<=std::stoi(argv[3]);attempt++){captured.clear();if(!p.next())continue;
for(auto& r:captured){if(r.emitted>=p.event.size())continue;auto s=p.event[r.emitted].p();if(std::abs(p.event[r.emitted].status())!=43||s.px()!=r.sister.px()||s.py()!=r.sister.py()||s.pz()!=r.sister.pz())continue;
out<<"{\"attempt\":"<<attempt<<",\"system\":"<<r.system<<",\"mother\":"<<r.mother+1<<",\"recoiler\":"<<r.recoiler+1<<",\"emitted\":"<<r.emitted+1<<",\"inputs\":[";for(int i=0;i<8;++i){if(i)out<<',';out<<r.inputs[i];}out<<"],\"matrix\":[";for(int i=0;i<16;++i){if(i)out<<',';out<<r.matrix.value(i/4,i%4);}out<<"]}\n";
}
out<<"{\"attempt\":"<<attempt<<",\"particles\":[";for(int i=0;i<p.event.size();i++){if(i)out<<',';auto v=p.event[i];out<<'['<<v.id()<<','<<v.status()<<','<<v.px()<<','<<v.py()<<','<<v.pz()<<','<<v.e()<<']';}out<<"]}\n";
}return out?0:5;}
