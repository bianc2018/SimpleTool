/*
* demo ≤‚ ‘SocketUtil
*/
#include "Net/SocketUtil.hpp"
#include "Logger.hpp"
using namespace sim;
int main(int argc, char* argv[])
{
    SIM_LOG_CONSOLE(sim::LDebug);

    //sim::net::SocketUtil::Init();
    sim::String strHost = "www.baidu.com";
    tVector<net::StruIpAddr> vAddrs;
    if (net::SocketUtil::GetIpAddrList(strHost.c_str(), vAddrs))
    {
        SIM_LINFO("Get IpAddr:" << strHost << " size:" << vAddrs.size());
        for (int i = 0; i < vAddrs.size(); ++i)
        {
            SIM_LINFO("Get IpAddr:" << vAddrs[i].strIp << " type:" << vAddrs[i].eType);
        }
    }
    else
    {
        SIM_LINFO("Get IpAddr error");
        return -1;
    }
    return 0;
}