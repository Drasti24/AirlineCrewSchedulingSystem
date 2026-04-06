//Group - 7 Drasti Patel , Komalpreet kaur , Jiya Pandit
#include "ClientConnection.h"
#include "../Packets.h"
#include "../Logger.h"
#include <limits>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <cctype>

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

bool IsValidDateFormat(const char* date)
{
    if (strlen(date) != 10)
        return false;

    if (date[4] != '-' || date[7] != '-')
        return false;

    for (int i = 0; i < 10; i++)
    {
        if (i == 4 || i == 7)
            continue;

        if (!isdigit(static_cast<unsigned char>(date[i])))
            return false;
    }

    int month = (date[5] - '0') * 10 + (date[6] - '0');
    int day = (date[8] - '0') * 10 + (date[9] - '0');

    if (month < 1 || month > 12)
        return false;

    if (day < 1 || day > 31)
        return false;

    return true;
}

int main()
{
    ClientConnection client;

    if (!client.Initialize())
    {
        cout << "Client initialization failed.\n";
        return 1;
    }

    if (!client.ConnectToServer())
    {
        cout << "Unable to connect to server.\n";
        return 1;
    }

    ConnectRequestPacket connectRequest{};
    connectRequest.header.packetType = CONNECT_REQUEST;
    connectRequest.header.dataSize = sizeof(connectRequest);
    strcpy_s(connectRequest.clientName, sizeof(connectRequest.clientName), "AirlineClientApp");

    if (!client.SendData(reinterpret_cast<const char*>(&connectRequest), sizeof(connectRequest)))
    {
        cout << "Failed to send connect request.\n";
        client.Close();
        return 1;
    }

    Logger::Log(
        "client_log.txt",
        "TX",
        "CONNECT_REQUEST ClientName=" + string(connectRequest.clientName)
    );

    ConnectResponsePacket connectResponse{};
    if (!client.ReceiveData(reinterpret_cast<char*>(&connectResponse), sizeof(connectResponse)))
    {
        cout << "Failed to receive connect response.\n";
        client.Close();
        return 1;
    }

    Logger::Log(
        "client_log.txt",
        "RX",
        "CONNECT_RESPONSE Status=" + GetStatusText(connectResponse.statusCode) +
        " Message=" + string(connectResponse.message)
    );

    if (connectResponse.statusCode != STATUS_OK)
    {
        cout << "Connection verification failed: " << connectResponse.message << endl;
        client.Close();
        return 1;
    }

    cout << "Connected to server successfully.\n";
    cout << "Server response: " << connectResponse.message << endl;

    while (true)
    {
        int choice;

        cout << "\n===== Airline Crew System =====\n";
        cout << "1. Get Pilot Schedule\n";
        cout << "2. Assign Flight\n";
        cout << "3. Remove Flight\n";
        cout << "4. Download Monthly Report\n";
        cout << "5. Update Flight\n";
        cout << "0. Exit\n";
        cout << "Enter choice: ";

        if (!(cin >> choice))
        {
            cout << "Invalid input. Please enter a number.\n";
            cin.clear();
            cin.ignore((numeric_limits<streamsize>::max)(), '\n');
            continue;
        }

        if (choice == 0)
        {
            Logger::Log("client_log.txt", "INFO", "Client chose to exit");
            client.Close();
            return 0;
        }

        if (choice == 1)
        {
            int pilotId;

            while (true)
            {
                cout << "\nEnter Pilot ID to retrieve schedule (or 0 to cancel): ";

                if (!(cin >> pilotId))
                {
                    cout << "Invalid input. Please enter a numeric Pilot ID.\n";
                    cin.clear();
                    cin.ignore((numeric_limits<streamsize>::max)(), '\n');
                    continue;
                }

                if (pilotId == 0)
                {
                    break;
                }

                if (pilotId < 0)
                {
                    cout << "Pilot ID must be a positive number.\n";
                    continue;
                }

                ScheduleRequestPacket scheduleRequest{};
                scheduleRequest.header.packetType = GET_SCHEDULE_REQUEST;
                scheduleRequest.header.dataSize = sizeof(scheduleRequest);
                scheduleRequest.pilotId = pilotId;

                if (!client.SendData(reinterpret_cast<const char*>(&scheduleRequest), sizeof(scheduleRequest)))
                {
                    cout << "Failed to send schedule request.\n";
                    Logger::Log("client_log.txt", "ERROR", "Failed to send schedule request");
                    client.Close();
                    return 1;
                }

                Logger::Log(
                    "client_log.txt",
                    "TX",
                    "GET_SCHEDULE_REQUEST PilotID=" + to_string(pilotId)
                );

                ScheduleResponsePacket scheduleResponse{};
                if (!client.ReceiveData(reinterpret_cast<char*>(&scheduleResponse), sizeof(scheduleResponse)))
                {
                    cout << "Failed to receive schedule response.\n";
                    Logger::Log("client_log.txt", "ERROR", "Failed to receive schedule response");
                    client.Close();
                    return 1;
                }

                Logger::Log(
                    "client_log.txt",
                    "RX",
                    "GET_SCHEDULE_RESPONSE PilotID=" + to_string(scheduleResponse.pilotId) +
                    " Status=" + GetStatusText(scheduleResponse.statusCode) +
                    " FlightCount=" + to_string(scheduleResponse.flightCount)
                );

                if (scheduleResponse.statusCode == STATUS_NOT_FOUND)
                {
                    cout << "\nPilot with ID " << pilotId << " was not found.\n";
                    break;
                }

                if (scheduleResponse.statusCode != STATUS_OK)
                {
                    cout << "\nFailed to retrieve pilot schedule.\n";
                    client.Close();
                    return 1;
                }

                cout << "\nPilot ID: " << scheduleResponse.pilotId << endl;
                cout << "Pilot Name: " << scheduleResponse.pilotName << endl;
                cout << "Flights Assigned: " << scheduleResponse.flightCount << endl;

                if (scheduleResponse.flightCount == 0)
                {
                    cout << "No flights assigned.\n";
                }

                for (int i = 0; i < scheduleResponse.flightCount; i++)
                {
                    FlightInfo flight{};
                    if (!client.ReceiveData(reinterpret_cast<char*>(&flight), sizeof(flight)))
                    {
                        cout << "Failed to receive flight data.\n";
                        Logger::Log("client_log.txt", "ERROR", "Failed to receive flight data");
                        client.Close();
                        return 1;
                    }

                    Logger::Log(
                        "client_log.txt",
                        "RX",
                        "FLIGHT_INFO FlightID=" + to_string(flight.flightId) +
                        " Origin=" + string(flight.origin) +
                        " Destination=" + string(flight.destination) +
                        " Date=" + string(flight.date)
                    );

                    cout << "\nFlight " << (i + 1) << endl;
                    cout << "Flight ID: " << flight.flightId << endl;
                    cout << "Origin: " << flight.origin << endl;
                    cout << "Destination: " << flight.destination << endl;
                    cout << "Date: " << flight.date << endl;
                }

                cout << "\nYou can return to the menu now.\n";
                break;
            }
        }
        else if (choice == 2)
        {
            int pilotId;
            FlightInfo flight{};

            cout << "\nEnter Pilot ID: ";
            if (!(cin >> pilotId))
            {
                cout << "Invalid Pilot ID.\n";
                cin.clear();
                cin.ignore((numeric_limits<streamsize>::max)(), '\n');
                continue;
            }

            if (pilotId <= 0)
            {
                cout << "Pilot ID must be a positive number.\n";
                continue;
            }

            cout << "Enter Flight ID: ";
            if (!(cin >> flight.flightId))
            {
                cout << "Invalid Flight ID.\n";
                cin.clear();
                cin.ignore((numeric_limits<streamsize>::max)(), '\n');
                continue;
            }

            if (flight.flightId <= 0)
            {
                cout << "Flight ID must be a positive number.\n";
                continue;
            }

            cin.ignore((numeric_limits<streamsize>::max)(), '\n');

            cout << "Enter Origin: ";
            cin.getline(flight.origin, sizeof(flight.origin));

            cout << "Enter Destination: ";
            cin.getline(flight.destination, sizeof(flight.destination));

            cout << "Enter Date (YYYY-MM-DD): ";
            cin.getline(flight.date, sizeof(flight.date));

            if (!IsValidDateFormat(flight.date))
            {
                cout << "Invalid date format. Please use YYYY-MM-DD.\n";
                Logger::Log("client_log.txt", "ERROR", "Invalid date entered for assign flight");
                continue;
            }

            AssignFlightPacket assignPacket{};
            assignPacket.header.packetType = ASSIGN_FLIGHT_REQUEST;
            assignPacket.header.dataSize = sizeof(assignPacket);
            assignPacket.pilotId = pilotId;
            assignPacket.flight = flight;

            if (!client.SendData(reinterpret_cast<const char*>(&assignPacket), sizeof(assignPacket)))
            {
                cout << "Failed to send assign flight request.\n";
                Logger::Log("client_log.txt", "ERROR", "Failed to send assign flight request");
                client.Close();
                return 1;
            }

            Logger::Log(
                "client_log.txt",
                "TX",
                "ASSIGN_FLIGHT_REQUEST PilotID=" + to_string(pilotId) +
                " FlightID=" + to_string(flight.flightId)
            );

            OperationResponsePacket response{};
            if (!client.ReceiveData(reinterpret_cast<char*>(&response), sizeof(response)))
            {
                cout << "Failed to receive assign flight response.\n";
                Logger::Log("client_log.txt", "ERROR", "Failed to receive assign flight response");
                client.Close();
                return 1;
            }

            Logger::Log(
                "client_log.txt",
                "RX",
                "OPERATION_RESPONSE Status=" + GetStatusText(response.statusCode) +
                " Message=" + string(response.message)
            );

            cout << "\nServer response: " << response.message << endl;
        }
        else if (choice == 3)
        {
            int pilotId;
            int flightId;

            cout << "\nEnter Pilot ID: ";
            if (!(cin >> pilotId))
            {
                cout << "Invalid Pilot ID.\n";
                cin.clear();
                cin.ignore((numeric_limits<streamsize>::max)(), '\n');
                continue;
            }

            if (pilotId <= 0)
            {
                cout << "Pilot ID must be a positive number.\n";
                continue;
            }

            cout << "Enter Flight ID to remove: ";
            if (!(cin >> flightId))
            {
                cout << "Invalid Flight ID.\n";
                cin.clear();
                cin.ignore((numeric_limits<streamsize>::max)(), '\n');
                continue;
            }

            if (flightId <= 0)
            {
                cout << "Flight ID must be a positive number.\n";
                continue;
            }

            RemoveFlightPacket removePacket{};
            removePacket.header.packetType = REMOVE_FLIGHT_REQUEST;
            removePacket.header.dataSize = sizeof(removePacket);
            removePacket.pilotId = pilotId;
            removePacket.flightId = flightId;

            if (!client.SendData(reinterpret_cast<const char*>(&removePacket), sizeof(removePacket)))
            {
                cout << "Failed to send remove flight request.\n";
                Logger::Log("client_log.txt", "ERROR", "Failed to send remove flight request");
                client.Close();
                return 1;
            }

            Logger::Log(
                "client_log.txt",
                "TX",
                "REMOVE_FLIGHT_REQUEST PilotID=" + to_string(pilotId) +
                " FlightID=" + to_string(flightId)
            );

            OperationResponsePacket response{};
            if (!client.ReceiveData(reinterpret_cast<char*>(&response), sizeof(response)))
            {
                cout << "Failed to receive remove flight response.\n";
                Logger::Log("client_log.txt", "ERROR", "Failed to receive remove flight response");
                client.Close();
                return 1;
            }

            Logger::Log(
                "client_log.txt",
                "RX",
                "OPERATION_RESPONSE Status=" + GetStatusText(response.statusCode) +
                " Message=" + string(response.message)
            );

            cout << "\nServer response: " << response.message << endl;
        }
        else if (choice == 4)
        {
            DownloadReportRequestPacket reportRequest{};
            reportRequest.header.packetType = DOWNLOAD_REPORT_REQUEST;
            reportRequest.header.dataSize = sizeof(reportRequest);
            strcpy_s(reportRequest.month, sizeof(reportRequest.month), "2026-04");

            if (!client.SendData(reinterpret_cast<const char*>(&reportRequest), sizeof(reportRequest)))
            {
                cout << "Failed to send report download request.\n";
                Logger::Log("client_log.txt", "ERROR", "Failed to send report download request");
                client.Close();
                return 1;
            }

            Logger::Log(
                "client_log.txt",
                "TX",
                "DOWNLOAD_REPORT_REQUEST Month=" + string(reportRequest.month)
            );

            FileInfoPacket fileInfo{};
            if (!client.ReceiveData(reinterpret_cast<char*>(&fileInfo), sizeof(fileInfo)))
            {
                cout << "Failed to receive file info packet.\n";
                Logger::Log("client_log.txt", "ERROR", "Failed to receive file info packet");
                client.Close();
                return 1;
            }

            Logger::Log(
                "client_log.txt",
                "RX",
                "FILE_INFO_PACKET FileName=" + string(fileInfo.fileName) +
                " Size=" + to_string(fileInfo.totalFileSize)
            );

            if (fileInfo.statusCode != STATUS_OK)
            {
                cout << "Server failed to prepare the report file.\n";
                continue;
            }

            string outputFileName = "downloaded_" + string(fileInfo.fileName);
            ofstream outputFile(outputFileName, ios::binary);

            if (!outputFile.is_open())
            {
                cout << "Failed to create local file for download.\n";
                Logger::Log("client_log.txt", "ERROR", "Failed to create local file for download");
                continue;
            }

            int totalBytesReceived = 0;

            while (totalBytesReceived < fileInfo.totalFileSize)
            {
                FileChunkPacket chunkPacket{};
                if (!client.ReceiveData(reinterpret_cast<char*>(&chunkPacket), sizeof(chunkPacket)))
                {
                    cout << "Failed to receive file chunk.\n";
                    Logger::Log("client_log.txt", "ERROR", "Failed to receive file chunk");
                    outputFile.close();
                    client.Close();
                    return 1;
                }

                outputFile.write(chunkPacket.data, chunkPacket.bytesInChunk);
                totalBytesReceived += chunkPacket.bytesInChunk;

                int percentage = (totalBytesReceived * 100) / fileInfo.totalFileSize;

                cout << "\rDownloading report... "
                    << totalBytesReceived << " / " << fileInfo.totalFileSize
                    << " bytes (" << percentage << "%)" << flush;

                Logger::Log(
                    "client_log.txt",
                    "RX",
                    "FILE_CHUNK_PACKET Bytes=" + to_string(chunkPacket.bytesInChunk)
                );
            }

            outputFile.close();

            cout << endl;
            cout << "Report downloaded successfully.\n";
            cout << "Saved as: " << outputFileName << endl;
            cout << "Total bytes received: " << totalBytesReceived << endl;

            Logger::Log(
                "client_log.txt",
                "INFO",
                "Report downloaded successfully FileName=" + outputFileName +
                " Bytes=" + to_string(totalBytesReceived)
            );
        }
        else if (choice == 5)
        {
            int pilotId;
            int flightIdToUpdate;
            FlightInfo updatedFlight{};
            vector<FlightInfo> existingFlights;

            cout << "\nEnter Pilot ID: ";
            if (!(cin >> pilotId))
            {
                cout << "Invalid Pilot ID.\n";
                cin.clear();
                cin.ignore((numeric_limits<streamsize>::max)(), '\n');
                continue;
            }

            if (pilotId <= 0)
            {
                cout << "Pilot ID must be a positive number.\n";
                continue;
            }

            ScheduleRequestPacket scheduleRequest{};
            scheduleRequest.header.packetType = GET_SCHEDULE_REQUEST;
            scheduleRequest.header.dataSize = sizeof(scheduleRequest);
            scheduleRequest.pilotId = pilotId;

            if (!client.SendData(reinterpret_cast<const char*>(&scheduleRequest), sizeof(scheduleRequest)))
            {
                cout << "Failed to send pilot validation request.\n";
                Logger::Log("client_log.txt", "ERROR", "Failed to send pilot validation request for update flight");
                client.Close();
                return 1;
            }

            Logger::Log(
                "client_log.txt",
                "TX",
                "GET_SCHEDULE_REQUEST PilotID=" + to_string(pilotId) + " [Validation for Update Flight]"
            );

            ScheduleResponsePacket scheduleResponse{};
            if (!client.ReceiveData(reinterpret_cast<char*>(&scheduleResponse), sizeof(scheduleResponse)))
            {
                cout << "Failed to receive pilot validation response.\n";
                Logger::Log("client_log.txt", "ERROR", "Failed to receive pilot validation response for update flight");
                client.Close();
                return 1;
            }

            Logger::Log(
                "client_log.txt",
                "RX",
                "GET_SCHEDULE_RESPONSE PilotID=" + to_string(scheduleResponse.pilotId) +
                " Status=" + GetStatusText(scheduleResponse.statusCode) +
                " FlightCount=" + to_string(scheduleResponse.flightCount) +
                " [Validation for Update Flight]"
            );

            if (scheduleResponse.statusCode == STATUS_NOT_FOUND)
            {
                cout << "\nPilot with ID " << pilotId << " was not found.\n";
                continue;
            }

            if (scheduleResponse.statusCode != STATUS_OK)
            {
                cout << "\nFailed to validate pilot.\n";
                Logger::Log("client_log.txt", "ERROR", "Pilot validation failed for update flight");
                client.Close();
                return 1;
            }

            cout << "\nPilot found: " << scheduleResponse.pilotName << endl;
            cout << "Flights Assigned: " << scheduleResponse.flightCount << endl;

            if (scheduleResponse.flightCount == 0)
            {
                cout << "This pilot has no flights to update.\n";
                Logger::Log("client_log.txt", "INFO", "Update flight cancelled because pilot has no assigned flights");
                continue;
            }

            for (int i = 0; i < scheduleResponse.flightCount; i++)
            {
                FlightInfo flight{};
                if (!client.ReceiveData(reinterpret_cast<char*>(&flight), sizeof(flight)))
                {
                    cout << "Failed to receive flight data.\n";
                    Logger::Log("client_log.txt", "ERROR", "Failed to receive flight data during update validation");
                    client.Close();
                    return 1;
                }

                existingFlights.push_back(flight);

                Logger::Log(
                    "client_log.txt",
                    "RX",
                    "FLIGHT_INFO FlightID=" + to_string(flight.flightId) +
                    " Origin=" + string(flight.origin) +
                    " Destination=" + string(flight.destination) +
                    " Date=" + string(flight.date)
                );

                cout << "\nFlight " << (i + 1) << endl;
                cout << "Flight ID: " << flight.flightId << endl;
                cout << "Origin: " << flight.origin << endl;
                cout << "Destination: " << flight.destination << endl;
                cout << "Date: " << flight.date << endl;
            }

            cout << "\nEnter Flight ID to update: ";
            if (!(cin >> flightIdToUpdate))
            {
                cout << "Invalid Flight ID.\n";
                cin.clear();
                cin.ignore((numeric_limits<streamsize>::max)(), '\n');
                continue;
            }

            if (flightIdToUpdate <= 0)
            {
                cout << "Flight ID must be a positive number.\n";
                continue;
            }

            bool flightExists = false;
            for (const auto& flight : existingFlights)
            {
                if (flight.flightId == flightIdToUpdate)
                {
                    flightExists = true;
                    break;
                }
            }

            if (!flightExists)
            {
                cout << "Flight ID " << flightIdToUpdate << " was not found for this pilot.\n";
                Logger::Log(
                    "client_log.txt",
                    "INFO",
                    "Update flight cancelled because FlightID=" + to_string(flightIdToUpdate) +
                    " was not found for PilotID=" + to_string(pilotId)
                );
                continue;
            }

            updatedFlight.flightId = flightIdToUpdate;

            cin.ignore((numeric_limits<streamsize>::max)(), '\n');

            cout << "Enter New Origin: ";
            cin.getline(updatedFlight.origin, sizeof(updatedFlight.origin));
            if (strlen(updatedFlight.origin) == 0)
            {
                cout << "Origin cannot be empty.\n";
                continue;
            }

            cout << "Enter New Destination: ";
            cin.getline(updatedFlight.destination, sizeof(updatedFlight.destination));
            if (strlen(updatedFlight.destination) == 0)
            {
                cout << "Destination cannot be empty.\n";
                continue;
            }

            cout << "Enter New Date (YYYY-MM-DD): ";
            cin.getline(updatedFlight.date, sizeof(updatedFlight.date));
            if (!IsValidDateFormat(updatedFlight.date))
            {
                cout << "Invalid date format. Please use YYYY-MM-DD.\n";
                Logger::Log("client_log.txt", "ERROR", "Invalid date entered for update flight");
                continue;
            }

            UpdateFlightPacket updatePacket{};
            updatePacket.header.packetType = UPDATE_FLIGHT_REQUEST;
            updatePacket.header.dataSize = sizeof(updatePacket);
            updatePacket.pilotId = pilotId;
            updatePacket.flight = updatedFlight;

            if (!client.SendData(reinterpret_cast<const char*>(&updatePacket), sizeof(updatePacket)))
            {
                cout << "Failed to send update flight request.\n";
                Logger::Log("client_log.txt", "ERROR", "Failed to send update flight request");
                client.Close();
                return 1;
            }

            Logger::Log(
                "client_log.txt",
                "TX",
                "UPDATE_FLIGHT_REQUEST PilotID=" + to_string(pilotId) +
                " FlightID=" + to_string(updatedFlight.flightId) +
                " Origin=" + string(updatedFlight.origin) +
                " Destination=" + string(updatedFlight.destination) +
                " Date=" + string(updatedFlight.date)
            );

            OperationResponsePacket response{};
            if (!client.ReceiveData(reinterpret_cast<char*>(&response), sizeof(response)))
            {
                cout << "Failed to receive update flight response.\n";
                Logger::Log("client_log.txt", "ERROR", "Failed to receive update flight response");
                client.Close();
                return 1;
            }

            Logger::Log(
                "client_log.txt",
                "RX",
                "UPDATE_FLIGHT_RESPONSE Status=" + GetStatusText(response.statusCode) +
                " Message=" + string(response.message)
            );

            cout << "\nServer response: " << response.message << endl;
        }
        else
        {
            cout << "Invalid option.\n";
        }
    }

    client.Close();
    return 0;
}