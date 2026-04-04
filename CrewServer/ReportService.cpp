//Group - 7 Drasti Patel , Komalpreet kaur , Jiya Pandit  
#include "ReportService.h"
#include <fstream>
#include <iostream>

bool ReportService::GenerateReport(const ScheduleRepository& repository, const std::string& fileName)
{
    std::ofstream reportFile(fileName, std::ios::out);

    if (!reportFile.is_open())
    {
        std::cout << "Failed to create report file.\n";
        return false;
    }

    const auto& schedules = repository.GetAllSchedules();

    reportFile << "===== MONTHLY CREW SCHEDULE REPORT =====\n\n";

    int repeatCount = 5000;

    for (int copy = 0; copy < repeatCount; copy++)
    {
        reportFile << "----- Report Copy " << (copy + 1) << " -----\n";

        for (const auto& schedule : schedules)
        {
            reportFile << "Pilot ID: " << schedule.pilotId << "\n";
            reportFile << "Pilot Name: " << schedule.pilotName << "\n";
            reportFile << "Flights Assigned: " << schedule.flights.size() << "\n";

            for (size_t i = 0; i < schedule.flights.size(); i++)
            {
                const auto& flight = schedule.flights[i];

                reportFile << "  Flight " << (i + 1) << "\n";
                reportFile << "    Flight ID: " << flight.flightId << "\n";
                reportFile << "    Origin: " << flight.origin << "\n";
                reportFile << "    Destination: " << flight.destination << "\n";
                reportFile << "    Date: " << flight.date << "\n";
            }

            reportFile << "\n";
        }

        reportFile << "========================================\n\n";
    }

    reportFile.close();

    std::ifstream sizeCheck(fileName, std::ios::binary | std::ios::ate);
    if (!sizeCheck.is_open())
    {
        std::cout << "Failed to reopen report file for size check.\n";
        return false;
    }

    std::streamsize fileSize = sizeCheck.tellg();
    sizeCheck.close();

    std::cout << "Report generated successfully: " << fileName << "\n";
    std::cout << "Report size: " << fileSize << " bytes\n";

    return true;
}