#include "Logger.hpp"
#include <set>
#include "Net/AsyncNet.hpp"
using namespace sim;

struct MyStructComparator {
    bool operator()(const sim::RefObject<net::Channel>& lhs, const sim::RefObject<net::Channel>& rhs) const {
        return lhs.c_get() < rhs.c_get();
    }
};


class TestPro:public net::Protocol
{
public:
    TestPro() {};
    virtual ~TestPro() {};

public:
    //链接事件，eConnResult 链接结果
    virtual void OnConnect(sim::RefObject<net::Channel> ch, net::EnumNetError eConnResult)
    {
        SIM_LDEBUG("TestPro: OnConnect " << (void*)ch.get() << " eConnResult: " << eConnResult);
        RefBuff stTempBuff(10*1024*1024);
        ch->StartRead(stTempBuff,true);

        RefBuff stWriteBuff(1024);
        snprintf(stWriteBuff.get(), stWriteBuff.size(), "%s", "9999999");
        ch->StartWrite(stWriteBuff);
    }

    //接受链接事件，ch_srv 接受链接的服务通道，ch 生成的链接
    //E_NET_ERROR_SUCCESS !EnumNetError 协议栈内部回收ch 拒绝链接
    virtual net::EnumNetError OnAccept(sim::RefObject<net::Channel> ch_srv, sim::RefObject<net::Channel> ch)
    { 
        SIM_LDEBUG("TestPro: OnAccept " << (void*)ch_srv.get() << " ch: " << (void*)ch.get());
        RefBuff stTempBuff(10*1024*1024);
        ch->StartRead(stTempBuff,true);
        m_chs.insert(ch);
        return net::E_NET_ERROR_SUCCESS; 
    }

    //链接关闭事件，eCloseResult 关闭原因
    virtual void OnClose(sim::RefObject<net::Channel> ch, net::EnumNetError eCloseResult) {
        SIM_LDEBUG("TestPro: OnClose " << (void*)ch.get() << " eCloseResult: " << eCloseResult);
        m_chs.erase(ch);
    };

    //收到报文,stIpAddr 来源地址
    virtual void OnReaded(sim::RefObject<net::Channel> ch, RefBuff& stBuff, UInt32 bytes_transfered, net::StruIpAddr stIpAddr)
    { 
        RefBuff stTempBuff(stBuff.get(), bytes_transfered);
        SIM_LDEBUG("TestPro: OnReaded " << (void*)ch.get() <<" bytes_transfered:"<< bytes_transfered/*<<" stBuff: " << stTempBuff.get()*/<<" stIpAddr:"<< stIpAddr.strIp<<":"<< stIpAddr.usPort);
        ch->StartWrite(stTempBuff);
        return ; 
    };

    //发送报文成功
    virtual void OnWrited(sim::RefObject<net::Channel> ch, RefBuff& stBuff, UInt32 bytes_transfered, net::EnumNetError eWriteResult)
    { 
        SIM_LDEBUG("TestPro: OnWrited " << (void*)ch.get() << " bytes_transfered:" << bytes_transfered/*<< " stBuff: " << stBuff.get()*/);
        return ; 
    };
private:
    tSet< sim::RefObject<net::Channel>, MyStructComparator > m_chs;
};


net::AsyncManager<sim::NetManager> myGnet;
int main(int argc, char* argv[])
{
    SIM_LOG_CONSOLE(sim::LDebug);

    myGnet.Init(6);

    sim::RefObject <net::Protocol> refPro(new TestPro());
    sim::RefObject<net::Channel> pSrv = myGnet.CreateChannel(SIM_NET_CHANNEL_TYPE_TCP, refPro);
    sim::RefObject<net::Channel> pChan=myGnet.CreateChannel(SIM_NET_CHANNEL_TYPE_TCP, refPro);

    net::StruIpAddr stBind;
    stBind.eType = sim::net::E_IP_ADDR_TYPE_IPV4;
    stBind.usPort = 6451;
    //stBind.strIp = "127.0.0.1";
    pSrv->Bind(stBind);
    pSrv->StartAccept();

    /*sim::String strHost = "www.baidu.com";
    tVector<net::StruIpAddr> vAddrs;
    if (net::SocketUtil::GetIpAddrList(strHost.c_str(), vAddrs,"http") || vAddrs.size() != 0)
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
    pChan->StartConnect(vAddrs[0]);*/

    stBind.strIp = "127.0.0.1";
    pChan->StartConnect(stBind);

    //net.Poll(10);

    //net.UnInit();
    getchar();
    return 0;
}