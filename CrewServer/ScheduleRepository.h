//Group - 7 Drasti Patel , Komalpreet kaur , Jiya Pandit    
#pragma once

#include "../Packets.h"
#include <vector>

using namespace std;

struct PilotSchedule
{
    int pilotId;
    char pilotName[50];
    vector<FlightInfo> flights;
};

class ScheduleRepository
{
private:
    vector<PilotSchedule> schedules;

public:
    void LoadSampleData();
    bool GetScheduleByPilotId(int pilotId, PilotSchedule& result);
    bool AssignFlight(int pilotId, const FlightInfo& flight);
    bool RemoveFlight(int pilotId, int flightId);
    const vector<PilotSchedule>& GetAllSchedules() const;
};