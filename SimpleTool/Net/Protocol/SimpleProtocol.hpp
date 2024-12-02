/*
* 简单协议，网络内部进行测试的
*/
#ifndef SIM_NET_SIMPLE_PROTOCOL_HPP_
#define SIM_NET_SIMPLE_PROTOCOL_HPP_
#include "Net/NetBase.hpp"
#include "Net/SocketUtil.hpp"
#include "Thread/Mutex.hpp"
#include "Thread/Thread.hpp"
namespace sim
{
    namespace net
    {
        //magic+v+t+seq+len = 4+1+1+8+8 = 22
        const char gSimpleMagic[] = {0x01,0x02,0x03,0x04};
        struct SimplePack
        {
            //版本 1 
            UInt8 version;
            //type 1 request/ 2 response
            UInt8 type;
            //seq 序号 请求=回复
            UInt64 seq;
            RefBuff data;
            ////数据长度
            //Int64 datalen;
            ////数据缓存
            //unsigned char* data;
            //校验和 异或
            UInt8 checksum;

        };

        //tcp网络回调，需要管理子链接
        template<typename ChnProtocol>
        class TCPSrvProtocol :public Protocol
        {
            struct NetChannelComparator {
                bool operator()(const sim::RefObject<net::Channel>& lhs, const sim::RefObject<net::Channel>& rhs) const {
                    return lhs.c_get() < rhs.c_get();
                }
            };
        public:
            TCPSrvProtocol()
            {

            }
            virtual ~TCPSrvProtocol() {
                CloseAll();
            }
            tList<sim::RefObject<net::Channel> > GetChnList()
            {
                tList<sim::RefObject<net::Channel> > myList;
                AutoMutex lk(m_mtxchs);
                for (tSet< sim::RefObject<net::Channel>, NetChannelComparator >::iterator it = m_schs.begin(); it != m_schs.end(); ++it)
                {
                    myList.push_back(*it);
                }
                return myList;
            }
            void CloseChn(sim::RefObject<net::Channel> ch)
            {
                if (!ch)
                    return;

                {
                    AutoMutex lk(m_mtxchs);
                    m_schs.erase(ch);
                }
                ch->Close();
            }
            void CloseAll()
            {
               tList<sim::RefObject<net::Channel> > lChn = GetChnList();
               for (tList<sim::RefObject<net::Channel> >::iterator it = lChn.begin();it!=lChn.end();++it)
               {
                   CloseChn(*it);
               }
            }
        protected:
            //接受链接事件，ch_srv 接受链接的服务通道，ch 生成的链接
            //E_NET_ERROR_SUCCESS !EnumNetError 协议栈内部回收ch 拒绝链接
            virtual EnumNetError OnAccept(sim::RefObject<Channel> ch_srv, sim::RefObject<Channel> ch)
            {
                SIM_LINFO("TCPSrvProtocol: OnAccept " << (void*)ch_srv.get() << " ch: " << (void*)ch.get());
                
                {
                    AutoMutex lk(m_mtxchs);
                    m_schs.insert(ch);
                }
                sim::RefObject<Protocol> pchPro(new ChnProtocol);
                ch->Switch(pchPro);
                //回调上层处理
                pchPro->OnConnect(ch, net::E_NET_ERROR_SUCCESS);
                return net::E_NET_ERROR_SUCCESS;
            }

            //链接关闭事件，eCloseResult 关闭原因
            virtual void OnClose(sim::RefObject<Channel> ch, EnumNetError eCloseResult)
            {
                SIM_LINFO("TCPSrvProtocol: OnClose " << (void*)ch.get() << " eCloseResult: " << eCloseResult);
                CloseChn(ch);
            }

        private:
            Mutex m_mtxchs;
            tSet< sim::RefObject<net::Channel>, NetChannelComparator > m_schs;
        };

        //关联具体的协议交互
        class SimpleProtocol:public Protocol
        {

        public:
            SimpleProtocol():m_bConn(false)
                , m_eConnResult(E_NET_ERROR_TIMEOUT)
            {

            }
            virtual ~SimpleProtocol()
            { }

            virtual void OnPack(sim::RefObject<Channel> ch, const SimplePack& pack, StruIpAddr stIpAddr)
            {
                SIM_LINFO("SimpleProtocol: OnPack " << (void*)ch.get() << " type: " << (Int32)pack.type<<" seq:"<< pack.seq);
                if (pack.type == 2)
                {
                    //缓存
                    AutoMutex lk(m_mtxResponse);
                    m_mpResponse[pack.seq] = pack;

                }
                else if (pack.type == 1)
                {
                    //echo
                    SimplePack respack = pack;
                    respack.type = 2;
                    Send(ch, respack, &stIpAddr);
                }
                
            }
            virtual EnumNetError Send(sim::RefObject<Channel> ch, const SimplePack& pack, StruIpAddr *pstIpAddr)
            {
                RefBuff buff = Print(pack);
                return Write(ch, buff, pstIpAddr);
            }

            virtual bool WaitRes(UInt64 seq, SimplePack& pack, UInt32 nWaitMs = 5000)
            {
                Int32 nMyWaitMs = nWaitMs;
                do
                {
                    {
                        AutoMutex lk(m_mtxResponse);
                        tMap<UInt64, SimplePack>::iterator it = m_mpResponse.find(seq);
                        if (it != m_mpResponse.end())
                        {
                            pack = it->second;
                            m_mpResponse.erase(it);
                            return true;
                        }
                    }

                    if (nMyWaitMs == 0)
                        break;
                    Thread::Sleep(50);
                    nMyWaitMs -= 50;
                } while (nMyWaitMs > 0);

                return false;
            }

            virtual void CleanAllWaitRes()
            {
                {
                    AutoMutex lk(m_mtxResponse);
                    m_mpResponse.clear();
                }
            }

            virtual bool WaitConnect(EnumNetError &eConnResult, UInt32 nWaitMs = 5000)
            {
                Int32 nMyWaitMs = nWaitMs;
                do
                {
                    if (m_bConn)
                    {
                        eConnResult = m_eConnResult;
                        return true;
                    }

                    if (nMyWaitMs == 0)
                        break;
                    Thread::Sleep(50);
                    nMyWaitMs -= 50;
                } while (nMyWaitMs > 0);

                return false;
            }
        protected:
            //处理基层协议的回调事件
            //链接事件，eConnResult 链接结果
            virtual void OnConnect(sim::RefObject<Channel> ch, EnumNetError eConnResult)
            {
                SIM_LINFO("SimpleProtocol: OnConnect " << (void*)ch.get() << " eConnResult: " << eConnResult);
                //链接成功之后，马上进行读数据
                RefBuff stTempBuff(10 * 1024 * 1024);
                ch->StartRead(stTempBuff, true);
                m_eConnResult = eConnResult;
                m_bConn = true;
            }

            //收到报文,stIpAddr 来源地址
            virtual void OnReaded(sim::RefObject<Channel> ch, RefBuff& stBuff, UInt32 bytes_transfered, StruIpAddr stIpAddr)
            {
                SIM_LINFO("SimpleProtocol: OnReaded " << (void*)ch.get() << " bytes_transfered: "<< bytes_transfered);
                m_stCacheBuff = m_stCacheBuff + RefBuff(stBuff.get(), bytes_transfered);
                DoParse(ch, stIpAddr);
            }

            //发送报文成功
            virtual void OnWrited(sim::RefObject<Channel> ch, RefBuff& stBuff, UInt32 offset, UInt32 bytes_transfered, net::EnumNetError eWriteResult)
            {
                SIM_LINFO("SimpleProtocol: OnWrited " << (void*)ch.get() << "offset:"<< offset <<" bytes_transfered: " << bytes_transfered);
            }
        protected:
            inline UInt8 CheckSum(const char* pData, UInt64 len)
            {
                UInt8 check = 0;
                for (UInt64 i = 0; i < len; ++i)
                    check ^= pData[i];
                return check;
            }

            void DoParse(sim::RefObject<Channel> ch, StruIpAddr stIpAddr)
            {
                if (m_stCacheBuff.size() <= 0)
                    return;
                UInt64 nUsed = 0;
               
                char* pData = m_stCacheBuff.get();
                while (true)
                {
                    if (m_stCacheBuff.size()- nUsed < 23)
                        break;

                    if (pData[0] == gSimpleMagic[0]
                        || pData[1] == gSimpleMagic[1]
                        || pData[2] == gSimpleMagic[2]
                        || pData[3] == gSimpleMagic[3])
                    {
                        ////magic+v+t+seq+len = 4+1+1+8+8
                        UInt64* pRawlen =(UInt64*) & pData[4 + 1 + 1 + 8];
                        UInt64 len = SocketUtil::NetworkToHost64(*pRawlen);
                        UInt64 packlen = 4 + 1 + 1 + 8 +8+ len + 1;
                        if (packlen > m_stCacheBuff.size() - nUsed)
                        {
                            break;
                        }

                        //完整的，进行解析
                        UInt8 check1 = pData[packlen - 1];
                        UInt8 check2 = CheckSum(pData, packlen - 1);
                        if (check1 != check2)
                        {
                            SIM_LINFO("SimpleProtocol: DoParse " << (void*)ch.get() << " check1 error");
                            //错误的节点
                            pData++;
                            nUsed++;
                        }
                        else
                        {
                            UInt64 offset = 4;
                            SimplePack pack;
                            pack.checksum = check2;
                            pack.version = pData[offset];
                            offset++;
                            pack.type = pData[offset];
                            offset++;
                            pack.seq = SocketUtil::NetworkToHost64(*(UInt64*)(pData+ offset));
                            offset += 8;
                            offset += 8;//len
                            pack.data = RefBuff(pData + offset, len);

                            OnPack(ch, pack, stIpAddr);

                            pData += packlen;
                            nUsed += packlen;
                        }
                    }
                    else
                    {
                        pData++;
                        nUsed++;
                    }
                }

                if (nUsed > 0)
                {
                    m_stCacheBuff = RefBuff(m_stCacheBuff.get(), m_stCacheBuff.size() - nUsed);
                }
            }

            RefBuff Print(const SimplePack& pack)
            {
                ////magic+v+t+seq+len = 4+1+1+8+8 = 22
                //+checksum = 32 +datalen
                if (pack.data.size()<0)
                {
                    return RefBuff();
                }
                UInt64 nUsed = 0;
                RefBuff buff(pack.data.size() + 23);
                memcpy(buff.get() + nUsed, gSimpleMagic, sizeof(char) * 4);
                nUsed += sizeof(char) * 4;
                memcpy(buff.get() + nUsed, &pack.version, sizeof(char) * 1);
                nUsed += sizeof(char) * 1;
                memcpy(buff.get() + nUsed, &pack.type, sizeof(char) * 1);
                nUsed += sizeof(char) * 1;
                UInt64 seq = SocketUtil::HostToNetwork64(pack.seq);
                memcpy(buff.get() + nUsed, &seq, sizeof(seq));
                nUsed += sizeof(seq);
                UInt64 s = SocketUtil::HostToNetwork64(pack.data.size());
                memcpy(buff.get() + nUsed, &s, sizeof(s));
                nUsed += sizeof(s);
                memcpy(buff.get() + nUsed, pack.data.c_get(), sizeof(char)*pack.data.size());
                nUsed += sizeof(char) * pack.data.size();

                //
                UInt8 checksum = CheckSum(buff.get(), nUsed);
                memcpy(buff.get() + nUsed, &checksum, sizeof(char) * 1);
                return buff;
            }


        protected:
            //缓存，tcp需要调整为完整的报文才可解析
            RefBuff m_stCacheBuff;

            Mutex m_mtxResponse;
            tMap<UInt64, SimplePack> m_mpResponse;

            bool m_bConn;
            EnumNetError m_eConnResult;
        };
       
    }
}
#endif //!SIM_NET_SIMPLE_PROTOCOL_HPP_
