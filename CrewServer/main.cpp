#include "ServerConnection.h"
#include "Packets.h"
#include "StateMachine.h"

int main()
{
    ServerConnection server;
    StateMachine stateMachine;

    if (!server.Initialize())
    {
        cout << "Server initialization failed.\n";
        return 1;
    }

    if (!server.StartListening())
    {
        cout << "Server failed to accept client.\n";
        server.Close();
        return 1;
    }

    stateMachine.SetState(STATE_CONNECTED);

    ConnectRequestPacket connectRequest{};
    if (!server.ReceiveData(reinterpret_cast<char*>(&connectRequest), sizeof(connectRequest)))
    {
        cout << "Failed to receive connection request.\n";
        server.Close();
        return 1;
    }

    Logger::Log("server_log.txt", "RX", "Received CONNECT_REQUEST");

    ConnectResponsePacket connectResponse{};
    connectResponse.header.packetType = CONNECT_RESPONSE;
    connectResponse.header.dataSize = sizeof(connectResponse);

    if (connectRequest.header.packetType == CONNECT_REQUEST)
    {
        stateMachine.SetState(STATE_AUTHENTICATED);
        connectResponse.statusCode = STATUS_OK;
        strcpy_s(connectResponse.message, "Connection verified");
        cout << "Client verified successfully.\n";
    }
    else
    {
        stateMachine.SetState(STATE_ERROR);
        connectResponse.statusCode = STATUS_INVALID;
        strcpy_s(connectResponse.message, "Invalid connection request");
        cout << "Invalid connection request received.\n";
    }

    if (!server.SendData(reinterpret_cast<char*>(&connectResponse), sizeof(connectResponse)))
    {
        cout << "Failed to send connection response.\n";
        server.Close();
        return 1;
    }

    Logger::Log("server_log.txt", "TX", "Sent CONNECT_RESPONSE");

    cout << "Current server state: " << stateMachine.GetCurrentState() << endl;

    ScheduleRequestPacket scheduleRequest{};
    if (!server.ReceiveData(reinterpret_cast<char*>(&scheduleRequest), sizeof(scheduleRequest)))
    {
        cout << "Failed to receive schedule request.\n";
        server.Close();
        return 1;
    }

    Logger::Log("server_log.txt", "RX", "Received GET_SCHEDULE_REQUEST");

    if (scheduleRequest.header.packetType == GET_SCHEDULE_REQUEST)
    {
        stateMachine.SetState(STATE_PROCESSING_REQUEST);

        ScheduleResponsePacket scheduleResponse{};
        scheduleResponse.header.packetType = GET_SCHEDULE_RESPONSE;
        scheduleResponse.header.dataSize = sizeof(scheduleResponse);
        scheduleResponse.statusCode = STATUS_OK;
        scheduleResponse.pilotId = scheduleRequest.pilotId;
        strcpy_s(scheduleResponse.pilotName, "Captain Smith");
        scheduleResponse.flightCount = 2;

        FlightInfo flights[2]{};

        flights[0].flightId = 101;
        strcpy_s(flights[0].origin, "Toronto");
        strcpy_s(flights[0].destination, "New York");
        strcpy_s(flights[0].date, "2026-03-30");

        flights[1].flightId = 205;
        strcpy_s(flights[1].origin, "Montreal");
        strcpy_s(flights[1].destination, "Chicago");
        strcpy_s(flights[1].date, "2026-03-31");

        stateMachine.SetState(STATE_SENDING_SCHEDULE);

        if (!server.SendData(reinterpret_cast<char*>(&scheduleResponse), sizeof(scheduleResponse)))
        {
            cout << "Failed to send schedule response.\n";
            server.Close();
            return 1;
        }

        Logger::Log("server_log.txt", "TX", "Sent GET_SCHEDULE_RESPONSE");

        for (int i = 0; i < scheduleResponse.flightCount; i++)
        {
            if (!server.SendData(reinterpret_cast<char*>(&flights[i]), sizeof(FlightInfo)))
            {
                cout << "Failed to send flight info.\n";
                server.Close();
                return 1;
            }

            Logger::Log("server_log.txt", "TX", "Sent FLIGHT_INFO");
        }

        stateMachine.SetState(STATE_AUTHENTICATED);
        cout << "Schedule sent successfully.\n";
    }
    else
    {
        cout << "Invalid schedule request received.\n";
        stateMachine.SetState(STATE_ERROR);
    }

    cout << "Current server state: " << stateMachine.GetCurrentState() << endl;

    system("pause");
    server.Close();
    return 0;
}