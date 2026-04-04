//Group - 7 Drasti Patel , Komalpreet kaur , Jiya Pandit  
#include "ServerConnection.h"
#include "../Packets.h"
#include "../StateMachine.h"
#include "../Logger.h"
#include "ScheduleRepository.h"
#include "ReportService.h"

int main()
{
    ServerConnection server;
    StateMachine stateMachine;
    ScheduleRepository repository;
    repository.LoadSampleData();

    ReportService reportService;
    reportService.GenerateReport(repository, "monthly_report.txt");

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

    if (connectRequest.header.packetType == CONNECT_REQUEST &&
        connectRequest.header.dataSize == sizeof(ConnectRequestPacket))
    {
        stateMachine.SetState(STATE_AUTHENTICATED);
        connectResponse.statusCode = STATUS_OK;
        strcpy_s(connectResponse.message, sizeof(connectResponse.message), "Connection verified");
        cout << "Client verified successfully.\n";
    }
    else
    {
        stateMachine.SetState(STATE_ERROR);
        connectResponse.statusCode = STATUS_INVALID;
        strcpy_s(connectResponse.message, sizeof(connectResponse.message), "Invalid connection request");
        cout << "Invalid connection request received.\n";
    }

    if (!server.SendData(reinterpret_cast<const char*>(&connectResponse), sizeof(connectResponse)))
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
        PacketHeader header{};

        bool received = server.ReceiveData(reinterpret_cast<char*>(&header), sizeof(header));
        if (!received)
        {
            cout << "Client disconnected.\n";
            Logger::Log("server_log.txt", "INFO", "Client disconnected");
            break;
        }

        if (header.packetType == GET_SCHEDULE_REQUEST)
        {
            ScheduleRequestPacket scheduleRequest{};
            scheduleRequest.header = header;

            if (!server.ReceiveData(
                reinterpret_cast<char*>(&scheduleRequest) + sizeof(PacketHeader),
                sizeof(ScheduleRequestPacket) - sizeof(PacketHeader)))
            {
                cout << "Failed to receive full schedule request.\n";
                Logger::Log("server_log.txt", "ERROR", "Failed to receive full schedule request");
                break;
            }

            Logger::Log(
                "server_log.txt",
                "RX",
                "GET_SCHEDULE_REQUEST PilotID=" + to_string(scheduleRequest.pilotId)
            );

            if (scheduleRequest.header.dataSize != sizeof(ScheduleRequestPacket))
            {
                cout << "Invalid schedule request received.\n";
                Logger::Log("server_log.txt", "ERROR", "Invalid schedule request packet size");
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
                strcpy_s(scheduleResponse.pilotName, sizeof(scheduleResponse.pilotName), pilotSchedule.pilotName);
                scheduleResponse.flightCount = static_cast<int>(pilotSchedule.flights.size());

                if (!server.SendData(reinterpret_cast<const char*>(&scheduleResponse), sizeof(scheduleResponse)))
                {
                    cout << "Failed to send schedule response header.\n";
                    Logger::Log("server_log.txt", "ERROR", "Failed to send schedule response");
                    break;
                }

                Logger::Log(
                    "server_log.txt",
                    "TX",
                    "GET_SCHEDULE_RESPONSE PilotID=" + to_string(scheduleRequest.pilotId) +
                    " Status=OK FlightCount=" + to_string(scheduleResponse.flightCount)
                );

                stateMachine.SetState(STATE_SENDING_SCHEDULE);

                for (const auto& flight : pilotSchedule.flights)
                {
                    if (!server.SendData(reinterpret_cast<const char*>(&flight), sizeof(flight)))
                    {
                        cout << "Failed to send flight information.\n";
                        Logger::Log("server_log.txt", "ERROR", "Failed to send flight info");
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
                strcpy_s(scheduleResponse.pilotName, sizeof(scheduleResponse.pilotName), "Unknown");
                scheduleResponse.flightCount = 0;

                if (!server.SendData(reinterpret_cast<const char*>(&scheduleResponse), sizeof(scheduleResponse)))
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
        }
        else if (header.packetType == ASSIGN_FLIGHT_REQUEST)
        {
            AssignFlightPacket assignPacket{};
            assignPacket.header = header;

            if (!server.ReceiveData(
                reinterpret_cast<char*>(&assignPacket) + sizeof(PacketHeader),
                sizeof(AssignFlightPacket) - sizeof(PacketHeader)))
            {
                cout << "Failed to receive assign flight packet.\n";
                Logger::Log("server_log.txt", "ERROR", "Failed to receive assign flight packet");
                break;
            }

            Logger::Log(
                "server_log.txt",
                "RX",
                "ASSIGN_FLIGHT_REQUEST PilotID=" + to_string(assignPacket.pilotId) +
                " FlightID=" + to_string(assignPacket.flight.flightId)
            );

            if (assignPacket.header.dataSize != sizeof(AssignFlightPacket))
            {
                cout << "Invalid assign flight packet received.\n";
                Logger::Log("server_log.txt", "ERROR", "Invalid assign flight packet size");
                stateMachine.SetState(STATE_ERROR);
                break;
            }

            stateMachine.SetState(STATE_PROCESSING_REQUEST);

            bool success = repository.AssignFlight(assignPacket.pilotId, assignPacket.flight);

            OperationResponsePacket response{};
            response.header.packetType = OPERATION_RESPONSE;
            response.header.dataSize = sizeof(response);

            if (success)
            {
                response.statusCode = STATUS_OK;
                strcpy_s(response.message, sizeof(response.message), "Flight assigned successfully");
                cout << "Flight assigned successfully to Pilot ID " << assignPacket.pilotId << ".\n";
            }
            else
            {
                response.statusCode = STATUS_NOT_FOUND;
                strcpy_s(response.message, sizeof(response.message), "Pilot not found");
                cout << "Pilot ID " << assignPacket.pilotId << " not found for assignment.\n";
            }

            if (!server.SendData(reinterpret_cast<const char*>(&response), sizeof(response)))
            {
                cout << "Failed to send assign flight response.\n";
                Logger::Log("server_log.txt", "ERROR", "Failed to send assign flight response");
                break;
            }

            Logger::Log(
                "server_log.txt",
                "TX",
                "OPERATION_RESPONSE Status=" + string(response.message)
            );

            stateMachine.SetState(STATE_AUTHENTICATED);
        }
        else if (header.packetType == REMOVE_FLIGHT_REQUEST)
        {
            RemoveFlightPacket removePacket{};
            removePacket.header = header;

            if (!server.ReceiveData(
                reinterpret_cast<char*>(&removePacket) + sizeof(PacketHeader),
                sizeof(RemoveFlightPacket) - sizeof(PacketHeader)))
            {
                cout << "Failed to receive remove flight packet.\n";
                Logger::Log("server_log.txt", "ERROR", "Failed to receive remove flight packet");
                break;
            }

            Logger::Log(
                "server_log.txt",
                "RX",
                "REMOVE_FLIGHT_REQUEST PilotID=" + to_string(removePacket.pilotId) +
                " FlightID=" + to_string(removePacket.flightId)
            );

            if (removePacket.header.dataSize != sizeof(RemoveFlightPacket))
            {
                cout << "Invalid remove flight packet received.\n";
                Logger::Log("server_log.txt", "ERROR", "Invalid remove flight packet size");
                stateMachine.SetState(STATE_ERROR);
                break;
            }

            stateMachine.SetState(STATE_PROCESSING_REQUEST);

            OperationResponsePacket response{};
            response.header.packetType = OPERATION_RESPONSE;
            response.header.dataSize = sizeof(response);

            PilotSchedule pilotSchedule{};
            bool pilotExists = repository.GetScheduleByPilotId(removePacket.pilotId, pilotSchedule);

            if (!pilotExists)
            {
                response.statusCode = STATUS_NOT_FOUND;
                strcpy_s(response.message, sizeof(response.message), "Pilot not found");
                cout << "Pilot ID " << removePacket.pilotId << " not found for removal.\n";
            }
            else
            {
                bool removed = repository.RemoveFlight(removePacket.pilotId, removePacket.flightId);

                if (removed)
                {
                    response.statusCode = STATUS_OK;
                    strcpy_s(response.message, sizeof(response.message), "Flight removed successfully");
                    cout << "Flight ID " << removePacket.flightId
                        << " removed successfully from Pilot ID " << removePacket.pilotId << ".\n";
                }
                else
                {
                    response.statusCode = STATUS_NOT_FOUND;
                    strcpy_s(response.message, sizeof(response.message), "Flight not found");
                    cout << "Flight ID " << removePacket.flightId
                        << " not found for Pilot ID " << removePacket.pilotId << ".\n";
                }
            }

            if (!server.SendData(reinterpret_cast<const char*>(&response), sizeof(response)))
            {
                cout << "Failed to send remove flight response.\n";
                Logger::Log("server_log.txt", "ERROR", "Failed to send remove flight response");
                break;
            }

            Logger::Log(
                "server_log.txt",
                "TX",
                "OPERATION_RESPONSE Status=" + string(response.message)
        );

        stateMachine.SetState(STATE_AUTHENTICATED);
        }
        else
        {
            cout << "Invalid packet received.\n";
            Logger::Log("server_log.txt", "ERROR", "Unknown packet type received");
            stateMachine.SetState(STATE_ERROR);
            break;
        }

        cout << "Current server state: " << stateMachine.GetCurrentState() << endl;
    }

    server.Close();
    return 0;
}