#include "gw_development_commands.h"
#include <iostream>
int main()try{
 using namespace fw::gw::development;auto check=[](bool b,const char* why){if(!b)throw std::runtime_error(why);};
 check(parse(u"/.GM").kind==Kind::gm&&parse(u"/.gm").requires_authorization()&&parse(u"/gm").kind==Kind::unknown,"onlydottedGM command admitted");
 check(parse(u"/.gm on").kind==Kind::invalid&&parse(u"/.die Player").kind==Kind::invalid,"toggle/selfdeath no invented targetargument");
 auto rename=parse(u"/setname Cmon Meow = New Character");check(rename.kind==Kind::setname&&rename.target==u"Cmon Meow"&&rename.text==u"New Character"&&rename.requires_authorization()&&rename.declaration->tier==Tier::private_extension,"private targetedrename preserves exactnames andauthorization requirement");
 check(parse(u"/setname 42 = Chinese \u4e2d\u6587").kind==Kind::setname,"unicode names andnumericactorselector remaintyped");
 for(auto text:{u"/setname = Empty",u"/setname Player =",u"/setname Player New Name",u"/setname A = B = C",u"/setname A = Twenty Characters Name"})check(parse(text).kind==Kind::invalid,"ambiguous/empty/long rename denied");
 std::u16string surrogate=u"/setname Player = ";surrogate.push_back(0xd800);check(parse(surrogate).kind==Kind::invalid,"malformedUTF16rename denied");
 auto pet=parse(u"/namepet Pet Name");check(pet.kind==Kind::namepet&&pet.text==u"Pet Name"&&pet.target.empty()&&!pet.requires_authorization(),"ordinarypetrename neverresolvesotherplayer");
 check(parse(u"/petname").kind==Kind::namepet&&parse(u"/petname").text.empty(),"empty petname resetsdefault");
 check(parse(u"/.exp 50000").numbers[0]==50000&&parse(u"/.exp 50001").kind==Kind::invalid,"wikiexperience bound");
 check(parse(u"/.faction 1 10000").numbers==std::array<uint32_t,2>{1,10000}&&parse(u"/.faction 2 10000").kind==Kind::invalid,"wikiFactiondomain");
 check(parse(u"/.titlepoints 42 4294967295").kind==Kind::title_points&&parse(u"/.titlepoints 43 0").kind==Kind::invalid&&parse(u"/.titlepoints 1 4294967296").kind==Kind::invalid,"titleindex andintegeroverflow bounds");
 check(parse(u"/.guildtrim 9").kind==Kind::guild_trim&&parse(u"/.guildtrim 4").kind==Kind::invalid,"wiki guildtrim sparsevalues");
 check(parse(u"/.item Foo").declaration->tier==Tier::staging_unknown&&parse(u"/.monitor").declaration->tier==Tier::undocumented,"unknown effects neverpromoted toverified");
 check(parse(u"/% Hello World").text==u"Hello World","devchat bodypreserved");
 for(std::size_t i=0;i<declarations.size();++i)for(std::size_t j=i+1;j<declarations.size();++j)check(declarations[i].keyword!=declarations[j].keyword,"declarationkeywords unique");
 std::cout<<"development commands PASS\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
