#include "SectorReader.h"

#include <sstream>
#include <stdexcept>
#include <string>

#undef DWORD
typedef unsigned long DWORD;

// using std::string;

void ReadSector(std::string drive, int readPoint, BYTE sector[512]) {
  std::stringstream builder;
  builder << R"(\\.\)" << drive << ":";

  std::string temp1 = builder.str();
  std::wstring temp2(temp1.begin(), temp1.end());
  LPCWSTR drivePath = temp2.c_str();

  DWORD bytesRead;

  HANDLE device = NULL;

  device = CreateFileW(drivePath,                           // Drive to open
                       GENERIC_READ,                        // Access mode
                       FILE_SHARE_READ | FILE_SHARE_WRITE,  // Share Mode
                       NULL,           // Security Descriptor
                       OPEN_EXISTING,  // How to create
                       0,              // File attributes
                       NULL);          // Handle to template

  // Open Error
  if (device == INVALID_HANDLE_VALUE) {
    throw std::invalid_argument("Cannot open");
  }

  SetFilePointer(device, readPoint, NULL, FILE_BEGIN);  // Set a Point to Read

  if (!ReadFile(device, sector, 512, &bytesRead, NULL)) {
    throw std::invalid_argument("Unable to read");
  }

  CloseHandle(device);
}
