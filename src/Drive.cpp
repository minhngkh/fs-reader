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
    builder << R"(\\.\)" << drive << R"(:)";

    this->driveAccess = builder.str();

    this->name = drive;
  }

  Sector sector;
  readSector(0, sector);

  std::string oemID = Utils::readString(sector, 0x03, sizeof(QWORD));

  if (oemID == "NTFS    ") {
    this->fileSytem = FileSystem::NTFS;
  } else {
    this->fileSytem = FileSystem::Other;
  }
}

void Drive::readSector(Index readPoint, Sector &sector) {
  std::ifstream driveStream(this->driveAccess, std::ios::binary);

  if (!driveStream) throw std::runtime_error("Unable to access");

  Index offset = readPoint * 512;
  if (!driveStream.seekg(offset, std::ios::beg)) {
    throw std::runtime_error("Reach the end of the file");
  }
  driveStream.read((char *)sector.data(), sector.size());
}

void Drive::readSector(Index readPoint, std::ifstream& ifs) {
  ifs.open(this->driveAccess, std::ios::binary);

  if (!ifs) throw std::runtime_error("Unable to access");

  Index offset = readPoint * 512;
  ifs.seekg(offset, std::ios::beg);
}

FileSystem Drive::getFileSystem() { return this->fileSytem; }

std::string Drive::getName() { return this->name + ":"; }