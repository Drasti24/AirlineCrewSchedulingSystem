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

    Logger::Log("client_log.txt", "TX", "Sent CONNECT_REQUEST");

    ConnectResponsePacket connectResponse{};
    if (!client.ReceiveData(reinterpret_cast<char*>(&connectResponse), sizeof(connectResponse)))
    {
        cout << "Failed to receive connect response.\n";
        client.Close();
        return 1;
    }

    Logger::Log("client_log.txt", "RX", "Received CONNECT_RESPONSE");

    if (connectResponse.statusCode != STATUS_OK)
    {
        cout << "Connection verification failed: " << connectResponse.message << endl;
        client.Close();
        return 1;
    }

    cout << "Server response: " << connectResponse.message << endl;

    int pilotId;
    cout << "\nEnter Pilot ID to retrieve schedule: ";
    cin >> pilotId;

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

    Logger::Log("client_log.txt", "TX", "Sent GET_SCHEDULE_REQUEST");

    ScheduleResponsePacket scheduleResponse{};
    if (!client.ReceiveData(reinterpret_cast<char*>(&scheduleResponse), sizeof(scheduleResponse)))
    {
        cout << "Failed to receive schedule response.\n";
        client.Close();
        return 1;
    }

    Logger::Log("client_log.txt", "RX", "Received GET_SCHEDULE_RESPONSE");

    if (scheduleResponse.statusCode != STATUS_OK)
    {
        cout << "Pilot schedule not found.\n";
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

        cout << "\nFlight " << (i + 1) << endl;
        cout << "Flight ID: " << flight.flightId << endl;
        cout << "Origin: " << flight.origin << endl;
        cout << "Destination: " << flight.destination << endl;
        cout << "Date: " << flight.date << endl;
    }

    system("pause");
    client.Close();
    return 0;
}