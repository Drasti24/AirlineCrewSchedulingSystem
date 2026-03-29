#include "StateMachine.h"

StateMachine::StateMachine()
{
    currentState = STATE_DISCONNECTED;
}

ServerState StateMachine::GetCurrentState() const
{
    return currentState;
}

void StateMachine::SetState(ServerState newState)
{
    if (IsValidTransition(newState))
    {
        currentState = newState;
    }
    else
    {
        currentState = STATE_ERROR;
    }
}

bool StateMachine::IsValidTransition(ServerState newState) const
{
    switch (currentState)
    {
    case STATE_DISCONNECTED:
        return newState == STATE_CONNECTED;

    case STATE_CONNECTED:
        return newState == STATE_AUTHENTICATED || newState == STATE_DISCONNECTED;

    case STATE_AUTHENTICATED:
        return newState == STATE_PROCESSING_REQUEST || newState == STATE_DISCONNECTED;

    case STATE_PROCESSING_REQUEST:
        return newState == STATE_SENDING_SCHEDULE ||
            newState == STATE_SENDING_REPORT ||
            newState == STATE_AUTHENTICATED ||
            newState == STATE_ERROR;

    case STATE_SENDING_SCHEDULE:
    case STATE_SENDING_REPORT:
        return newState == STATE_AUTHENTICATED;

    case STATE_ERROR:
        return newState == STATE_AUTHENTICATED || newState == STATE_DISCONNECTED;

    default:
        return false;
    }
}