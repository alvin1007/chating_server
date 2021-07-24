#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment(lib, "ws2_32.lib")
#include <stdio.h>
#include <stdlib.h>
#include <WinSock2.h>
#include <vector>
#include <iostream>
#include <thread>
#include <string>
#include <conio.h>
#include <iomanip>
#include <sstream>
#include <openssl/sha.h>

#define BUFFERSIZE 1024

struct ClientInform
{
    std::string id;
    std::string password;
};

bool isRogin = false;
bool isSignUp = false;
bool end;

void ErrorHandling(const char* message);

std::string sha256(const std::string str)
{
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, str.c_str(), str.size());
    SHA256_Final(hash, &sha256);
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
    {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

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

void cinThread(SOCKET hSocket, ClientInform clientInformation)
{
    char id[100], password[100];
    for (int i = 0; i < (int)clientInformation.id.size(); i++)
        id[i] = clientInformation.id[i];
    for (int i = 0; i < (int)clientInformation.password.size(); i++)
        password[i] = clientInformation.password[i];
    id[clientInformation.id.size()] = '\0';
    password[clientInformation.password.size()] = '\0';
    end = false;
    while (1)
    {
        if (isRogin || isSignUp)
        {
            if (isRogin)
                send(hSocket, "1", 2, 0);
            else
                send(hSocket, "0", 2, 0);
            send(hSocket, id, (int)strlen(id) + 1, 0);
            send(hSocket, password, (int)strlen(password) + 1, 0);
            isRogin = false;
            isSignUp = false;
        }
        else 
        {
            char input[BUFFERSIZE];
            std::cin >> input;
            int size = (int)strlen(input);
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
}

int main(int argc, char* argv[])
{
    char ch;
    int x;
    std::string id, password;
    ClientInform clientInformation;
    std::vector<std::thread*> threadlist;
    WSADATA wsaData;
    SOCKET hSocket;
    SOCKADDR_IN servAddr;

    if (argc != 3)
    {
        std::cout << "client.exe <IP> <PORT>";
        return 0;
    }

    std::cout << "login : 1\nsign up : 0\n>";
    std::cin >> x;
    if (x == 1)
    {
        std::cout << "id >";
        std::cin >> id;
        id[id.size()] = '\0';
        std::cout << "password >";
        while ((ch = _getch()) != 13)
        {
            password += ch;
            std::cout << '*';
        }
        password[password.size()] = '\0';
        std::cout << std::endl;
        isRogin = true;
    }
    else if (x == 0)
    {
        std::cout << "id >";
        std::cin >> id;
        id[id.size()] = '\0';
        std::cout << "password >";
        while ((ch = _getch()) != 13)
        {
            password += ch;
            std::cout << '*';
        }
        password[password.size()] = '\0';
        std::cout << std::endl;
        isSignUp = true;
    }
    else
    {
        std::cout << "error";
        return 0;
    }

    clientInformation.id = id;
    clientInformation.password = sha256(password);

    password.clear();

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
    threadlist.push_back(new std::thread(cinThread, hSocket, clientInformation));

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