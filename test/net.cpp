#include "Test.hpp"
#include "Net/AsyncNet.hpp"
#include "Net/Protocol/SimpleProtocol.hpp"
using namespace sim;

SIM_TEST(TcpIPv4)
{
	net::TypeNetChannel typeflag = SIM_NET_CHANNEL_TYPE_TCP;
	net::AsyncManager<NetManager> myGnet;
	SIM_ASSERT_IS_EQUAL(net::E_NET_ERROR_SUCCESS, myGnet.Init(1));

	sim::RefObject <net::Protocol> refSrvPro(new net::TCPSrvProtocol<net::SimpleProtocol>());
	sim::RefObject <net::SimpleProtocol> refCliPro(new net::SimpleProtocol());
	sim::RefObject<net::Channel> pSrv = myGnet.CreateChannel(typeflag, refSrvPro);
	sim::RefObject<net::Channel> pChan = myGnet.CreateChannel(typeflag,sim::reinterpret_pointer_cast<net::Protocol>(refCliPro));
	SIM_ASSERT_IS_TRUE(pSrv);
	SIM_ASSERT_IS_TRUE(pChan);

	//服务绑定
	net::StruIpAddr stBind;
	stBind.eType = sim::net::E_IP_ADDR_TYPE_IPV4;
	stBind.usPort = 6451;
	SIM_ASSERT_IS_EQUAL(net::E_NET_ERROR_SUCCESS, pSrv->Bind(stBind));
	SIM_ASSERT_IS_EQUAL(net::E_NET_ERROR_SUCCESS, pSrv->StartAccept());

	//链接
	stBind.strIp = "127.0.0.1";
	SIM_ASSERT_IS_EQUAL(net::E_NET_ERROR_SUCCESS, pChan->StartConnect(stBind));

	//等待链接完成
	net::EnumNetError eConnResult = net::E_NET_ERROR_TIMEOUT;
	SIM_ASSERT_IS_TRUE(refCliPro->WaitConnect(eConnResult,5000));
	SIM_ASSERT_IS_EQUAL(net::E_NET_ERROR_SUCCESS, eConnResult);

	//发送请求
	net::SimplePack reqpack1;
	reqpack1.version = 1;
	reqpack1.type = 1;
	reqpack1.seq = 1;
	reqpack1.data = RefBuff("sadhajksdhwkadjkhwkd");
	SIM_ASSERT_IS_EQUAL(net::E_NET_ERROR_SUCCESS, refCliPro->Send(pChan, reqpack1, NULL));
	
	//等待
	net::SimplePack respack1;
	SIM_ASSERT_IS_TRUE(refCliPro->WaitRes(reqpack1.seq, respack1, 5000));

	//比较
	SIM_ASSERT_IS_EQUAL(reqpack1.version, respack1.version);
	SIM_ASSERT_IS_EQUAL((UInt8)2, respack1.type);
	SIM_ASSERT_IS_EQUAL(reqpack1.seq, respack1.seq);
	SIM_ASSERT_IS_EQUAL(reqpack1.data.size(), respack1.data.size());
	for(int i=0;i< respack1.data.size();++i)
		SIM_ASSERT_IS_EQUAL(reqpack1.data.c_get()[i], respack1.data.c_get()[i]);

	myGnet.UnInit();
}

SIM_TEST_MAIN(sim::noisy)