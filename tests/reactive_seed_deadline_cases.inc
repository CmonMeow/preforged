// Included after the reactive Seed fixture definitions.
unsigned reactive_seed_deadline_cases(){
 unsigned cases=0;
 for(unsigned target_id:{2u,3u,4u})for(int rr:{-1,0,12,20})for(bool late:{false,true}){
  ++cases;world::Instance w;const unsigned monk_id=target_id==2?3:2;
  for(unsigned id:{1u,monk_id,target_id}){w.join(id,"Actor",id==1?0:id==monk_id?100:60,0,std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);w.set_vitals(id,{id==1?1000u:500u,1000,100,100},100);}
  const auto foe=*w.effect_scope(1),monk=*w.effect_scope(monk_id),target=*w.effect_scope(target_id);
  check(w.party_action(target_id,158,monk_id,100)&&w.party_action(monk_id,154,target_id,100));check(w.complete_healing_seed(monk,target,*effects::healing_seed(12),0,100));if(rr>=0)check(w.status_mutation(target,307,900,rr,100,8100,true));
  check(w.degeneration_hex(foe,target,*effects::degeneration_hex(19,12),100));check(w.apply_condition(foe,target,combat::conditions::Kind::blind,200,1000));const auto before=w.resources(target_id)->health,ally=w.resources(monk_id)->health;
  if(!late){w.tick(1100);w.tick(1200);}w.tick(2000);
  if(w.resources(target_id)->health!=before-17+25||w.resources(monk_id)->health!=ally+25)throw std::runtime_error("Seed deadline order target="+std::to_string(target_id)+" rr="+std::to_string(rr)+" late="+std::to_string(late)+" before="+std::to_string(before)+" actual="+std::to_string(w.resources(target_id)->health)+" ally_before="+std::to_string(ally)+" ally="+std::to_string(w.resources(monk_id)->health));
  w.tick(2100);check(w.resources(target_id)->health==before-17+25);
 }
 for(unsigned rank:{0u,12u,20u})for(bool late:{false,true})for(bool reverse:{false,true}){
  ++cases;world::Instance w;
  for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",id==1?0:id==2?60:100,0,std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);w.set_vitals(id,{id==1?1000u:500u,1000,100,100},100);}
  const auto foe=*w.effect_scope(1),a=*w.effect_scope(2),b=*w.effect_scope(3);check(w.party_action(2,158,3,100)&&w.party_action(3,154,2,100));
  check(w.complete_healing_seed(reverse?a:b,reverse?b:a,*effects::healing_seed(rank),0,100));check(w.complete_healing_seed(reverse?b:a,reverse?a:b,*effects::healing_seed(rank),0,100));check(w.degeneration_hex(foe,a,*effects::degeneration_hex(19,12),100));
  check(w.apply_condition(foe,a,combat::conditions::Kind::blind,200,1000)&&w.apply_condition(foe,b,combat::conditions::Kind::blind,200,1000));
  if(!late){w.tick(1100);w.tick(1200);}w.tick(2000);const auto wanted=500+4*effects::healing_seed(rank)->healing-34;check(w.resources(2)->health==wanted&&w.resources(3)->health==wanted);w.tick(2100);check(w.resources(2)->health==wanted&&w.resources(3)->health==wanted);
 }
 for(unsigned skill:{37u,3373u,44u,2054u})for(unsigned target_id:{2u,3u})for(bool late:{false,true}){
  ++cases;world::Instance w;const unsigned monk_id=target_id==2?3:2;
  for(unsigned id:{1u,monk_id,target_id}){w.join(id,"Actor",id==1?0:id==monk_id?100:60,0,std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);w.set_vitals(id,{id==1?1000u:500u,1000,100,100},100);}
  const auto foe=*w.effect_scope(1),monk=*w.effect_scope(monk_id),target=*w.effect_scope(target_id);check(w.party_action(target_id,158,monk_id,100)&&w.party_action(monk_id,154,target_id,100));check(w.degeneration_hex(foe,target,*effects::degeneration_hex(19,12),100));
  unsigned boundary=5100,wanted=508;
  if(skill==37||skill==3373){check(w.complete_healing_seed(monk,target,*effects::healing_seed(12),0,100));check(w.status_mutation(target,skill,950,0,100,5100,true));}
  else{boundary=10100;if(skill==44){check(w.wound_spell(foe,target,*effects::wound_spell(44,0),20,100));wanted=500-20*effects::wound_spell(44,0)->degeneration-100-17+20;}else{check(w.degeneration_hex(foe,target,*effects::degeneration_hex(2054,0),100));wanted=500-20*effects::degeneration_hex(2054,0)->pips-17+25;}check(w.complete_healing_seed(monk,target,*effects::healing_seed(12),0,200));}
  if(!late)w.tick(boundary);w.tick(boundary+100);if(w.resources(target_id)->health!=wanted||w.resources(monk_id)->health!=525)throw std::runtime_error("Seed generated-condition deadline skill="+std::to_string(skill)+" target="+std::to_string(target_id)+" late="+std::to_string(late)+" hp="+std::to_string(w.resources(target_id)->health)+" expected="+std::to_string(wanted));
 }
 check(cases==52);return cases;
}
