#include "network.h"
#include <stdio.h>
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
#include <stdlib.h>
#endif

#ifndef WIN32
typedef	int	SOCKET;
#endif

#define RECV_BUFF_LENGTH		65536
#define PACKAGE_HEAD_TYPE		short
#define PACKAGE_HEAD_LENGTH		((int)sizeof(PACKAGE_HEAD_TYPE))

struct SendData
{
	char*				pData;
	int					iLength;
	struct SendData*	pNext;
};

enum NetworkStatus
{
	NETWORK_STATUS_IDLE,
	NETWORK_STATUS_CONNECTING,
	NETWORK_STATUS_CONNECTED,
	NETWORK_STATUS_DISCONNECTED,
};

struct MyNetwork
{
	enum NetworkStatus	eNetworkStatus;
	SOCKET				kSocket;
	fd_set				kSocketReadSet;
	fd_set				kSocketWriteSet;
	fd_set				kSocketErrorSet;
	char				pRecvBuffer[RECV_BUFF_LENGTH];
	char				pDataBuffer[RECV_BUFF_LENGTH];
	int					iDataLength;
	const char*			pNetCallback;
	struct SendData*	pSendList;
	struct SendData*	pSendListLast;
	lua_State*			pLuaState;
};

/* init in lua
	_G["network_callback_scene"] = function (pData) print(pData) end
	initMyNetwork("network_callback_scene")
*/
static struct MyNetwork* initMyNetwork(lua_State* L, const char* pCallback)
{
	struct MyNetwork* pNet = (struct MyNetwork*)malloc(sizeof(struct MyNetwork));
	if (pNet)
	{
#ifdef WIN32
		WSADATA wsaData;
		WSAStartup(MAKEWORD(2, 2), &wsaData);
		pNet->kSocket = INVALID_SOCKET;
#else
		pNet->kSocket = -1;
#endif
		pNet->eNetworkStatus = NETWORK_STATUS_IDLE;
		pNet->iDataLength = 0;
		pNet->pNetCallback = pCallback;
		pNet->pSendList = NULL;
		pNet->pSendListLast = NULL;
		pNet->pLuaState = L;
	}
	return pNet;
}

int to_lua_initMyNetwork(lua_State* L)
{
	const char* pCallback = lua_tostring(L, 1);
	struct MyNetwork* pNet = initMyNetwork(L, pCallback);
	lua_pushlightuserdata(L, pNet);

	return 1;
}

static void closeSocket(struct MyNetwork* pNet)
{
#ifdef WIN32
	if (pNet->kSocket == INVALID_SOCKET)
		return;
	closesocket(pNet->kSocket);
	pNet->kSocket = INVALID_SOCKET;
#else
	if (pNet->kSocket < 0)
		return;
	shutdown(pNet->kSocket, 2);
	pNet->kSocket = -1;
#endif
	pNet->eNetworkStatus = NETWORK_STATUS_IDLE;
}

int to_lua_closeSocket(lua_State* L)
{
	struct MyNetwork* pNet = lua_touserdata(L, 1);
	closeSocket(pNet);

	return 0;
}

static void destoryNetwork(struct MyNetwork* pNet)
{
	if (pNet)
	{
		closeSocket(pNet);
		free(pNet);
#ifdef WIN32
		WSACleanup();
#endif
	}
}

static int connectServer(struct MyNetwork *pNet, const char * pIp, unsigned short sPort)
{
	if (NETWORK_STATUS_CONNECTING == pNet->eNetworkStatus || NETWORK_STATUS_CONNECTED == pNet->eNetworkStatus)
		return 1;
#ifdef WIN32
	if ((pNet->kSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_IP)) == INVALID_SOCKET)
#else
	if ((pNet->kSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_IP)) < 0)
#endif
	{
		printf("create socket fail.\n");
		return 1;
	}
	//set non-blocking
#ifdef WIN32
	u_long iMode = 1;
	ioctlsocket(pNet->kSocket, FIONBIO, &iMode);
#else
	int flags = fcntl(pNet->kSocket, F_GETFL, 0);
	fcntl(pNet->kSocket, F_SETFL, flags | O_NONBLOCK);
#endif

	struct sockaddr_in kAddrServer;
	memset(&kAddrServer, 0, sizeof(struct sockaddr_in));
	kAddrServer.sin_family = AF_INET;
	kAddrServer.sin_port = htons(sPort);
	kAddrServer.sin_addr.s_addr = inet_addr(pIp);

	int iRet = connect(pNet->kSocket, (struct sockaddr*)&kAddrServer, sizeof(struct sockaddr));
	if (-1 == iRet)
		printf("connect fail, errno: %d\n", errno);

	pNet->eNetworkStatus = NETWORK_STATUS_CONNECTING;
	return 0;
}

int to_lua_connectServer(lua_State* L)
{
	struct MyNetwork* pNet = lua_touserdata(L, 1);
	const char* pIp = lua_tostring(L, 2);
	unsigned short sPort = lua_tointeger(L, 3);

	int iRet = connectServer(pNet, pIp, sPort);
	lua_pushinteger(L, iRet);

	return 1;
}

static void sendData(struct MyNetwork* pNet, const char* pBuffer, int iLength)
{
	if (NULL == pBuffer || iLength <= 0)
		return;
	struct SendData* pSendData = (struct SendData*)malloc(sizeof(struct SendData));
	if (pSendData)
	{
		pSendData->iLength = iLength + PACKAGE_HEAD_LENGTH;
		pSendData->pNext = NULL;
		pSendData->pData = (char*)malloc(pSendData->iLength);
		if (pSendData->pData)
		{
			memcpy(pSendData->pData, &iLength, PACKAGE_HEAD_LENGTH);
			memcpy(pSendData->pData + PACKAGE_HEAD_LENGTH, pBuffer, iLength);
			if (NULL == pNet->pSendList)
			{
				pNet->pSendList = pSendData;
				pNet->pSendListLast = pSendData;
			}
			else
			{
				pNet->pSendListLast->pNext = pSendData;
				pNet->pSendListLast = pSendData;
			}
		}
		else
			free(pSendData);
	}
}

int to_lua_sendData(lua_State* L)
{
	struct MyNetwork* pNet = lua_touserdata(L, 1);
	const char* pBuffer = lua_tostring(L, 2);
	int iLength = lua_tointeger(L, 3);

	sendData(pNet, pBuffer, iLength);

	return 0;
}

static void processData(struct MyNetwork *pNet, char* pBuffer, int iLength)
{
	int		iBufferOffset = 0;
	while (iBufferOffset < iLength)
	{
		if (pNet->iDataLength < PACKAGE_HEAD_LENGTH)
		{
			if (iLength - iBufferOffset <= PACKAGE_HEAD_LENGTH - pNet->iDataLength)
			{
				memcpy(pNet->pDataBuffer + pNet->iDataLength, pBuffer + iBufferOffset, iLength - iBufferOffset);
				pNet->iDataLength += iLength - iBufferOffset;
				return; //data is not enough
			}
			else
			{
				memcpy(pNet->pDataBuffer + pNet->iDataLength, pBuffer + iBufferOffset, PACKAGE_HEAD_LENGTH - pNet->iDataLength);
				iBufferOffset += PACKAGE_HEAD_LENGTH - pNet->iDataLength;
				pNet->iDataLength = PACKAGE_HEAD_LENGTH;
			}
		}

		int		iPackageLength = (int)((PACKAGE_HEAD_TYPE*)pNet->pDataBuffer);
		if (iPackageLength <= 0 || iPackageLength > RECV_BUFF_LENGTH - PACKAGE_HEAD_LENGTH)
		{
			pNet->iDataLength = 0;
			return; //package length error
		}
		if (iLength - iBufferOffset < iPackageLength)
		{
			memcpy(pNet->pDataBuffer + pNet->iDataLength, pBuffer + iBufferOffset, iLength - iBufferOffset);
			pNet->iDataLength += iLength - iBufferOffset;
			return; //data is not enough
		}
		else
		{
			memcpy(pNet->pDataBuffer + pNet->iDataLength, pBuffer + iBufferOffset, iPackageLength - PACKAGE_HEAD_LENGTH);
			iBufferOffset += iPackageLength - PACKAGE_HEAD_LENGTH;
			pNet->iDataLength = 0;

			lua_getglobal(pNet->pLuaState, "_G");
			if (lua_istable(pNet->pLuaState, -1))
			{
				lua_getfield(pNet->pLuaState, -1, pNet->pNetCallback);
				if (lua_isfunction(pNet->pLuaState, -1))
				{
					lua_pushlstring(pNet->pLuaState, pNet->pDataBuffer + PACKAGE_HEAD_LENGTH, iPackageLength - PACKAGE_HEAD_LENGTH);
					lua_pcall(pNet->pLuaState, 1, 0, 0);
				}
			}
		}
	}
}

static int processNetwork(struct MyNetwork* pNet)
{
	if (NETWORK_STATUS_CONNECTED != pNet->eNetworkStatus && NETWORK_STATUS_CONNECTING != pNet->eNetworkStatus)
		return 1;
	struct timeval tm;
	tm.tv_sec = 10;
	tm.tv_usec = 0;

	FD_ZERO(&pNet->kSocketReadSet);
	FD_ZERO(&pNet->kSocketWriteSet);
	FD_ZERO(&pNet->kSocketErrorSet);
	FD_SET(pNet->kSocket, &pNet->kSocketReadSet);
	FD_SET(pNet->kSocket, &pNet->kSocketWriteSet);
	FD_SET(pNet->kSocket, &pNet->kSocketErrorSet);

	int iRet = select(pNet->kSocket + 1, &pNet->kSocketReadSet, &pNet->kSocketWriteSet, &pNet->kSocketErrorSet, &tm);
	if (-1 == iRet)
	{
		printf("select fail, errno: %d\n", errno);
		pNet->eNetworkStatus = NETWORK_STATUS_DISCONNECTED;
		return 1;
	}
	else if (iRet > 0)
	{
		pNet->eNetworkStatus = NETWORK_STATUS_CONNECTED;
		//read
		if (FD_ISSET(pNet->kSocket, &pNet->kSocketReadSet))
		{
			int	iRecvSize = recv(pNet->kSocket, pNet->pRecvBuffer, RECV_BUFF_LENGTH, 0);
			if (iRecvSize <= 0)
			{
				pNet->eNetworkStatus = NETWORK_STATUS_DISCONNECTED;
				printf("socket disconnect\n");
				return 1;
			}
			else
				processData(pNet, pNet->pRecvBuffer, iRecvSize);
		}
		//write
		if (FD_ISSET(pNet->kSocket, &pNet->kSocketWriteSet))
		{
			while (pNet->pSendList)
			{
				struct SendData* pSendData = pNet->pSendList;
				pNet->pSendList = pNet->pSendList->pNext;
				send(pNet->kSocket, pSendData->pData, pSendData->iLength, 0);
				free(pSendData->pData);
				free(pSendData);
			}
		}
		//error
		if (FD_ISSET(pNet->kSocket, &pNet->kSocketErrorSet))
		{
			printf("select error \n");
			pNet->eNetworkStatus = NETWORK_STATUS_DISCONNECTED;
		}
	}
	return 0;
}

int to_lua_processNetwork(lua_State* L)
{
	struct MyNetwork* pNet = lua_touserdata(L, 1);

	int iRet = processNetwork(pNet);
	lua_pushinteger(L, iRet);

	return 1;
}

void register_network_function(lua_State* L)
{
	lua_register(L, "initMyNetwork", to_lua_initMyNetwork);
	lua_register(L, "closeSocket", to_lua_closeSocket);
	lua_register(L, "connectServer", to_lua_connectServer);
	lua_register(L, "sendData", to_lua_sendData);
	lua_register(L, "processNetwork", to_lua_processNetwork);
}