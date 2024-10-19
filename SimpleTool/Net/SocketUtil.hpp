/*
* socket ����
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
    #pragma comment (lib, "ws2_32.lib")  //���� ws2_32.dll
    namespace sim
    {
    //��ʼ������
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
                //��ֹ DLL ��ʹ��
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
		//����
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
			//��װ�ĺ��� ͬ���ӿ�
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

			//�ȴ�ʱ�� ,int wait_ms=-1
			enum WAIT_TYPE
			{
				WAIT_READ,
				WAIT_WRITE,
			};
			static EnumNetError WaitTimeOut(SOCKET socket, WAIT_TYPE type, int wait_ms);

			static EnumNetError Connect(SOCKET socket, const StruIpAddr& stIpAddr, int wait_ms=-1);
		public:
			static void Init();

			//ָ����Ҫ�ͷ�
			static bool IpToSockAddr(const StruIpAddr &stIpAddr,struct sockaddr*p);
			static bool SockAddrToIp(const struct sockaddr* addr, StruIpAddr& stIpAddr);

			static bool SetNonBlock(SOCKET socket, bool is_non_block);

			static bool SetReusePort(SOCKET socket, bool set);

			//�Ƿ�Ϊ��Ч�׽���
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
			hints.ai_family = AF_UNSPEC; // AF_INET �� AF_INET6 ��ָ�� IPv4 �� IPv6
			hints.ai_socktype = 0;// SOCK_STREAM; // SOCK_DGRAM ���� UDP

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

			freeaddrinfo(res); // �ͷ�����
			return bOk;
		}
		
		inline EnumNetError SocketUtil::Bind(SOCKET socket, const StruIpAddr& stIpAddr)
		{
			//����sockaddr_in�ṹ�����
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

			//����sockaddr_in�ṹ�����
			struct sockaddr_in6 addr,*paddr=NULL;
			memset(&addr, 0, sizeof(addr));  //ÿ���ֽڶ���0���

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
			//����sockaddr_in�ṹ�����
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

			//�Ǹ�from����������ǶԷ��ĵ�ַ����һ�������������������ġ�
			//���׽��ֺ�IP���˿ڰ�
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
			�������select�Ĳ�����
			int maxfdp��һ������ֵ����ָ�����������ļ��������ķ�Χ���������ļ������������ֵ��1�����ܴ�
				��Windows�����������ֵ����ν���������ò���ȷ��
			fd_set *readfds��ָ��fd_set�ṹ��ָ�룬���������Ӧ�ð����ļ���������������Ҫ������Щ�ļ��������Ķ��仯�ģ�
				�����ǹ����Ƿ���Դ���Щ�ļ��� ��ȡ�����ˣ���������������һ���ļ��ɶ���select�ͻ᷵��һ������0��ֵ��
				��ʾ���ļ��ɶ������û�пɶ����ļ��������timeout�������ж� �Ƿ�ʱ��������timeout��ʱ�䣬select����0��
				���������󷵻ظ�ֵ�����Դ���NULLֵ����ʾ�������κ��ļ��Ķ��仯��
			fd_set *writefds��ָ��fd_set�ṹ��ָ�룬���������Ӧ�ð����ļ���������������Ҫ������Щ�ļ���������д�仯�ģ�
				�����ǹ����Ƿ��������Щ�ļ� ��д�������ˣ���������������һ���ļ���д��select�ͻ᷵��һ������0��ֵ��
				��ʾ���ļ���д�����û�п�д���ļ��������timeout�������� ���Ƿ�ʱ��������timeout��ʱ�䣬select����0��
				���������󷵻ظ�ֵ�����Դ���NULLֵ����ʾ�������κ��ļ���д�仯��
			fd_set *errorfdsͬ����������������ͼ�����������ļ������쳣��
			struct timeval* timeout��select�ĳ�ʱʱ�䣬�������������Ҫ��������ʹselect��������״̬����һ������NULL���βδ��룬
			��������ʱ��ṹ������ ��select��������״̬��һ���ȵ������ļ�������������ĳ���ļ������������仯Ϊֹ���ڶ���
			����ʱ��ֵ��Ϊ0��0���룬�ͱ��һ������ķ����������� �����ļ��������Ƿ��б仯�������̷��ؼ���ִ�У��ļ��ޱ仯����0��
			�б仯����һ����ֵ��������timeout��ֵ����0������ǵȴ��ĳ�ʱʱ�䣬�� select��timeoutʱ����������
			��ʱʱ��֮�����¼������ͷ����ˣ������ڳ�ʱ�󲻹�����һ�����أ�����ֵͬ������
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


			if (FD_ISSET(socket, &fds))  //��������룬��stdin�л�ȡ�����ַ�  
			{
				//�¼����
				return E_NET_ERROR_SUCCESS;
			}
			//��Ӧ��������
			//SIM_LERROR("select error ret=" << ret << " errno=" << errno);
			return E_NET_ERROR_FAILED;
		}

		inline void SocketUtil::Init()
		{
#ifdef OS_WINDOWS
			//��ʼ������
			static WsInit g_init;
#endif
		}
		inline bool SocketUtil::SetNonBlock(SOCKET socket, bool is_non_block)
		{

#ifdef OS_WINDOWS
			unsigned long ul = is_non_block;
			int ret = ioctlsocket(socket, FIONBIO, (unsigned long*)&ul);    //���óɷ�����ģʽ
			if (ret == SOCKET_ERROR)   //����ʧ��
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
			if (ret == SOCKET_ERROR)   //����ʧ��
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
			//����sockaddr_in�ṹ�����
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

			//���׽��ֺ�IP���˿ڰ�
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
				// ʹ��inet_pton��IPv6��ַ�ַ���ת��Ϊ��������ʽ
				if (inet_pton(AF_INET6, stIpAddr.strIp.c_str(), &serv_addr6->sin6_addr) <= 0) {
					// ���inet_ptonʧ�ܣ����ش���
					perror("inet_pton:");
					return false;
				}

				// ���õ�ַ��ΪAF_INET6
				serv_addr6->sin6_family = AF_INET6;

				// ���ö˿ں�
				serv_addr6->sin6_port = htons(stIpAddr.usPort);

				// ���sin6_flowinfo��sin6_scope_id�������Ҫ�Ļ���
				serv_addr6->sin6_flowinfo = 0;
				serv_addr6->sin6_scope_id = 0; // ��������Ҫָ���ض���������ID

				return true;
			}
			else if (stIpAddr.eType == E_IP_ADDR_TYPE_IPV4)
			{
				struct sockaddr_in* serv_addr = (struct sockaddr_in*)p;
				// ʹ��inet_pton��IPv4��ַ�ַ���ת��Ϊ��������ʽ
				if (inet_pton(AF_INET, stIpAddr.strIp.c_str(), &serv_addr->sin_addr) <= 0) {
					// ���inet_ptonʧ�ܣ����ش���
					perror("inet_pton:");
					return false;
				}

				// ���õ�ַ��ΪAF_INET6
				serv_addr->sin_family = AF_INET;

				// ���ö˿ں�
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
					// ���inet_ptonʧ�ܣ����ش���
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
					// ���inet_ptonʧ�ܣ����ش���
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