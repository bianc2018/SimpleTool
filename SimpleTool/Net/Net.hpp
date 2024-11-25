/*
* ÍøÂç½Ó¿ÚÀà
*/
#ifndef SIM_NET_HPP_
#define SIM_NET_HPP_
#include "Types.hpp"

#ifdef OS_WINDOWS
#include "Impl/Win/Iocp/NetIocp.hpp"
#endif // OS_WINDOWS

#ifdef OS_LINUX
#include "Impl/Linux/Epoll/NetEpoll.hpp"
#endif // OS_LINUX

namespace sim
{
#ifdef OS_WINDOWS
       typedef net::IocpManager NetManager ;
#endif // OS_WINDOWS
#ifdef OS_LINUX
       typedef net::EpollManager NetManager;
#endif // OS_WINDOWS

}
#endif //!SIM_NET_BASE_HPP_