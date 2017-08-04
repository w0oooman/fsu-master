#include <stdio.h>
#include <stdlib.h>
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
	cout << "---------------------------------------------------------------------" << endl;
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
	//else cout << "sendto:len=" << len << " sendlen=" << sendlen <<" time="<<GetTickCount()<< endl;
	return 0;
}

fsul *InitFsu(void *user)
{
	fsul *fsu = fsu_create(0x11223344, (1 << 12), 1<<12, (void*)user);
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
	//在本机做了端口映射,发现外网还是不能访问部署在本机的服务器,发现本地路由器上IP跟网站获取的IP不同，
	//后来才知道路由器地址是经过了运营商转换了一次的私有地址，
	//此地址不能再公网上使用，即不能做服务器被公网访问,除非到运营商做端口映射...
	addr.sin_addr.s_addr = inet_addr("192.168.2.172");
	addr.sin_port = htons(8888);

	int nMode = 1; //0：阻塞
	ioctlsocket(soc,FIONBIO, (u_long FAR*) &nMode);//非阻塞设置
	//fcntl( soc, F_SETFL, O_NONBLOCK);
	int nRecvBuf = 48 * 1024;//设置为48K
	setsockopt(soc, SOL_SOCKET, SO_RCVBUF, (const char*)&nRecvBuf, sizeof(int));

	char buf[4096];
	int num = 10;
	int addrlen = sizeof(addr);
	srand(GetTickCount());

	cout << "wait to send..." << endl;
	int number = 0;
	int _myinit = 0,sendcount = 0;
	fsul *fsu;

	while (1/*num-- > 0*/)
	{
		number++;
		memset(buf, 0, sizeof(buf));
		//number = rand() % 1000 + 1000;
		*(int*)buf = number;

		if (_myinit == 0)
		{
			myuser *user = new myuser;
			user->soc = soc;
			user->addr = addr;
			fsu = InitFsu(user);
			_myinit = 1;
		}

		static int sendnum_sum = 0;
		//只发送20次
		if (number < 25)
		{
			//int sendnum = rand() % 3096 + 900;
			int sendnum = rand() % 1096 + 500;
			for (int i = 0; i < sendnum; i++)
				sprintf_s(buf + 4 + i, sizeof(buf) - 4 - i, "%s", "-");
			fsu_send(fsu, buf, strlen(buf + 4) + 4);

			sendnum_sum += sendnum+4;
			int size = min(FSU_MSS_LEN, sendnum);
			while (size > 0)
			{
				sendnum -= size;
				sendcount++;
				size = min(FSU_MSS_LEN, sendnum);
			}
		}
		else
		{
			static int i = 0;
			if (i == 0){
				i = 1;
				cout << ",,,,,,,,," << sendcount << "  sendnum_sum=" << sendnum_sum << ",,,,,,,," << endl;
			}
		}

		//    
		//if (-1 == fsu_update(fsu))
		//{
		//	cout<<"release"<<endl;
		//	fsu_release(fsu);
		//	cout<<"release over"<<endl;
	 //       break;
		//}

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
				int input_len = fsu_input(fsu, buf, len);

			}
		}

		while (fsu){
			static int recvcount = 0;
			char temp[FSU_INIT_MTU] = { 0 };
			int hr = fsu_recv(fsu, temp, sizeof(temp)); //whb hr==-1
			if (hr > 0)
			{
				//	cout << "-------:" << *(int*)temp << endl;
				recvcount++;
			}
			else
				break;
		}

		if (-1 == fsu_update(fsu))
		{
			cout<<"release"<<endl;
			fsu_release(fsu);
			cout<<"release over"<<endl;
			break;
		}
	}

	cout<<"over"<<endl;
	Sleep(3000);
	system("pause");
	return 0;
}

