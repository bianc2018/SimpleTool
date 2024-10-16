/*
* socket ����
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
    #error "��֧�ֵ�ƽ̨"
#endif


#include "NetBase.hpp"

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
			// SOCK_STREAM tcp SOCK_DGRAM udp
			static SOCKET CreateSocket(SockType type);

			static SOCKET CreateSocket(int af, int type, int protocol);

		public:
			//��װ�ĺ��� ͬ���ӿ�
			//���� false ��ʶ��ֹ
			typedef bool(*GetHostByNameCallBack)(const char* ip, void* pdata);
			static EnumNetError GetHostByName(const char* szHost,
				GetHostByNameCallBack cb, void* pdata);

			static bool GetFirstIpByName(const char* szHost, char* ip_buff, int ip_buff_len);

			static EnumNetError Connect(SOCKET socket,const char* ipaddr, unsigned short port);

			/*
				��Ҫ����Ϊ�Ƕ��������򷵻��쳣
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

			/***** 2021/03/05 ���� ��ʱ�ӿ� ******/

			//�ȴ�ʱ�� ,int wait_ms=-1
			enum WAIT_TYPE
			{
				WAIT_READ,
				WAIT_WRITE,
			};
			static EnumNetError WaitTimeOut(SOCKET socket, WAIT_TYPE type, int wait_ms);
		public:
			static void Init();

			//�ṹ��ת��
			static bool IpToAddressV4(const char* ipaddr, unsigned short port
				, struct sockaddr_in* out_addr);
			static bool AddressToIpV4(const struct sockaddr_in* addr,
				char* ipaddr, unsigned int ipaddr_len, unsigned short* port
			);

			static bool SetNonBlock(SOCKET socket, bool is_non_block);

			static bool SetReusePort(SOCKET socket, bool set);

			//�Ƿ�Ϊ��Ч�׽���
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

			//����sockaddr_in�ṹ�����
			struct sockaddr_in serv_addr;
			if (!IpToAddressV4(ipaddr, port, &serv_addr))
				return E_NET_ERROR_PARAM;
			//���׽��ֺ�IP���˿ڰ�
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
			//����sockaddr_in�ṹ�����
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

			//����sockaddr_in�ṹ�����
			struct sockaddr_in addr;
			memset(&addr, 0, sizeof(addr));  //ÿ���ֽڶ���0���

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
			//����sockaddr_in�ṹ�����
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

			//�Ǹ�from����������ǶԷ��ĵ�ַ����һ�������������������ġ�
			//���׽��ֺ�IP���˿ڰ�
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
		inline bool SocketUtil::IpToAddressV4(const char* ipaddr, unsigned short port, sockaddr_in* out_addr)
		{
			if (NULL == out_addr)
				return false;
			memset(out_addr, 0, sizeof(*out_addr));  //ÿ���ֽڶ���0���
			out_addr->sin_family = AF_INET;  //ʹ��IPv4��ַ
			if (ipaddr)
				out_addr->sin_addr.s_addr = inet_addr(ipaddr);  //�����IP��ַ
			else
				out_addr->sin_addr.s_addr = htonl(INADDR_ANY);  //����ip
			out_addr->sin_port = htons(port);  //�˿�
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
			int ret = ioctlsocket(socket, FIONBIO, (unsigned long*)&ul);    //���óɷ�����ģʽ
			if (ret == SOCKET_ERROR)   //����ʧ��
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
			if (ret == SOCKET_ERROR)   //����ʧ��
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