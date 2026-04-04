//Group - 7 Drasti Patel , Komalpreet kaur , Jiya Pandit   
#pragma once

#include "ScheduleRepository.h"
#include <string>

class ReportService
{
public:
    bool GenerateReport(const ScheduleRepository& repository, const std::string& fileName);
};