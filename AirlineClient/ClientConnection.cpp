#include "ClientConnection.h"
#include "Constants.h"

ClientConnection::ClientConnection()
{
    clientSocket = INVALID_SOCKET;
}

bool ClientConnection::Initialize()
{
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);

    if (result != 0)
    {
        cout << "WSAStartup failed.\n";
        return false;
    }

    clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (clientSocket == INVALID_SOCKET)
    {
        cout << "Socket creation failed.\n";
        WSACleanup();
        return false;
    }

    return true;
}

bool ClientConnection::ConnectToServer()
{
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr);

    int result = connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));

    if (result == SOCKET_ERROR)
    {
        cout << "Connection to server failed.\n";
        closesocket(clientSocket);
        WSACleanup();
        return false;
    }

    cout << "Connected to server successfully.\n";
    Logger::Log("client_log.txt", "INFO", "Connected to server");
    return true;
}

bool ClientConnection::SendData(const char* data, int size)
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

bool ClientConnection::ReceiveData(char* buffer, int size)
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

SOCKET ClientConnection::GetSocket() const
{
    return clientSocket;
}

void ClientConnection::Close()
{
    if (clientSocket != INVALID_SOCKET)
    {
        closesocket(clientSocket);
    }

    WSACleanup();
}