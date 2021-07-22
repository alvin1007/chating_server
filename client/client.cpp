#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment(lib, "ws2_32.lib")
#include <stdio.h>
#include <stdlib.h>
#include <WinSock2.h>
#include <vector>
#include <iostream>
#include <thread>

#define BUFFERSIZE 1024

bool end;

void ErrorHandling(const char* message);

void recvThread(SOCKET hSocket)
{
    std::vector<char> buffer;
    char x;

    while (1)
    {
        if (recv(hSocket, &x, sizeof(char), 0) == SOCKET_ERROR) //recv 함수 호출을 통해서 서버로부터 전송되는 데이터를 수신하고 있다.
        {
            ErrorHandling("recv() error");
            break;
        }
        buffer.push_back(x);
        if (x == '\0')
        {
            if (end)
                break;
            for (int i = 0; i < (int)buffer.size(); i++)
                std::cout << buffer[i];
            buffer.clear();
            continue;
        }
    }
}

void cinThread(SOCKET hSocket)
{
    end = false;
    while (1)
    {
        char input[BUFFERSIZE];
        std::cin >> input;
        int size = strlen(input);
        *(input + size + 1) = '\r';
        *(input + size + 2) = '\n';
        send(hSocket, input, size + 3, 0);
        if (*input == 'e' && *(input + 1) == 'x' && *(input + 2) == 'i' && *(input + 3) == 't')
        {
            end = true;
            break;
        }
    }
}

int main(int argc, char* argv[])
{
    std::vector<std::thread*> threadlist;
    WSADATA wsaData;
    SOCKET hSocket;
    SOCKADDR_IN servAddr;

    if (argc != 3)
    {
        printf("Usage:%s <IP> <port>\n", argv[0]);
        exit(1);
    }

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) //소켓 라이브러리를 초기화하고 있다
        ErrorHandling("WSAStartup() error!");

    hSocket = socket(PF_INET, SOCK_STREAM, 0); //소켓을 생성하고

    if (hSocket == INVALID_SOCKET)
        ErrorHandling("socket() error");

    memset(&servAddr, 0, sizeof(servAddr));

    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = inet_addr(argv[1]);
    servAddr.sin_port = htons(atoi(argv[2]));

    if (connect(hSocket, (SOCKADDR*)&servAddr, sizeof(servAddr)) == SOCKET_ERROR) //생성된 소켓을 바탕으로 서버에 연결요청을 하고 있다
        ErrorHandling("connect() error!");

    threadlist.push_back(new std::thread(recvThread, hSocket));
    threadlist.push_back(new std::thread(cinThread, hSocket));

    for (auto ptr = threadlist.begin(); ptr < threadlist.end(); ptr++)
    {
        (*ptr)->join();
    }

    closesocket(hSocket); //소켓 라이브러리 해제
    WSACleanup();
    return 0;
}

void ErrorHandling(const char* message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}