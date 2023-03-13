#include "Drive.hpp"

#include <array>
#include <fstream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string>

#include "Utils.hpp"

using std::string;

void Drive::configure(string drive) {
  if (Utils::getOSName() == Utils::OS::Windows) {
    std::stringstream builder;
    builder << R"(\\.\)" << drive << ":";

    this->driveAccess = builder.str();

    this->name = drive;
  }

  Sector sector;
  this->readSector(0, sector);

  std::string oemID = Utils::readString(sector, 0x03, sizeof(QWORD));

  if (oemID == "NTFS    ") {
    this->fileSytem = FileSystem::NTFS;
  } else {
    this->fileSytem = FileSystem::Other;
  }
}

void Drive::readSector(Index readPoint, Sector &sector) {
  std::ifstream driveStream(this->driveAccess, std::ios::binary);

  if (driveStream.fail()) throw std::runtime_error("Unable to access");

  driveStream.seekg(readPoint, std::ios::beg);
  driveStream.read((char *)sector.data(), sector.size());

  driveStream.close();
}

FileSystem Drive::getFileSystem() { return this->fileSytem; }

std::string Drive::getName() { return this->name + ":"; }
