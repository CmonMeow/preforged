#pragma once
#include "gw_identification_catalog.h"
#include "gw_salvage_catalog.h"
#include "gw_consumables.h"
#include <cstdlib>
#include <utility>
namespace fw::gw::items {
struct CatalogPaths {
    std::optional<std::filesystem::path> identification,salvage,consumables;
    static CatalogPaths environment(){CatalogPaths p;if(auto s=std::getenv("FW_GW_IDENTIFICATION_CATALOG"))p.identification=s;if(auto s=std::getenv("FW_GW_SALVAGE_CATALOG"))p.salvage=s;if(auto s=std::getenv("FW_GW_CONSUMABLE_CATALOG"))p.consumables=s;return p;}
};
// Host-owned snapshot. Loading finishes before listeners start; sessions only
// receive const providers. Changes to environment/files require a new host.
class Catalogs {
    const CatalogPaths paths_;
    identification::Catalog identification_;
    salvage::Catalog salvage_;
    consumables::Catalog consumables_;
public:
    explicit Catalogs(CatalogPaths paths):paths_(std::move(paths)){
        if(paths_.identification)try{identification_.load(*paths_.identification);}
            catch(const std::exception& e){throw std::invalid_argument("Identification catalog "+paths_.identification->string()+": "+e.what());}
        if(paths_.salvage)try{salvage_.load(*paths_.salvage);}
            catch(const std::exception& e){throw std::invalid_argument("Salvage catalog "+paths_.salvage->string()+": "+e.what());}
        if(paths_.consumables)try{consumables_.load(*paths_.consumables);}catch(const std::exception& e){throw std::invalid_argument("Consumable catalog "+paths_.consumables->string()+": "+e.what());}
    }
    const consumables::Catalog& consumables()const{return consumables_;}
    const identification::Catalog& identification()const{return identification_;}
    const salvage::Catalog& salvage()const{return salvage_;}
    const CatalogPaths& paths()const{return paths_;}
};
}
