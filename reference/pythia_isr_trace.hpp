// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <Pythia8/Pythia.h>
struct RecordedIsr {int system,mother,recoiler,emitted;Pythia8::RotBstMatrix matrix;Pythia8::Vec4 sister;std::array<double,10> inputs;Pythia8::RotBstMatrix from_rest;Pythia8::Vec4 sister_rest;Pythia8::RotBstMatrix rest_boost;};
extern std::vector<RecordedIsr> captured;
inline void record_isr(int system,int mother,int recoiler,int emitted,Pythia8::RotBstMatrix matrix,const Pythia8::Event& event,double q2,double z,double m2,double x1,double x2,double nx1,double nx2,double pt2,double phi,Pythia8::RotBstMatrix from_rest,Pythia8::Vec4 sister_rest,Pythia8::RotBstMatrix rest_boost,double theta){captured.push_back({system,mother,recoiler,emitted,matrix,event[emitted].p(),{q2,z,m2,x1,x2,nx1,nx2,pt2,phi,theta},from_rest,sister_rest,rest_boost});}
