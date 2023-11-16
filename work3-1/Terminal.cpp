#include "Terminal.h"

Terminal::Terminal()
{
    udpSocket = socket(AF_INET, SOCK_DGRAM, 0); //创建一个udpSocket，并且绑定地址
    if (udpSocket == INVALID_SOCKET) {
        cerr << "创建udpSocket失败" << endl;
    }


    unsigned long mode = 1;                     //将socket设置为非阻塞
    ioctlsocket(udpSocket, FIONBIO, &mode);


    sendBuffer = new char[BUFFER_MAX_SIZE];     //缓冲区分配空间
    recvBuffer = new char[BUFFER_MAX_SIZE];
}

Header* Terminal::loadFile(string filepath)
{
    ifstream file(filepath, ios::binary |  ios::ate);    //打开文件并将文件指针指向文件末尾，便于获取文件长度
    streampos filesize = file.tellg();                             //获取文件长度
    file.seekg(0, ios::beg);                                       //文件指针移动到文件开头
    //创建header，现在假定一个文件可以和一个Header封装为一个包
    Header header;
    header.setPayloadSize(filesize);
    //拼接
    memcpy(sendBuffer, &header, sizeof(header));
    if (file.read(sendBuffer + sizeof(header), filesize))
    {
        //读文件成功
        cout << "文件读取成功" << endl;
        file.close();
        return (Header*)sendBuffer;
    }
    else {
        cerr << "文件读取失败" << endl;
        file.close();
        return nullptr;
    }
}

void Terminal::buildConnect(string IP, int port)
{
    //设置接收方的IP地址和端口号
    sockaddr_in dstAddr;
    dstAddr.sin_family = AF_INET;
    dstAddr.sin_port = htons(port);
    dstAddr.sin_addr.s_addr = inet_addr(IP.c_str());
    int len_dstAddr = sizeof(dstAddr);

    cout << "尝试建立连接" << endl;



sendPacket1:
    prepareHeader(true, false, false);
    if (sendto(udpSocket, sendBuffer, HSZ, 0, (sockaddr*)&dstAddr, sizeof(dstAddr)) > 0)
    {
        cout << "第一次握手发送" << endl;
    }
    else {
        cout << "第一次握手发送失败" << endl;
        goto sendPacket1;
    }

    clock_t start = clock();
wait:
    while (recvfrom(udpSocket, recvBuffer, BUFFER_MAX_SIZE, 0, (sockaddr*)&dstAddr, &len_dstAddr) <= 0)
    {
        clock_t end = clock();
        if (end - start > TIMEOUT)
        {
            cout << "超时重发第一次握手" << endl;
            goto sendPacket1;          //退出后重新进行第一次握手
        }
    }
    Header *recvPacket = (Header*)recvBuffer;
    if (recvPacket->testCheckSum() && recvPacket->testACK() && recvPacket->testSYN()) 
    {
        cout << "第二次握手确认成功，准备发送第三次握手" << endl;
    }
    else {
        cout << "第二次握手确认失败，继续等待" << endl;
        goto wait;
    }

sendPacket2:
    prepareHeader(false, true, false);
    if (sendto(udpSocket, sendBuffer, HSZ, 0, (sockaddr*)&dstAddr, sizeof(dstAddr)) > 0)
    {
        cout << "第三次握手发送" << endl;
    }
    else {
        cout << "第三次握手发送失败" << endl;
        goto sendPacket2;
    }


    cout << "三次握手完成，连接建立成功" << endl;
    RecvAddr = dstAddr;
    len_RecvAddr = len_dstAddr;
}


void Terminal::waitConnedct(int port)
{
    //为自己的socket绑定端口
    sockaddr_in myAddr;
    myAddr.sin_family = AF_INET;
    myAddr.sin_port = htons(port);
    myAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    if (bind(udpSocket, (SOCKADDR*)&myAddr, sizeof(myAddr)) == SOCKET_ERROR)
    {
        cout << "bind error" << endl;
    }
    else
    {
        cout << "端口" << port << "绑定完成" << endl;
    }

    //定义对方(发送方)地址
    sockaddr_in dstAddr;
    int len_dstAddr = sizeof(dstAddr);

    cout << "等待连接请求" << endl;

    
wait1:
    while (recvfrom(udpSocket, recvBuffer, BUFFER_MAX_SIZE, 0, (sockaddr*)&dstAddr, &len_dstAddr) <= 0); //阻塞接收

    Header *recvPacket1 = (Header*)recvBuffer;
    if (recvPacket1->testCheckSum() && recvPacket1->testSYN())
    {
        cout << "第一次握手接收成功" << endl;
    }
    else {
        cout << "第一次握手确认失败，继续等待" << endl;
        goto wait1;
    }


sendPacket:
    prepareHeader(true, true, false);
    if (sendto(udpSocket, sendBuffer, HSZ, 0, (sockaddr*)&dstAddr, sizeof(dstAddr)) > 0)
    {
        cout << "第二次握手发送" << endl;
    }
    else{
        cout << "第二次握手发送失败" << endl;
        goto sendPacket;
    }

    clock_t start = clock();
wait2:
    while (recvfrom(udpSocket, recvBuffer, BUFFER_MAX_SIZE, 0, (sockaddr*)&dstAddr, &len_dstAddr) <= 0)
    {
        clock_t end = clock();
        if (end - start > TIMEOUT) 
        {
            cout << "超时重发第二次握手" << endl;
            goto sendPacket;
        }
    }
    Header *recvPacket2 = (Header*)recvBuffer;
    if (recvPacket2->testCheckSum() && recvPacket2->testACK())
    {
        cout << "第三次握手接收成功" << endl;
    }
    else {
        cout << "第三次握手确认失败，继续等待" << endl;
        goto wait2;
    }


    cout << "连接建立完成" << endl;
    SendAddr = dstAddr;          //存下对方的地址
    len_SendAddr = len_dstAddr;
}


void Terminal::prepareHeader(bool SYN, bool ACK, bool FIN, uint32_t seqNum, uint32_t ackNum, uint32_t payloadSize)
{
    Header *newHeader = new Header();
    if (SYN) newHeader->setSYN();
    if (ACK) newHeader->setACK();
    if (FIN) newHeader->setFIN();
    newHeader->setSeqNum(seqNum);
    newHeader->setAckNum(ackNum);
    newHeader->setPayloadSize(payloadSize);
    memcpy(sendBuffer, newHeader, HSZ);
    delete newHeader;
    //应该复制到缓冲区之后再计算校验和
    ((Header*)sendBuffer)->calCheckSum();
}


void Header::calCheckSum()
{
    checksum = 0;               //校验和清零
    int totalSize = HSZ + payloadSize;
    int count = totalSize / 2;
    u_short *p = (u_short*)this;    //一个指向两字节大小数据的指针
    u_long sum = 0;
    while (count--)
    {
        sum += *(p++);
        if (sum & 0xffff0000)   //有溢出
        {
            sum &= 0xffff;
            sum += 1;
        }
    }

    if (totalSize % 2 == 1)     //总字节数是奇数，最后多出来一个字节，需要填充0再算
    {
        u_short tmp = *p;
        tmp &= 0xff00;
        sum += tmp;
        if (sum & 0xffff0000)   //有溢出
        {
            sum &= 0xffff;
            sum += 1;
        }
    }

    checksum = ~(sum & 0xffff);
}

bool Header::testCheckSum()
{
    int totalSize = HSZ + payloadSize;
    int count = totalSize / 2;
    u_short *p = (u_short*)this;    //一个指向两字节大小数据的指针
    u_long sum = 0;
    while (count--)
    {
        sum += *(p++);
        if (sum & 0xffff0000)   //有溢出
        {
            sum &= 0xffff;
            sum += 1;
        }
    }

    if (totalSize % 2 == 1)     //总字节数是奇数，最后多出来一个字节，需要填充0再算
    {
        u_short tmp = *p;
        tmp &= 0xff00;
        sum += tmp;
        if (sum & 0xffff0000)   //有溢出
        {
            sum &= 0xffff;
            sum += 1;
        }
    }

    return sum == 0xFFFF;
}

void WinSockInit()
{
    WSADATA wsData;
    WORD ver = MAKEWORD(2, 2);
    int wsOK = WSAStartup(ver, &wsData);
    if (wsOK != 0) {
        cerr << "winsock初始化失败" << endl;
    }
}

void Terminal::sendFile(string filePath)
{
    ifstream file(filePath, ios::binary | ios::ate);
    if (!file.is_open()) 
    {
        cout << "指定文件打开失败" << endl;
        return;
    }
    else
    {
        cout << "文件" << filePath << "打开成功" << endl;
    }

    long long remainFileSize = file.tellg();    //剩余要发送的文件大小
    file.seekg(0, ios::beg);

    uint32_t seqNum = 0;                        //这里的实现为两端都默认从序号为0的包开始收发
    int count = 1;                              //用于计数发送了多少个包

    cout << "文件大小为" << remainFileSize << endl;
    while (remainFileSize > 0)
    {
        int sizeToSend;
        if (remainFileSize < MDS)
        {
            sizeToSend = remainFileSize;
            remainFileSize = 0;
        }
        else {
            sizeToSend = MDS;
            remainFileSize -= MDS;
        }

        if (!file.read(sendBuffer + HSZ, sizeToSend))
        {
            cout << "读取文件失败" << endl;
            file.close();
            return;
        }

        //下面开始发送包，若超时则重传
        bool sendOK = false;    //发送成功，收到了正确确认
        while (!sendOK)
        {
            prepareHeader(false, false, remainFileSize == 0, seqNum, 0, sizeToSend);    //如果是最后一个包则发送FIN
            if (sendto(udpSocket, sendBuffer, HSZ + sizeToSend, 0, (sockaddr*)&RecvAddr, sizeof(RecvAddr)) > 0)
            {
                cout << "第" << count << "个包发送" << endl;
            }
            else {
                cout << "第" << count << "个包发送失败" << endl;
                continue;
            }

            clock_t start = clock();
            bool timeout = false;
            while (!sendOK)
            {
                while (recvfrom(udpSocket, recvBuffer, BUFFER_MAX_SIZE, 0, (sockaddr*)&RecvAddr, &len_RecvAddr) <= 0)
                {
                    clock_t end = clock();
                    if (end - start > TIMEOUT)
                    {
                        cout << "第" << count << "个包超时，重新发送" << endl;
                        timeout = true;
                        break;
                    }
                }
                if (timeout) break;

                Header *recvPacket = (Header*)recvBuffer;
                if (recvPacket->testCheckSum() && recvPacket->getAckNum() == seqNum && (!recvPacket->testFIN() || recvPacket->testACK()))
                {
                    cout << "第" << count << "个包已确认" << endl;
                    sendOK = true;
                }
                else {
                    cout << "第" << count << "个包确认失败" << endl;
                }
            }
        }

        seqNum ^= 1;
        count += 1;
    }

    cout << "文件传输完毕" << endl;
    file.close();
}

void Terminal::recvFile(string filePath)
{
    ofstream file(filePath, ios::binary);
    if (!file.is_open())
    {
        cout << "文件打开失败" << endl;
        return;
    }
    else
    {
        cout << "文件" << filePath << "创建成功" << endl;
    }


    uint32_t ackNum = 0;
    int count = 1;
    while (true)
    {
        while (recvfrom(udpSocket, recvBuffer, BUFFER_MAX_SIZE, 0, (sockaddr*)&SendAddr, &len_SendAddr) <= 0);

        Header *recvPacket = (Header*)recvBuffer;
        if (!recvPacket->testCheckSum())                //被污染
        {
            cout << "收到的第" << count << "个包被污染" << endl;
            continue;
        }        
        else if (recvPacket->getSeqNum() == ackNum)    //序号正确
        {
            prepareHeader(false, recvPacket->testFIN(), false, 0, ackNum);
            if (sendto(udpSocket, sendBuffer, HSZ, 0, (sockaddr*)&SendAddr, sizeof(SendAddr)) > 0)
            {
                cout << "第" << count << "个包确认" << endl;
            }

            //写入文件
            file.write(recvBuffer + HSZ, recvPacket->getPayLoadSize());


            if (recvPacket->testFIN()) break;   //接收文件结束
            count += 1;
            ackNum ^= 1;
        }

        else                                    //序号不正确
        {
            prepareHeader(false, false, false, 0, ackNum^1);
            if (sendto(udpSocket, sendBuffer, HSZ, 0, (sockaddr*)&SendAddr, sizeof(SendAddr)) > 0)
            {
                cout << "第" << count << "个包序号错误，需要重发" << endl;
            }
        }
    }

    cout << "文件接收完成" << endl;
    file.close();
}