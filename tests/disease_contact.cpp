#include "gw_disease_contact.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw::combat::conditions;
void check(bool value){if(!value)throw std::runtime_error("Disease contact regression");}
int main()try{
 DiseaseContactActor a{{1,1,1},0,0,0,true,"human",true,false},b{{1,2,1},166,0,0,true,"human",true,false};
 const Entry disease{1000,1,100,1};
 check(disease_contact(a,b,disease,100)==DiseaseContact::eligible);
 auto changed=b;changed.x=166.001;check(disease_contact(a,changed,disease,100)==DiseaseContact::no_contact);
 changed=b;changed.type="norn";check(disease_contact(a,changed,disease,100)==DiseaseContact::no_contact);
 changed=b;changed.type.reset();check(disease_contact(a,changed,disease,100)==DiseaseContact::unknown);
 changed=b;changed.fleshy.reset();check(disease_contact(a,changed,disease,100)==DiseaseContact::unknown);
 changed=b;changed.immune.reset();check(disease_contact(a,changed,disease,100)==DiseaseContact::unknown);
 changed=b;changed.fleshy=false;changed.type.reset();check(disease_contact(a,changed,disease,100)==DiseaseContact::no_contact);
 changed=b;changed.immune=true;check(disease_contact(a,changed,disease,100)==DiseaseContact::no_contact);
 changed=b;changed.plane=1;check(disease_contact(a,changed,disease,100)==DiseaseContact::no_contact);
 changed=b;changed.alive=false;check(disease_contact(a,changed,disease,100)==DiseaseContact::no_contact);
 changed=b;changed.scope.instance=2;check(disease_contact(a,changed,disease,100)==DiseaseContact::no_contact);
 changed=b;changed.scope.generation=0;check(disease_contact(a,changed,disease,100)==DiseaseContact::invalid);
 changed=b;changed.x=std::nan("");check(disease_contact(a,changed,disease,100)==DiseaseContact::invalid);
 changed=b;changed.type="Human";check(disease_contact(a,changed,disease,100)==DiseaseContact::invalid);
 check(disease_contact(a,a,disease,100)==DiseaseContact::no_contact);
 check(disease_contact(a,b,disease,99)==DiseaseContact::no_contact&&disease_contact(a,b,disease,1000)==DiseaseContact::no_contact);
 auto invalid=disease;invalid.cause=0;check(disease_contact(a,b,invalid,100)==DiseaseContact::invalid);
 State clean;
 const auto first=disease_transfer(a,b,disease,clean,400);
 check(first.contact==DiseaseContact::eligible&&first.changed);
 check(first.state.lookup(Kind::disease)->expires==1000&&first.state.lookup(Kind::disease)->applied==400);
 check(first.state.lookup(Kind::disease)->cause==1&&!clean.has(Kind::disease,400));
 auto covered=first.state;check(covered.apply(Kind::blind,2,450,2000));
 const auto same=disease_transfer(a,b,disease,covered,500);
 check(!same.changed&&same.state.newest(500)==Kind::blind);
 check(same.state.lookup(Kind::disease)->order==covered.lookup(Kind::disease)->order);
 auto longer=clean;check(longer.apply(Kind::disease,3,200,2000));
 const auto retained=disease_transfer(a,b,disease,longer,500);
 check(!retained.changed&&retained.state.lookup(Kind::disease)->expires==2200&&retained.state.lookup(Kind::disease)->cause==3);
 auto shorter=clean;check(shorter.apply(Kind::disease,3,200,400));
 const auto extended=disease_transfer(a,b,disease,shorter,500);
 check(extended.changed&&extended.state.lookup(Kind::disease)->expires==1000);
 auto c=b;c.scope.actor=3;c.x=165;
 const auto chained=disease_transfer(b,c,*first.state.lookup(Kind::disease),clean,900);
 check(chained.changed&&chained.state.lookup(Kind::disease)->expires==1000);
 check(!disease_transfer(b,c,*first.state.lookup(Kind::disease),clean,1000).changed);
 auto unknown=b;unknown.type.reset();
 const auto refused=disease_transfer(a,unknown,disease,clean,500);
 check(refused.contact==DiseaseContact::unknown&&!refused.changed&&!refused.state.has(Kind::disease,500));
 State infection_a;check(infection_a.apply(Kind::disease,1,100,900));
 State infection_b;check(infection_b.apply(Kind::disease,2,100,1500));
 auto near_a=a,near_b=b,destination=c;near_a.x=0;near_b.x=0;destination.x=100;
 std::vector<DiseasePopulationActor> population{{near_a,infection_a},{near_b,infection_b},{destination,clean}};
 auto batch=disease_contacts(population,500);check(batch&&batch->transmissions.size()==2);
 const auto chosen=batch->transmissions.back();
 check(chosen.recipient==destination.scope&&chosen.source==near_b.scope&&chosen.after.lookup(Kind::disease)->expires==1600);
 std::reverse(population.begin(),population.end());
 const auto reversed=disease_contacts(population,500);
 check(reversed&&reversed->transmissions.size()==batch->transmissions.size());
 for(unsigned i=0;i<batch->transmissions.size();++i){
  check(batch->transmissions[i].source==reversed->transmissions[i].source&&batch->transmissions[i].recipient==reversed->transmissions[i].recipient);
 }
 // No instantaneous A -> B -> C chain: C is outside A's adjacency.
 near_b.x=160;destination.x=320;
 population={{near_a,infection_a},{near_b,clean},{destination,clean}};
 batch=disease_contacts(population,500);check(batch&&batch->transmissions.size()==1&&batch->transmissions[0].recipient==near_b.scope);
 population[1].conditions=batch->transmissions[0].after;
 batch=disease_contacts(population,600);check(batch&&batch->transmissions.size()==1&&batch->transmissions[0].recipient==destination.scope&&batch->transmissions[0].after.lookup(Kind::disease)->expires==1000);
 population.push_back(population.front());check(!disease_contacts(population,600));
 population.pop_back();population.back().actor.type.reset();
 batch=disease_contacts(population,600);check(batch&&batch->unknown_contacts==1&&batch->transmissions.empty());
 std::cout<<"PASS Disease contact boundary,type/flesh/immunity unknowns,planes,lifetimes and exact infection expiry; remaining-duration proposals and contact-chain expiry; World propagation pending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
