#pragma once

#include <array>
#include <fstream>
#include <string>

#include "Global.hpp"

enum class FileSystem { NTFS, FAT32, Other };

class Drive {
 private:
  std::string name;
  std::string driveAccess;
  FileSystem fileSytem;

 public:
  Drive() = default;

  ~Drive() = default;

  std::string getName();
  void configure(std::string drive);
  void readSector(Index readPoint, Sector &sector);
  void readSector(Index readPoint, std::ifstream &ifs);
  FileSystem getFileSystem();
};
