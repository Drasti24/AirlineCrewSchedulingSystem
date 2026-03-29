#pragma once

#include "Common.h"
#include "Logger.h"

class ClientConnection
{
private:
    SOCKET clientSocket;

public:
    ClientConnection();

    bool Initialize();
    bool ConnectToServer();

    bool SendData(const char* data, int size);
    bool ReceiveData(char* buffer, int size);

    SOCKET GetSocket() const;
    void Close();
};