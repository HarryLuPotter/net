#include "Terminal.h"
int main()
{
    WinSockInit();
    cout << "发送端" << endl;
    Terminal term;
    term.buildConnect("127.0.0.1", 1500);
    term.sendFile("猫头棍哥.jpg");
}




