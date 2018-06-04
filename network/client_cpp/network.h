#ifndef MY_NETWORK_H
#define MY_NETWORK_H

#ifdef WIN32
#include <winsock2.h>
#pragma comment(lib,"ws2_32.lib")
#else
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#endif

#include <list>
using namespace std;

#ifndef WIN32
typedef	int	SOCKET;
#endif

#define RECV_BUFF_LENGTH	10240

class MyNetwork
{
public:
	struct PackageHead
	{
		int		iLength;
		int		iProtocol;
	};
	struct SendData
	{
		char*	pData;
		int		iLength;
	};
	enum NetworkStatus
	{
		NETWORK_STATUS_IDLE,
		NETWORK_STATUS_CONNECTING,
		NETWORK_STATUS_CONNECTED,
		NETWORK_STATUS_DISCONNECTED,
	};
	typedef void (netCallback)(char* pBuffer);
public:
	MyNetwork(netCallback pCallback);
	~MyNetwork(void);

	void	connectServer(const char* pIp, unsigned short sPort);
	int		processNetwork(void);
	void	sendData(char* pBuffer, int iLength);
	void	closeSocket(void);
private:
	void	processData(char* pBuffer, int iLength);
private:
	NetworkStatus		eNetworkStatus;
	SOCKET				kSocket;
	fd_set				kSocketReadSet;
	fd_set				kSocketWriteSet;
	fd_set				kSocketErrorSet;
	char				pRecvBuffer[RECV_BUFF_LENGTH];
	char				pDataBuffer[RECV_BUFF_LENGTH];
	int					iDataLength;
	netCallback*		pNetCallback;
	list<SendData*>		pSendList;
};

#endif //MY_NETWORK_H
