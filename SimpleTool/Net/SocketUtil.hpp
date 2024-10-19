/*
* socket 工具
*/
#ifndef SIM_SOCKET_HPP_
#define SIM_SOCKET_HPP_
#define _WINSOCKAPI_
#include "NetBase.hpp"
#include <vector>
#if defined(OS_WINDOWS)
	#ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
		#define _WINSOCK_DEPRECATED_NO_WARNINGS 1
	#endif
	
    #include <stdio.h>
	#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN  
	#endif
	#include <WinSock2.h>
	#include <ws2tcpip.h>

    #ifndef INVALID_SOCKET
        #define INVALID_SOCKET (SOCKET)(~0)
    #endif
    #pragma comment (lib, "ws2_32.lib")  //加载 ws2_32.dll
    namespace sim
    {
    //初始化函数
        class WsInit
        {
        public:
            WsInit()
            {
                WSADATA wsaData;
                WSAStartup(MAKEWORD(2, 2), &wsaData);
            }
            ~WsInit()
            {
                //终止 DLL 的使用
                WSACleanup();
            }
        };
    }
#elif defined(OS_LINUX)
	#include <errno.h>
    #include <stdio.h>
    #include <string.h>
    #include <stdlib.h>
    #include <unistd.h>
    #include <netdb.h>
	#include <fcntl.h>
    #include <arpa/inet.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    typedef int SOCKET;
    #ifndef INVALID_SOCKET
        #define INVALID_SOCKET -1
    #endif
#endif

namespace sim
{
	namespace net
	{
		//类型
		enum SockType
		{
			TCP,
			UDP
		};

		//INVALID_SOCKET
		class  SocketUtil
		{
		public:
			
			static SOCKET CreateSocket(int af, int type, int protocol);

			static SOCKET CreateSocket(TypeNetChannel typeflag);
		public:
			//封装的函数 同步接口
			static bool GetIpAddrList(const char* szHost,tVector<StruIpAddr>&vAddrs, const char* szService=NULL);

			static EnumNetError Bind(SOCKET socket, const StruIpAddr& stIpAddr);

			static EnumNetError Listen(SOCKET socket, int backlog);

			static EnumNetError Accept(SOCKET socket, SOCKET& client,StruIpAddr* pstIpAddr, int wait_ms = -1);

			static EnumNetError Send(SOCKET socket, const char* data, unsigned int data_len, int wait_ms = -1);

			static EnumNetError SendTo(SOCKET socket, const char* data, unsigned int data_len, \
				const StruIpAddr& stIpAddr, int wait_ms = -1);

			static EnumNetError Recv(SOCKET socket, char* data, unsigned int data_len, int wait_ms = -1);

			static EnumNetError Recvfrom(SOCKET socket, char* data, unsigned int data_len, \
				StruIpAddr& stIpAddr, int wait_ms = -1);

			static EnumNetError Close(SOCKET socket);

			//等待时间 ,int wait_ms=-1
			enum WAIT_TYPE
			{
				WAIT_READ,
				WAIT_WRITE,
			};
			static EnumNetError WaitTimeOut(SOCKET socket, WAIT_TYPE type, int wait_ms);

			static EnumNetError Connect(SOCKET socket, const StruIpAddr& stIpAddr, int wait_ms=-1);
		public:
			static void Init();

			//指针需要释放
			static bool IpToSockAddr(const StruIpAddr &stIpAddr,struct sockaddr*p);
			static bool SockAddrToIp(const struct sockaddr* addr, StruIpAddr& stIpAddr);

			static bool SetNonBlock(SOCKET socket, bool is_non_block);

			static bool SetReusePort(SOCKET socket, bool set);

			//是否为有效套接字
			static bool IsVaild(SOCKET socket)
			{

				return INVALID_SOCKET != socket;
			}
		private:

		};
		

		inline SOCKET SocketUtil::CreateSocket(int af, int type, int protocol)
		{
			Init();
			return socket(af, type, protocol);
		}
		inline SOCKET SocketUtil::CreateSocket(TypeNetChannel typeflag)
		{
			int af= AF_INET;
			int type= SOCK_DGRAM;
			int protocol=0;
			if (typeflag & SIM_NET_CHANNEL_TYPE_TCP)
			{
				type = SOCK_STREAM;
			}
			if (typeflag & SIM_NET_CHANNEL_TYPE_IPV6)
			{
				af = AF_INET6;
			}
			return CreateSocket(af,type,protocol);
		}

		inline bool SocketUtil::GetIpAddrList(const char* szHost, tVector<StruIpAddr>& vAddrs, const char* szService)
		{
			if (!szHost)
				return false;

			Init();

			struct addrinfo hints, * res, * p;
			int status;
			char ipstr[INET6_ADDRSTRLEN];

			memset(&hints, 0, sizeof hints);
			hints.ai_family = AF_UNSPEC; // AF_INET 或 AF_INET6 来指定 IPv4 或 IPv6
			hints.ai_socktype = 0;// SOCK_STREAM; // SOCK_DGRAM 用于 UDP

			if ((status = getaddrinfo(szHost, szService, &hints, &res)) != 0) {
				fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
				return false;
			}

			bool bOk = true;
			for (p = res; p != NULL; p = p->ai_next) 
			{
				StruIpAddr stIpaddr;
				if (!SockAddrToIp(p->ai_addr, stIpaddr))
				{
					bOk = false;
					break;
				}
				vAddrs.push_back(stIpaddr);
			}

			freeaddrinfo(res); // 释放链表
			return bOk;
		}
		
		inline EnumNetError SocketUtil::Bind(SOCKET socket, const StruIpAddr& stIpAddr)
		{
			//创建sockaddr_in结构体变量
			struct sockaddr_in serv_addr;
			memset(&serv_addr, 0, sizeof(serv_addr));
			struct sockaddr_in6 serv_addr6;
			memset(&serv_addr6, 0, sizeof(serv_addr6));
			struct sockaddr* addr_name = NULL;
			int addr_namelen = 0;
			if (stIpAddr.eType == E_IP_ADDR_TYPE_IPV6)
			{
				if (!IpToSockAddr(stIpAddr, (struct sockaddr*)&serv_addr6))
					return E_NET_ERROR_PARAM;
				addr_name = (struct sockaddr*)&serv_addr6;
				addr_namelen = sizeof(serv_addr6);
			}
			else if (stIpAddr.eType == E_IP_ADDR_TYPE_IPV4)
			{

				if (!IpToSockAddr(stIpAddr, (struct sockaddr*)&serv_addr))
					return E_NET_ERROR_PARAM;
				addr_name = (struct sockaddr*)&serv_addr;
				addr_namelen = sizeof(serv_addr);
			}
			else
			{
				return E_NET_ERROR_PARAM;
			}

			int nRet = ::bind(socket, (struct sockaddr*)addr_name, addr_namelen);
			if (nRet < 0)
			{
				perror("accept:");
				return E_NET_ERROR_FAILED;
			}
			return E_NET_ERROR_SUCCESS;
		}
		inline EnumNetError SocketUtil::Listen(SOCKET socket, int backlog)
		{
			int nRet = ::listen(socket, backlog);
			if (nRet < 0)
			{
				return E_NET_ERROR_FAILED;
			}
			return E_NET_ERROR_SUCCESS;
		}
		inline EnumNetError SocketUtil::Accept(SOCKET socket, SOCKET& client, StruIpAddr* pstIpAddr, int wait_ms)
		{
			/*if (NULL == s)
				return SOCK_FAILURE;*/

			EnumNetError wait_ret = WaitTimeOut(socket, WAIT_READ, wait_ms);
			if (wait_ret != E_NET_ERROR_SUCCESS)
			{
				return wait_ret;
			}

			//创建sockaddr_in结构体变量
			struct sockaddr_in6 addr,*paddr=NULL;
			memset(&addr, 0, sizeof(addr));  //每个字节都用0填充

#ifdef OS_WINDOWS
			int addr_len = sizeof(addr);
#endif

#ifdef OS_LINUX
			socklen_t addr_len = sizeof(addr);
#endif
			if (pstIpAddr)
				paddr = &addr;

			SOCKET accept_cli = ::accept(socket, (struct sockaddr*)paddr, &addr_len);
			if (accept_cli == INVALID_SOCKET)
			{
				perror("accept:");
				return E_NET_ERROR_FAILED;
			}

			if (pstIpAddr)
			{
				if (!SockAddrToIp((struct sockaddr*)&addr, *pstIpAddr))
				{
					return E_NET_ERROR_FAILED;
				}
			}
			client = accept_cli;
			return E_NET_ERROR_SUCCESS;
		}

		inline EnumNetError SocketUtil::Send(SOCKET socket, const char* data, unsigned int data_len, int wait_ms)
		{
			EnumNetError wait_ret = WaitTimeOut(socket, WAIT_WRITE, wait_ms);
			if (wait_ret != E_NET_ERROR_SUCCESS)
			{
				return wait_ret;
			}

			int nRet = ::send(socket, data, data_len, 0);
			if (nRet < 0)
			{
				perror("send:");
				return E_NET_ERROR_FAILED;
			}
			return E_NET_ERROR_SUCCESS;

		}
		inline EnumNetError SocketUtil::SendTo(SOCKET socket, const char* data, unsigned int data_len, const StruIpAddr& stIpAddr, int wait_ms)
		{
			EnumNetError wait_ret = WaitTimeOut(socket, WAIT_WRITE, wait_ms);
			if (wait_ret != E_NET_ERROR_SUCCESS)
			{
				return wait_ret;
			}
			
			struct sockaddr_in serv_addr;
			memset(&serv_addr, 0, sizeof(serv_addr));
			struct sockaddr_in6 serv_addr6;
			memset(&serv_addr6, 0, sizeof(serv_addr6));
			struct sockaddr* addr_name = NULL;
			int addr_namelen = 0;
			if (stIpAddr.eType == E_IP_ADDR_TYPE_IPV6)
			{
				if (!IpToSockAddr(stIpAddr, (struct sockaddr*)&serv_addr6))
					return E_NET_ERROR_PARAM;
				addr_name =(struct sockaddr*) & serv_addr6;
				addr_namelen = sizeof(serv_addr6);
			}
			else if (stIpAddr.eType == E_IP_ADDR_TYPE_IPV4)
			{

				if (!IpToSockAddr(stIpAddr, (struct sockaddr*)&serv_addr))
					return E_NET_ERROR_PARAM;
				addr_name = (struct sockaddr*)&serv_addr;
				addr_namelen = sizeof(serv_addr);
			}
			else
			{
				return E_NET_ERROR_PARAM;
			}
			int nRet = ::sendto(socket, data, data_len, 0,
				(struct sockaddr*)addr_name, addr_namelen);
			if (nRet < 0)
			{
				perror("sendto:");
				return E_NET_ERROR_FAILED;
			}
			return E_NET_ERROR_SUCCESS;
		}

		inline EnumNetError SocketUtil::Recv(SOCKET socket, char* data, unsigned int data_len, int wait_ms)
		{
			EnumNetError wait_ret = WaitTimeOut(socket, WAIT_READ, wait_ms);
			if (wait_ret != E_NET_ERROR_SUCCESS)
			{
				return wait_ret;
			}

			int nRet = ::recv(socket, data, data_len, 0);
			if (nRet < 0)
			{
				perror("recv:");
				return E_NET_ERROR_FAILED;
			}
			return E_NET_ERROR_SUCCESS;
		}
		inline EnumNetError SocketUtil::Recvfrom(SOCKET socket, char* data, unsigned int data_len, StruIpAddr& stIpAddr, int wait_ms)
		{
			//创建sockaddr_in结构体变量
			struct sockaddr_in6 serv_addr;
#ifdef OS_WINDOWS
			int add_len = sizeof(serv_addr);
#endif

#ifdef OS_LINUX
			socklen_t add_len = sizeof(serv_addr);
#endif
			/*if (!IpToAddressV4(ipaddr, port, &serv_addr))
				return -1;*/
			::memset(&serv_addr, 0, sizeof(serv_addr));

			EnumNetError wait_ret = WaitTimeOut(socket, WAIT_READ, wait_ms);
			if (wait_ret != E_NET_ERROR_SUCCESS)
			{
				return wait_ret;
			}

			//那个from参数输出的是对方的地址。是一个输出参数，不是输入的。
			//将套接字和IP、端口绑定
			int ret = ::recvfrom(socket, data, data_len, 0,
				(struct sockaddr*)&serv_addr, &add_len);
			if (ret > 0)
			{
				perror("recvfrom:");
				if (!SockAddrToIp((struct sockaddr*)&serv_addr, stIpAddr))
				{
					return E_NET_ERROR_FAILED;
				}
				return E_NET_ERROR_SUCCESS;
			}
			return E_NET_ERROR_FAILED;
		}

		inline EnumNetError SocketUtil::Close(SOCKET socket)
		{
#ifdef OS_WINDOWS
			/*struct linger so_linger;
			so_linger.l_onoff = 1;
			so_linger.l_linger = 0;
			setsockopt(sock_, SOL_SOCKET, SO_LINGER,(const char*) &so_linger, sizeof so_linger);*/
			int ret = ::closesocket(socket);
#endif

#ifdef OS_LINUX
			int ret = ::close(socket);
#endif

			if (ret > 0)
			{
				return E_NET_ERROR_SUCCESS;
			}
			return E_NET_ERROR_FAILED;
		}

		inline EnumNetError SocketUtil::WaitTimeOut(SOCKET socket, WAIT_TYPE type, int wait_ms)
		{
			/*
			具体解释select的参数：
			int maxfdp是一个整数值，是指集合中所有文件描述符的范围，即所有文件描述符的最大值加1，不能错！
				在Windows中这个参数的值无所谓，可以设置不正确。
			fd_set *readfds是指向fd_set结构的指针，这个集合中应该包括文件描述符，我们是要监视这些文件描述符的读变化的，
				即我们关心是否可以从这些文件中 读取数据了，如果这个集合中有一个文件可读，select就会返回一个大于0的值，
				表示有文件可读，如果没有可读的文件，则根据timeout参数再判断 是否超时，若超出timeout的时间，select返回0，
				若发生错误返回负值。可以传入NULL值，表示不关心任何文件的读变化。
			fd_set *writefds是指向fd_set结构的指针，这个集合中应该包括文件描述符，我们是要监视这些文件描述符的写变化的，
				即我们关心是否可以向这些文件 中写入数据了，如果这个集合中有一个文件可写，select就会返回一个大于0的值，
				表示有文件可写，如果没有可写的文件，则根据timeout参数再判 断是否超时，若超出timeout的时间，select返回0，
				若发生错误返回负值。可以传入NULL值，表示不关心任何文件的写变化。
			fd_set *errorfds同上面两个参数的意图，用来监视文件错误异常。
			struct timeval* timeout是select的超时时间，这个参数至关重要，它可以使select处于三种状态，第一，若将NULL以形参传入，
			即不传入时间结构，就是 将select置于阻塞状态，一定等到监视文件描述符集合中某个文件描述符发生变化为止；第二，
			若将时间值设为0秒0毫秒，就变成一个纯粹的非阻塞函数， 不管文件描述符是否有变化，都立刻返回继续执行，文件无变化返回0，
			有变化返回一个正值；第三，timeout的值大于0，这就是等待的超时时间，即 select在timeout时间内阻塞，
			超时时间之内有事件到来就返回了，否则在超时后不管怎样一定返回，返回值同上述。
			*/
			struct timeval tv, * p = NULL;
			if (wait_ms >= 0)
			{
				tv.tv_sec = wait_ms / 1000;
				tv.tv_usec = (wait_ms % 1000) * 1000;
				p = &tv;
			}

			fd_set fds;
			FD_ZERO(&fds);
			FD_SET(socket, &fds);

			int ret = -1;
			if (WAIT_READ == type)
			{
				ret = select(socket + 1, &fds, NULL, NULL, p);
			}
			else if (WAIT_WRITE == type)
			{
				ret = select(socket + 1, NULL, &fds, NULL, p);
			}

			if (ret < 0)
			{
				//SIM_LERROR("select error ret=" << ret << " errno=" << errno);
				perror("select:");
				return E_NET_ERROR_FAILED;
			}
			else if (ret == 0)
			{
				//SIM_LERROR("select timeout ");
				return E_NET_ERROR_TIMEOUT;
			}


			if (FD_ISSET(socket, &fds))  //如果有输入，从stdin中获取输入字符  
			{
				//事件完成
				return E_NET_ERROR_SUCCESS;
			}
			//不应该在这里
			//SIM_LERROR("select error ret=" << ret << " errno=" << errno);
			return E_NET_ERROR_FAILED;
		}

		inline void SocketUtil::Init()
		{
#ifdef OS_WINDOWS
			//初始化函数
			static WsInit g_init;
#endif
		}
		inline bool SocketUtil::SetNonBlock(SOCKET socket, bool is_non_block)
		{

#ifdef OS_WINDOWS
			unsigned long ul = is_non_block;
			int ret = ioctlsocket(socket, FIONBIO, (unsigned long*)&ul);    //设置成非阻塞模式
			if (ret == SOCKET_ERROR)   //设置失败
			{
				perror("ioctlsocket:");
				return E_NET_ERROR_FAILED;
			}
			return E_NET_ERROR_SUCCESS;
#endif
#ifdef OS_LINUX
			int old_option = fcntl(socket, F_GETFL);
			int new_option = old_option | (is_non_block ? O_NONBLOCK : (~O_NONBLOCK));
			if (fcntl(socket, F_SETFL, new_option) < 0)
			{
				perror("fcntl:");
				return E_NET_ERROR_FAILED;
			}
			return E_NET_ERROR_SUCCESS;
#endif
			return E_NET_ERROR_FAILED;
		}
		inline bool SocketUtil::SetReusePort(SOCKET socket, bool set)

		{
#ifdef OS_WINDOWS
			int opt = set ? 1 : 0;
			int ret = setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, \
				(const char*)&opt, sizeof(opt));
			if (ret == SOCKET_ERROR)   //设置失败
			{
				perror("setsockopt:");
				return E_NET_ERROR_FAILED;
			}
#endif
#ifdef OS_LINUX
			int opt = set ? 1 : 0;
			int ret = setsockopt(sock_, SOL_SOCKET, SO_REUSEPORT,
				&opt, static_cast<socklen_t>(sizeof(opt)));
			if (ret < 0)
			{
				perror("setsockopt:");
				return E_NET_ERROR_FAILED;
			}
#endif
			return E_NET_ERROR_SUCCESS;
		}

		inline EnumNetError SocketUtil::Connect(SOCKET socket, const StruIpAddr& stIpAddr, int wait_ms)
		{
			//创建sockaddr_in结构体变量
			struct sockaddr_in serv_addr;
			memset(&serv_addr, 0, sizeof(serv_addr));
			struct sockaddr_in6 serv_addr6;
			memset(&serv_addr6, 0, sizeof(serv_addr6));
			struct sockaddr* addr_name = NULL;
			int addr_namelen = 0;
			if (stIpAddr.eType == E_IP_ADDR_TYPE_IPV6)
			{
				if (!IpToSockAddr(stIpAddr, (struct sockaddr*)&serv_addr6))
					return E_NET_ERROR_PARAM;
				addr_namelen = sizeof(serv_addr6);
			}
			else if (stIpAddr.eType == E_IP_ADDR_TYPE_IPV4)
			{
				
				if (!IpToSockAddr(stIpAddr, (struct sockaddr*)&serv_addr))
					return E_NET_ERROR_PARAM;
				addr_namelen = sizeof(serv_addr);
			}
			else
			{
				return E_NET_ERROR_PARAM;
			}

			//将套接字和IP、端口绑定
			int nRet = ::connect(socket, (struct sockaddr*)addr_name, addr_namelen);
			if (nRet < 0)
			{
				perror("connect:");
				return E_NET_ERROR_FAILED;
			}

			if (wait_ms == -1)
				return E_NET_ERROR_SUCCESS;

			EnumNetError eRet = WaitTimeOut(socket, WAIT_WRITE, wait_ms);
			if (eRet != E_NET_ERROR_SUCCESS)
			{
				return eRet;
			}

			int error = 0;
#ifdef OS_WINDOWS
			int length = sizeof(error);
#endif

#ifdef OS_LINUX
			socklen_t length = sizeof(error);
#endif

			if (getsockopt(socket, SOL_SOCKET, SO_ERROR, (char*)&error, &length) < 0)
			{
				//SIM_LERROR("get socket option failed");
				return E_NET_ERROR_FAILED;
			}

			if (error != 0)
			{
				//SIM_LERROR("connection failed after select with the error:"<< error);
				return E_NET_ERROR_FAILED;
			}

			return E_NET_ERROR_SUCCESS;
		}
		
		inline bool SocketUtil::IpToSockAddr(const StruIpAddr& stIpAddr, sockaddr* p)
		{
			if(NULL == p)
				return false;

			if (stIpAddr.eType == E_IP_ADDR_TYPE_IPV6)
			{
				struct sockaddr_in6* serv_addr6 = (struct sockaddr_in6*)p;
				// 使用inet_pton将IPv6地址字符串转换为二进制形式
				if (inet_pton(AF_INET6, stIpAddr.strIp.c_str(), &serv_addr6->sin6_addr) <= 0) {
					// 如果inet_pton失败，返回错误
					perror("inet_pton:");
					return false;
				}

				// 设置地址族为AF_INET6
				serv_addr6->sin6_family = AF_INET6;

				// 设置端口号
				serv_addr6->sin6_port = htons(stIpAddr.usPort);

				// 清除sin6_flowinfo和sin6_scope_id（如果需要的话）
				serv_addr6->sin6_flowinfo = 0;
				serv_addr6->sin6_scope_id = 0; // 除非你需要指定特定的作用域ID

				return true;
			}
			else if (stIpAddr.eType == E_IP_ADDR_TYPE_IPV4)
			{
				struct sockaddr_in* serv_addr = (struct sockaddr_in*)p;
				// 使用inet_pton将IPv4地址字符串转换为二进制形式
				if (inet_pton(AF_INET, stIpAddr.strIp.c_str(), &serv_addr->sin_addr) <= 0) {
					// 如果inet_pton失败，返回错误
					perror("inet_pton:");
					return false;
				}

				// 设置地址族为AF_INET6
				serv_addr->sin_family = AF_INET;

				// 设置端口号
				serv_addr->sin_port = htons(stIpAddr.usPort);
				return true;
			}

			return false;
		}
		inline bool SocketUtil::SockAddrToIp(const sockaddr* addr, StruIpAddr& stIpAddr)
		{
			if (NULL == addr)
			{
				return false;
			}

			if (addr->sa_family == AF_INET6)
			{
				struct sockaddr_in6* serv_addr6 = (struct sockaddr_in6*)addr;
				char IPAddrBuff[INET6_ADDRSTRLEN+16];
				memset(IPAddrBuff, 0, sizeof(IPAddrBuff));
				if (inet_ntop(AF_INET6,& serv_addr6->sin6_addr, (char*)IPAddrBuff, sizeof(IPAddrBuff)) <= 0) {
					// 如果inet_pton失败，返回错误
					perror("inet_ntop:");
					return false;
				}
				stIpAddr.strIp = IPAddrBuff;
				stIpAddr.eType = E_IP_ADDR_TYPE_IPV6;
				stIpAddr.usPort = ntohs(serv_addr6->sin6_port);

				return true;
			}
			else if (addr->sa_family == AF_INET)
			{
				struct sockaddr_in* serv_addr = (struct sockaddr_in*)addr;
				char IPAddrBuff[INET_ADDRSTRLEN + 16];
				memset(IPAddrBuff, 0, sizeof(IPAddrBuff));
				if (inet_ntop(AF_INET, &serv_addr->sin_addr, (char*)IPAddrBuff, sizeof(IPAddrBuff)) <= 0) {
					// 如果inet_pton失败，返回错误
					perror("inet_ntop:");
					return false;
				}
				stIpAddr.strIp = IPAddrBuff;
				stIpAddr.eType = E_IP_ADDR_TYPE_IPV4;
				stIpAddr.usPort = ntohs(serv_addr->sin_port);
				return true;	
			}
			return false;
		}
	}
}
#endif