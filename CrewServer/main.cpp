#include "ServerConnection.h"
#include "Packets.h"
#include "StateMachine.h"
#include "ScheduleRepository.h"

int main()
{
    ServerConnection server;
    StateMachine stateMachine;
    ScheduleRepository repository;

    repository.LoadSampleData();

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

    Logger::Log(
        "server_log.txt",
        "RX",
        "CONNECT_REQUEST ClientName=" + string(connectRequest.clientName)
    );

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

    string connectStatus = (connectResponse.statusCode == STATUS_OK) ? "OK" : "INVALID";

    Logger::Log(
        "server_log.txt",
        "TX",
        "CONNECT_RESPONSE Status=" + connectStatus +
        " Message=" + string(connectResponse.message)
    );

    cout << "Current server state: " << stateMachine.GetCurrentState() << endl;


    while (true)
    {
        ScheduleRequestPacket scheduleRequest{};

        bool received = server.ReceiveData(reinterpret_cast<char*>(&scheduleRequest), sizeof(scheduleRequest));
        if (!received)
        {
            cout << "Client disconnected.\n";
            Logger::Log("server_log.txt", "INFO", "Client disconnected");
            break;
        }

        Logger::Log(
            "server_log.txt",
            "RX",
            "GET_SCHEDULE_REQUEST PilotID=" + to_string(scheduleRequest.pilotId)
        );

        if (scheduleRequest.header.packetType != GET_SCHEDULE_REQUEST)
        {
            cout << "Invalid schedule request received.\n";
            Logger::Log("server_log.txt", "ERROR", "Invalid packet received from client");
            stateMachine.SetState(STATE_ERROR);
            break;
        }

        stateMachine.SetState(STATE_PROCESSING_REQUEST);

        ScheduleResponsePacket scheduleResponse{};
        scheduleResponse.header.packetType = GET_SCHEDULE_RESPONSE;
        scheduleResponse.header.dataSize = sizeof(scheduleResponse);
        scheduleResponse.pilotId = scheduleRequest.pilotId;

        PilotSchedule pilotSchedule{};
        bool found = repository.GetScheduleByPilotId(scheduleRequest.pilotId, pilotSchedule);

        if (found)
        {
            scheduleResponse.statusCode = STATUS_OK;
            strcpy_s(scheduleResponse.pilotName, pilotSchedule.pilotName);
            scheduleResponse.flightCount = static_cast<int>(pilotSchedule.flights.size());

            stateMachine.SetState(STATE_SENDING_SCHEDULE);

            if (!server.SendData(reinterpret_cast<char*>(&scheduleResponse), sizeof(scheduleResponse)))
            {
                cout << "Failed to send schedule response.\n";
                Logger::Log("server_log.txt", "ERROR", "Failed to send GET_SCHEDULE_RESPONSE");
                break;
            }

            Logger::Log(
                "server_log.txt",
                "TX",
                "GET_SCHEDULE_RESPONSE PilotID=" + to_string(scheduleResponse.pilotId) +
                " Status=OK FlightCount=" + to_string(scheduleResponse.flightCount)
            );

            for (const auto& flight : pilotSchedule.flights)
            {
                if (!server.SendData(reinterpret_cast<const char*>(&flight), sizeof(FlightInfo)))
                {
                    cout << "Failed to send flight info.\n";
                    Logger::Log("server_log.txt", "ERROR", "Failed to send FLIGHT_INFO");
                    break;
                }

                Logger::Log(
                    "server_log.txt",
                    "TX",
                    "FLIGHT_INFO FlightID=" + to_string(flight.flightId) +
                    " Origin=" + string(flight.origin) +
                    " Destination=" + string(flight.destination) +
                    " Date=" + string(flight.date)
                );
            }

            cout << "Schedule sent successfully for Pilot ID " << scheduleRequest.pilotId << ".\n";
            stateMachine.SetState(STATE_AUTHENTICATED);
        }
        else
        {
            scheduleResponse.statusCode = STATUS_NOT_FOUND;
            strcpy_s(scheduleResponse.pilotName, "Unknown");
            scheduleResponse.flightCount = 0;

            if (!server.SendData(reinterpret_cast<char*>(&scheduleResponse), sizeof(scheduleResponse)))
            {
                cout << "Failed to send not found response.\n";
                Logger::Log("server_log.txt", "ERROR", "Failed to send not found response");
                break;
            }

            Logger::Log(
                "server_log.txt",
                "TX",
                "GET_SCHEDULE_RESPONSE PilotID=" + to_string(scheduleRequest.pilotId) +
                " Status=NOT_FOUND FlightCount=0"
            );

            cout << "Pilot ID " << scheduleRequest.pilotId << " not found.\n";
            stateMachine.SetState(STATE_AUTHENTICATED);
        }

        cout << "Current server state: " << stateMachine.GetCurrentState() << endl;
    }

    system("pause");
    server.Close();
    return 0;
}