/*
* 网络工具库的IOCP的实现
*/
#ifndef SIM_NET_IOCP_HPP_
#define SIM_NET_IOCP_HPP_
#include "Types.hpp"
#ifdef  OS_WINDOWS

#include "Net/SocketUtil.hpp"
#include "Logger.hpp"
#include "Thread/Mutex.hpp"
#include <MSWSock.h>
namespace sim
{
    namespace net
    {
        class IocpChannel;
        class IocpManager;
        //事件类型
        enum IOCPType
        {
            //连接建立事件
            IOCPConnect,

            //接受连接事件
            IOCPAccept,

            //接收数据事件
            IOCPRecv,

            //数据发送事件
            IOCPSend,

            //连接关闭事件
            IOCPClose,
        };

        //wsa拓展函数加载对象
        class WsaExFunction
        {
        public:
            WsaExFunction()
            {
                //初始化socket
                SocketUtil::Init();

                //创建一个空的socket
                SOCKET socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

                //加载拓展函数
                __AcceptEx = (LPFN_ACCEPTEX)_GetExFunctnion(socket, WSAID_ACCEPTEX);
                __ConnectEx = (LPFN_CONNECTEX)_GetExFunctnion(socket, WSAID_CONNECTEX);
                __AcceptExScokAddrs = (LPFN_GETACCEPTEXSOCKADDRS)_GetExFunctnion(socket, WSAID_GETACCEPTEXSOCKADDRS);
                __DisconnectionEx = (LPFN_DISCONNECTEX)_GetExFunctnion(socket, WSAID_DISCONNECTEX);

                //关闭连接
                closesocket(socket);
            }
        private:
            //加载拓展函数
            static void* _GetExFunctnion(const SOCKET& socket, const GUID& which)
            {
                void* func = nullptr;
                DWORD bytes = 0;
                WSAIoctl(socket, SIO_GET_EXTENSION_FUNCTION_POINTER, (LPVOID)&which,
                    sizeof(which), &func, sizeof(func), &bytes, NULL, NULL);

                return func;

            }
        public:
            LPFN_ACCEPTEX                __AcceptEx;
            LPFN_CONNECTEX               __ConnectEx;
            LPFN_GETACCEPTEXSOCKADDRS    __AcceptExScokAddrs;
            LPFN_DISCONNECTEX            __DisconnectionEx;
        };

        //异步事件
        class IocpNetEvent
        {
        public:
            //IO重叠对象
            OVERLAPPED  overlapped;

            //事件类型
            IOCPType type;

            //子连接存在 acceptex时候启用
            SOCKET accepted;

            //缓存
            RefBuff buff;
            //buff的数据引用
            WSABUF wsa_buf;

            //传输的字节数目
            DWORD bytes_transfered;

            //传输偏移，相对于buff
            UInt32 offset;

            //地址 用于SendTo or Recvfrom
            struct sockaddr_in6 temp_addr;
            int temp_addr_len;

            RefWeakObject<IocpChannel> refChannel;

            //异步操作结果
            EnumNetError eRet;

            //初始化
            IocpNetEvent(RefObject<IocpChannel> refChanel_) :bytes_transfered(0)
                , refChannel(refChanel_)
                , eRet(E_NET_ERROR_SUCCESS)
                , accepted(INVALID_SOCKET)
                , type(IOCPConnect)
            {
                memset(&overlapped, 0, sizeof(overlapped));
                memset(&wsa_buf, 0, sizeof(wsa_buf));
                memset(&temp_addr, 0, sizeof(temp_addr));
                temp_addr_len = sizeof(temp_addr);
            }
            ~IocpNetEvent()
            {
                if (SocketUtil::IsVaild(accepted))
                {
                    SocketUtil::Close(accepted);
                    accepted = INVALID_SOCKET;
                }
            }

        };

        //Iocp通道
        class IocpChannel :public Channel
        {
            friend class IocpManager;
        public:
            IocpChannel(IocpManager& myIocpManager, SOCKET socket);
            virtual ~IocpChannel();

            //切换通道绑定的协议
            virtual EnumNetError Switch(sim::RefObject <Protocol> pro);

            //网络接口
            //接收一个 StruIpAddr 类型的参数，返回一个 EnumNetError 类型的值
            virtual EnumNetError Bind(const StruIpAddr& stIpAddr);


            //接收一个 StruIpAddr 类型的参数，返回一个 EnumNetError 类型的值
            virtual EnumNetError StartConnect(const StruIpAddr& stIpAddr);

            //开始接受一个链接
            virtual EnumNetError StartAccept();

            //没有参数，也没有返回值
            virtual void Close();

            //异步发送数据
            //stIpAddr只有当udp而且没有进行链接有效，其他情况会被忽略掉
            virtual EnumNetError StartWrite(RefBuff& stBuff, StruIpAddr* stIpAddr = NULL);

            //开始读取数据，bKeep 是否一直读取，false 只会进行一次读取，true一直读取，直到链接断开
            virtual EnumNetError StartRead(RefBuff stBuff = RefBuff(), bool bKeep = true);

        public:
            //返回类型，见SIM_NET_CHANNEL_TYPE_定义
            virtual TypeNetChannel Type() {
                return m_Typeflag;
            }

            //是否已经链接
            virtual bool IsConnect()
            {
                return m_socket != INVALID_SOCKET && m_bConnectFlag;
            }

            virtual bool SetAutoMTU(UInt32 mtu = 0)
            {
                m_nMtu = mtu;
                return true;
            }
        private:
            //网络事件
            virtual EnumNetError HandleEvent(IocpNetEvent* pE);

            //设置自身调用
            virtual void SetSelf(RefObject<IocpChannel> self);

            virtual EnumNetError HandleConnect(IocpNetEvent* pE);
            virtual EnumNetError HandleAccept(IocpNetEvent* pE);
            virtual EnumNetError HandleRecv(IocpNetEvent* pE);
            virtual EnumNetError HandleSend(IocpNetEvent* pE);
            //virtual EnumNetError HandleClose(IocpNetEvent* pE);
        private:
            TypeNetChannel m_Typeflag;
            sim::RefObject <Protocol> m_pPro;
            IocpManager& m_myIocpManager;
            SOCKET m_socket;
            RefWeakObject<IocpChannel> m_pSelf;

            bool m_bBindFlag;
            bool m_bStartReadFlag;
            bool m_bStartAcceptFlag;
            bool m_bConnectFlag;
            bool m_bListenFlag;
            bool m_bKeepReadFlag;

            UInt32 m_nMtu;
        };


        //Iocp网络管理器基类
        class IocpManager :public Manager
        {
            friend class IocpChannel;
        public:
            IocpManager();

            virtual ~IocpManager();

            //显式初始化
            virtual EnumNetError Init(int nThreadnum = 8);
            virtual EnumNetError UnInit();

            //创建通道
            //typeflag  类型，见SIM_NET_CHANNEL_TYPE_定义
            //pro       在这个通道上面的网络协议，可以为空
            //创建失败返回空
            virtual sim::RefObject<Channel> CreateChannel(TypeNetChannel typeflag, sim::RefObject <Protocol> pro );

            virtual sim::RefObject<IocpChannel> CreateChannelBySocket(SOCKET sock);

            //主动解绑通道，Manager不再管理这个通道，之后ch不可用
            virtual EnumNetError UnBindChannel(::sim::RefObject<Channel> ch);

            //事件循环，不推出，直到执行Exit
            //wait_ms 等待时间,bOnce 执行一次事件后退出
            virtual EnumNetError Poll(int wait_ms, bool bOnce = false);

            //退出Poll，所有堵塞Poll线程退出
            virtual void ExitPoll();

            //获取当前的通道数量
            virtual UInt64 GetChannelSize();

        protected:
            void IncChannelSize();
            void DecChannelSize();
            WsaExFunction& GetExFunc();
        protected:
            HANDLE m_hIocp;
            bool m_bExitflag;

            Mutex m_mtxChannelSize;
            UInt64 m_uChannelSize;
        };


        inline IocpManager::IocpManager() :
            m_hIocp(INVALID_HANDLE_VALUE),
            m_bExitflag(false),
            m_uChannelSize(0)
        {
            //IocpManager::Init();
        }

        inline IocpManager::~IocpManager()
        {
            IocpManager::ExitPoll();
            IocpManager::UnInit();
        }
        inline EnumNetError IocpManager::Init(int nThreadnum)
        {
            //初始化
            SocketUtil::Init();
            if (INVALID_HANDLE_VALUE == m_hIocp)
            {
                m_hIocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 16);
                SIM_LDEBUG("IocpManager init 0x" << SIM_HEX(m_hIocp) << " thread_num " << 1);
                if (INVALID_HANDLE_VALUE == m_hIocp || NULL == m_hIocp)
                {
                    return E_NET_ERROR_FAILED;
                }
            }
            return E_NET_ERROR_SUCCESS;
        }

        inline EnumNetError IocpManager::UnInit()
        {
            SIM_FUNC_DEBUG();
            if (m_hIocp != INVALID_HANDLE_VALUE)
            {
                SIM_LDEBUG("IocpManager CloseHandle 0x" << SIM_HEX(m_hIocp));
                CloseHandle(m_hIocp);
                m_hIocp = INVALID_HANDLE_VALUE;
            }
            return E_NET_ERROR_SUCCESS;
        }
        inline sim::RefObject<Channel> IocpManager::CreateChannel(TypeNetChannel typeflag, sim::RefObject <Protocol> pro)
        {
            if (m_hIocp == INVALID_HANDLE_VALUE || m_bExitflag)
            {
                SIM_LERROR("IocpManager is not init");
                return NULL;
            }

            if (typeflag & SIM_NET_CHANNEL_TYPE_SSL)
            {
                SIM_LERROR("IocpManager 0x" << SIM_HEX(m_hIocp) << " typeflag 0x" << SIM_HEX(typeflag) << " not support");
                return NULL;
            }
            SOCKET sock = SocketUtil::CreateSocket(typeflag);
            if (sock <= 0)
            {
                SIM_LERROR("IocpManager 0x" << SIM_HEX(m_hIocp) << " CreateSocket 0x" << SIM_HEX(typeflag) << " sock=" << sock);
                return NULL;
            }
            sim::RefObject<IocpChannel> ch = CreateChannelBySocket(sock);
            if (ch)
            {
                ch->Switch(pro);
                ch->m_Typeflag = typeflag;
            }
            return sim::reinterpret_pointer_cast<Channel>(ch);
        }
        inline EnumNetError IocpManager::UnBindChannel(::sim::RefObject<Channel> ch)
        {
            if (ch)
            {
                ch->Close();
            }
            return E_NET_ERROR_SUCCESS;
        }
        inline EnumNetError IocpManager::Poll(int wait_ms, bool bOnce)
        {
            //传输数据长度
            DWORD               bytes_transfered = 0;
            //iocp ctx
            SOCKET socket = 0;
            //IO重叠对象指针
            OVERLAPPED* over_lapped = NULL;
            EnumNetError eRet = E_NET_ERROR_SUCCESS;
            while (!m_bExitflag)
            {
                over_lapped = NULL;
                bytes_transfered = 0;
                socket = 0;

                //事务处理
                //取事件数据
                BOOL res = GetQueuedCompletionStatus(m_hIocp,
                    &bytes_transfered, PULONG_PTR(&socket),
                    &over_lapped, wait_ms);

                //调试打印
               /* SIM_LDEBUG("GetQueuedCompletionStatus[" << m_hIocp << "] res=" << res
                    << " socket " << socket << " bytes_transfered " << bytes_transfered
                    << " over_lapped=0x" << SIM_HEX(over_lapped));*/

                if (over_lapped)
                {
                    SIM_LDEBUG("GetQueuedCompletionStatus[" << m_hIocp << "] res=" << res
                        << " socket " << socket << " bytes_transfered " << bytes_transfered
                        << " over_lapped=0x" << SIM_HEX(over_lapped));

                    //转换为事件对象
                    IocpNetEvent* socket_event = CONTAINING_RECORD(over_lapped, IocpNetEvent, overlapped);
                    socket_event->bytes_transfered = bytes_transfered;
                    RefObject<IocpChannel> pChan = socket_event->refChannel.ref_object();
                    if (pChan)
                    {
                        //获取错误码
                        DWORD dw_err = GetLastError();
                        if (FALSE == res)
                        {
                            //发生错误 后续细化
                            if (dw_err == WAIT_TIMEOUT)
                            {
                                socket_event->eRet = E_NET_ERROR_TIMEOUT;
                            }
                            else
                            {
                                socket_event->eRet = E_NET_ERROR_FAILED;
                            }
                            SIM_LERROR("OnError dw_err=" << dw_err << " by socket " << pChan->m_socket
                                << " type " << socket_event->type);
                        }
                        else
                        {
                            socket_event->eRet = E_NET_ERROR_SUCCESS;
                        }

                        pChan->HandleEvent(socket_event);
                    }
                    else
                    {
                        delete socket_event;
                        eRet = E_NET_ERROR_FAILED;
                    }
                }
                else if(res == FALSE)
                {
                    //over_lapped==NULL 出现异常
                    eRet = E_NET_ERROR_FAILED;
                   /* SIM_LERROR("GetQueuedCompletionStatus Failed[" << m_hIocp << "] res=" << res
                        << " socket " << socket << " bytes_transfered " << bytes_transfered
                        << " over_lapped=0x" << SIM_HEX(over_lapped));*/
                }


                if (bOnce)
                {
                    break;
                }
            };
            return eRet;
        }
        inline void IocpManager::ExitPoll()
        {
            m_bExitflag = true;
        }
        inline UInt64 IocpManager::GetChannelSize()
        {
            AutoMutex lk(m_mtxChannelSize);
            return m_uChannelSize;
        }
        inline void IocpManager::IncChannelSize()
        {
            AutoMutex lk(m_mtxChannelSize);
            ++m_uChannelSize;
        }
        inline void IocpManager::DecChannelSize()
        {
            AutoMutex lk(m_mtxChannelSize);
            --m_uChannelSize;
        }

        inline WsaExFunction& IocpManager::GetExFunc()
        {
            static WsaExFunction gs_WsaExFunction;
            return gs_WsaExFunction;
        }
        inline sim::RefObject<IocpChannel> IocpManager::CreateChannelBySocket(SOCKET sock)
        {
            SocketUtil::SetNonBlock(sock, true);

            //创建绑定
            HANDLE iocp_handler = CreateIoCompletionPort((HANDLE)sock, m_hIocp,
                (ULONG_PTR)(sock), 0);
            if (NULL == iocp_handler)
            {
                //ReleaseCtx(ref->sock.GetSocket());
                SIM_LERROR("IocpManager 0x" << SIM_HEX(m_hIocp)<<" sock=" << sock << "  WSAGetLastError()=" << WSAGetLastError());
                SocketUtil::Close(sock);
                return NULL;
            }

            ::sim::RefObject<IocpChannel> pChan = sim::RefObject<IocpChannel>(new IocpChannel(*this, sock));
            pChan->SetSelf(pChan);
            return  pChan;
        }

        inline IocpChannel::IocpChannel(IocpManager& myIocpManager, SOCKET socket)
            :m_pSelf(/*使用空指针填充先*/)
            , m_myIocpManager(myIocpManager)
            , m_Typeflag(0)
            , m_pPro(NULL)
            , m_socket(socket)
            , m_bBindFlag(false)
            , m_bStartReadFlag(false)
            , m_bStartAcceptFlag(false)
            , m_bConnectFlag(false)
            , m_bKeepReadFlag(false)
            , m_bListenFlag(false)
            , m_nMtu(0)
        {
            m_myIocpManager.IncChannelSize();
        }

        inline IocpChannel::~IocpChannel()
        {
            IocpChannel::Close();
            m_myIocpManager.DecChannelSize();
        }

        inline void IocpChannel::SetSelf(RefObject<IocpChannel> self)
        {
            m_pSelf = self;
        }

        inline EnumNetError IocpChannel::Switch(sim::RefObject<Protocol> pro)
        {
            m_pPro = pro;
            return E_NET_ERROR_SUCCESS;
        }

        inline EnumNetError IocpChannel::Bind(const net::StruIpAddr& stIpAddr)
        {
            EnumNetError eRet = SocketUtil::Bind(m_socket, stIpAddr);
            if (eRet == E_NET_ERROR_SUCCESS)
            {
                m_bBindFlag = true;
            }
            //udp绑定的时候就已经完成链接
            if (!(m_Typeflag & SIM_NET_CHANNEL_TYPE_TCP))
            {
                if (m_pPro)
                {
                    m_pPro->OnConnect(sim::reinterpret_pointer_cast<Channel>(m_pSelf.ref_object()), eRet);
                }
                if (eRet == E_NET_ERROR_SUCCESS)
                {
                    m_bConnectFlag = true;
                }
            }
            return eRet;
        }

        inline EnumNetError IocpChannel::StartConnect(const net::StruIpAddr& stIpAddr)
        {
            RefObject<IocpChannel> refSelf = m_pSelf.ref_object();
            if (!refSelf)
            {
                SIM_LERROR("crefSelf error ");
                return E_NET_ERROR_OBJECT;
            }

            struct sockaddr_in serv_addr;
            memset(&serv_addr, 0, sizeof(serv_addr));
            struct sockaddr_in6 serv_addr6;
            memset(&serv_addr6, 0, sizeof(serv_addr6));
            struct sockaddr* addr_name = NULL;
            int addr_namelen = 0;
            if (stIpAddr.eType == E_IP_ADDR_TYPE_IPV6)
            {
                if (!SocketUtil::IpToSockAddr(stIpAddr, (struct sockaddr*)&serv_addr6))
                    return E_NET_ERROR_PARAM;
                addr_name = (struct sockaddr*)&serv_addr6;
                addr_namelen = sizeof(serv_addr6);
            }
            else if (stIpAddr.eType == E_IP_ADDR_TYPE_IPV4)
            {
                if (!SocketUtil::IpToSockAddr(stIpAddr, (struct sockaddr*)&serv_addr))
                    return E_NET_ERROR_PARAM;
                addr_name = (struct sockaddr*)&serv_addr;
                addr_namelen = sizeof(serv_addr);
            }
            else
            {
                return E_NET_ERROR_PARAM;
            }

            if (!m_bBindFlag)
            {
                net::StruIpAddr stNullIpAddr = stIpAddr;
                stNullIpAddr.strIp = "";
                stNullIpAddr.usPort = 0;
                EnumNetError eRet = SocketUtil::Bind(m_socket, stNullIpAddr);
                if (eRet == E_NET_ERROR_SUCCESS)
                {
                    m_bBindFlag = true;
                }
                else
                {
                    return eRet;
                }
            }

            //新建事件
            IocpNetEvent* e = new IocpNetEvent(refSelf);
            if (NULL == e)
            {
                SIM_LERROR("create IocpNetEvent error ");
                return E_NET_ERROR_NEW_BUFF;
            }

            OVERLAPPED* pol = &e->overlapped;
            e->type = IOCPConnect;
            
            WsaExFunction exfunc = m_myIocpManager.GetExFunc();
            int res = exfunc.__ConnectEx(m_socket,
                (sockaddr*)addr_name, addr_namelen, (PVOID)NULL, 0, (DWORD*)&(e->bytes_transfered), pol);

            if ((SOCKET_ERROR == res) && (WSA_IO_PENDING != WSAGetLastError())) {
                delete e;
                SIM_LERROR("exfunc.__ConnectEx error res=" << res << "  WSAGetLastError()=" << WSAGetLastError());
                return E_NET_ERROR_FAILED;
            }
            return E_NET_ERROR_SUCCESS;
        }

        inline EnumNetError IocpChannel::StartAccept()
        {
            RefObject<IocpChannel> refSelf = m_pSelf.ref_object();
            if (!refSelf)
            {
                SIM_LERROR("crefSelf error ");
                return E_NET_ERROR_OBJECT;
            }

            if (!m_bListenFlag)
            {
                EnumNetError eRet = SocketUtil::Listen(m_socket, 1024);
                if (eRet == E_NET_ERROR_SUCCESS)
                {
                    m_bListenFlag = true;
                }
                else
                {
                    return eRet;
                }
            }

            //新建事件
            IocpNetEvent* e = new IocpNetEvent(refSelf);
            if (NULL == e)
            {
                SIM_LERROR("create IocpNetEvent error ");
                return E_NET_ERROR_NEW_BUFF;
            }

            OVERLAPPED* pol = &e->overlapped;
            e->type = IOCPAccept;

            int addr_len = sizeof(SOCKADDR_IN);
            if(m_Typeflag&E_IP_ADDR_TYPE_IPV6)
                addr_len = sizeof(SOCKADDR_IN6);
            addr_len += 16;

            e->buff = RefBuff(addr_len * 2);
            e->buff.set(0);
            e->wsa_buf.buf = e->buff.get();
            e->wsa_buf.len = e->buff.size();

            //创建子连接
            e->accepted = SocketUtil::CreateSocket(m_Typeflag);
            if (e->accepted <= 0)
            {
                SIM_LERROR("create socket error ");
                delete e;
                return E_NET_ERROR_FAILED;
            }
            DWORD dwFlags = 0;

            //使用AcceptEx
            WsaExFunction exfunc = m_myIocpManager.GetExFunc();
            int res = exfunc.__AcceptEx(m_socket, e->accepted,
                e->wsa_buf.buf, 0, addr_len, addr_len,
                /*(DWORD*)&e->bytes_transfered*/ NULL, pol);

            if ((SOCKET_ERROR == res) && (WSA_IO_PENDING != WSAGetLastError())) {
                SocketUtil::Close(e->accepted);
                delete e;
                SIM_LERROR("exfunc.__AcceptEx error res=" << res << "  WSAGetLastError()=" << WSAGetLastError());
                return E_NET_ERROR_FAILED;
            }
            m_bStartAcceptFlag = true;


            return E_NET_ERROR_SUCCESS;
        }

        inline void IocpChannel::Close()
        {
            if (SocketUtil::IsVaild(m_socket))
            {
                SocketUtil::Close(m_socket);
                m_socket = INVALID_SOCKET;
            }

            if (m_pPro)
            {
                m_pPro->OnClose(sim::reinterpret_pointer_cast<Channel>(m_pSelf.ref_object()), E_NET_ERROR_FAILED);
                m_pPro = NULL;
            }
            m_pSelf.reset();
        }

        inline EnumNetError IocpChannel::StartWrite(RefBuff& stBuff, StruIpAddr* stIpAddr)
        {
            RefObject<IocpChannel> refSelf = m_pSelf.ref_object();
            if (!refSelf)
            {
                SIM_LERROR("crefSelf error ");
                return E_NET_ERROR_OBJECT;
            }

            UInt64 nOffset = 0;

            do
            {
                //新建事件
                IocpNetEvent* e = new IocpNetEvent(refSelf);
                if (NULL == e)
                {
                    SIM_LERROR("create IocpNetEvent error ");
                    return E_NET_ERROR_NEW_BUFF;
                }

                OVERLAPPED* pol = &e->overlapped;
                e->type = IOCPSend;
                DWORD dwFlags = 0;
                e->buff = stBuff;
                e->wsa_buf.buf = stBuff.get()+ nOffset;
                e->offset = nOffset;
                if (m_nMtu == 0)
                {
                    e->wsa_buf.len = stBuff.size();
                }
                else
                {
                    UInt64 leftsize = stBuff.size() - nOffset;
                    if (leftsize > m_nMtu)
                    {
                        e->wsa_buf.len = m_nMtu;
                    }
                    else
                    {
                        e->wsa_buf.len = leftsize;
                    }
                }

                nOffset += e->wsa_buf.len;

                DWORD* bytes_transfered = &e->bytes_transfered;

                if (stIpAddr)
                {
                    if (!SocketUtil::IpToSockAddr(*stIpAddr, (struct sockaddr*)&e->temp_addr))
                    {
                        delete e;
                        return E_NET_ERROR_PARAM;
                    }

                    //这里使用WSASendTo接口
                    int res = WSASendTo(m_socket, &e->wsa_buf, 1,
                        bytes_transfered, dwFlags, (struct sockaddr*)&e->temp_addr, e->temp_addr_len, pol, nullptr);

                    if ((SOCKET_ERROR == res) && (WSA_IO_PENDING != WSAGetLastError())) {
                        delete e;
                        SIM_LERROR("WSASend error res=" << res << "  WSAGetLastError()=" << WSAGetLastError());
                        return E_NET_ERROR_FAILED;
                    }
                }
                else
                {
                    //发送请求
                    int res = WSASend(m_socket, &e->wsa_buf, 1,
                        bytes_transfered, dwFlags, pol, nullptr);

                    if ((SOCKET_ERROR == res) && (WSA_IO_PENDING != WSAGetLastError())) {
                        delete e;//失败删除事件
                        SIM_LERROR(m_socket << " WSASend error res=" << res << "  WSAGetLastError()=" << WSAGetLastError());
                        return E_NET_ERROR_FAILED;
                    }
                    
                }

                if (nOffset >= stBuff.size())
                    return E_NET_ERROR_SUCCESS;
            } while (true);
        }

        inline EnumNetError IocpChannel::StartRead(RefBuff stBuff, bool bKeep)
        {
            RefObject<IocpChannel> refSelf = m_pSelf.ref_object();
            if (!refSelf)
            {
                SIM_LERROR("crefSelf error ");
                return E_NET_ERROR_OBJECT;
            }
            m_bKeepReadFlag = bKeep;

            //新建事件
            IocpNetEvent* e = new IocpNetEvent(refSelf);
            if (NULL == e)
            {
                SIM_LERROR("create IocpNetEvent error ");
                return E_NET_ERROR_NEW_BUFF;
            }

            OVERLAPPED* pol = &e->overlapped;
            e->type = IOCPRecv;
            if (stBuff.size() > 0)
                e->buff = stBuff;
            else
                e->buff = RefBuff(4 * 1024);//入参没有，则自动申请，后续配置化
            e->wsa_buf.buf = e->buff.get();
            e->wsa_buf.len = e->buff.size();

            DWORD dwFlags = 0;
            int res = SOCKET_ERROR;
            if (m_Typeflag & SIM_NET_CHANNEL_TYPE_TCP)
            {
                res = WSARecv(m_socket, &e->wsa_buf, 1, (DWORD*)&e->bytes_transfered, &dwFlags, pol, nullptr);
            }
            else
            {
                res = WSARecvFrom(m_socket, &e->wsa_buf, 1, (DWORD*)&e->bytes_transfered, &dwFlags,
                    (struct sockaddr*)&e->temp_addr, &e->temp_addr_len, pol, nullptr);
            }
           
            int err = WSAGetLastError();
            if ((SOCKET_ERROR == res) && (WSA_IO_PENDING != WSAGetLastError())) {
                delete e;
                //printf("delete event %p at %d\n", e, __LINE__);
                SIM_LERROR("WSARecv error res=" << res << " WSAGetLastError = " << err);
                return E_NET_ERROR_FAILED;
            }
            
            m_bKeepReadFlag = bKeep;
            return E_NET_ERROR_SUCCESS;
        }

        inline EnumNetError IocpChannel::HandleEvent(net::IocpNetEvent* pE)
        {
            if (!pE)
            {
                return E_NET_ERROR_PARAM;
            }
            
            switch (pE->type)
            {
            case IOCPConnect:
                return HandleConnect(pE);
            case IOCPAccept:
                return HandleAccept(pE);
            case IOCPRecv:
                return HandleRecv(pE);
            case IOCPSend:
                return HandleSend(pE);
           /* case IOCPClose:
                return HandleClose(pE);*/
            default:
                break;
            }

            return E_NET_ERROR_UNDEF;
        }

        inline EnumNetError IocpChannel::HandleConnect(IocpNetEvent* pE)
        {
            if (m_pPro)
            {
                m_pPro->OnConnect(sim::reinterpret_pointer_cast<Channel>(m_pSelf.ref_object()), pE->eRet);
            }

            if (pE->eRet == E_NET_ERROR_SUCCESS)
            {
                m_bConnectFlag = true;
            }
            else
            {
                m_bConnectFlag = false;
                Close();
            }
            return pE->eRet;
        }

        inline EnumNetError IocpChannel::HandleAccept(IocpNetEvent* pE)
        {
            if (pE->eRet != E_NET_ERROR_SUCCESS)
            {
                Close();
                return pE->eRet;
            }

            bool bCloseAccept = false;
            if (m_pPro)
            {
                sim::RefObject<IocpChannel> pChan = m_myIocpManager.CreateChannelBySocket(pE->accepted);
                if (pChan)
                {
                    pChan->Switch(m_pPro);
                    pChan->m_Typeflag = m_Typeflag;
                    pChan->SetAutoMTU(m_nMtu);//继承
                    EnumNetError eAcceptError=m_pPro->OnAccept(sim::reinterpret_pointer_cast<Channel>(m_pSelf.ref_object())
                        , sim::reinterpret_pointer_cast<Channel>(pChan));
                    if (eAcceptError != E_NET_ERROR_SUCCESS)
                    {
                        pChan->Switch(NULL);
                        pChan->Close();
                    }

                }
                else
                {
                    bCloseAccept = true;
                }
            }

            if (bCloseAccept)
            {
                SocketUtil::Close(pE->accepted);
            }
            //防止被自动回收
            pE->accepted = INVALID_SOCKET;

            //开始监听下一个链接
            EnumNetError eRet = StartAccept();
            if (eRet != E_NET_ERROR_SUCCESS)
            {
                //失败关闭
                SIM_LERROR("StartAccept error:"<<eRet);
                Close();
            }
            return E_NET_ERROR_SUCCESS;
        }

        inline EnumNetError IocpChannel::HandleRecv(IocpNetEvent* pE)
        {
            if (pE->eRet != E_NET_ERROR_SUCCESS)
            {
                Close();
                return pE->eRet;
            }

            if (pE->bytes_transfered == 0)
            {
                //0 链接断开
                Close();
                return pE->eRet;
            }

            if (m_pPro)
            {
                StruIpAddr stIpAddr;
                if ((!(m_Typeflag & SIM_NET_CHANNEL_TYPE_TCP))
                    &&!SocketUtil::SockAddrToIp((const sockaddr*)&pE->temp_addr, stIpAddr))
                {
                    return E_NET_ERROR_PARAM;
                }

                m_pPro->OnReaded(sim::reinterpret_pointer_cast<Channel>(m_pSelf.ref_object()), pE->buff, pE->bytes_transfered, stIpAddr);
            }

            if (m_bKeepReadFlag)
            {
                //读下一个
                EnumNetError eRet = StartRead(pE->buff, m_bKeepReadFlag);
                if (eRet != E_NET_ERROR_SUCCESS)
                {
                    //失败关闭
                    SIM_LERROR("StartRead error:" << eRet);
                    Close();
                }
            }
            return E_NET_ERROR_SUCCESS;
        }

        inline EnumNetError IocpChannel::HandleSend(IocpNetEvent* pE)
        {
            if (m_pPro)
            {
                m_pPro->OnWrited(sim::reinterpret_pointer_cast<Channel>(m_pSelf.ref_object()), pE->buff, pE->offset, pE->bytes_transfered, pE->eRet);
            }
            return E_NET_ERROR_SUCCESS;
        }
    }
}
#endif // OS_WINDOWS
#endif //!SIM_NET_BASE_HPP_