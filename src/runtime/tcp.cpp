#include "tcp.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdexcept>
namespace fw::tcp {
namespace {
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
Socket create(){Socket value(uintptr_t(socket(AF_INET,SOCK_STREAM,IPPROTO_TCP)));check(bool(value),"TCP socket creation");return value;}
void nonblocking(uintptr_t socket){u_long value=1;check(!ioctlsocket(SOCKET(socket),FIONBIO,&value),"TCP nonblocking mode");}
}
Socket::~Socket(){if(*this)closesocket(SOCKET(value_));}
Runtime::Runtime(){WSADATA data{};check(!WSAStartup(MAKEWORD(2,2),&data),"TCP Winsock startup");}
Runtime::~Runtime(){WSACleanup();}
Listener::Listener(const std::string& address,uint16_t port):socket_(create()) {
    sockaddr_in local{};local.sin_family=AF_INET;local.sin_port=htons(port);
    check(inet_pton(AF_INET,address.c_str(),&local.sin_addr)==1,"TCP bind requires an IPv4 address");
    BOOL exclusive=TRUE;
    check(!setsockopt(SOCKET(socket_.get()),SOL_SOCKET,SO_EXCLUSIVEADDRUSE,reinterpret_cast<char*>(&exclusive),sizeof(exclusive)),"TCP exclusive bind");
    if(bind(SOCKET(socket_.get()),reinterpret_cast<sockaddr*>(&local),sizeof(local))){
        const auto error=WSAGetLastError();
        throw std::runtime_error("TCP listener bind "+address+":"+std::to_string(port)+
            " failed (Winsock "+std::to_string(error)+")");
    }
    check(!listen(SOCKET(socket_.get()),16),"TCP listener backlog");nonblocking(socket_.get());
    int length=sizeof(local);check(!getsockname(SOCKET(socket_.get()),reinterpret_cast<sockaddr*>(&local),&length),"TCP listener address");
    port_=ntohs(local.sin_port);
}
Socket Listener::accept(std::string& address,uint32_t wait_ms) {
    check(wait_ms<=1000,"TCP accept wait bound");
    fd_set read;FD_ZERO(&read);FD_SET(SOCKET(socket_.get()),&read);
    timeval timeout{long(wait_ms/1000),long(wait_ms%1000*1000)};
    int ready=select(0,&read,nullptr,nullptr,&timeout);check(ready>=0,"TCP accept wait");if(!ready)return Socket{};
    sockaddr_in peer{};int length=sizeof(peer);
    Socket result(uintptr_t(::accept(SOCKET(socket_.get()),reinterpret_cast<sockaddr*>(&peer),&length)));
    if(!result){if(WSAGetLastError()==WSAEWOULDBLOCK)return Socket{};throw std::runtime_error("TCP accept failed");}
    char text[INET_ADDRSTRLEN]{};check(inet_ntop(AF_INET,&peer.sin_addr,text,sizeof(text))!=nullptr,"TCP peer address");
    address=text;return result;
}
Socket connect(const std::string& host,uint16_t port,uint32_t timeout_ms) {
    check(!host.empty() && host.size()<=253 && port && timeout_ms && timeout_ms<=10000,"TCP connection parameters");
    addrinfo hints{};hints.ai_family=AF_INET;hints.ai_socktype=SOCK_STREAM;hints.ai_protocol=IPPROTO_TCP;
    addrinfo* raw=nullptr;auto service=std::to_string(port);
    check(!getaddrinfo(host.c_str(),service.c_str(),&hints,&raw) && raw,"TCP DNS resolution failed");
    std::unique_ptr<addrinfo,decltype(&freeaddrinfo)> addresses(raw,freeaddrinfo);
    auto end=GetTickCount64()+timeout_ms;
    for(auto* address=raw;address;address=address->ai_next) {
        Socket result=create();nonblocking(result.get());
        if(!::connect(SOCKET(result.get()),address->ai_addr,int(address->ai_addrlen)))return result;
        if(WSAGetLastError()!=WSAEWOULDBLOCK)continue;
        auto now=GetTickCount64();if(now>=end)break;
        fd_set write,error;FD_ZERO(&write);FD_ZERO(&error);FD_SET(SOCKET(result.get()),&write);FD_SET(SOCKET(result.get()),&error);
        timeval timeout{long((end-now)/1000),long((end-now)%1000*1000)};
        if(select(0,nullptr,&write,&error,&timeout)<=0)continue;
        int status=0,length=sizeof(status);
        if(!getsockopt(SOCKET(result.get()),SOL_SOCKET,SO_ERROR,reinterpret_cast<char*>(&status),&length) && !status)return result;
    }
    throw std::runtime_error("TCP connection failed or timed out");
}
}
