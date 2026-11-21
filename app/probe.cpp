// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/core.hpp"
#include <iostream>
#include <iomanip>
#include <string>
int main() {
 std::cout << std::setprecision(17);
 std::string op; int nf; double a,b,c;
 try {
 while(std::cin >> op >> nf >> a >> b >> c) {
  jewel::Kernels k; k.nf=nf; double value=0; std::size_t draws=0;
  if(op=="PQQ") value=k.pqq(a);
  else if(op=="PGQ") value=k.pgq(a);
  else if(op=="PGG") value=k.pgg(a);
  else if(op=="PQG") value=k.pqg(a);
  else if(op=="ALPHAS") value=k.alphas(a,b);
  else if(op=="INTPQQ") value=k.intpqq(a,b);
  else if(op=="INTPGGLOW") value=k.intpgglow(a,b);
  else if(op=="INTPGGHIGH") value=k.intpgghigh(a,b);
  else if(op=="GETT") {
   std::size_t n; if(!(std::cin>>n)||n>1000000) throw std::runtime_error("invalid tape size");
   std::vector<double> tape(n); for(auto& r:tape) if(!(std::cin>>r)) throw std::runtime_error("short tape");
   jewel::ReplayRandom random(tape); value=k.gett(a,b,c,random); draws=random.checkpoint();
  } else throw std::runtime_error("unknown probe");
  std::cout << value << ' ' << draws << '\n';
 }
 if(!std::cin.eof()) throw std::runtime_error("malformed input");
 } catch(const std::exception& e) { std::cerr << e.what() << '\n'; return 2; }
}
