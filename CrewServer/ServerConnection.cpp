#include "ServerConnection.h"
#include "../Constants.h"

ServerConnection::ServerConnection()
{
    listenSocket = INVALID_SOCKET;
    clientSocket = INVALID_SOCKET;
}

bool ServerConnection::Initialize()
{
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);

    if (result != 0)
    {
        cout << "WSAStartup failed.\n";
        return false;
    }

    listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (listenSocket == INVALID_SOCKET)
    {
        cout << "Socket creation failed.\n";
        WSACleanup();
        return false;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr);

    result = bind(listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
    if (result == SOCKET_ERROR)
    {
        cout << "Bind failed.\n";
        closesocket(listenSocket);
        WSACleanup();
        return false;
    }

    return true;
}

bool ServerConnection::StartListening()
{
    if (listen(listenSocket, 1) == SOCKET_ERROR)
    {
        cout << "Listen failed.\n";
        return false;
    }

    cout << "Server waiting for client on port " << SERVER_PORT << "...\n";

    clientSocket = accept(listenSocket, nullptr, nullptr);

    if (clientSocket == INVALID_SOCKET)
    {
        cout << "Accept failed.\n";
        return false;
    }

    cout << "Client connected successfully.\n";
    Logger::Log("server_log.txt", "INFO", "Client connected");
    return true;
}

bool ServerConnection::SendData(const char* data, int size)
{
    int totalSent = 0;

    while (totalSent < size)
    {
        int bytesSent = send(clientSocket, data + totalSent, size - totalSent, 0);
        if (bytesSent == SOCKET_ERROR || bytesSent == 0)
        {
            return false;
        }
        totalSent += bytesSent;
    }

    return true;
}

bool ServerConnection::ReceiveData(char* buffer, int size)
{
    int totalReceived = 0;

    while (totalReceived < size)
    {
        int bytesReceived = recv(clientSocket, buffer + totalReceived, size - totalReceived, 0);
        if (bytesReceived == SOCKET_ERROR || bytesReceived == 0)
        {
            return false;
        }
        totalReceived += bytesReceived;
    }

    return true;
}

SOCKET ServerConnection::GetClientSocket() const
{
    return clientSocket;
}

void ServerConnection::Close()
{
    if (clientSocket != INVALID_SOCKET)
    {
        closesocket(clientSocket);
    }

    if (listenSocket != INVALID_SOCKET)
    {
        closesocket(listenSocket);
    }

    WSACleanup();
}
