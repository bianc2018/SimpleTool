/*
* socket 工具
*/
#ifndef SIM_SOCKET_HPP_
#define SIM_SOCKET_HPP_

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
    #ifndef OS_WINDOWS
        #define OS_WINDOWS
    #endif
	#ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
		#define _WINSOCK_DEPRECATED_NO_WARNINGS 1
	#endif
	
    #include <stdio.h>
	#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN  
	#endif
	#include <WinSock2.h>
	
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
#elif defined(linux) || defined(__linux) || defined(__linux__)
    #ifndef OS_LINUX
        #define OS_LINUX
    #endif  
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
#else
    #error "不支持的平台"
#endif


#include "NetBase.hpp"

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
			// SOCK_STREAM tcp SOCK_DGRAM udp
			static SOCKET CreateSocket(SockType type);

			static SOCKET CreateSocket(int af, int type, int protocol);

		public:
			//封装的函数 同步接口
			//返回 false 标识终止
			typedef bool(*GetHostByNameCallBack)(const char* ip, void* pdata);
			static EnumNetError GetHostByName(const char* szHost,
				GetHostByNameCallBack cb, void* pdata);

			static bool GetFirstIpByName(const char* szHost, char* ip_buff, int ip_buff_len);

			static EnumNetError Connect(SOCKET socket,const char* ipaddr, unsigned short port);

			/*
				需要设置为非堵塞，否则返回异常
			*/
			static EnumNetError ConnectTimeOut(SOCKET socket, const char* ipaddr, unsigned short port, int wait_ms = -1);

			static EnumNetError Bind(SOCKET socket, const char* ipaddr, unsigned short port);

			static EnumNetError Listen(SOCKET socket, int backlog);

			static EnumNetError Accept(SOCKET socket, SOCKET& client, int wait_ms = -1);

			static EnumNetError Accept(SOCKET socket, SOCKET& client, char* remote_ip, unsigned int ip_len,
				unsigned short* remote_port, int wait_ms = -1);

			static EnumNetError Send(SOCKET socket, const char* data, unsigned int data_len, int wait_ms = -1);

			static EnumNetError SendTo(SOCKET socket, const char* data, unsigned int data_len, \
				const char* ipaddr, unsigned short port, int wait_ms = -1);

			static EnumNetError Recv(SOCKET socket, char* data, unsigned int data_len, int wait_ms = -1);

			static EnumNetError Recvfrom(SOCKET socket, char* data, unsigned int data_len, \
				char* remote_ip, unsigned int ip_len,
				unsigned short* remote_port, int wait_ms = -1);

			static EnumNetError Close(SOCKET socket);

			/***** 2021/03/05 新增 超时接口 ******/

			//等待时间 ,int wait_ms=-1
			enum WAIT_TYPE
			{
				WAIT_READ,
				WAIT_WRITE,
			};
			static EnumNetError WaitTimeOut(SOCKET socket, WAIT_TYPE type, int wait_ms);
		public:
			static void Init();

			//结构体转换
			static bool IpToAddressV4(const char* ipaddr, unsigned short port
				, struct sockaddr_in* out_addr);
			static bool AddressToIpV4(const struct sockaddr_in* addr,
				char* ipaddr, unsigned int ipaddr_len, unsigned short* port
			);

			static bool SetNonBlock(SOCKET socket, bool is_non_block);

			static bool SetReusePort(SOCKET socket, bool set);

			//是否为有效套接字
			static bool IsVaild(SOCKET socket)
			{

				return INVALID_SOCKET != socket;
			}
		private:

		};

		inline SOCKET SocketUtil::CreateSocket(SockType type)
		{
			SocketUtil::Init();
			if (type == TCP)
				return socket(AF_INET, SOCK_STREAM, 0);
			else if (type == UDP)
				return  socket(AF_INET, SOCK_DGRAM, 0);
			else
				return INVALID_SOCKET;
		}
		inline SOCKET SocketUtil::CreateSocket(int af, int type, int protocol)
		{
			Init();
			return socket(af, type, protocol);
		}
		
		inline EnumNetError SocketUtil::GetHostByName(const char* szHost, GetHostByNameCallBack cb, void* pdata)
		{
			if (NULL == szHost || NULL == cb)
			{
				return E_NET_ERROR_PARAM;//
			}
			hostent* pHost = gethostbyname(szHost);
			if (NULL == pHost)
			{
				return E_NET_ERROR_FAILED;
			}
			int i;
			in_addr addr;
			for (i = 0;; i++)
			{
				char* p = pHost->h_addr_list[i];
				if (p == NULL)
				{
					break;
				}
#ifdef OS_WINDOWS
				memcpy(&addr.S_un.S_addr, p, pHost->h_length);
				const char* strIp = ::inet_ntoa(addr);
#endif // OS_WINDOWS
#ifdef OS_LINUX
				const socklen_t buff_size = 128;
				char buff[buff_size] = { 0 };
				const char* strIp = inet_ntop(pHost->h_addrtype,
					p, buff, buff_size);
#endif 
				if (!cb(strIp, pdata))
					return E_NET_ERROR_FAILED;
			}
			return E_NET_ERROR_SUCCESS;
		}
		inline bool SocketUtil::GetFirstIpByName(const char* szHost, char* ip_buff, int ip_buff_len)
		{
			if (NULL == szHost)
			{
				return E_NET_ERROR_PARAM;//
			}
			hostent* pHost = gethostbyname(szHost);
			if (NULL == pHost)
			{
				return E_NET_ERROR_FAILED;
			}
			int i;
			in_addr addr;
			for (i = 0;; i++)
			{
				char* p = pHost->h_addr_list[i];
				if (p == NULL)
				{
					break;
				}
#ifdef OS_WINDOWS
				memcpy(&addr.S_un.S_addr, p, pHost->h_length);
				const char* strIp = ::inet_ntoa(addr);
#endif // OS_WINDOWS
#ifdef OS_LINUX
				const socklen_t buff_size = 128;
				char buff[buff_size] = { 0 };
				const char* strIp = inet_ntop(pHost->h_addrtype,
					p, buff, buff_size);
#endif 
				int ip_len = strlen(strIp);
				if (ip_len > ip_buff_len)
					return E_NET_ERROR_PARAM;//
				memcpy(ip_buff, strIp, ip_len);
				return E_NET_ERROR_SUCCESS;
			}
			return E_NET_ERROR_FAILED;
		}
		inline EnumNetError SocketUtil::Connect(SOCKET socket, const char* ipaddr, unsigned short port)
		{
			/*int wait_ret = WaitTimeOut(WAIT_READ, wait_ms);
			if (wait_ret != SOCK_SUCCESS)
			{
				return wait_ret;
			}*/

			//创建sockaddr_in结构体变量
			struct sockaddr_in serv_addr;
			if (!IpToAddressV4(ipaddr, port, &serv_addr))
				return E_NET_ERROR_PARAM;
			//将套接字和IP、端口绑定
			int nRet =  ::connect(socket, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
			if (nRet < 0)
			{
				return E_NET_ERROR_FAILED;
			}
			return E_NET_ERROR_SUCCESS;
		}
		inline EnumNetError SocketUtil::ConnectTimeOut(SOCKET socket, const char* ipaddr, unsigned short port, int wait_ms)
		{
			if (wait_ms < 0)
				return Connect(socket,ipaddr, port);
			
			EnumNetError eRet =  Connect(socket,ipaddr, port);
			if (eRet == E_NET_ERROR_SUCCESS)
			{
				return E_NET_ERROR_SUCCESS;
			}

			eRet = WaitTimeOut(socket,WAIT_WRITE, wait_ms);
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
		inline EnumNetError SocketUtil::Bind(SOCKET socket, const char* ipaddr, unsigned short port)
		{
			//创建sockaddr_in结构体变量
			struct sockaddr_in serv_addr;
			if (!IpToAddressV4(ipaddr, port, &serv_addr))
				return E_NET_ERROR_FAILED;

			int nRet = ::bind(socket, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
			if (nRet < 0)
			{
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
		inline EnumNetError SocketUtil::Accept(SOCKET socket, SOCKET& client, int wait_ms)
		{
			/*if (NULL == s)
				return SOCK_FAILURE;*/

			EnumNetError wait_ret = WaitTimeOut(socket,WAIT_READ, wait_ms);
			if (wait_ret != E_NET_ERROR_SUCCESS)
			{
				return wait_ret;
			}

			SOCKET accept_cli = ::accept(socket, NULL, 0);
			client = accept_cli;
			return E_NET_ERROR_SUCCESS;
		}
		inline EnumNetError SocketUtil::Accept(SOCKET socket, SOCKET& client,
			char* remote_ip, unsigned int ip_len, unsigned short* remote_port, int wait_ms)
		{
			/*if (NULL == s)
				return SOCK_FAILURE;*/

			EnumNetError wait_ret = WaitTimeOut(socket, WAIT_READ, wait_ms);
			if (wait_ret != E_NET_ERROR_SUCCESS)
			{
				return wait_ret;
			}

			//创建sockaddr_in结构体变量
			struct sockaddr_in addr;
			memset(&addr, 0, sizeof(addr));  //每个字节都用0填充

#ifdef OS_WINDOWS
			int addr_len = sizeof(addr);
#endif

#ifdef OS_LINUX
			socklen_t addr_len = sizeof(addr);
#endif
			SOCKET accept_cli = ::accept(socket, (struct sockaddr*)&addr, &addr_len);
			if (accept_cli == INVALID_SOCKET)
			{
				return E_NET_ERROR_FAILED;
			}
			if (!AddressToIpV4(&addr, remote_ip, ip_len, remote_port))
			{
				return E_NET_ERROR_FAILED;
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
				return E_NET_ERROR_FAILED;
			}
			return E_NET_ERROR_SUCCESS;

		}
		inline EnumNetError SocketUtil::SendTo(SOCKET socket, const char* data, unsigned int data_len, const char* ipaddr, unsigned short port, int wait_ms)
		{
			EnumNetError wait_ret = WaitTimeOut(socket, WAIT_WRITE, wait_ms);
			if (wait_ret != E_NET_ERROR_SUCCESS)
			{
				return wait_ret;
			}
			
			struct sockaddr_in serv_addr;
			if (!IpToAddressV4(ipaddr, port, &serv_addr))
				return E_NET_ERROR_FAILED;

			int nRet = ::sendto(socket, data, data_len, 0,
				(struct sockaddr*)&serv_addr, sizeof(serv_addr));
			if (nRet < 0)
			{
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
				return E_NET_ERROR_FAILED;
			}
			return E_NET_ERROR_SUCCESS;
		}
		inline EnumNetError SocketUtil::Recvfrom(SOCKET socket, char* data, unsigned int data_len, char* remote_ip, unsigned int ip_len,
			unsigned short* remote_port, int wait_ms)
		{
			//创建sockaddr_in结构体变量
			struct sockaddr_in serv_addr;
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
				AddressToIpV4(&serv_addr, remote_ip, ip_len, remote_port);
				return E_NET_ERROR_SUCCESS;
			}
			return E_NET_ERROR_FAILED;
		}
		inline EnumNetError SocketUtil::Close(SOCKET socket)
		{
			SOCKET temp = socket;
			socket = INVALID_SOCKET;

#ifdef OS_WINDOWS
			/*struct linger so_linger;
			so_linger.l_onoff = 1;
			so_linger.l_linger = 0;
			setsockopt(sock_, SOL_SOCKET, SO_LINGER,(const char*) &so_linger, sizeof so_linger);*/
			int ret = ::closesocket(temp);
#endif

#ifdef OS_LINUX
			int ret = ::close(temp);
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
		inline bool SocketUtil::IpToAddressV4(const char* ipaddr, unsigned short port, sockaddr_in* out_addr)
		{
			if (NULL == out_addr)
				return false;
			memset(out_addr, 0, sizeof(*out_addr));  //每个字节都用0填充
			out_addr->sin_family = AF_INET;  //使用IPv4地址
			if (ipaddr)
				out_addr->sin_addr.s_addr = inet_addr(ipaddr);  //具体的IP地址
			else
				out_addr->sin_addr.s_addr = htonl(INADDR_ANY);  //所有ip
			out_addr->sin_port = htons(port);  //端口
			return E_NET_ERROR_SUCCESS;
		}
		inline bool SocketUtil::AddressToIpV4(const sockaddr_in* addr, char* ipaddr, unsigned int ipaddr_len, unsigned short* port)
		{
			if (NULL == addr)
				return false;

			if (ipaddr && ipaddr_len >= 0)
			{
				snprintf(ipaddr, ipaddr_len, "%s", inet_ntoa(addr->sin_addr));
			}
			if (port)
			{
				*port = ntohs(addr->sin_port);
			}
			return E_NET_ERROR_SUCCESS;
		}
		inline bool SocketUtil::SetNonBlock(SOCKET socket, bool is_non_block)
		{

#ifdef OS_WINDOWS
			unsigned long ul = is_non_block;
			int ret = ioctlsocket(socket, FIONBIO, (unsigned long*)&ul);    //设置成非阻塞模式
			if (ret == SOCKET_ERROR)   //设置失败
			{
				return E_NET_ERROR_FAILED;
			}
			return E_NET_ERROR_SUCCESS;
#endif
#ifdef OS_LINUX
			int old_option = fcntl(socket, F_GETFL);
			int new_option = old_option | (is_non_block ? O_NONBLOCK : (~O_NONBLOCK));
			if (fcntl(socket, F_SETFL, new_option) < 0)
			{
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
				return E_NET_ERROR_FAILED;
			}
#endif
#ifdef OS_LINUX
			int opt = set ? 1 : 0;
			int ret = setsockopt(sock_, SOL_SOCKET, SO_REUSEPORT,
				&opt, static_cast<socklen_t>(sizeof(opt)));
			if (ret < 0)
			{
				return E_NET_ERROR_FAILED;
			}
#endif
			return E_NET_ERROR_SUCCESS;
		}
	}
}
#endif