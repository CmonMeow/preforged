#pragma once
#include <functional>
#include <utility>
namespace fw::gw::pets {
// Runtime owns this lease, never the client. Capture only the exact receipt
// and a safe World lifetime handle. Cleanup must not emit client packets.
class CastLease {
    std::function<void()> cancel_;
public:
    explicit CastLease(std::function<void()> cancel):cancel_(std::move(cancel)){}
    CastLease(const CastLease&)=delete;
    CastLease& operator=(const CastLease&)=delete;
    CastLease(CastLease&& other)noexcept:cancel_(std::exchange(other.cancel_,{})){}
    CastLease& operator=(CastLease&&)=delete;
    ~CastLease()noexcept {if(cancel_)try{cancel_();}catch(...){}}
    // Use only after the exact attempt was terminally consumed by World.
    void consumed()noexcept {cancel_=nullptr;}
};
}
