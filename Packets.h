#pragma once

#include "Common.h"
#include "PacketTypes.h"
#include "Constants.h"

struct PacketHeader
{
    int packetType;
    int dataSize;
};

struct ConnectRequestPacket
{
    PacketHeader header;
    char clientName[MAX_NAME_LENGTH];
};

struct ConnectResponsePacket
{
    PacketHeader header;
    int statusCode;
    char message[MAX_MESSAGE_LENGTH];
};

struct ScheduleRequestPacket
{
    PacketHeader header;
    int pilotId;
};

struct FlightInfo
{
    int flightId;
    char origin[MAX_ROUTE_LENGTH];
    char destination[MAX_ROUTE_LENGTH];
    char date[MAX_DATE_LENGTH];
};

struct ScheduleResponsePacket
{
    PacketHeader header;
    int statusCode;
    int pilotId;
    char pilotName[MAX_NAME_LENGTH];
    int flightCount;
};

struct AssignFlightPacket
{
    PacketHeader header;
    int pilotId;
    FlightInfo flight;
};

struct RemoveFlightPacket
{
    PacketHeader header;
    int pilotId;
    int flightId;
};

struct OperationResponsePacket
{
    PacketHeader header;
    int statusCode;
    char message[MAX_MESSAGE_LENGTH];
};

struct DownloadReportRequestPacket
{
    PacketHeader header;
    char month[20];
};

struct FileInfoPacket
{
    PacketHeader header;
    int statusCode;
    char fileName[MAX_FILE_NAME_LENGTH];
    int totalFileSize;
    int chunkSize;
};

struct FileChunkPacket
{
    PacketHeader header;
    int bytesInChunk;
    char data[REPORT_CHUNK_SIZE];
};