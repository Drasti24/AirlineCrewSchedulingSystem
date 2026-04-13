//Group - 7 Drasti Patel , Komalpreet kaur , Jiya Pandit
#include "ServerConnection.h"
#include "../Packets.h"
#include "../StateMachine.h"
#include "../Logger.h"
#include "ScheduleRepository.h"
#include "ReportService.h"
#include <fstream>
#include <iostream>

using namespace std;

string GetStatusText(int statusCode)
{
    if (statusCode == STATUS_OK)
        return "OK";
    else if (statusCode == STATUS_NOT_FOUND)
        return "NOT_FOUND";
    else if (statusCode == STATUS_INVALID)
        return "INVALID";
    else
        return "FAILED";
}

int main()
{
    ServerConnection server;
    StateMachine stateMachine;
    ScheduleRepository repository;
    ReportService reportService;

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
    cout << "Current server state: "
        << ServerStateToString(stateMachine.GetCurrentState()) << endl;

    ConnectRequestPacket connectRequest{};
    if (!server.ReceiveData(reinterpret_cast<char*>(&connectRequest), sizeof(connectRequest)))
    {
        cout << "Failed to receive connection request.\n";
        stateMachine.SetState(STATE_ERROR);
        cout << "Current server state: "
            << ServerStateToString(stateMachine.GetCurrentState()) << endl;
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
        stateMachine.SetState(STATE_ERROR);
        cout << "Current server state: "
            << ServerStateToString(stateMachine.GetCurrentState()) << endl;
        server.Close();
        return 1;
    }

    Logger::Log(
        "server_log.txt",
        "TX",
        "CONNECT_RESPONSE Status=" + GetStatusText(connectResponse.statusCode) +
        " Message=" + string(connectResponse.message)
    );

    cout << "Current server state: "
        << ServerStateToString(stateMachine.GetCurrentState()) << endl;

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
                stateMachine.SetState(STATE_ERROR);
                cout << "Current server state: "
                    << ServerStateToString(stateMachine.GetCurrentState()) << endl;
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
                cout << "Current server state: "
                    << ServerStateToString(stateMachine.GetCurrentState()) << endl;
                break;
            }

            stateMachine.SetState(STATE_PROCESSING_REQUEST);
            cout << "Current server state: "
                << ServerStateToString(stateMachine.GetCurrentState()) << endl;

            ScheduleResponsePacket scheduleResponse{};
            scheduleResponse.header.packetType = GET_SCHEDULE_RESPONSE;
            scheduleResponse.header.dataSize = sizeof(scheduleResponse);
            scheduleResponse.pilotId = scheduleRequest.pilotId;

            PilotSchedule pilotSchedule{};
            bool found = repository.GetScheduleByPilotId(scheduleRequest.pilotId, pilotSchedule);

            ScheduleDataPacket schedulePacket{};
            schedulePacket.header.packetType = GET_SCHEDULE_RESPONSE;
            schedulePacket.header.dataSize = 0; // temporary until serialized
            schedulePacket.pilotId = scheduleRequest.pilotId;

            if (found)
            {
                schedulePacket.statusCode = STATUS_OK;
                strcpy_s(schedulePacket.pilotName, sizeof(schedulePacket.pilotName), pilotSchedule.pilotName);
                schedulePacket.flights = pilotSchedule.flights;
            }
            else
            {
                schedulePacket.statusCode = STATUS_NOT_FOUND;
                strcpy_s(schedulePacket.pilotName, sizeof(schedulePacket.pilotName), "Unknown");
                schedulePacket.flights.clear();
            }

            // First serialize once to calculate real total size
            std::vector<char> serialized = SerializeScheduleDataPacket(schedulePacket);

            // Now set correct size in header and serialize again
            schedulePacket.header.dataSize = static_cast<int>(serialized.size());
            serialized = SerializeScheduleDataPacket(schedulePacket);

            if (!server.SendData(serialized.data(), static_cast<int>(serialized.size())))
            {
                cout << "Failed to send schedule packet.\n";
                Logger::Log("server_log.txt", "ERROR", "Failed to send serialized schedule packet");
                stateMachine.SetState(STATE_ERROR);
                cout << "Current server state: "
                    << ServerStateToString(stateMachine.GetCurrentState()) << endl;
                break;
            }

            Logger::Log(
                "server_log.txt",
                "TX",
                "SCHEDULE_DATA_PACKET PilotID=" + to_string(scheduleRequest.pilotId) +
                " Status=" + GetStatusText(schedulePacket.statusCode) +
                " FlightCount=" + to_string(schedulePacket.flights.size())
            );

            for (const auto& flight : schedulePacket.flights)
            {
                Logger::Log(
                    "server_log.txt",
                    "TX",
                    "SCHEDULE_DATA_PACKET_FLIGHT FlightID=" + to_string(flight.flightId) +
                    " Origin=" + string(flight.origin) +
                    " Destination=" + string(flight.destination) +
                    " Date=" + string(flight.date)
                );
            }

            if (found)
            {
                stateMachine.SetState(STATE_SENDING_SCHEDULE);
                cout << "Current server state: "
                    << ServerStateToString(stateMachine.GetCurrentState()) << endl;

                cout << "Schedule sent successfully for Pilot ID " << scheduleRequest.pilotId << ".\n";
            }
            else
            {
                cout << "Pilot ID " << scheduleRequest.pilotId << " not found.\n";
            }

            stateMachine.SetState(STATE_AUTHENTICATED);

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
                stateMachine.SetState(STATE_ERROR);
                cout << "Current server state: "
                    << ServerStateToString(stateMachine.GetCurrentState()) << endl;
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
                cout << "Current server state: "
                    << ServerStateToString(stateMachine.GetCurrentState()) << endl;
                break;
            }

            stateMachine.SetState(STATE_PROCESSING_REQUEST);
            cout << "Current server state: "
                << ServerStateToString(stateMachine.GetCurrentState()) << endl;

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
                stateMachine.SetState(STATE_ERROR);
                cout << "Current server state: "
                    << ServerStateToString(stateMachine.GetCurrentState()) << endl;
                break;
            }

            Logger::Log(
                "server_log.txt",
                "TX",
                "OPERATION_RESPONSE Status=" + GetStatusText(response.statusCode) +
                " Message=" + string(response.message)
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
                stateMachine.SetState(STATE_ERROR);
                cout << "Current server state: "
                    << ServerStateToString(stateMachine.GetCurrentState()) << endl;
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
                cout << "Current server state: "
                    << ServerStateToString(stateMachine.GetCurrentState()) << endl;
                break;
            }

            stateMachine.SetState(STATE_PROCESSING_REQUEST);
            cout << "Current server state: "
                << ServerStateToString(stateMachine.GetCurrentState()) << endl;

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
                stateMachine.SetState(STATE_ERROR);
                cout << "Current server state: "
                    << ServerStateToString(stateMachine.GetCurrentState()) << endl;
                break;
            }

            Logger::Log(
                "server_log.txt",
                "TX",
                "OPERATION_RESPONSE Status=" + GetStatusText(response.statusCode) +
                " Message=" + string(response.message)
            );

            stateMachine.SetState(STATE_AUTHENTICATED);
        }
        else if (header.packetType == DOWNLOAD_REPORT_REQUEST)
        {
            DownloadReportRequestPacket reportRequest{};
            reportRequest.header = header;

            if (!server.ReceiveData(
                reinterpret_cast<char*>(&reportRequest) + sizeof(PacketHeader),
                sizeof(DownloadReportRequestPacket) - sizeof(PacketHeader)))
            {
                cout << "Failed to receive report request packet.\n";
                Logger::Log("server_log.txt", "ERROR", "Failed to receive report request packet");
                stateMachine.SetState(STATE_ERROR);
                cout << "Current server state: "
                    << ServerStateToString(stateMachine.GetCurrentState()) << endl;
                break;
            }

            Logger::Log(
                "server_log.txt",
                "RX",
                "DOWNLOAD_REPORT_REQUEST Month=" + string(reportRequest.month)
            );

            if (reportRequest.header.dataSize != sizeof(DownloadReportRequestPacket))
            {
                cout << "Invalid report request packet received.\n";
                Logger::Log("server_log.txt", "ERROR", "Invalid report request packet size");
                stateMachine.SetState(STATE_ERROR);
                cout << "Current server state: "
                    << ServerStateToString(stateMachine.GetCurrentState()) << endl;
                break;
            }

            stateMachine.SetState(STATE_PROCESSING_REQUEST);
            cout << "Current server state: "
                << ServerStateToString(stateMachine.GetCurrentState()) << endl;

            string reportFileName = "monthly_report.txt";

            if (!reportService.GenerateReport(repository, reportFileName))
            {
                FileInfoPacket fileInfo{};
                fileInfo.header.packetType = FILE_INFO_PACKET;
                fileInfo.header.dataSize = sizeof(fileInfo);
                fileInfo.statusCode = STATUS_FAILED;
                strcpy_s(fileInfo.fileName, sizeof(fileInfo.fileName), "monthly_report.txt");
                fileInfo.totalFileSize = 0;
                fileInfo.chunkSize = REPORT_CHUNK_SIZE;

                if (!server.SendData(reinterpret_cast<const char*>(&fileInfo), sizeof(fileInfo)))
                {
                    Logger::Log("server_log.txt", "ERROR", "Failed to send failed file info packet");
                    stateMachine.SetState(STATE_ERROR);
                    cout << "Current server state: "
                        << ServerStateToString(stateMachine.GetCurrentState()) << endl;
                    break;
                }

                Logger::Log("server_log.txt", "ERROR", "Report generation failed");
                stateMachine.SetState(STATE_AUTHENTICATED);
                cout << "Current server state: "
                    << ServerStateToString(stateMachine.GetCurrentState()) << endl;
                continue;
            }

            ifstream reportFile(reportFileName, ios::binary | ios::ate);
            if (!reportFile.is_open())
            {
                FileInfoPacket fileInfo{};
                fileInfo.header.packetType = FILE_INFO_PACKET;
                fileInfo.header.dataSize = sizeof(fileInfo);
                fileInfo.statusCode = STATUS_FAILED;
                strcpy_s(fileInfo.fileName, sizeof(fileInfo.fileName), "monthly_report.txt");
                fileInfo.totalFileSize = 0;
                fileInfo.chunkSize = REPORT_CHUNK_SIZE;

                if (!server.SendData(reinterpret_cast<const char*>(&fileInfo), sizeof(fileInfo)))
                {
                    Logger::Log("server_log.txt", "ERROR", "Failed to send failed file info packet");
                    stateMachine.SetState(STATE_ERROR);
                    cout << "Current server state: "
                        << ServerStateToString(stateMachine.GetCurrentState()) << endl;
                    break;
                }

                Logger::Log("server_log.txt", "ERROR", "Failed to open report file for sending");
                stateMachine.SetState(STATE_AUTHENTICATED);
                cout << "Current server state: "
                    << ServerStateToString(stateMachine.GetCurrentState()) << endl;
                continue;
            }

            int totalFileSize = static_cast<int>(reportFile.tellg());
            reportFile.seekg(0, ios::beg);

            FileInfoPacket fileInfo{};
            fileInfo.header.packetType = FILE_INFO_PACKET;
            fileInfo.header.dataSize = sizeof(fileInfo);
            fileInfo.statusCode = STATUS_OK;
            strcpy_s(fileInfo.fileName, sizeof(fileInfo.fileName), reportFileName.c_str());
            fileInfo.totalFileSize = totalFileSize;
            fileInfo.chunkSize = REPORT_CHUNK_SIZE;

            if (!server.SendData(reinterpret_cast<const char*>(&fileInfo), sizeof(fileInfo)))
            {
                cout << "Failed to send file info packet.\n";
                Logger::Log("server_log.txt", "ERROR", "Failed to send file info packet");
                reportFile.close();
                stateMachine.SetState(STATE_ERROR);
                cout << "Current server state: "
                    << ServerStateToString(stateMachine.GetCurrentState()) << endl;
                break;
            }

            Logger::Log(
                "server_log.txt",
                "TX",
                "FILE_INFO_PACKET FileName=" + string(fileInfo.fileName) +
                " Size=" + to_string(fileInfo.totalFileSize)
            );

            stateMachine.SetState(STATE_SENDING_REPORT);
            cout << "Current server state: "
                << ServerStateToString(stateMachine.GetCurrentState()) << endl;

            bool chunkSendFailed = false;

            while (!reportFile.eof())
            {
                FileChunkPacket chunkPacket{};
                chunkPacket.header.packetType = FILE_CHUNK_PACKET;
                chunkPacket.header.dataSize = sizeof(chunkPacket);

                reportFile.read(chunkPacket.data, REPORT_CHUNK_SIZE);
                streamsize bytesRead = reportFile.gcount();

                if (bytesRead <= 0)
                {
                    break;
                }

                chunkPacket.bytesInChunk = static_cast<int>(bytesRead);

                if (!server.SendData(reinterpret_cast<const char*>(&chunkPacket), sizeof(chunkPacket)))
                {
                    cout << "Failed to send file chunk.\n";
                    Logger::Log("server_log.txt", "ERROR", "Failed to send file chunk");
                    stateMachine.SetState(STATE_ERROR);
                    chunkSendFailed = true;
                    break;
                }

                Logger::Log(
                    "server_log.txt",
                    "TX",
                    "FILE_CHUNK_PACKET Bytes=" + to_string(chunkPacket.bytesInChunk)
                );
            }

            reportFile.close();

            if (chunkSendFailed)
            {
                break;
            }

            cout << "Report file sent successfully.\n";
            Logger::Log("server_log.txt", "INFO", "Report file sent successfully");
            stateMachine.SetState(STATE_AUTHENTICATED);
        }
        else if (header.packetType == UPDATE_FLIGHT_REQUEST)
        {
            UpdateFlightPacket updatePacket{};
            updatePacket.header = header;

            if (!server.ReceiveData(
                reinterpret_cast<char*>(&updatePacket) + sizeof(PacketHeader),
                sizeof(UpdateFlightPacket) - sizeof(PacketHeader)))
            {
                cout << "Failed to receive update flight packet.\n";
                Logger::Log("server_log.txt", "ERROR", "Failed to receive update flight packet");
                stateMachine.SetState(STATE_ERROR);
                cout << "Current server state: "
                    << ServerStateToString(stateMachine.GetCurrentState()) << endl;
                break;
            }

            Logger::Log(
                "server_log.txt",
                "RX",
                "UPDATE_FLIGHT_REQUEST PilotID=" + to_string(updatePacket.pilotId) +
                " FlightID=" + to_string(updatePacket.flight.flightId)
            );

            if (updatePacket.header.dataSize != sizeof(UpdateFlightPacket))
            {
                cout << "Invalid update flight packet received.\n";
                Logger::Log("server_log.txt", "ERROR", "Invalid update flight packet size");
                stateMachine.SetState(STATE_ERROR);
                cout << "Current server state: "
                    << ServerStateToString(stateMachine.GetCurrentState()) << endl;
                break;
            }

            stateMachine.SetState(STATE_PROCESSING_REQUEST);
            cout << "Current server state: "
                << ServerStateToString(stateMachine.GetCurrentState()) << endl;

            OperationResponsePacket response{};
            response.header.packetType = OPERATION_RESPONSE;
            response.header.dataSize = sizeof(response);

            PilotSchedule pilotSchedule{};
            bool pilotExists = repository.GetScheduleByPilotId(updatePacket.pilotId, pilotSchedule);

            if (!pilotExists)
            {
                response.statusCode = STATUS_NOT_FOUND;
                strcpy_s(response.message, sizeof(response.message), "Pilot not found");
                cout << "Pilot ID " << updatePacket.pilotId << " not found for update.\n";
            }
            else
            {
                bool updated = repository.UpdateFlight(updatePacket.pilotId, updatePacket.flight);

                if (updated)
                {
                    response.statusCode = STATUS_OK;
                    strcpy_s(response.message, sizeof(response.message), "Flight updated successfully");
                    cout << "Flight ID " << updatePacket.flight.flightId
                        << " updated successfully for Pilot ID " << updatePacket.pilotId << ".\n";
                }
                else
                {
                    response.statusCode = STATUS_NOT_FOUND;
                    strcpy_s(response.message, sizeof(response.message), "Flight not found");
                    cout << "Flight ID " << updatePacket.flight.flightId
                        << " not found for Pilot ID " << updatePacket.pilotId << ".\n";
                }
            }

            if (!server.SendData(reinterpret_cast<const char*>(&response), sizeof(response)))
            {
                cout << "Failed to send update flight response.\n";
                Logger::Log("server_log.txt", "ERROR", "Failed to send update flight response");
                stateMachine.SetState(STATE_ERROR);
                cout << "Current server state: "
                    << ServerStateToString(stateMachine.GetCurrentState()) << endl;
                break;
            }

            Logger::Log(
                "server_log.txt",
                "TX",
                "OPERATION_RESPONSE Status=" + GetStatusText(response.statusCode) +
                " Message=" + string(response.message)
            );

            stateMachine.SetState(STATE_AUTHENTICATED);
        }
        else
        {
            cout << "Invalid packet received.\n";
            Logger::Log("server_log.txt", "ERROR", "Unknown packet type received");
            stateMachine.SetState(STATE_ERROR);
            cout << "Current server state: "
                << ServerStateToString(stateMachine.GetCurrentState()) << endl;
            break;
        }

        cout << "Current server state: "
            << ServerStateToString(stateMachine.GetCurrentState()) << endl;
    }

    server.Close();
    return 0;
}