#include <stdio.h>
#include <WinSock2.h>
#include <iostream>
#include "../fsu(fast safe udp)/fsu.h"
using namespace std;
#pragma comment (lib,"ws2_32.lib")

struct myuser
{
	SOCKET soc;
	sockaddr_in addr;
};

void MyWriteLog(/*struct FSULogic *fsu, */const char *log, void *user)
{
	cout << "---------------------------------------" << endl;
	cout << log << endl;
	//Sleep(30000);
}

int udp_output(const char *buf, int len, void *user)
{
	myuser *p = (myuser*)user;
	int addrlen = sizeof(sockaddr_in);
	int sendlen = sendto(p->soc, buf, len, 0, (sockaddr*)&p->addr, addrlen);
	if (sendlen <= 0)
	{
		cout << "sendto error.id=" << WSAGetLastError() << endl;
	}
	//else cout << "sendto:len=" << len << " sendlen=" << sendlen<<" time=" << GetTickCount() << endl;
	return 0;
}

fsul *InitKcp(void *user)
{
	fsul *fsu = fsu_create(0x11223344, 1 << 16, 1 << 16, (void*)user);
	fsu->output = udp_output;
	fsu->writelog = MyWriteLog;

	return fsu;
}

int main()
{
	sockaddr_in addr;
	WSADATA ws;
	if (WSAStartup(MAKEWORD(2, 2), &ws) != 0)
	{
		cout << "wsastartup error.id=" << WSAGetLastError() << endl;
		return 0;
	}
	SOCKET soc = socket(AF_INET, SOCK_DGRAM, 0);
	if (soc == INVALID_SOCKET)
	{
		cout << "create socket error.id=" << WSAGetLastError() << endl;
		return 0;
	}
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(8888);
	int addrlen = sizeof(addr);

	if (::bind(soc, (sockaddr*)&addr, addrlen) < 0)
	{
		cout << "bind error.id=" << WSAGetLastError() << endl;
		return 0;
	}

	int nMode = 1; //0：阻塞
	ioctlsocket(soc,FIONBIO, (u_long FAR*) &nMode);//非阻塞设置
	//fcntl( soc, F_SETFL, O_NONBLOCK);
	int nRecvBuf = 48 * 1024;//设置为48K
	setsockopt(soc, SOL_SOCKET, SO_RCVBUF, (const char*)&nRecvBuf, sizeof(int));

	char buf[2048];
	int num = 10;

	cout << "waiting..." << endl;
	int seq = 0;
	int _myinit = 0;
	fsul *fsu = NULL;

	while (1)
	{
		if (fsu != NULL)
		{
		    if(-1 == fsu_update(fsu))
			{
				cout <<"-1 == fsu_update" << endl;
			    fsu_release(fsu);
				break;
			}
		}

		while (1){
			memset(buf, 0, sizeof(buf));
			int len = recvfrom(soc, buf, sizeof(buf), 0, (sockaddr*)&addr, &addrlen);
			if (len < 0)
			{
				//cout << "recvfrom error.id=" << WSAGetLastError() << endl;
				Sleep(2);
				break;
			}
			else
			{
				//cout << "recvfrom:len=" << len << endl;
				if (_myinit == 0)
				{
					myuser *user = new myuser;
					user->soc = soc;
					user->addr = addr;
					fsu = InitKcp(user);
					_myinit = 1;
				}

				int input_len = fsu_input(fsu, buf, len);
			}
		}

		while (fsu){
			static int recvcount = 0;
			char temp[FSU_INIT_MTU] = { 0 };
			int hr = fsu_recv(fsu, temp, sizeof(temp)); //whb hr==-1
			if (hr > 0)
			{
				//cout << "--------" << *(int*)temp << endl;
				fsu_send(fsu, temp, hr);
				recvcount++;
			}
			else
				break;
		}

	}

	system("pause");
	return 0;
}