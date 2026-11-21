// SPDX-License-Identifier: GPL-2.0-or-later
// Recoil storage section of DOKINEMATICS, JEWEL 2.6.0.
#include "jewel/recoil.hpp"
#include <cmath>
#include <stdexcept>
namespace jewel {
void store_recoil(RecoilStorage& storage,RecoilStorageConfig config,int flavour,
 Momentum incoming,double mass,Momentum outgoing,bool soft,bool rejected){
 if(config.mode<0||config.mode>3)throw std::domain_error("invalid recoil mode");
 if(storage.used>storage.capacity||storage.used>storage.records.size())
  throw std::domain_error("invalid recoil storage bounds");
 if(!(config.write_centres||config.subtract)||rejected||storage.used==storage.capacity)return;
 // Stock increments NSCATCEN even when modes 0/1 skip a soft recoil.
 auto slot=storage.used++;
 if(storage.records.size()<=slot)storage.records.resize(slot+1);
 if(soft&&(config.mode==0||config.mode==1))return;
 auto& record=storage.records[slot];record.flavour=flavour;
 bool transfer=config.mode==2||(config.mode==3&&soft);
 record.centre=transfer?Momentum{outgoing.px-incoming.px,outgoing.py-incoming.py,
  outgoing.pz-incoming.pz,outgoing.energy-incoming.energy}:incoming;
 const auto& p=record.centre;
 double square=p.energy*p.energy-p.px*p.px-p.py*p.py-p.pz*p.pz;
 record.centre_mass=transfer?std::copysign(std::sqrt(std::abs(square)),square):mass;
 Momentum direction=config.mode==1?outgoing:record.centre;
 if(direction.energy==0)throw std::domain_error("zero energy in recoil dummy projection");
 constexpr double dummy_energy=1.e-6;
 record.dummy={direction.px*dummy_energy/direction.energy,direction.py*dummy_energy/direction.energy,
  direction.pz*dummy_energy/direction.energy,dummy_energy};
 const auto& d=record.dummy;
 double dummy_square=transfer?d.px*d.px+d.py*d.py+d.pz*d.pz-d.energy*d.energy:
  d.energy*d.energy-d.px*d.px-d.py*d.py-d.pz*d.pz;
 if(dummy_square<0||!std::isfinite(dummy_square))throw std::domain_error("nonphysical recoil dummy projection");
 record.dummy_mass=(transfer?-1.:1.)*std::sqrt(dummy_square);
 if(transfer&&p.energy<0){record.dummy.px*=-1;record.dummy.py*=-1;record.dummy.pz*=-1;}
}
}
