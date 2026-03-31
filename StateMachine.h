//Group - 7 Drasti Patel , Komalpreet kaur , Jiya Pandit  
#pragma once

#include "PacketTypes.h"

class StateMachine
{
private:
    ServerState currentState;

public:
    StateMachine();

    ServerState GetCurrentState() const;
    void SetState(ServerState newState);
    bool IsValidTransition(ServerState newState) const;
};