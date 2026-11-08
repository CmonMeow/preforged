#include "gw_skill_casting.h"
#include <stdexcept>
static void check(bool value){if(!value)throw std::runtime_error("captured dual follow-up47 wire");}
int main(){
 const std::vector<std::uint8_t> expected{158,0,47,0,0,0,25,0,0,0,0,0,0,0};
 check(fw::gw::skills::casting::terminal(47,25)==expected);
 for(unsigned kind:{46u,49u,58u,59u})check(fw::gw::skills::casting::terminal(kind,25).size()==14);
 for(unsigned kind:{0u,45u,48u,50u,57u,60u}){
  bool refused=false;try{fw::gw::skills::casting::terminal(kind,25);}catch(const std::invalid_argument&){refused=true;}check(refused);
 }
}
