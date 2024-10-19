/*
* 网络工具库的基本头定义
*/
#ifndef SIM_NET_BASE_HPP_
#define SIM_NET_BASE_HPP_
#include "Types.hpp"
#include "RefObject.hpp"

//通道类型
//TCP 1 or UDP 0
#define SIM_NET_CHANNEL_TYPE_TCP 0x01
//IPV6 1 or IPV4 0
#define SIM_NET_CHANNEL_TYPE_IPV6 0x02
//基于SSL的
#define SIM_NET_CHANNEL_TYPE_SSL 0x04

namespace sim
{
    //网络库的命名空间
    namespace net
    {
        enum EnumNetError
        {
            E_NET_ERROR_SUCCESS = 0,
            E_NET_ERROR_FAILED = -1,
            E_NET_ERROR_PARAM =  -2,//参数异常
            E_NET_ERROR_TIMEOUT =  -3,//操作超时
            E_NET_ERROR_UNDEF = -3,//操作未定义
        };

        typedef UInt64 TypeNetChannel;

        /**
         * @brief IP地址类型枚举
         *
         * 枚举表示IP地址的类型。
         *
         * E_IP_ADDR_TYPE_IPV4: IPv4地址类型
         * E_IP_ADDR_TYPE_IPV6: IPv6地址类型
         */
        enum EnumIpAddrType
        {
            E_IP_ADDR_TYPE_IPV4 = 0,
            E_IP_ADDR_TYPE_IPV6 = 1,
        };

        struct StruIpAddr
        {
            // IP地址类型
            EnumIpAddrType eType;
            // IP地址字符串
            String strIp;
            // 端口号
            UInt16 usPort;
        };

        //声明前置
        class Channel;

        //协议基类
        //关联具体的协议交互
        class Protocol
        {
        public:
            //处理基层协议的回调事件

            //链接事件，eConnResult 链接结果
            virtual void OnConnect(sim::RefWeakObject<Channel> ch, EnumNetError eConnResult) {}

            //接受链接事件，ch_srv 接受链接的服务通道，ch 生成的链接
            //E_NET_ERROR_SUCCESS !EnumNetError 协议栈内部回收ch 拒绝链接
            virtual EnumNetError OnAccept(sim::RefWeakObject<Channel> ch_srv, sim::RefWeakObject<Channel> ch) {return E_NET_ERROR_UNDEF;}

            //链接关闭事件，eCloseResult 关闭原因
            virtual void OnClose(sim::RefWeakObject<Channel> ch, EnumNetError eCloseResult) {};

            //收到报文,stIpAddr 来源地址
            virtual EnumNetError OnReaded(sim::RefWeakObject<Channel> ch, RefBuff& stBuff, StruIpAddr stIpAddr){ return E_NET_ERROR_UNDEF; };

            //发送报文成功
            virtual EnumNetError OnWrited(sim::RefWeakObject<Channel> ch, RefBuff& stBuff) { return E_NET_ERROR_UNDEF; };
        };

        //网络通道基类
        //负责基本的数据交互
        class Channel
        {
        public:
            //切换通道绑定的协议
            virtual EnumNetError Switch(Protocol* pro) = 0;

            //网络接口
            //接收一个 StruIpAddr 类型的参数，返回一个 EnumNetError 类型的值
            virtual EnumNetError Bind(const StruIpAddr& stIpAddr)=0;


            //接收一个 StruIpAddr 类型的参数，返回一个 EnumNetError 类型的值
            virtual EnumNetError StartConnect(const StruIpAddr& stIpAddr)=0;

            //开始接受一个链接
            virtual EnumNetError StartAccept()=0;

            //没有参数，也没有返回值
            virtual void Close()=0;

            //异步发送数据
            //stIpAddr只有当udp而且没有进行链接有效，其他情况会被忽略掉
            virtual EnumNetError StartWrite(RefBuff& stBuff, StruIpAddr* stIpAddr=NULL)=0;

            //开始读取数据，bKeep 是否一直读取，false 只会进行一次读取，true一直读取，直到链接断开
            virtual EnumNetError StartRead(bool bKeep=true) = 0;

        public:
            //返回类型，见SIM_NET_CHANNEL_TYPE_定义
            virtual TypeNetChannel Type() = 0;

            //是否已经链接
            virtual bool IsConnect() = 0;
            //是否
        private:

        };


        //网络管理器基类
        class Manager
        {
        public:
            //显式初始化
            virtual EnumNetError Init() = 0;
            virtual EnumNetError UnInit() = 0;

            //创建通道
            //typeflag  类型，见SIM_NET_CHANNEL_TYPE_定义
            //pro       在这个通道上面的网络协议，可以为空
            //创建失败返回空
            virtual sim::RefObject<Channel> CreateChannel(TypeNetChannel typeflag, Protocol* pro = NULL) = 0;

            //主动解绑通道，Manager不再管理这个通道，之后ch不可用
            virtual EnumNetError UnBindChannel(sim::RefObject<Channel> ch) = 0;

            //事件循环，执行一次事件后退出
            //wait_ms 等待时间
            virtual EnumNetError PollOne(int wait_ms) = 0;

            //事件循环，不推出，直到执行Exit
            //wait_ms 等待时间
            virtual EnumNetError Poll(int wait_ms) = 0;

            //退出Poll，所有堵塞Poll线程退出
            virtual void ExitPoll() = 0;
        };

    }
}
#endif //!SIM_NET_BASE_HPP_