//Group - 7 Drasti Patel , Komalpreet kaur , Jiya Pandit  
#pragma once

#include "../Common.h"
#include "../Logger.h"

class ServerConnection
{
private:
    SOCKET listenSocket;
    SOCKET clientSocket;

public:
    ServerConnection();

    bool Initialize();
    bool StartListening();

    bool SendData(const char* data, int size);
    bool ReceiveData(char* buffer, int size);

    SOCKET GetClientSocket() const;
    void Close();
};