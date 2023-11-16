#ifndef TERMINAL_H
#define TERMINAL_H

#include <winsock2.h>
#include <iostream>
#include <fstream>
#include <time.h>
#pragma comment(lib, "Ws2_32.lib")

using namespace std;

#define BUFFER_MAX_SIZE (20 * 1024 * 1024)
#define HSZ sizeof(Header)                           //自定义的Header长度
#define MTU 572                                     //保守假设的MTU（最大传输单元）
#define MDS MTU - 20 - 8 - HSZ                      //一个自定义包中实际数据部分的最大长度 528 Byte
#define TIMEOUT 1000


void WinSockInit();

//自己设计的协议头部，总共占16Byte
class Header
{
    public:
        Header() : flags(0), checksum(0), seqNum(0), ackNum(0), payloadSize(0) {}
        bool testSYN() {return flags & 2;}
        bool testFIN() {return flags & 1;}
        bool testACK() {return flags & 16;}
        void setSYN() {flags |= 2;}
        void setFIN() {flags |= 1;}
        void setACK() {flags |= 16;}
        void setSeqNum(uint32_t _seqNum) {seqNum = _seqNum;}
        void setAckNum(uint32_t _ackNum) {ackNum = _ackNum;}
        uint32_t getAckNum() {return ackNum;}
        uint32_t getSeqNum() {return seqNum;}
        uint32_t getPayLoadSize() {return payloadSize;}
        void setPayloadSize(uint32_t _payloadSize) {payloadSize = _payloadSize;}
        void calCheckSum();                         //计算整个包的校验和并存在checksum字段中
        bool testCheckSum();                        //检验校验和是否正确
    private:
        uint16_t flags;                             //状态编码
        uint16_t checksum;                          //校验和
        uint32_t seqNum;                            //发送报文段的序列号
        uint32_t ackNum;                            //确认序列号
        uint32_t payloadSize;                       //后面紧跟的数据长度（字节为单位）
};

//抽象出来的终端类
class Terminal    
{
    public:
        Terminal();
        void buildConnect(string IP, int port);     //与接收端建立连接关系（三次握手）
        void waitConnedct(int port);                //监听并且与发送端建立连接（三次握手）
        void prepareHeader(bool SYN, bool ACK, bool FIN, uint32_t seqNum = 0, uint32_t ackNum = 0, uint32_t payloadSize = 0);   //准备好一个Header，设置好所有的成员
        Header* loadFile(string filepath);          //从指定文件中读取数据，并生成Header，两者拼接存入发送缓冲区
        void sendFile(string filePath);             //根据指定的文件路径分割并传输文件
        void recvFile(string filePath);             //接收文件并且存于指定文件路径中
    private:

    private:
        char *sendBuffer;   //发送缓冲区
        char *recvBuffer;   //接收缓冲区
        SOCKET udpSocket;   
        sockaddr_in SendAddr;//发送端的地址
        int len_SendAddr;
        sockaddr_in RecvAddr;//接收端的地址
        int len_RecvAddr;   
};  



#endif 