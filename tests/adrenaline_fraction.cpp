#include "gw_adrenaline.h"
#include <iostream>
#include <source_location>
#include <string>
using namespace fw::gw;
void check(bool b,std::source_location at=std::source_location::current()){if(!b)throw std::runtime_error("Adrenaline fraction line "+std::to_string(at.line()));}
int main()try{
 skills::Slots bar{};bar[0]={335,0};bar[1]={336,0};bar[2]={335,1};
 combat::adrenaline::State state;check(state.configure(bar,100));
 check(state.gain_halves(75,101));for(const auto& p:state.pools())check(p.units==37&&p.half==1);
 check(!state.ready({335,0},101));check(state.gain_halves(75,102));for(const auto& p:state.pools())check(p.units==75&&!p.half);
 check(!state.ready({335,0},102));check(state.gain_halves(9,103));for(const auto& p:state.pools())check(p.units==79&&p.half==1);
 check(!state.ready({335,0},103));check(state.gain_halves(1,104));check(state.ready({335,0},104));
 check(state.spend({335,0},105));check(state.pools()[0].units==0&&!state.pools()[0].half);
 check(state.pools()[1].units==55&&!state.pools()[1].half&&state.pools()[2].units==55);
 state.reset();check(state.gain_halves(51,106));check(state.gain(1,107));for(const auto& p:state.pools())check(p.units==26&&p.half==1);
 auto reordered=bar;std::swap(reordered[0],reordered[1]);check(state.configure(reordered,108));
 for(const auto& p:state.pools())check(p.units==26&&p.half==1);
 check(state.disable({335,0},200,109));check(state.gain_halves(75,110));
 check(state.pools()[1].units==0&&!state.pools()[1].half);check(state.pools()[0].units==64&&!state.pools()[0].half);
 check(state.gain_halves(1,200));check(state.pools()[1].units==0&&state.pools()[1].half==1);
 // Spending one skill subtracts25 exactly from other pools,including25.5.
 state.reset();check(state.configure(bar,201));check(state.disable({336,0},203,201));
 check(state.gain(80,202));check(state.gain_halves(51,203));
 check(state.spend({335,0},204));check(state.pools()[0].units==0&&!state.pools()[0].half);
 check(state.pools()[1].units==0&&state.pools()[1].half==1);check(state.pools()[2].units==55&&!state.pools()[2].half);
 const auto saved=state.pools();auto invalid=bar;invalid[3]=invalid[0];check(!state.configure(invalid,204));
 check(state.pools()[1].units==saved[1].units&&state.pools()[1].half==saved[1].half);
 state.reset();check(state.gain_halves(UINT64_MAX,205));for(const auto& p:state.pools())check(p.units==p.cost&&!p.half);
 check(state.spend_all({335,1},206));for(const auto& p:state.pools())check(p.units==0&&!p.half);
 check(state.gain_halves(1,207));state.tick(25206);for(const auto& p:state.pools())check(p.half==1);
 state.tick(25207);for(const auto& p:state.pools())check(p.units==0&&!p.half);
 check(state.gain_halves(1,26000));check(state.configure(bar,51000));for(const auto& p:state.pools())check(p.units==0&&!p.half);
 // Damage gains retain fractional bonuses after the existing base quantization.
 state.reset();check(state.damage(10,1000,52000,3));
 for(const auto& p:state.pools())check(p.units==1&&p.half==1);
 check(state.damage(10,1000,52001,3));for(const auto& p:state.pools())check(p.units==3&&!p.half);
 check(!state.damage(10,1000,52002,5));for(const auto& p:state.pools())check(p.units==3&&!p.half);
 check(state.damage(10,1000,52003));for(const auto& p:state.pools())check(p.units==4&&!p.half);
 // Wild Blow-style loss clears every fractional/copy pool without readiness,
 // does not clear disable deadlines,and rejects historical requests atomically.
 state.reset();check(state.configure(bar,53000));check(state.gain_halves(75,53001));
 check(state.disable({336,0},54000,53002));const auto before_loss=state.pools();
 check(!state.lose_all(53001));for(unsigned i=0;i<before_loss.size();++i)check(state.pools()[i].units==before_loss[i].units&&state.pools()[i].half==before_loss[i].half);
 check(state.lose_all(53003));for(const auto& p:state.pools())check(!p.units&&!p.half);
 check(state.pools()[1].disabled_until==54000);check(state.gain_halves(1,53004));
 check(state.pools()[0].half==1&&!state.pools()[1].half&&state.pools()[2].half==1);
 state.tick(78003);check(state.pools()[0].half==1);state.tick(78004);check(!state.pools()[0].half);
 check(state.lose_all(78005));
 // Wire format is unchanged and conservatively publishes whole units.
 combat::adrenaline::Pool fractional{{335,0},80,37,0,1};auto whole=fractional;whole.half=0;
 check(combat::adrenaline::snapshot_2022(25,fractional)==combat::adrenaline::snapshot_2022(25,whole));
 bool rejected=false;fractional.half=2;try{(void)combat::adrenaline::snapshot_2022(25,fractional);}catch(const std::invalid_argument&){rejected=true;}check(rejected);
 std::cout<<"PASS fractional adrenaline exact gain,copy/reorder preservation,caps,spend/disable/reset/expiry,overflow-safe saturation and unchanged whole-unit wire format\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
