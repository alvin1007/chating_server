#define _CRT_SECURE_NO_WARNINGS
#pragma comment(lib, "ws2_32.lib")
#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <WinSock2.h>
#include <vector>
#include <thread>
#include <time.h>
#include <string>
#include <iomanip>
#include <sstream>
#include <mysql.h>
#include <openssl/sha.h>

#define BUFFERSIZE 1024;

struct clientInformation
{
    SOCKET hClntSock;
    std::thread::id id;
    char* name;
};

std::string sha256(const std::string str);
void ErrorHandling(const char* message);
void client(SOCKET hClntSock, SOCKADDR_IN clntAddr, std::vector<std::thread*>* clientlist, std::vector<clientInformation*>* clientInform, MYSQL* conn);

// ���� ��Ʈ�� 9090�̴�.

int main(int argc, char* argv[])
{
    /* ������ ���̽� ���� ------------------------*/
    MYSQL* conn;
    MYSQL* conn_result;
    unsigned int timeout_sec = 1;

    conn = mysql_init(NULL);
    mysql_options(conn, MYSQL_OPT_CONNECT_TIMEOUT, &timeout_sec);
    conn_result = mysql_real_connect(conn, "127.0.0.1", "root", "alvin1007", "chat", 3307, NULL, 0);
    if (conn_result == NULL)
        std::cout << "DB Connection Fail" << std::endl;
    else
        std::cout << "DB Connection Success" << std::endl;
    /*-------------------------------------------*/

    std::vector<std::thread*> clientlist;
    std::vector<clientInformation*> clientInform;

    WSADATA wsaData;
    SOCKET hServSock;
    SOCKADDR_IN servAddr;

    const char* message = "connect done";

    int szClntAddr;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) //���� ���̺귯�� �ʱ�ȭ
        ErrorHandling("WSAStartup() error!");

    hServSock = socket(PF_INET, SOCK_STREAM, 0); //���ϻ���

    if (hServSock == INVALID_SOCKET)
        ErrorHandling("socket() error");

    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(9090); // ���� ��Ʈ

    if (bind(hServSock, (SOCKADDR*)&servAddr, sizeof(servAddr)) == SOCKET_ERROR) //���Ͽ� IP�ּҿ� PORT ��ȣ �Ҵ�
        ErrorHandling("bind() error");

    if (listen(hServSock, 5) == SOCKET_ERROR) //listen �Լ�ȣ���� ���ؼ� ������ ������ ���� �������� �ϼ�
        ErrorHandling("listen() error");

    std::cout << "server start" << std::endl;

    /* -------------------------------------------------------- */

    while (1)
    {
        szClntAddr = sizeof(SOCKADDR_IN);
        SOCKADDR_IN clntAddr;
        SOCKET hClntSock = accept(hServSock, (SOCKADDR*)&clntAddr, &szClntAddr); //Ŭ���̾�Ʈ �����û �����ϱ� ���� accept�Լ� ȣ��
        clientlist.push_back(new std::thread(client, hClntSock, clntAddr, &clientlist, &clientInform, conn)); //������ ����
    }

    /* ------------------------------------------------------- */

    for (auto ptr = clientlist.begin(); ptr < clientlist.end(); ptr++)
    {
        (*ptr)->join();
    }

    closesocket(hServSock);
    WSACleanup(); //���α׷� ���� ���� �ʱ�ȭ�� ���� ���̺귯�� ����
    return 0;
}

void ErrorHandling(const char* message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

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

void echo(char* pa, int size)
{
    for (int i = size; i > 0; i--)
    {
        if (i == size)
        {
            *(pa + i + 1) = '\n';
            *(pa + i + 2) = '\0';
        }
        *(pa + i) = *(pa + i - 1);
    }
    *pa = '\n';
}

void sendAll(std::vector<char>* buffer, std::vector<std::thread*>* clientlist, std::vector<clientInformation*>* clientInform)
{
    char a[100];
    char* name = NULL;
    for (int i = 0; i < (int)(*buffer).size(); i++)
        a[i] = (*buffer)[i];
    echo(a, (int)strlen(a));
    for (auto ptr1 = clientlist->begin(); ptr1 < clientlist->end(); ptr1++)
    {
        // thread ���̵� ���� ���� ã�Ƽ�
        if ((*ptr1)->get_id() == std::this_thread::get_id())
        {
            for (auto ptr2 = clientInform->begin(); ptr2 < clientInform->end(); ptr2++)
            {
                if ((*ptr1)->get_id() == (*ptr2)->id)
                    name = (*ptr2)->name;
            }
        }
    }
    for (auto ptr1 = clientlist->begin(); ptr1 < clientlist->end(); ptr1++)
    {
        // thread ���̵� �ٸ� ���� ã�Ƽ�
        if ((*ptr1)->get_id() != std::this_thread::get_id())
        {
            for (auto ptr2 = clientInform->begin(); ptr2 < clientInform->end(); ptr2++)
            {
                if ((*ptr1)->get_id() == (*ptr2)->id)
                {
                    send((*ptr2)->hClntSock, "\n", 2, 0);
                    send((*ptr2)->hClntSock, name, (int)strlen(name), 0);
                    send((*ptr2)->hClntSock, a, (int)strlen(a) + 2, 0);
                    send((*ptr2)->hClntSock, "\n", 2, 0);
                }
            }
        }
    }
}

void client(SOCKET hClntSock, SOCKADDR_IN clntAddr, std::vector<std::thread*>* clientlist, std::vector<clientInformation*>* clientInform, MYSQL* conn)
{
    /* Ŭ���̾�Ʈ ���� ���*/
    clientInformation clientIn;
    clientIn.hClntSock = hClntSock;
    clientIn.id = std::this_thread::get_id();
    (*clientInform).push_back(&clientIn);
    /* -------------------*/

    std::vector<char> buffer;
    char x;

    /*�α��ΰ� ȸ������*/
    char id[100];
    char password[100];

    char sql[1024];

    send(hClntSock, "login : 1\nsign up : 0\n>", 24, 0); //send�Լ� ȣ���� ���ؼ� ����� Ŭ���̾�Ʈ�� �����͸� ����
    recv(hClntSock, &x, sizeof(char), 0);
    if (x == '1')
    {
        MYSQL_RES* result;
        MYSQL_ROW row;
        int fields;

        send(hClntSock, "id >", 5, 0);

        /* ���� ���� */
        recv(hClntSock, &x, sizeof(char), 0);
        recv(hClntSock, &x, sizeof(char), 0);
        recv(hClntSock, &x, sizeof(char), 0);
        /* --------- */

        while (1)
        {
            if (recv(hClntSock, &x, sizeof(char), 0) == SOCKET_ERROR)
            {
                ErrorHandling("recv() error");
                break;
            }
            if (buffer.size() > 0 && *(buffer.end() - 1) == '\r' && x == '\n')
            {
                for (int i = 0; i < (int)buffer.size(); i++)
                    id[i] = buffer[i];
                id[(int)buffer.size()] = '\0';
                buffer.clear();
                break;
            }
            buffer.push_back(x);
        }

        send(hClntSock, "password >", 11, 0);

        while (1)
        {
            if (recv(hClntSock, &x, sizeof(char), 0) == SOCKET_ERROR)
            {
                ErrorHandling("recv() error");
                break;
            }
            if (buffer.size() > 0 && *(buffer.end() - 1) == '\r' && x == '\n')
            {
                for (int i = 0; i < (int)buffer.size(); i++)
                    password[i] = buffer[i];
                password[(int)buffer.size()] = '\0';
                buffer.clear();
                break;
            }
            buffer.push_back(x);
        }

        strcpy(sql, "SELECT * FROM client");
        
        /*-----------�Էµ� ������ �ùٸ��� �Ǵ�--------------*/
        bool isRightId = false;
        bool isRightPassword = false;
        if (mysql_query(conn, sql) == 0) 
        {
            bool first = true;
            result = mysql_store_result(conn);
            fields = mysql_num_fields(result);
            while ((row = mysql_fetch_row(result)) != NULL) 
            {
                for (int i = 0; i < fields; i++)
                {
                    if (first && strcmp(row[i], id))
                        isRightId = true;
                    else if (!first && strcmp(row[i], password))
                        isRightPassword = true;
                    first = false;
                }
                first = true;
            }
            mysql_free_result(result);
        }
        else 
        {
            std::cout << "error";
            send(hClntSock, "error\n", 7, 0);
            return;
        }
        /*------------------------------------------------------*/
        password[0] = '\0';
        if (isRightId && isRightPassword)
        {
            clientIn.name = id;
            std::cout << id << "���� �����Ͽ����ϴ�." << std::endl;
            send(hClntSock, "Login Success\n", 15, 0);
        }
        else
        {
            send(hClntSock, "Login Fail\nSign up First", 25, 0);
            return;
        }
    }
    else if (x == '0')
    {
        send(hClntSock, "id >", 5, 0);

        /* ���� ���� */
        recv(hClntSock, &x, sizeof(char), 0);
        recv(hClntSock, &x, sizeof(char), 0);
        recv(hClntSock, &x, sizeof(char), 0);
        /* --------- */

        while (1)
        {
            if (recv(hClntSock, &x, sizeof(char), 0) == SOCKET_ERROR)
            {
                ErrorHandling("recv() error");
                break;
            }
            if (buffer.size() > 0 && *(buffer.end() - 1) == '\r' && x == '\n')
            {
                for (int i = 0; i < (int)buffer.size(); i++)
                    id[i] = buffer[i];
                id[(int)buffer.size()] = '\0';
                buffer.clear();
                break;
            }
            buffer.push_back(x);
        }

        send(hClntSock, "password >", 11, 0);

        while (1)
        {
            if (recv(hClntSock, &x, sizeof(char), 0) == SOCKET_ERROR)
            {
                ErrorHandling("recv() error");
                break;
            }
            if (buffer.size() > 0 && *(buffer.end() - 1) == '\r' && x == '\n')
            {
                for (int i = 0; i < (int)buffer.size(); i++)
                    password[i] = buffer[i];
                password[(int)buffer.size()] = '\0';
                buffer.clear();
                break;
            }
            buffer.push_back(x);
        }
        char hash_password[100];
        std::string string_hash_password = sha256(password);
        for (int i = 0; i < sha256(password).size(); i++)
            hash_password[i] = string_hash_password[i];
        hash_password[string_hash_password.size()] = '\0';
        sprintf(sql, "INSERT INTO client VALUES('%s', '%s');", id, hash_password);
        password[0] = '\0';
        hash_password[0] = '\0';
        string_hash_password = "";
        if (mysql_query(conn, sql) != 0)
        {
            std::cout << sql;
            send(hClntSock, "error\n", 7, 0);
            return;
        }
        else
        {
            clientIn.name = id;
            std::cout << id << "���� �����Ͽ����ϴ�." << std::endl;
            send(hClntSock, "Sign Up Success\n", 17, 0);
        }
    }

    /*--------------------------------------------------------------------*/

    while (1)
    {
        if (recv(hClntSock, &x, sizeof(char), 0) == SOCKET_ERROR)
        {
            ErrorHandling("recv() error");
            break;
        }
        // ���� ������ ���� �����̶��
        if (buffer.size() > 0 && *(buffer.end() - 1) == '\r' && x == '\n')
        {
            // ���� exit�� �ԷµǸ� ���� ��⸦ ����
            if (*buffer.begin() == 'e' && *(buffer.begin() + 1) == 'x' && *(buffer.begin() + 2) == 'i' && *(buffer.begin() + 3) == 't')
                break;
            sendAll(&buffer, clientlist, clientInform);
            buffer.clear();
            continue;
        }
        buffer.push_back(x);
    }

    /*-----------------------------------------------------------------------------------------*/

    std::cout << clientIn.name << "���� �����̽��ϴ�.";
    closesocket(hClntSock);
    for (auto ptr = clientlist->begin(); ptr < clientlist->end(); ptr++)
    {
        // thread ���̵� ���� ���� ã�Ƽ�
        if ((*ptr)->get_id() == std::this_thread::get_id())
        {
            // ����Ʈ���� ����.
            clientlist->erase(ptr);
            break;
        }
    }
    for (auto ptr = clientInform->begin(); ptr < clientInform->end(); ptr++)
    {
        if ((*ptr)->id == std::this_thread::get_id())
        {
            clientInform->erase(ptr);
            break;
        }
    }
}