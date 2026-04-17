#include "Packets.h"
#include <cstring>

std::vector<char> SerializeScheduleDataPacket(const ScheduleDataPacket& packet)
{
    const int flightCount = static_cast<int>(packet.flights.size());

    const int totalSize =
        sizeof(PacketHeader) +
        sizeof(int) +                 // statusCode
        sizeof(int) +                 // pilotId
        MAX_NAME_LENGTH +             // pilotName
        sizeof(int) +                 // flightCount
        (flightCount * static_cast<int>(sizeof(FlightInfo)));

    std::vector<char> buffer(totalSize);
    size_t offset = 0;

    std::memcpy(buffer.data() + offset, &packet.header, sizeof(PacketHeader));
    offset += sizeof(PacketHeader);

    std::memcpy(buffer.data() + offset, &packet.statusCode, sizeof(int));
    offset += sizeof(int);

    std::memcpy(buffer.data() + offset, &packet.pilotId, sizeof(int));
    offset += sizeof(int);

    std::memcpy(buffer.data() + offset, packet.pilotName, MAX_NAME_LENGTH);
    offset += MAX_NAME_LENGTH;

    std::memcpy(buffer.data() + offset, &flightCount, sizeof(int));
    offset += sizeof(int);

    if (flightCount > 0)
    {
        std::memcpy(
            buffer.data() + offset,
            packet.flights.data(),
            flightCount * sizeof(FlightInfo));
    }

    return buffer;
}

bool DeserializeScheduleDataPacket(const std::vector<char>& buffer, ScheduleDataPacket& packet)
{
    const size_t minimumSize =
        sizeof(PacketHeader) +
        sizeof(int) +
        sizeof(int) +
        MAX_NAME_LENGTH +
        sizeof(int);

    if (buffer.size() < minimumSize)
    {
        return false;
    }

    size_t offset = 0;

    std::memcpy(&packet.header, buffer.data() + offset, sizeof(PacketHeader));
    offset += sizeof(PacketHeader);

    std::memcpy(&packet.statusCode, buffer.data() + offset, sizeof(int));
    offset += sizeof(int);

    std::memcpy(&packet.pilotId, buffer.data() + offset, sizeof(int));
    offset += sizeof(int);

    std::memcpy(packet.pilotName, buffer.data() + offset, MAX_NAME_LENGTH);
    offset += MAX_NAME_LENGTH;

    int flightCount = 0;
    std::memcpy(&flightCount, buffer.data() + offset, sizeof(int));
    offset += sizeof(int);

    if (flightCount < 0)
    {
        return false;
    }

    const size_t expectedSize = minimumSize + (flightCount * sizeof(FlightInfo));
    if (buffer.size() != expectedSize)
    {
        return false;
    }

    packet.flights.clear();
    packet.flights.resize(flightCount);

    if (flightCount > 0)
    {
        std::memcpy(
            packet.flights.data(),
            buffer.data() + offset,
            flightCount * sizeof(FlightInfo));
    }

    return true;
}