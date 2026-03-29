#include "Logger.h"

void Logger::Log(const string& fileName, const string& direction, const string& message)
{
    ofstream logFile(fileName, ios::app);

    if (!logFile.is_open())
    {
        return;
    }

    time_t now = time(0);
    tm localTime;
    localtime_s(&localTime, &now);

    logFile << put_time(&localTime, "%Y-%m-%d %H:%M:%S")
        << " [" << direction << "] "
        << message << endl;

    logFile.close();
}