/*
* ���繤�߿�Ļ���ͷ����
*/
#ifndef SIM_NET_BASE_HPP_
#define SIM_NET_BASE_HPP_
#include "Types.hpp"
#include "RefObject.hpp"

//ͨ������
//TCP 1 or UDP 0
#define SIM_NET_CHANNEL_TYPE_TCP 0x01
//IPV6 1 or IPV4 0
#define SIM_NET_CHANNEL_TYPE_IPV6 0x02
//����SSL��
#define SIM_NET_CHANNEL_TYPE_SSL 0x04

namespace sim
{
    //�����������ռ�
    namespace net
    {
        enum EnumNetError
        {
            E_NET_ERROR_SUCCESS = 0,
            E_NET_ERROR_FAILED = -1,
            E_NET_ERROR_PARAM =  -2,//�����쳣
            E_NET_ERROR_TIMEOUT =  -3,//������ʱ
            E_NET_ERROR_UNDEF = -3,//����δ����
        };

        typedef UInt64 TypeNetChannel;

        /**
         * @brief IP��ַ����ö��
         *
         * ö�ٱ�ʾIP��ַ�����͡�
         *
         * E_IP_ADDR_TYPE_IPV4: IPv4��ַ����
         * E_IP_ADDR_TYPE_IPV6: IPv6��ַ����
         */
        enum EnumIpAddrType
        {
            E_IP_ADDR_TYPE_IPV4 = 0,
            E_IP_ADDR_TYPE_IPV6 = 1,
        };

        struct StruIpAddr
        {
            // IP��ַ����
            EnumIpAddrType eType;
            // IP��ַ�ַ���
            String strIp;
            // �˿ں�
            UInt16 usPort;
        };

        //����ǰ��
        class Channel;

        //Э�����
        //���������Э�齻��
        class Protocol
        {
        public:
            //�������Э��Ļص��¼�

            //�����¼���eConnResult ���ӽ��
            virtual void OnConnect(sim::RefWeakObject<Channel> ch, EnumNetError eConnResult) {}

            //���������¼���ch_srv �������ӵķ���ͨ����ch ���ɵ�����
            //E_NET_ERROR_SUCCESS !EnumNetError Э��ջ�ڲ�����ch �ܾ�����
            virtual EnumNetError OnAccept(sim::RefWeakObject<Channel> ch_srv, sim::RefWeakObject<Channel> ch) {return E_NET_ERROR_UNDEF;}

            //���ӹر��¼���eCloseResult �ر�ԭ��
            virtual void OnClose(sim::RefWeakObject<Channel> ch, EnumNetError eCloseResult) {};

            //�յ�����,stIpAddr ��Դ��ַ
            virtual EnumNetError OnReaded(sim::RefWeakObject<Channel> ch, RefBuff& stBuff, StruIpAddr stIpAddr){ return E_NET_ERROR_UNDEF; };

            //���ͱ��ĳɹ�
            virtual EnumNetError OnWrited(sim::RefWeakObject<Channel> ch, RefBuff& stBuff) { return E_NET_ERROR_UNDEF; };
        };

        //����ͨ������
        //������������ݽ���
        class Channel
        {
        public:
            //�л�ͨ���󶨵�Э��
            virtual EnumNetError Switch(Protocol* pro) = 0;

            //����ӿ�
            //����һ�� StruIpAddr ���͵Ĳ���������һ�� EnumNetError ���͵�ֵ
            virtual EnumNetError Bind(const StruIpAddr& stIpAddr)=0;


            //����һ�� StruIpAddr ���͵Ĳ���������һ�� EnumNetError ���͵�ֵ
            virtual EnumNetError StartConnect(const StruIpAddr& stIpAddr)=0;

            //��ʼ����һ������
            virtual EnumNetError StartAccept()=0;

            //û�в�����Ҳû�з���ֵ
            virtual void Close()=0;

            //�첽��������
            //stIpAddrֻ�е�udp����û�н���������Ч����������ᱻ���Ե�
            virtual EnumNetError StartWrite(RefBuff& stBuff, StruIpAddr* stIpAddr=NULL)=0;

            //��ʼ��ȡ���ݣ�bKeep �Ƿ�һֱ��ȡ��false ֻ�����һ�ζ�ȡ��trueһֱ��ȡ��ֱ�����ӶϿ�
            virtual EnumNetError StartRead(bool bKeep=true) = 0;

        public:
            //�������ͣ���SIM_NET_CHANNEL_TYPE_����
            virtual TypeNetChannel Type() = 0;

            //�Ƿ��Ѿ�����
            virtual bool IsConnect() = 0;
            //�Ƿ�
        private:

        };


        //�������������
        class Manager
        {
        public:
            //��ʽ��ʼ��
            virtual EnumNetError Init() = 0;
            virtual EnumNetError UnInit() = 0;

            //����ͨ��
            //typeflag  ���ͣ���SIM_NET_CHANNEL_TYPE_����
            //pro       �����ͨ�����������Э�飬����Ϊ��
            //����ʧ�ܷ��ؿ�
            virtual sim::RefObject<Channel> CreateChannel(TypeNetChannel typeflag, Protocol* pro = NULL) = 0;

            //�������ͨ����Manager���ٹ������ͨ����֮��ch������
            virtual EnumNetError UnBindChannel(sim::RefObject<Channel> ch) = 0;

            //�¼�ѭ����ִ��һ���¼����˳�
            //wait_ms �ȴ�ʱ��
            virtual EnumNetError PollOne(int wait_ms) = 0;

            //�¼�ѭ�������Ƴ���ֱ��ִ��Exit
            //wait_ms �ȴ�ʱ��
            virtual EnumNetError Poll(int wait_ms) = 0;

            //�˳�Poll�����ж���Poll�߳��˳�
            virtual void ExitPoll() = 0;
        };

    }
}
#endif //!SIM_NET_BASE_HPP_