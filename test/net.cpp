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
	tVector<net::SimplePack> vReq;
	
	net::SimplePack reqpack;
	reqpack.version = 1;
	reqpack.type = 1;
	reqpack.seq = 1;
	reqpack.data = RefBuff(1);
	SIM_ASSERT_IS_EQUAL(net::E_NET_ERROR_SUCCESS, refCliPro->Send(pChan, reqpack, NULL));
	vReq.push_back(reqpack);

	reqpack.seq ++;
	reqpack.data = RefBuff(2);
	SIM_ASSERT_IS_EQUAL(net::E_NET_ERROR_SUCCESS, refCliPro->Send(pChan, reqpack, NULL));
	vReq.push_back(reqpack);

	reqpack.seq++;
	reqpack.data = RefBuff(4);
	SIM_ASSERT_IS_EQUAL(net::E_NET_ERROR_SUCCESS, refCliPro->Send(pChan, reqpack, NULL));
	vReq.push_back(reqpack);

	reqpack.seq++;
	reqpack.data = RefBuff(1024);
	SIM_ASSERT_IS_EQUAL(net::E_NET_ERROR_SUCCESS, refCliPro->Send(pChan, reqpack, NULL));
	vReq.push_back(reqpack);

	reqpack.seq++;
	reqpack.data = RefBuff(65535);
	SIM_ASSERT_IS_EQUAL(net::E_NET_ERROR_SUCCESS, refCliPro->Send(pChan, reqpack, NULL));
	vReq.push_back(reqpack);

	reqpack.seq++;
	reqpack.data = RefBuff(65535*100);
	SIM_ASSERT_IS_EQUAL(net::E_NET_ERROR_SUCCESS, refCliPro->Send(pChan, reqpack, NULL));
	vReq.push_back(reqpack);
	//等待
	for (int i = 0; i < vReq.size(); ++i)
	{
		net::SimplePack &reqpack1 = vReq[i];
		net::SimplePack respack1;
		SIM_ASSERT_IS_TRUE(refCliPro->WaitRes(reqpack1.seq, respack1, 5000));

		//比较
		SIM_ASSERT_IS_EQUAL(reqpack1.version, respack1.version);
		SIM_ASSERT_IS_EQUAL((UInt8)2, respack1.type);
		SIM_ASSERT_IS_EQUAL(reqpack1.seq, respack1.seq);
		SIM_ASSERT_IS_EQUAL(reqpack1.data.size(), respack1.data.size());
		/*for (int i = 0; i < respack1.data.size(); ++i)
			SIM_ASSERT_IS_EQUAL(reqpack1.data.c_get()[i], respack1.data.c_get()[i]);*/
	}

	myGnet.UnInit();
}

SIM_TEST(TcpIPv6)
{
	net::TypeNetChannel typeflag = SIM_NET_CHANNEL_TYPE_TCP| SIM_NET_CHANNEL_TYPE_IPV6;
	net::AsyncManager<NetManager> myGnet;
	SIM_ASSERT_IS_EQUAL(net::E_NET_ERROR_SUCCESS, myGnet.Init(1));

	sim::RefObject <net::Protocol> refSrvPro(new net::TCPSrvProtocol<net::SimpleProtocol>());
	sim::RefObject <net::SimpleProtocol> refCliPro(new net::SimpleProtocol());
	sim::RefObject<net::Channel> pSrv = myGnet.CreateChannel(typeflag, refSrvPro);
	sim::RefObject<net::Channel> pChan = myGnet.CreateChannel(typeflag, sim::reinterpret_pointer_cast<net::Protocol>(refCliPro));
	SIM_ASSERT_IS_TRUE(pSrv);
	SIM_ASSERT_IS_TRUE(pChan);

	//服务绑定
	net::StruIpAddr stBind;
	stBind.eType = sim::net::E_IP_ADDR_TYPE_IPV6;
	stBind.usPort = 6452;
	SIM_ASSERT_IS_EQUAL(net::E_NET_ERROR_SUCCESS, pSrv->Bind(stBind));
	SIM_ASSERT_IS_EQUAL(net::E_NET_ERROR_SUCCESS, pSrv->StartAccept());

	//链接
	stBind.strIp = "::1";
	SIM_ASSERT_IS_EQUAL(net::E_NET_ERROR_SUCCESS, pChan->StartConnect(stBind));

	//等待链接完成
	net::EnumNetError eConnResult = net::E_NET_ERROR_TIMEOUT;
	SIM_ASSERT_IS_TRUE(refCliPro->WaitConnect(eConnResult, 5000));
	SIM_ASSERT_IS_EQUAL(net::E_NET_ERROR_SUCCESS, eConnResult);

	//发送请求
	tVector<net::SimplePack> vReq;

	net::SimplePack reqpack;
	reqpack.version = 1;
	reqpack.type = 1;
	reqpack.seq = 1;
	reqpack.data = RefBuff(1);
	SIM_ASSERT_IS_EQUAL(net::E_NET_ERROR_SUCCESS, refCliPro->Send(pChan, reqpack, NULL));
	vReq.push_back(reqpack);

	reqpack.seq++;
	reqpack.data = RefBuff(2);
	SIM_ASSERT_IS_EQUAL(net::E_NET_ERROR_SUCCESS, refCliPro->Send(pChan, reqpack, NULL));
	vReq.push_back(reqpack);

	reqpack.seq++;
	reqpack.data = RefBuff(4);
	SIM_ASSERT_IS_EQUAL(net::E_NET_ERROR_SUCCESS, refCliPro->Send(pChan, reqpack, NULL));
	vReq.push_back(reqpack);

	reqpack.seq++;
	reqpack.data = RefBuff(1024);
	SIM_ASSERT_IS_EQUAL(net::E_NET_ERROR_SUCCESS, refCliPro->Send(pChan, reqpack, NULL));
	vReq.push_back(reqpack);

	reqpack.seq++;
	reqpack.data = RefBuff(65535);
	SIM_ASSERT_IS_EQUAL(net::E_NET_ERROR_SUCCESS, refCliPro->Send(pChan, reqpack, NULL));
	vReq.push_back(reqpack);

	reqpack.seq++;
	reqpack.data = RefBuff(65535 * 100);
	SIM_ASSERT_IS_EQUAL(net::E_NET_ERROR_SUCCESS, refCliPro->Send(pChan, reqpack, NULL));
	vReq.push_back(reqpack);
	//等待
	for (int i = 0; i < vReq.size(); ++i)
	{
		net::SimplePack& reqpack1 = vReq[i];
		net::SimplePack respack1;
		SIM_ASSERT_IS_TRUE(refCliPro->WaitRes(reqpack1.seq, respack1, 5000));

		//比较
		SIM_ASSERT_IS_EQUAL(reqpack1.version, respack1.version);
		SIM_ASSERT_IS_EQUAL((UInt8)2, respack1.type);
		SIM_ASSERT_IS_EQUAL(reqpack1.seq, respack1.seq);
		SIM_ASSERT_IS_EQUAL(reqpack1.data.size(), respack1.data.size());
		/*for (int i = 0; i < respack1.data.size(); ++i)
			SIM_ASSERT_IS_EQUAL(reqpack1.data.c_get()[i], respack1.data.c_get()[i]);*/
	}

	myGnet.UnInit();
}

SIM_TEST(UdpIPv4)
{
	net::TypeNetChannel typeflag = 0;
	net::AsyncManager<NetManager> myGnet;
	SIM_ASSERT_IS_EQUAL(net::E_NET_ERROR_SUCCESS, myGnet.Init(2));

	sim::RefObject <net::Protocol> refSrvPro(new net::SimpleProtocol());
	sim::RefObject <net::SimpleProtocol> refCliPro(new net::SimpleProtocol());
	sim::RefObject<net::Channel> pSrv = myGnet.CreateChannel(typeflag, refSrvPro);
	sim::RefObject<net::Channel> pChan = myGnet.CreateChannel(typeflag, sim::reinterpret_pointer_cast<net::Protocol>(refCliPro));
	SIM_ASSERT_IS_TRUE(pSrv);
	SIM_ASSERT_IS_TRUE(pChan);

	pSrv->SetAutoMTU(4096);
	pChan->SetAutoMTU(4096);

	//服务绑定
	net::StruIpAddr stSrv;
	stSrv.eType = sim::net::E_IP_ADDR_TYPE_IPV4;
	stSrv.usPort = 6453;
	SIM_ASSERT_IS_EQUAL(net::E_NET_ERROR_SUCCESS, pSrv->Bind(stSrv));

	stSrv.strIp = "127.0.0.1";
	net::StruIpAddr stCli;
	stCli.eType = sim::net::E_IP_ADDR_TYPE_IPV4;
	stCli.usPort = 6454;
	SIM_ASSERT_IS_EQUAL(net::E_NET_ERROR_SUCCESS, pChan->Bind(stCli));

	//发送请求
	tVector<net::SimplePack> vReq;

	net::SimplePack reqpack;
	reqpack.version = 1;
	reqpack.type = 1;
	reqpack.seq = 1;
	reqpack.data = RefBuff(1);
	SIM_ASSERT_IS_EQUAL(net::E_NET_ERROR_SUCCESS, refCliPro->Send(pChan, reqpack, &stSrv));
	vReq.push_back(reqpack);

	reqpack.seq++;
	reqpack.data = RefBuff(2);
	SIM_ASSERT_IS_EQUAL(net::E_NET_ERROR_SUCCESS, refCliPro->Send(pChan, reqpack, &stSrv));
	vReq.push_back(reqpack);

	reqpack.seq++;
	reqpack.data = RefBuff(4);
	SIM_ASSERT_IS_EQUAL(net::E_NET_ERROR_SUCCESS, refCliPro->Send(pChan, reqpack, &stSrv));
	vReq.push_back(reqpack);

	reqpack.seq++;
	reqpack.data = RefBuff(1024);
	SIM_ASSERT_IS_EQUAL(net::E_NET_ERROR_SUCCESS, refCliPro->Send(pChan, reqpack, &stSrv));
	vReq.push_back(reqpack);

	/*reqpack.seq++;
	reqpack.data = RefBuff(65535);
	SIM_ASSERT_IS_EQUAL(net::E_NET_ERROR_SUCCESS, refCliPro->Send(pChan, reqpack, &stSrv));
	vReq.push_back(reqpack);*/

	/*reqpack.seq++;
	reqpack.data = RefBuff(65535 * 100);
	SIM_ASSERT_IS_EQUAL(net::E_NET_ERROR_SUCCESS, refCliPro->Send(pChan, reqpack, &stSrv));
	vReq.push_back(reqpack);*/
	//等待
	for (int i = 0; i < vReq.size(); ++i)
	{
		net::SimplePack& reqpack1 = vReq[i];
		net::SimplePack respack1;
		SIM_ASSERT_IS_TRUE(refCliPro->WaitRes(reqpack1.seq, respack1, 5000));

		//比较
		SIM_ASSERT_IS_EQUAL(reqpack1.version, respack1.version);
		SIM_ASSERT_IS_EQUAL((UInt8)2, respack1.type);
		SIM_ASSERT_IS_EQUAL(reqpack1.seq, respack1.seq);
		SIM_ASSERT_IS_EQUAL(reqpack1.data.size(), respack1.data.size());
		/*for (int i = 0; i < respack1.data.size(); ++i)
			SIM_ASSERT_IS_EQUAL(reqpack1.data.c_get()[i], respack1.data.c_get()[i]);*/
	}

	myGnet.UnInit();
}

SIM_TEST(UdpIPv6)
{
	net::TypeNetChannel typeflag = SIM_NET_CHANNEL_TYPE_IPV6;
	net::AsyncManager<NetManager> myGnet;
	SIM_ASSERT_IS_EQUAL(net::E_NET_ERROR_SUCCESS, myGnet.Init(1));

	sim::RefObject <net::Protocol> refSrvPro(new net::SimpleProtocol());
	sim::RefObject <net::SimpleProtocol> refCliPro(new net::SimpleProtocol());
	sim::RefObject<net::Channel> pSrv = myGnet.CreateChannel(typeflag, refSrvPro);
	sim::RefObject<net::Channel> pChan = myGnet.CreateChannel(typeflag, sim::reinterpret_pointer_cast<net::Protocol>(refCliPro));
	SIM_ASSERT_IS_TRUE(pSrv);
	SIM_ASSERT_IS_TRUE(pChan);

	pSrv->SetAutoMTU(4096);
	pChan->SetAutoMTU(4096);

	//服务绑定
	net::StruIpAddr stSrv;
	stSrv.eType = sim::net::E_IP_ADDR_TYPE_IPV6;
	stSrv.usPort = 6459;
	SIM_ASSERT_IS_EQUAL(net::E_NET_ERROR_SUCCESS, pSrv->Bind(stSrv));

	stSrv.strIp = "::1";
	net::StruIpAddr stCli;
	stCli.eType = sim::net::E_IP_ADDR_TYPE_IPV6;
	stCli.usPort = 6458;
	SIM_ASSERT_IS_EQUAL(net::E_NET_ERROR_SUCCESS, pChan->Bind(stCli));

	//发送请求
	tVector<net::SimplePack> vReq;

	net::SimplePack reqpack;
	reqpack.version = 1;
	reqpack.type = 1;
	reqpack.seq = 1;
	reqpack.data = RefBuff(1);
	SIM_ASSERT_IS_EQUAL(net::E_NET_ERROR_SUCCESS, refCliPro->Send(pChan, reqpack, &stSrv));
	vReq.push_back(reqpack);

	reqpack.seq++;
	reqpack.data = RefBuff(2);
	SIM_ASSERT_IS_EQUAL(net::E_NET_ERROR_SUCCESS, refCliPro->Send(pChan, reqpack, &stSrv));
	vReq.push_back(reqpack);

	reqpack.seq++;
	reqpack.data = RefBuff(4);
	SIM_ASSERT_IS_EQUAL(net::E_NET_ERROR_SUCCESS, refCliPro->Send(pChan, reqpack, &stSrv));
	vReq.push_back(reqpack);

	reqpack.seq++;
	reqpack.data = RefBuff(1024);
	SIM_ASSERT_IS_EQUAL(net::E_NET_ERROR_SUCCESS, refCliPro->Send(pChan, reqpack, &stSrv));
	vReq.push_back(reqpack);

	/*reqpack.seq++;
	reqpack.data = RefBuff(65535);
	SIM_ASSERT_IS_EQUAL(net::E_NET_ERROR_SUCCESS, refCliPro->Send(pChan, reqpack, &stSrv));
	vReq.push_back(reqpack);*/

	/*reqpack.seq++;
	reqpack.data = RefBuff(65535 * 100);
	SIM_ASSERT_IS_EQUAL(net::E_NET_ERROR_SUCCESS, refCliPro->Send(pChan, reqpack, &stSrv));
	vReq.push_back(reqpack);*/
	//等待
	for (int i = 0; i < vReq.size(); ++i)
	{
		net::SimplePack& reqpack1 = vReq[i];
		net::SimplePack respack1;
		SIM_ASSERT_IS_TRUE(refCliPro->WaitRes(reqpack1.seq, respack1, 5000));

		//比较
		SIM_ASSERT_IS_EQUAL(reqpack1.version, respack1.version);
		SIM_ASSERT_IS_EQUAL((UInt8)2, respack1.type);
		SIM_ASSERT_IS_EQUAL(reqpack1.seq, respack1.seq);
		SIM_ASSERT_IS_EQUAL(reqpack1.data.size(), respack1.data.size());
		/*for (int i = 0; i < respack1.data.size(); ++i)
			SIM_ASSERT_IS_EQUAL(reqpack1.data.c_get()[i], respack1.data.c_get()[i]);*/
	}

	myGnet.UnInit();
}

SIM_TEST_MAIN(sim::noisy)