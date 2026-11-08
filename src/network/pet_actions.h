#pragma once
#include "network.h"
#include "gw_pet_lifecycle.h"
namespace gwserver {
// Bind only an authenticated character selected by the gateway. The World
// supplies the sealed current cast and prepares all publication before CAS.
inline fw::gw::pets::AdoptionCommit pet_adoption_commit(NetworkComponent& network,OwnedCharacter owner){
 return [&network,owner](const fw::gw::pets::AdoptionWrite& write){
  const auto result=network.adoptPet({owner,write.expected_revision,write.expected,write.replacement});
  if(result.status==FW_OK&&result.revision==write.expected_revision+1)return fw::gw::pets::CommitStatus::committed;
  return result.status==FW_CONFLICT?fw::gw::pets::CommitStatus::conflict:fw::gw::pets::CommitStatus::unavailable;
 };
}
}
