//Group - 7 Drasti Patel , Komalpreet kaur , Jiya Pandit  
#include "StateMachine.h"
#include "ScheduleRepository.h"
#include <cassert>
#include <iostream>

void TestInitialState()
{
    StateMachine sm;
    assert(sm.GetCurrentState() == STATE_DISCONNECTED);
}

void TestValidTransitions()
{
    StateMachine sm;

    sm.SetState(STATE_CONNECTED);
    assert(sm.GetCurrentState() == STATE_CONNECTED);

    sm.SetState(STATE_AUTHENTICATED);
    assert(sm.GetCurrentState() == STATE_AUTHENTICATED);

    sm.SetState(STATE_PROCESSING_REQUEST);
    assert(sm.GetCurrentState() == STATE_PROCESSING_REQUEST);

    sm.SetState(STATE_SENDING_SCHEDULE);
    assert(sm.GetCurrentState() == STATE_SENDING_SCHEDULE);

    sm.SetState(STATE_AUTHENTICATED);
    assert(sm.GetCurrentState() == STATE_AUTHENTICATED);
}

void TestInvalidTransition()
{
    StateMachine sm;

    sm.SetState(STATE_AUTHENTICATED);
    assert(sm.GetCurrentState() == STATE_ERROR);
}

void TestLoadAndFindValidPilot()
{
    ScheduleRepository repo;
    repo.LoadSampleData();

    PilotSchedule result{};
    bool found = repo.GetScheduleByPilotId(101, result);

    assert(found == true);
    assert(result.pilotId == 101);
    assert(result.flights.size() == 2);
}

void TestFindSecondPilot()
{
    ScheduleRepository repo;
    repo.LoadSampleData();

    PilotSchedule result{};
    bool found = repo.GetScheduleByPilotId(102, result);

    assert(found == true);
    assert(result.pilotId == 102);
    assert(result.flights.size() == 2);
}

void TestInvalidPilot()
{
    ScheduleRepository repo;
    repo.LoadSampleData();

    PilotSchedule result{};
    bool found = repo.GetScheduleByPilotId(999, result);

    assert(found == false);
}

void TestEmptyRepository()
{
    ScheduleRepository repo;

    PilotSchedule result{};
    bool found = repo.GetScheduleByPilotId(101, result);

    assert(found == false);
}

void TestAssignFlight()
{
    ScheduleRepository repo;
    repo.LoadSampleData();

    FlightInfo newFlight{};
    newFlight.flightId = 999;
    strcpy_s(newFlight.origin, sizeof(newFlight.origin), "Toronto");
    strcpy_s(newFlight.destination, sizeof(newFlight.destination), "Los Angeles");
    strcpy_s(newFlight.date, sizeof(newFlight.date), "2026-05-01");

    bool success = repo.AssignFlight(101, newFlight);
    assert(success == true);

    PilotSchedule result{};
    bool found = repo.GetScheduleByPilotId(101, result);

    assert(found == true);
    assert(result.flights.size() == 3);
    assert(result.flights[2].flightId == 999);
}

int main()
{
    TestInitialState();
    TestValidTransitions();
    TestInvalidTransition();
    TestLoadAndFindValidPilot();
    TestFindSecondPilot();
    TestInvalidPilot();
    TestEmptyRepository();
    TestAssignFlight();

    std::cout << "All Sprint 1 tests passed.\n";
    return 0;
}