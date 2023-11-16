#include "Terminal.h"

int main()
{
    WinSockInit();
    cout << "接收端" << endl;
    Terminal term;
    term.waitConnedct(1500);
    term.recvFile("test2.jpg");
}