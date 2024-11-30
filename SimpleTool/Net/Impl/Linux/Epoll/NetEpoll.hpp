/*
* 网络工具库的Epoll的实现
*/
#ifndef SIM_NET_EPOLL_HPP_
#define SIM_NET_EPOLL_HPP_
#include "Types.hpp"
#ifdef  OS_LINUX

#include "Net/SocketUtil.hpp"
#include "Logger.hpp"
#include "Thread/Mutex.hpp"
#include "RefObject.hpp"
#include "Struct/Queue.hpp"
#include <cstddef>
//引入linux平台接口
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
namespace sim
{
    namespace net
    {
        class EpollChannel;
        class EpollManager;

        //对象
        class myEpollEvent :public epoll_event
        {
        public:
            myEpollEvent()
            {
                events = 0;
                data.ptr=this;
                bAdd = false;
            }
        public:
            //通道引用
            RefWeakObject<EpollChannel> pWeakCh;
            bool bAdd;
        };

        //epoll 数据对象
        struct EpollDataObject
        {
            //数据缓存
            RefBuff stBuff;
            //写目标地址或者读来源，udp有用
            StruIpAddr stIpAddr;
            //写偏移
            UInt32 uWriteOffset;

            EpollDataObject()
            {
                uWriteOffset = 0;
            }
        };
        
        
        //Iocp通道
        class EpollChannel :public Channel
        {
            friend class EpollManager;
        public:
            EpollChannel(EpollManager& myEpollManager, SOCKET socket);
            virtual ~EpollChannel();

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

        private:
            myEpollEvent & GetEvent();
          
            //获取socket
            SOCKET GetSocket()
            {
                return m_socket;
            }

            //事件触发
            void HandleEvents(uint32_t events);

            void HandleIn();
            void HandleOut();
            void HandleErr();
            void HandleHub();

            //没有参数，也没有返回值
            virtual void InnerClose(EnumNetError eRet);
        private:
            TypeNetChannel m_Typeflag;
            sim::RefObject <Protocol> m_pPro;
            EpollManager& m_myEpollManager;
            SOCKET m_socket;
            myEpollEvent m_stEvent;

            bool m_bBindFlag;
            bool m_bStartReadFlag;
            bool m_bStartAcceptFlag;
            bool m_bConnectFlag;
            bool m_bListenFlag;
            
            //边缘模式,==0不设置边缘模式
            uint32_t m_uEtflag;

            sim::Mutex m_mtxRead;
            bool m_bKeepReadFlag;
            RefBuff m_stKeepBuff;
            //读缓存
            sim::Queue<EpollDataObject> m_qRead;

            //写缓存
            //写缓存偏移
            sim::Mutex m_mtxWrite;
            sim::Queue<EpollDataObject> m_qWrite;
        };


        //Iocp网络管理器基类
        class EpollManager :public Manager
        {
            friend class EpollChannel;
        public:
            EpollManager();

            virtual ~EpollManager();

            //显式初始化
            virtual EnumNetError Init(int nThreadnum = 8);
            virtual EnumNetError UnInit();

            //创建通道
            //typeflag  类型，见SIM_NET_CHANNEL_TYPE_定义
            //pro       在这个通道上面的网络协议，可以为空
            //创建失败返回空
            virtual sim::RefObject<Channel> CreateChannel(TypeNetChannel typeflag, sim::RefObject <Protocol> pro );

            virtual sim::RefObject<EpollChannel> CreateChannelBySocket(SOCKET sock);

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
            //epoll 操作
            //ch 通道，opt操作，flag标记，bset 是设置还是取消
            virtual bool EpollCtrl(sim::RefObject<EpollChannel> ch, int opt,UInt32 flag,bool bset = true);
        protected:
            bool m_bExitflag;

            Mutex m_mtxChannelSize;
            UInt64 m_uChannelSize;

            int m_FdEpoll;
        };


        inline EpollManager::EpollManager() :
            m_bExitflag(false),
            m_uChannelSize(0),
            m_FdEpoll(-1)
        {
            //EpollManager::Init();
        }

        inline EpollManager::~EpollManager()
        {
            EpollManager::ExitPoll();
            EpollManager::UnInit();
        }
        inline EnumNetError EpollManager::Init(int nThreadnum)
        {
            //初始化
            SocketUtil::Init();
            m_FdEpoll = epoll_create1(0);
            if (-1 == m_FdEpoll)
            {
                SIM_LERROR("Failed to create epoll context." << strerror(errno));
                return E_NET_ERROR_FAILED;
            }
            return E_NET_ERROR_SUCCESS;
        }

        inline EnumNetError EpollManager::UnInit()
        {
            SIM_FUNC_DEBUG();
            if (m_FdEpoll!=-1)
            {
                SIM_LDEBUG("EpollManager UnInit " << m_FdEpoll);
                close(m_FdEpoll);
                m_FdEpoll = -1;
            }
            return E_NET_ERROR_SUCCESS;
        }
        inline sim::RefObject<Channel> EpollManager::CreateChannel(TypeNetChannel typeflag, sim::RefObject<Protocol> pro)
        {
            if (-1 == m_FdEpoll|| m_bExitflag)
            {
                SIM_LERROR("EpollManager is not init");
                return NULL;
            }

            if (typeflag & SIM_NET_CHANNEL_TYPE_SSL)
            {
                SIM_LERROR("EpollManager 0x" << m_FdEpoll << " typeflag 0x" << SIM_HEX(typeflag) << " not support");
                return NULL;
            }
            SOCKET sock = SocketUtil::CreateSocket(typeflag);
            if (sock <= 0)
            {
                SIM_LERROR("EpollManager 0x" << m_FdEpoll << " CreateSocket 0x" << SIM_HEX(typeflag) << " sock=" << sock);
                return NULL;
            }
            sim::RefObject<EpollChannel> ch = CreateChannelBySocket(sock);
            if (ch)
            {
                ch->Switch(pro);
                ch->m_Typeflag = typeflag;
            }
            return sim::reinterpret_pointer_cast<Channel>(ch);
        }
        inline EnumNetError EpollManager::UnBindChannel(::sim::RefObject<Channel> ch)
        {
            if (ch)
            {
                ch->Close();
            }
            return E_NET_ERROR_SUCCESS;
        }
        inline EnumNetError EpollManager::Poll(int wait_ms, bool bOnce)
        {
           //一次最大取事件数目
			const unsigned int MAXEVENTS = 100;
			struct epoll_event events[MAXEVENTS];
            EnumNetError eRet = E_NET_ERROR_SUCCESS;
            while (!m_bExitflag)
            {
                int n = epoll_wait(m_FdEpoll, events, MAXEVENTS, wait_ms);
                if (-1 == n)
                {
                    SIM_LERROR("Failed to wait."<<strerror(errno));
                    return E_NET_ERROR_FAILED;
                }

                for (int i = 0; i < n; i++)
                {
                    myEpollEvent*pMyEvent = (myEpollEvent*)events[i].data.ptr;
                    if(pMyEvent)
                    {
                        sim::RefObject<EpollChannel> ch = pMyEvent->pWeakCh.ref_object();
                        if(ch)
                        {
                            SIM_LINFO( events[i].data.ptr<< " dp "<<SIM_HEX(events[i].events));
                            ch->HandleEvents(events[i].events);
                        }
                    }
                    //空不处理
                    SIM_LINFO( events[i].data.ptr<< " epoll_wait events "<<SIM_HEX(events[i].events));
                }
                
                if (bOnce)
                {
                    break;
                }
            };
            return eRet;
        }
        inline void EpollManager::ExitPoll()
        {
            m_bExitflag = true;
        }
        inline UInt64 EpollManager::GetChannelSize()
        {
            AutoMutex lk(m_mtxChannelSize);
            return m_uChannelSize;
        }
        inline void EpollManager::IncChannelSize()
        {
            AutoMutex lk(m_mtxChannelSize);
            ++m_uChannelSize;
        }
        inline void EpollManager::DecChannelSize()
        {
            AutoMutex lk(m_mtxChannelSize);
            --m_uChannelSize;
        }

        inline bool EpollManager::EpollCtrl(sim::RefObject<EpollChannel> ch, int opt, UInt32 flag,bool bset)
        {
            if(m_FdEpoll<=-1||!ch)
                return false;

            myEpollEvent & ch_event = ch->GetEvent();
            
            if(!ch_event.bAdd||opt == EPOLL_CTL_ADD)
            {
                 //设置通道
                 if(!ch_event.pWeakCh)
                    ch_event.pWeakCh = ch;

                //flag设置
			    ch_event.events |=flag;

                ch_event.bAdd = true;

                opt = EPOLL_CTL_ADD;
            }
            else if(opt == EPOLL_CTL_MOD)
            {
                //没有进行add
                if(!ch_event.pWeakCh)
                    return false;

                UInt32 newflag = ch_event.events;
                if(bset)
                {
                    newflag|=flag;
                }
                else
                {
                    newflag&=(~flag);//设置为0
                }

                if(ch_event.events == newflag)
                {
                    //没有变化
                    return true;
                }
                ch_event.events = newflag;
            }
            
			//套接字设置
			if (-1 == epoll_ctl(m_FdEpoll, opt, ch->GetSocket(), &ch_event))
			{
				SIM_LERROR( ch->GetSocket()<< " epoll_ctl opt "<< opt <<" flag "<<SIM_HEX(flag)<<" Failed." << strerror(errno));
 				return false;
			}
            SIM_LINFO( ch.get()<< " ["<<ch_event.data.ptr<<"]epoll_ctl opt "<< opt <<" flag "<<SIM_HEX(flag)<<" bset "<<bset<<" now events:"<<SIM_HEX(ch_event.events));
            //移除之后，释放掉缓存
            if(opt == EPOLL_CTL_DEL)
            {
                ch_event.pWeakCh.reset();
            }

			return true;
        }

        inline sim::RefObject<EpollChannel> EpollManager::CreateChannelBySocket(SOCKET sock)
        {
            SocketUtil::SetNonBlock(sock, true);

            sim::RefObject<EpollChannel> pChan = sim::RefObject<EpollChannel>(new EpollChannel(*this, sock));
            // if(EpollCtrl(pChan,EPOLL_CTL_ADD,EPOLLIN|EPOLLOUT /*EPOLLHUP |EPOLLERR|pChan->m_uEtflag*/))
            // {
            //     return  pChan;
            // }
            pChan->GetEvent().pWeakCh = pChan;
            //创建失败
            return pChan;
        }

        inline EpollChannel::EpollChannel(EpollManager& myEpollManager, SOCKET socket)
            : m_myEpollManager(myEpollManager)
            , m_Typeflag(0)
            , m_pPro(NULL)
            , m_socket(socket)
            , m_bBindFlag(false)
            , m_bStartReadFlag(false)
            , m_bStartAcceptFlag(false)
            , m_bConnectFlag(false)
            , m_bKeepReadFlag(false)
            , m_bListenFlag(false)
            , m_uEtflag(EPOLLET)//默认启用边缘模式
        {
            m_myEpollManager.IncChannelSize();
        }

        inline EpollChannel::~EpollChannel()
        {
            EpollChannel::InnerClose(E_NET_ERROR_FAILED);
            m_myEpollManager.DecChannelSize();
        }

        inline EnumNetError EpollChannel::Switch(sim::RefObject <Protocol> pro)
        {
            m_pPro = pro;
            return E_NET_ERROR_SUCCESS;
        }

        inline EnumNetError EpollChannel::Bind(const net::StruIpAddr& stIpAddr)
        {
            SocketUtil::SetReusePort(m_socket,true);

            EnumNetError eRet = SocketUtil::Bind(m_socket, stIpAddr);
            if (eRet == E_NET_ERROR_SUCCESS)
            {
                m_bBindFlag = true;
            }

            //udp绑定的时候就已经完成链接
            if(!(m_Typeflag&SIM_NET_CHANNEL_TYPE_TCP))
            {
                if (m_pPro)
                {
                    m_pPro->OnConnect(sim::reinterpret_pointer_cast<Channel>(GetEvent().pWeakCh.ref_object()), eRet);
                }
                if (eRet == E_NET_ERROR_SUCCESS)
                {
                    m_bConnectFlag = true;
                }
            }
            return eRet;
        }

        inline EnumNetError EpollChannel::StartConnect(const net::StruIpAddr& stIpAddr)
        {
            sim::RefObject<EpollChannel> ch =GetEvent().pWeakCh.ref_object();
            if(!ch)
                return  E_NET_ERROR_FAILED;

            EnumNetError eRet = SocketUtil::Connect(GetSocket(),stIpAddr);
            if(eRet == E_NET_ERROR_INPROGRESS)
            {
                //来监听 socket 的可写事件（EPOLLOUT），以判断连接是否成功
                if(!m_myEpollManager.EpollCtrl(ch,EPOLL_CTL_MOD,EPOLLOUT,true))
                {
                    return  E_NET_ERROR_FAILED;
                }
                return E_NET_ERROR_SUCCESS;
            }
            else
            {
                if (m_pPro)
                {
                    m_pPro->OnConnect(sim::reinterpret_pointer_cast<Channel>(ch), eRet);
                }
                if(eRet == E_NET_ERROR_SUCCESS)
                    m_bConnectFlag = true;
                return eRet;
            }
            
        }

        inline EnumNetError EpollChannel::StartAccept()
        {
            sim::RefObject<EpollChannel> ch =GetEvent().pWeakCh.ref_object();
            if(!ch)
                return  E_NET_ERROR_FAILED;

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

            //来监听 socket 的可读事件（EPOLLIN），以判断是否有新的链接
            if(!m_myEpollManager.EpollCtrl(ch,EPOLL_CTL_ADD,EPOLLIN,true))
            {
                return  E_NET_ERROR_FAILED;
            }
            m_bStartAcceptFlag = true;
            return E_NET_ERROR_SUCCESS;
        }

        inline void EpollChannel::Close()
        {
            InnerClose(E_NET_ERROR_USER_CLOSE);
        }

        inline EnumNetError EpollChannel::StartWrite(RefBuff& stBuff, StruIpAddr* stIpAddr)
        {
            if(stBuff.size()<=0)
                return E_NET_ERROR_SUCCESS;

            sim::RefObject<EpollChannel> ch =GetEvent().pWeakCh.ref_object();
            if(!ch)
                return  E_NET_ERROR_FAILED;

            //udp需要目标地址
            if(!(m_Typeflag&SIM_NET_CHANNEL_TYPE_TCP) && NULL == stIpAddr)
            {
                return E_NET_ERROR_PARAM;
            }

            EpollDataObject stObject;
            stObject.stBuff = stBuff;
            if(stIpAddr)
                stObject.stIpAddr = *stIpAddr;
            stObject.uWriteOffset = 0;
            
            {
                //写入缓冲中
                AutoMutex lk(m_mtxWrite);
                m_qWrite.PushBack(stObject);
            }

            if(!m_myEpollManager.EpollCtrl(ch,EPOLL_CTL_MOD,EPOLLOUT,true))
            {
                return  E_NET_ERROR_FAILED;
            }
            return E_NET_ERROR_SUCCESS;
        }

        inline EnumNetError EpollChannel::StartRead(RefBuff stBuff, bool bKeep)
        {
            SIM_LINFO( this << " StartRead "<<stBuff.size()<<" bKeep "<<bKeep);
            if(stBuff.size()<=0)
                stBuff = RefBuff(1024);

            sim::RefObject<EpollChannel> ch =GetEvent().pWeakCh.ref_object();
            if(!ch)
                return  E_NET_ERROR_FAILED;

            EpollDataObject stObject;
            stObject.stBuff = stBuff;
            
            {
                //写入缓冲中
                AutoMutex lk(m_mtxRead);
                m_qRead.PushBack(stObject);

                if(bKeep)
                {
                    m_stKeepBuff = stBuff;//进行保持
                }
                m_bKeepReadFlag = bKeep;
            }

            if(!m_myEpollManager.EpollCtrl(ch,EPOLL_CTL_MOD,EPOLLIN,true))
            {
                return  E_NET_ERROR_FAILED;
            }
            return E_NET_ERROR_SUCCESS;
        }

        inline myEpollEvent &EpollChannel::GetEvent()
        {
            m_stEvent.events|=m_uEtflag;
            return m_stEvent;
        }
        inline void EpollChannel::HandleEvents(uint32_t events)
        {
            if(events&EPOLLERR)
            {
                HandleErr();
                return ;
            }

            if(events&EPOLLHUP)
            {
                HandleHub();
                return ;
            }

            if(events&EPOLLIN)
            {
                HandleIn();
            }

            if(events&EPOLLOUT)
            {
                HandleOut();
            }
        }
        inline void EpollChannel::HandleIn()
        {
            sim::RefObject<EpollChannel> ch =GetEvent().pWeakCh.ref_object();
            if(!ch)
            {
                HandleErr();
                return ;
            }

            if(m_bStartAcceptFlag)
            {
                //在监听，这里就是监听服务
                bool bCloseAcceptSock = false;
                SOCKET client = INVALID_SOCKET;
                StruIpAddr stAcceptAddr;
                EnumNetError eRet = SocketUtil::Accept(m_socket,client,&stAcceptAddr);
                if(eRet == E_NET_ERROR_SUCCESS)
                {
                    eRet = E_NET_ERROR_FAILED;
                    sim::RefObject<EpollChannel> accept_ch = m_myEpollManager.CreateChannelBySocket(client);
                    if (m_pPro && accept_ch)
                    {
                        accept_ch->Switch(m_pPro);
                        accept_ch->m_Typeflag = m_Typeflag;
                        accept_ch->m_bConnectFlag = true;

                        eRet = m_pPro->OnAccept(sim::reinterpret_pointer_cast<Channel>(ch), sim::reinterpret_pointer_cast<Channel>(accept_ch));
                        if (eRet != E_NET_ERROR_SUCCESS)
                        {
                            accept_ch->Switch(NULL);
                            accept_ch->Close();
                        }
                    }
                    else if(!accept_ch)
                    {
                        bCloseAcceptSock = true;
                    }
                }

                if(bCloseAcceptSock)
                {
                    SocketUtil::Close(client);
                }
            }
            else
            {
                //可读
                EpollDataObject  stReadObject;
                bool bNeedRead = false;
                {
                    AutoMutex lk(m_mtxRead);
                    if(m_qRead.Size())
                    {
                        bNeedRead = m_qRead.PopFront(&stReadObject);
                    }
                }

                if(false == bNeedRead)
                {
                    //没有需要读的
                    RefBuff stBuff;
                    if(m_bKeepReadFlag)
                    {
                        AutoMutex lk(m_mtxRead);
                        stBuff =  m_stKeepBuff;
                    }
                    if(m_bKeepReadFlag)
                        StartRead(stBuff,m_bKeepReadFlag);
                    else 
                        m_myEpollManager.EpollCtrl(ch,EPOLL_CTL_MOD,EPOLLIN,false);
                    return ;
                }

                //读数据
                UInt32 bytes_transfered =0;
                EnumNetError eRet = E_NET_ERROR_FAILED;
                if(m_Typeflag&SIM_NET_CHANNEL_TYPE_TCP)
                {
                    bytes_transfered = stReadObject.stBuff.size();
                    eRet = SocketUtil::Recv(m_socket,stReadObject.stBuff.get(),bytes_transfered);
                }
                else
                {
                    bytes_transfered = stReadObject.stBuff.size();
                    eRet = SocketUtil::Recvfrom(m_socket,stReadObject.stBuff.get(),bytes_transfered,stReadObject.stIpAddr);
                }

                if(eRet == E_NET_ERROR_SUCCESS)
                {
                    if (m_pPro)
                    {
                        m_pPro->OnReaded(sim::reinterpret_pointer_cast<Channel>(ch), stReadObject.stBuff,bytes_transfered,stReadObject.stIpAddr);
                    }

                    RefBuff stBuff;
                    if(m_bKeepReadFlag)
                    {
                        AutoMutex lk(m_mtxRead);
                        stBuff =  m_stKeepBuff;
                    }
                    if(m_bKeepReadFlag)
                        StartRead(stBuff,m_bKeepReadFlag);
                    else 
                        m_myEpollManager.EpollCtrl(ch,EPOLL_CTL_MOD,EPOLLIN,false);
                }
                else if(eRet == E_NET_ERROR_DIS_CONNECT)
                {
                    HandleHub();
                }
                else{
                    //...
                }
            }
        }
        inline void EpollChannel::HandleOut()
        {
            sim::RefObject<EpollChannel> ch =GetEvent().pWeakCh.ref_object();
            if(!ch)
            {
                HandleErr();
                return ;
            }

            if(m_bConnectFlag)
            {
                //写数据
                UInt32 bytes_transfered =0;
                EnumNetError eRet = E_NET_ERROR_FAILED;
                EpollDataObject *pstWriteObj=NULL;
                {
                    AutoMutex lk(m_mtxWrite);
                    if(m_qWrite.Size()>=0)
                    {
                        pstWriteObj = & m_qWrite.Next(NULL)->data;
                    }
                }

                if(pstWriteObj)
                {
                    if(pstWriteObj->stBuff.size()<=pstWriteObj->uWriteOffset)
                    {
                        //写完了
                        AutoMutex lk(m_mtxWrite);
                        m_qWrite.PopFront(NULL);
                        return ;
                    }

                    bytes_transfered = pstWriteObj->stBuff.size()-pstWriteObj->uWriteOffset;
                    if(m_Typeflag&SIM_NET_CHANNEL_TYPE_TCP)
                    {
                        eRet = SocketUtil::Send(m_socket,pstWriteObj->stBuff.get()+pstWriteObj->uWriteOffset,bytes_transfered);
                    }
                    else
                    {
                        eRet = SocketUtil::SendTo(m_socket,pstWriteObj->stBuff.get()+pstWriteObj->uWriteOffset,bytes_transfered,pstWriteObj->stIpAddr);
                    }

                    if(eRet != E_NET_ERROR_SUCCESS)
                    {
                        if (m_pPro)
                        {
                            m_pPro->OnWrited(sim::reinterpret_pointer_cast<Channel>(ch), pstWriteObj->stBuff,pstWriteObj->uWriteOffset,eRet);
                        }

                        {
                            //写失败了，异常
                            AutoMutex lk(m_mtxWrite);
                            m_qWrite.PopFront(NULL);
                            return ;
                        }
                    }
                    else
                    {
                        pstWriteObj->uWriteOffset+=bytes_transfered;
                        if(pstWriteObj->stBuff.size()<=pstWriteObj->uWriteOffset)
                        {
                            if (m_pPro)
                            {
                                m_pPro->OnWrited(sim::reinterpret_pointer_cast<Channel>(ch), pstWriteObj->stBuff,pstWriteObj->uWriteOffset,eRet);
                            }
                            //写完了
                            AutoMutex lk(m_mtxWrite);
                            m_qWrite.PopFront(NULL);
                            return ;
                        }
                        //没有写完，下次继续
                    }
                }
            }
            else
            {
                //这里就是链接建立了
                m_bConnectFlag = true;
                if (m_pPro)
                {
                    m_pPro->OnConnect(sim::reinterpret_pointer_cast<Channel>(ch), SocketUtil::IsOk(m_socket));
                }
            }
        }

        inline void EpollChannel::HandleErr()
        {
            perror("HandleErr:");
            InnerClose(E_NET_ERROR_FAILED);
        }
        inline void EpollChannel::HandleHub()
        {
            perror("HandleHub:");
            InnerClose(E_NET_ERROR_DIS_CONNECT);
        }
        inline void EpollChannel::InnerClose(EnumNetError eRet)
        {
            m_myEpollManager.EpollCtrl(GetEvent().pWeakCh.ref_object(),EPOLL_CTL_DEL,0);

            if (SocketUtil::IsVaild(m_socket))
            {
                SocketUtil::Close(m_socket);
                m_socket = INVALID_SOCKET;
            }

            if (m_pPro)
            {
                sim::RefObject<EpollChannel> ch =GetEvent().pWeakCh.ref_object();
                m_pPro->OnClose(sim::reinterpret_pointer_cast<Channel>(ch), eRet);
                m_pPro = NULL;
            }
        }
    }
}
#endif // OS_LINUX
#endif //!SIM_NET_EPOLL_HPP_