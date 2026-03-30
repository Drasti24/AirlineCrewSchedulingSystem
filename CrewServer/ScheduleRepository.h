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
};