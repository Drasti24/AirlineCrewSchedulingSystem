#pragma once

#include "Common.h"

class Logger
{
public:
    static void Log(const string& fileName, const string& direction, const string& message);
};