#include "network.h"

MyNetwork::MyNetwork(netCallback pCallback)
{
#ifdef WIN32
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
	kSocket = INVALID_SOCKET;
#else
	kSocket = -1;
#endif
	eNetworkStatus = NETWORK_STATUS_IDLE;
	iDataLength = 0;
	pNetCallback = pCallback;
}

MyNetwork::~MyNetwork(void)
{
	closeSocket();
#ifdef WIN32
	WSACleanup();
#endif
}

void MyNetwork::connectServer(const char * pIp, unsigned short sPort)
{
	if (NETWORK_STATUS_CONNECTING == eNetworkStatus || NETWORK_STATUS_CONNECTED == eNetworkStatus)
		return;
#ifdef WIN32
	if ((kSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_IP)) == INVALID_SOCKET)
#else
	if ((kSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_IP)) < 0)
#endif
	{
		printf("create socket fail.\n");
		return;
	}
	//set non-blocking
#ifdef WIN32
	u_long iMode = 1;
	ioctlsocket(kSocket, FIONBIO, &iMode);
#else
	int flags = fcntl(kSocket, F_GETFL, 0);
	fcntl(kSocket, F_SETFL, flags | O_NONBLOCK);
#endif

	struct sockaddr_in kAddrServer;
	memset(&kAddrServer, 0, sizeof(sockaddr_in));
	kAddrServer.sin_family = AF_INET;
	kAddrServer.sin_port = htons(sPort);
	kAddrServer.sin_addr.s_addr = inet_addr(pIp);

	int iRet = connect(kSocket, (sockaddr*)&kAddrServer, sizeof(sockaddr));
	if (-1 == iRet)
		printf("connect fail, errno: %d\n", errno);

	eNetworkStatus = NETWORK_STATUS_CONNECTING;
}

void MyNetwork::sendData(char* pBuffer, int iLength)
{
	if (NULL == pBuffer || iLength <= 0)
		return;
	SendData* pSendData = (SendData*)malloc(sizeof(SendData));
	if (pSendData)
	{
		pSendData->iLength = iLength;
		pSendData->pData = (char*)malloc(iLength);
		if (pSendData->pData)
		{
			memcpy(pSendData->pData, pBuffer, iLength);
			pSendList.push_back(pSendData);
		}
		else
			free(pSendData);
	}
}

void MyNetwork::closeSocket()
{
#ifdef WIN32
	if (kSocket == INVALID_SOCKET)
		return;
	closesocket(kSocket);
	kSocket = INVALID_SOCKET;
#else
	if (kSocket < 0)
		return;
	shutdown(kSocket, 2);
	kSocket = -1;
#endif
	eNetworkStatus = NETWORK_STATUS_IDLE;
}

int MyNetwork::processNetwork(void)
{
	if (NETWORK_STATUS_CONNECTED != eNetworkStatus && NETWORK_STATUS_CONNECTING != eNetworkStatus)
		return 1;
	struct timeval tm;
	tm.tv_sec = 10;
	tm.tv_usec = 0;

	FD_ZERO(&kSocketReadSet);
	FD_ZERO(&kSocketWriteSet);
	FD_ZERO(&kSocketErrorSet);
	FD_SET(kSocket, &kSocketReadSet);
	FD_SET(kSocket, &kSocketWriteSet);
	FD_SET(kSocket, &kSocketErrorSet);

	int iRet = select(kSocket + 1, &kSocketReadSet, &kSocketWriteSet, &kSocketErrorSet, &tm);
	if (-1 == iRet)
	{
		printf("select fail, errno: %d\n", errno);
		eNetworkStatus = NETWORK_STATUS_DISCONNECTED;
		return 1;
	}
	else if (iRet > 0)
	{
		eNetworkStatus = NETWORK_STATUS_CONNECTED;
		//read
		if (FD_ISSET(kSocket, &kSocketReadSet))
		{
			int	iRecvSize = recv(kSocket, pRecvBuffer, RECV_BUFF_LENGTH, 0);
			if (iRecvSize <= 0)
			{
				eNetworkStatus = NETWORK_STATUS_DISCONNECTED;
				printf("socket disconnect\n");
				return 1;
			}
			else
				processData(pRecvBuffer, iRecvSize);
		}
		//write
		if (FD_ISSET(kSocket, &kSocketWriteSet))
		{
			while (!pSendList.empty())
			{
				SendData* pSendData = pSendList.front();
				pSendList.pop_front();
				send(kSocket, pSendData->pData, pSendData->iLength, 0);
				free(pSendData->pData);
				free(pSendData);
			}
		}
		//error
		if (FD_ISSET(kSocket, &kSocketErrorSet))
		{
			printf("select error \n");
			eNetworkStatus = NETWORK_STATUS_DISCONNECTED;
		}
	}
	return 0;
}

void MyNetwork::processData(char* pBuffer, int iLength)
{
	int		iBufferOffset = 0;
	while (iBufferOffset < iLength)
	{
		if (iDataLength < sizeof(PackageHead))
		{
			if (iLength - iBufferOffset < sizeof(PackageHead) - iDataLength)
			{
				memcpy(pDataBuffer + iDataLength, pBuffer + iBufferOffset, iLength - iBufferOffset);
				iDataLength += iLength - iBufferOffset;
				return; //data is not enough
			}
			else
			{
				memcpy(pDataBuffer + iDataLength, pBuffer + iBufferOffset, sizeof(PackageHead) - iDataLength);
				iBufferOffset += sizeof(PackageHead) - iDataLength;
				iDataLength = sizeof(PackageHead);
			}
		}
		int		iPackageLength = ((PackageHead*)pDataBuffer)->iLength;
		if (iPackageLength > RECV_BUFF_LENGTH || iPackageLength <= 0)
		{
			iDataLength = 0;
			return; //package length error
		}
		if (iLength - iBufferOffset < iPackageLength - sizeof(PackageHead))
		{
			if (iLength > iBufferOffset)
			{
				memcpy(pDataBuffer + iDataLength, pBuffer + iBufferOffset, iLength - iBufferOffset);
				iDataLength += iLength - iBufferOffset;
			}
			return; //data is not enough
		}
		else
		{
			if (iPackageLength > sizeof(PackageHead))
			{
				memcpy(pDataBuffer + iDataLength, pBuffer + iBufferOffset, iPackageLength - sizeof(PackageHead));
				iBufferOffset += iPackageLength - sizeof(PackageHead);
			}
			pNetCallback(pDataBuffer);
			iDataLength = 0;
		}
	}
}
