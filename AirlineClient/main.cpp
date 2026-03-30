#include "ClientConnection.h"
#include "Packets.h"

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
    strcpy_s(connectRequest.clientName, "AirlineClientApp");

    if (!client.SendData(reinterpret_cast<char*>(&connectRequest), sizeof(connectRequest)))
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

    string connectStatus = (connectResponse.statusCode == STATUS_OK) ? "OK" : "FAILED";

    Logger::Log(
        "client_log.txt",
        "RX",
        "CONNECT_RESPONSE Status=" + connectStatus +
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
        int pilotId;

        while (true)
        {
            cout << "\nEnter Pilot ID to retrieve schedule (or 0 to exit): ";

            if (!(cin >> pilotId))
            {
                cout << "Invalid input. Please enter a numeric Pilot ID.\n";
                cin.clear();
                cin.ignore(1000, '\n');
                continue;
            }

            if (pilotId == 0)
            {
                Logger::Log("client_log.txt", "INFO", "Client chose to exit");
                client.Close();
                return 0;
            }

            if (pilotId < 0)
            {
                cout << "Pilot ID must be a positive number.\n";
                continue;
            }

            break;
        }

        ScheduleRequestPacket scheduleRequest{};
        scheduleRequest.header.packetType = GET_SCHEDULE_REQUEST;
        scheduleRequest.header.dataSize = sizeof(scheduleRequest);
        scheduleRequest.pilotId = pilotId;

        if (!client.SendData(reinterpret_cast<char*>(&scheduleRequest), sizeof(scheduleRequest)))
        {
            cout << "Failed to send schedule request.\n";
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
            client.Close();
            return 1;
        }

        string scheduleStatus;
        if (scheduleResponse.statusCode == STATUS_OK)
        {
            scheduleStatus = "OK";
        }
        else if (scheduleResponse.statusCode == STATUS_NOT_FOUND)
        {
            scheduleStatus = "NOT_FOUND";
        }
        else
        {
            scheduleStatus = "FAILED";
        }

        Logger::Log(
            "client_log.txt",
            "RX",
            "GET_SCHEDULE_RESPONSE PilotID=" + to_string(scheduleResponse.pilotId) +
            " Status=" + scheduleStatus +
            " FlightCount=" + to_string(scheduleResponse.flightCount)
        );

        if (scheduleResponse.statusCode == STATUS_NOT_FOUND)
        {
            cout << "\nPilot with ID " << pilotId << " was not found.\n";
            continue;
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

        for (int i = 0; i < scheduleResponse.flightCount; i++)
        {
            FlightInfo flight{};
            if (!client.ReceiveData(reinterpret_cast<char*>(&flight), sizeof(flight)))
            {
                cout << "Failed to receive flight data.\n";
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

        cout << "\nYou can enter another Pilot ID now.\n";
    }

    client.Close();
    return 0;
}