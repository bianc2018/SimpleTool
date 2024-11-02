#include "Test.hpp"
#include "Net/SocketUtil.hpp"
using namespace sim;
SIM_TEST(Dns)
{
	tVector<StruIpAddr> vAddrs;
	SIM_TEST_IS_TRUE(SocketUtil::GetIpAddrList("localhost", vAddrs));
	SIM_TEST_IS_TRUE(vAddrs.size()>0);
}
SIM_TEST_MAIN(sim::noisy)