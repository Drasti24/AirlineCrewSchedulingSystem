//Group - 7 Drasti Patel , Komalpreet kaur , Jiya Pandit  
#pragma once
#include <string>

enum PacketType
{
    CONNECT_REQUEST = 1,
    CONNECT_RESPONSE,
    GET_SCHEDULE_REQUEST,
    GET_SCHEDULE_RESPONSE,
    ASSIGN_FLIGHT_REQUEST,
    REMOVE_FLIGHT_REQUEST,
    UPDATE_FLIGHT_REQUEST,
    OPERATION_RESPONSE,
    DOWNLOAD_REPORT_REQUEST,
    FILE_INFO_PACKET,
    FILE_CHUNK_PACKET,
    ERROR_PACKET
};

enum StatusCode
{
    STATUS_OK = 0,
    STATUS_FAILED = 1,
    STATUS_INVALID = 2,
    STATUS_NOT_FOUND = 3
};

enum ServerState
{
    STATE_DISCONNECTED = 0,
    STATE_CONNECTED,
    STATE_AUTHENTICATED,
    STATE_PROCESSING_REQUEST,
    STATE_SENDING_SCHEDULE,
    STATE_SENDING_REPORT,
    STATE_ERROR
};

inline std::string PacketTypeToString(int type)
{
    switch (type)
    {
    case CONNECT_REQUEST: return "CONNECT_REQUEST";
    case CONNECT_RESPONSE: return "CONNECT_RESPONSE";
    case GET_SCHEDULE_REQUEST: return "GET_SCHEDULE_REQUEST";
    case GET_SCHEDULE_RESPONSE: return "GET_SCHEDULE_RESPONSE";
    default: return "UNKNOWN_PACKET";
    }
}

inline std::string ServerStateToString(ServerState state)
{
    switch (state)
    {
    case STATE_DISCONNECTED: return "STATE_DISCONNECTED";
    case STATE_CONNECTED: return "STATE_CONNECTED";
    case STATE_AUTHENTICATED: return "STATE_AUTHENTICATED";
    case STATE_PROCESSING_REQUEST: return "STATE_PROCESSING_REQUEST";
    case STATE_SENDING_SCHEDULE: return "STATE_SENDING_SCHEDULE";
    case STATE_SENDING_REPORT: return "STATE_SENDING_REPORT";
    case STATE_ERROR: return "STATE_ERROR";
    default: return "UNKNOWN_STATE";
    }
}