/*
* 多线程执行的网络接口类
*/
#ifndef SIM_ASYNC_NET_HPP_
#define SIM_ASYNC_NET_HPP_
#include "Types.hpp"

#include "Net.hpp"
#include "Thread/Thread.hpp"

namespace sim
{
    namespace net{
        template<typename ManagerClass >
        class AsyncManagerThreadData
        {
        public:
            ManagerClass ONetManger;
            Thread thSelf;
        };

        template<typename ManagerClass >
        class AsyncManager
        {
            UInt32 m_nThreadnum;
            Mutex m_mtxNextNo;
            UInt32 m_nNextNo;

            AsyncManagerThreadData<ManagerClass>* m_pManagerData;
        public:
            AsyncManager();
            ~AsyncManager();

            //显式初始化
            virtual EnumNetError Init(int nThreadnum);
            virtual EnumNetError UnInit();

            //创建通道
            //typeflag  类型，见SIM_NET_CHANNEL_TYPE_定义
            //pro       在这个通道上面的网络协议，可以为空
            //创建失败返回空
            virtual RefObject<Channel> CreateChannel(TypeNetChannel typeflag, Protocol* pro = NULL);

            static ThRet MyThreadProc(LPVOID lpParam);
        private:
            UInt32 GetCurIndex();
        };
        template <typename ManagerClass>
        inline AsyncManager<ManagerClass>::AsyncManager()
        :m_nThreadnum(1),m_nNextNo(0),m_pManagerData(NULL)
        {
            
        }
        template <typename ManagerClass>
        inline AsyncManager<ManagerClass>::~AsyncManager()
        {
            UnInit();
        }
        template <typename ManagerClass>
        inline EnumNetError AsyncManager<ManagerClass>::Init(int nThreadnum)
        {
            if(m_pManagerData)
                return  E_NET_ERROR_REINIT;

            if(nThreadnum<=0 )
                m_nThreadnum = 1;
            else 
                m_nThreadnum = nThreadnum;

            m_pManagerData = new  AsyncManagerThreadData<ManagerClass>[m_nThreadnum];
            for(int i=0;i<m_nThreadnum;++i)
            {
                m_pManagerData[i].ONetManger.Init();
                if(!m_pManagerData[i].thSelf.Run(MyThreadProc,&m_pManagerData[i].ONetManger))
                {
                    UnInit();
                    return E_NET_ERROR_FAILED;
                }
            }
            return E_NET_ERROR_SUCCESS;
        }
        template <typename ManagerClass>
        inline EnumNetError AsyncManager<ManagerClass>::UnInit()
        {
            if(m_pManagerData)
            {
                for(UInt32 i=0;i<m_nThreadnum;++i)
                {
                    m_pManagerData[i].ONetManger.ExitPoll();
                    if(m_pManagerData[i].thSelf.JoinAble())
                    {
                        m_pManagerData[i].thSelf.Join();
                    }
                }
                delete []m_pManagerData;
                m_pManagerData = NULL;
            }
            return E_NET_ERROR_SUCCESS;
        }
        template <typename ManagerClass>
        inline RefObject<Channel> AsyncManager<ManagerClass>::CreateChannel(TypeNetChannel typeflag, Protocol *pro)
        {
             if(!m_pManagerData)
                return  NULL;
            return m_pManagerData[GetCurIndex()].ONetManger.CreateChannel(typeflag,pro);
        }
        template <typename ManagerClass>
        inline ThRet AsyncManager<ManagerClass>::MyThreadProc(LPVOID lpParam)
        {
            ManagerClass*pONetManger = (ManagerClass*)lpParam;
            //pONetManger->Init();
            pONetManger->Poll(10);
            pONetManger->UnInit();
            return ThRet(0);
        }
        template <typename ManagerClass>
        inline UInt32 AsyncManager<ManagerClass>::GetCurIndex()
        {
            AutoMutex lk(m_mtxNextNo);
            return (m_nNextNo++)%m_nThreadnum;
        }
    }
}
#endif //!SIM_ASYNC_NET_HPP_