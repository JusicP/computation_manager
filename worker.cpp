#include "worker.h"
#include "shared.h"

#include <math.h>
#include <cstdio>
#include <time.h>

#ifdef WIN32
#include <Windows.h>
#endif

double Function_G(int x)
{
    double result = 0.0;
    for (int i = 1; i < x; i++)
    {
        result += sqrt(i) / log(i);
        Sleep(400);
    }

    return result;
}

double Function_H(int x)
{
    double result = 0.0;
    for (int i = 1; i < x; i++)
    {
        result += sqrt(i) * log(i);
        Sleep(500);
    }

    return result;
}

double ProcessMsg(TaskMsg* msg, bool& failed)
{
    failed = false;
    switch (msg->componentSymbol)
    {
    case 'g':
        return Function_G(msg->x);
    case 'h':
        return Function_H(msg->x);        
    }

    failed = true;

    return 0.0;
}

int Worker_RunClient()
{
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0)
    {
        printf("WSAStartup failed: %d\n", iResult);
        return 1;
    }

    sockaddr_in servAddr = {};
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(MGR_PORT);
    servAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	// Create a SOCKET for connecting to server
    SOCKET sockFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sockFd == INVALID_SOCKET)
    {
		printf("socket failed with error: %d\n", WSAGetLastError());
		WSACleanup();
        return 1;
	}

    // change the socket into non-blocking state
    u_long iMode = 0;
    if (ioctlsocket(sockFd, FIONBIO, &iMode) == SOCKET_ERROR)
    {
        printf("ioctlsocket failed with error: %d\n", WSAGetLastError());
        closesocket(sockFd);
        WSACleanup();
        return 1;
    }

	// Connect to server.
    while (true)
    {
        iResult = connect(sockFd, (sockaddr*)&servAddr, sizeof(servAddr));
        if (iResult == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK)
        {
            printf("connect failed with error: %d\n", WSAGetLastError());
            closesocket(sockFd);
            WSACleanup();
            return 1;
        }
        else
        {
            break;
        }
    }
    
    // send hello message with task request
    const char* helloMsg = "HELLO";
    iResult = send(sockFd, helloMsg, strlen(helloMsg) + 1, 0);
    if (iResult == SOCKET_ERROR)
    {
        printf("send failed with error: %d\n", WSAGetLastError());
        closesocket(sockFd);
        WSACleanup();
        return 1;
    }

    // Receive until the peer closes the connection or task finishes
    const int recvBufLen = 128;
    char recvBuf[recvBufLen];
    while (true)
    {
        iResult = recv(sockFd, recvBuf, recvBufLen, 0);
        if (iResult > 0)
        {
            double startTime = clock();

            TaskMsg* msg = (TaskMsg*)recvBuf;
            bool failed;
            double result = ProcessMsg(msg, failed);
            if (failed)
            {
                break;
            }

            double elapsedTime = (double)(clock() - startTime) / CLOCKS_PER_SEC;

            ResultMsg resultMsg;
            resultMsg.result = result;
            resultMsg.elapsedTime = elapsedTime;

            // send result
            if (send(sockFd, (const char*)&resultMsg, sizeof(resultMsg), 0) != sizeof(resultMsg))
            {
#ifdef _DEBUG
                printf("send failed with: %d\n", WSAGetLastError());
#endif
                break;
            }
        }
        else if (iResult == 0)
        {
            // disconnected
            break;
        }
        else if (WSAGetLastError() != WSAEWOULDBLOCK)
        {
#ifdef _DEBUG
            printf("recv failed with: %d\n", WSAGetLastError());
#endif
            break;
        }
    }

    // cleanup
    closesocket(sockFd);
    WSACleanup();

    return 0;
}

int Worker_Run()
{
    return Worker_RunClient();
}
