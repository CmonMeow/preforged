#include "gw_shrine_sites.h"
#include <iostream>
#include <stdexcept>
#include <string>
#include <source_location>
using namespace fw::gw::shrine;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("shrine site prerequisite line "+std::to_string(at.line()));
}
int main()try{
    const Evidence fixture{Basis::fixture,"synthetic geometry only; no real shrine coordinate"};
    const auto geometry=[](Place p){return valid(p)&&std::abs(p.x)<=100&&std::abs(p.y)<=100&&p.cell==0&&p.plane<=1;};
    Site first{1,{10,0,0,0},fixture,{}};SiteCatalog production(42,7),catalog(42,7,true);
    check(production.review(first,geometry)==Review::rejected);
    check(catalog.review(first,geometry)==Review::destination_only&&!catalog.has_enabled());
    const Scope a{1,1,4},b{1,2,7};const Party party{1,1,5};Snapshot facts{party,{{a,true},{b,false}},100,1};
    const Attempt wipe{1,party,{a,b},1000,11000};SelectionOrigin origin{a,{0,0,0,0},{Basis::authored,"explicit synthetic nearest-origin/tie-by-ID policy"}};
    SiteActivation activated;check(activated.observe(catalog,facts,a,{0,0,0,0}));auto state=*activated.snapshot(party);
    check(prepare_site_recovery(catalog,state,wipe,origin,1).decision==SiteDecision::activation_unreviewed);
    const auto older=catalog;first.activation=Activation{ActivationKind::approach,Place{0,0,0,0},5,"",fixture};
    check(catalog.review(first,geometry)==Review::enabled&&catalog.has_enabled());
    check(activated.observe(catalog,facts,a,{10,0,0,0}));state=*activated.snapshot(party);
    check(state.sites.empty()&&prepare_site_recovery(catalog,state,wipe,origin,1).decision==SiteDecision::return_to_last_outpost_required);
    check(!activated.observe(older,facts,a,{0,0,0,0}));
    check(!activated.observe(catalog,facts,b,{0,0,0,0}));check(!activated.observe(catalog,facts,{1,3,1},{0,0,0,0}));
    ++facts.at;check(activated.observe(catalog,facts,a,{0,0,1,0}));check(activated.snapshot(party)->sites.empty());
    ++facts.at;check(activated.observe(catalog,facts,a,{3,0,0,0}));state=*activated.snapshot(party);check(state.sites==std::set<unsigned>{1});
    check(activated.observe(catalog,facts,a,{3,0,0,0})&&activated.snapshot(party)->revision==state.revision);
    const auto ready=prepare_site_recovery(catalog,state,wipe,origin,9);check(ready.decision==SiteDecision::ready&&ready.plan->site==1);
    check(current_site_plan(*ready.plan,catalog,state,wipe,9)&&!current_site_plan(*ready.plan,catalog,state,wipe,10));
    auto stale=wipe;++stale.serial;check(!current_site_plan(*ready.plan,catalog,state,stale,9));
    auto outsider=origin;outsider.source={1,3,1};check(prepare_site_recovery(catalog,state,wipe,outsider,9).decision==SiteDecision::invalid);
    Site bad=first;bad.destination.x=200;const auto revision=catalog.stamp();check(catalog.review(bad,geometry)==Review::rejected&&catalog.stamp()==revision);
    bad=first;bad.destination_evidence={Basis::authored,"invented destination forbidden"};check(catalog.review(bad,geometry)==Review::rejected);
    bad=first;bad.activation->evidence={};check(catalog.review(bad,geometry)==Review::rejected);
    Site second{2,{-10,0,0,0},fixture,Activation{ActivationKind::entry,{},0,"entrance-A",fixture}};
    check(catalog.review(second,geometry)==Review::enabled);++facts.at;
    check(activated.observe(catalog,facts,a,{3,0,0,0},"entrance-A"));state=*activated.snapshot(party);check(state.sites==std::set<unsigned>({1,2}));
    const auto tied=prepare_site_recovery(catalog,state,wipe,origin,10);check(tied.plan&&tied.plan->site==1);
    check(!current_site_plan(*ready.plan,catalog,state,wipe,9));
    auto forged=*tied.plan;forged.site=2;forged.destination=second.destination;check(!current_site_plan(forged,catalog,state,wipe,10));
    auto historical=facts;--historical.at;check(!activated.observe(catalog,historical,a,{0,0,0,0}));
    ++facts.party.epoch;++facts.at;check(activated.observe(catalog,facts,a,{50,0,0,0}));
    check(!activated.snapshot(party)&&activated.snapshot(facts.party)->sites.empty());
    SiteCatalog unavailable(42,8,true);SiteActivation empty;check(empty.observe(unavailable,facts,a,{0,0,0,0}));
    auto new_wipe=wipe;new_wipe.party=facts.party;
    check(prepare_site_recovery(unavailable,*empty.snapshot(facts.party),new_wipe,origin,1).decision==SiteDecision::catalog_unavailable);
    std::cout<<"Reviewed destination/activation separation, exact party activation and stale recovery-site plan rejection passed\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
