#include "ScheduleRepository.h"
#include <cstring>

void ScheduleRepository::LoadSampleData()
{
    schedules.clear();

    PilotSchedule pilot1{};
    pilot1.pilotId = 101;
    strcpy_s(pilot1.pilotName, sizeof(pilot1.pilotName), "Captain Smith");

    FlightInfo flight1{};
    flight1.flightId = 101;
    strcpy_s(flight1.origin, sizeof(flight1.origin), "Toronto");
    strcpy_s(flight1.destination, sizeof(flight1.destination), "New York");
    strcpy_s(flight1.date, sizeof(flight1.date), "2026-03-30");

    FlightInfo flight2{};
    flight2.flightId = 205;
    strcpy_s(flight2.origin, sizeof(flight2.origin), "Montreal");
    strcpy_s(flight2.destination, sizeof(flight2.destination), "Chicago");
    strcpy_s(flight2.date, sizeof(flight2.date), "2026-03-31");

    pilot1.flights.push_back(flight1);
    pilot1.flights.push_back(flight2);

    schedules.push_back(pilot1);

    PilotSchedule pilot2{};
    pilot2.pilotId = 102;
    strcpy_s(pilot2.pilotName, sizeof(pilot2.pilotName), "Captain Johnson");

    FlightInfo flight3{};
    flight3.flightId = 310;
    strcpy_s(flight3.origin, sizeof(flight3.origin), "Vancouver");
    strcpy_s(flight3.destination, sizeof(flight3.destination), "Calgary");
    strcpy_s(flight3.date, sizeof(flight3.date), "2026-04-01");

    FlightInfo flight4{};
    flight4.flightId = 411;
    strcpy_s(flight4.origin, sizeof(flight4.origin), "Calgary");
    strcpy_s(flight4.destination, sizeof(flight4.destination), "Edmonton");
    strcpy_s(flight4.date, sizeof(flight4.date), "2026-04-02");

    pilot2.flights.push_back(flight3);
    pilot2.flights.push_back(flight4);

    schedules.push_back(pilot2);
}

bool ScheduleRepository::GetScheduleByPilotId(int pilotId, PilotSchedule& result)
{
    for (const auto& schedule : schedules)
    {
        if (schedule.pilotId == pilotId)
        {
            result = schedule;
            return true;
        }
    }
    return false;
}