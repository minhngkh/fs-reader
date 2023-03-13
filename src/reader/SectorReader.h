#pragma once

#include <windows.h>

#include <string>

void ReadSector(std::string drive, int readPoint, BYTE sector[512]);
