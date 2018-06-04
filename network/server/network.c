#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>

int main(void)
{
	printf("begin\n");
	int iListenFd = socket(AF_INET, SOCK_STREAM, 0);	
	if (-1 == iListenFd)
	{
		printf("create socket fail\n");
		return 1;
	}

	int iTrueflag = 1;
	setsockopt(iListenFd, SOL_SOCKET, SO_REUSEADDR, &iTrueflag, sizeof(iTrueflag));

	struct sockaddr_in kServerAddr;
	memset(&kServerAddr, 0, sizeof(kServerAddr));
	kServerAddr.sin_family = AF_INET;
	kServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	kServerAddr.sin_port = htons(5566);
	if (-1 == bind(iListenFd, (struct sockaddr*)(&kServerAddr), sizeof(kServerAddr)))
	{
		printf("bind socket fail\n");
		return 1;
	}
	if (-1 == listen(iListenFd, SOMAXCONN))
	{
		printf("listen socket fail\n");
		return 1;
	}
	while(1)
	{
		struct sockaddr_in	kClientAddr;
		socklen_t			kClientAddrLen;
		int iClientFd = accept(iListenFd, (struct sockaddr*)&kClientAddr, &kClientAddrLen);
		if (-1 == iClientFd)
		{
			printf("socket accept fail\n");
			break;
		}
		printf("accept new socket\n");
		char pRecvBuffer[1024];
		while(1)
		{
			int iRecvSize = recv(iClientFd, pRecvBuffer, sizeof(pRecvBuffer), 0);
			if (iRecvSize <= 0)
			{
				printf("socket disconnect\n");
				break;
			}
			printf("recv size %d, data: %s\n", iRecvSize, pRecvBuffer);
			send(iClientFd, pRecvBuffer, iRecvSize, 0);
		}
	}
	return 0;
}
