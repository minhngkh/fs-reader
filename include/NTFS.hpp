#pragma once

#include <string>
#include <vector>

#include "Drive.hpp"
#include "Global.hpp"
#include "IReader.hpp"

namespace Ntfs {

struct FilePermissons {
  bool readOnly = false;
  bool hidden = false;
  bool system = false;
  bool archive = false;
  bool device = false;
  bool normal = false;
  bool temporary = false;
  bool sparseFile = false;
  bool reparsePoint = false;
  bool compressed = false;
  bool offline = false;
  bool notContentIndexed = false;
  bool encrypted = false;
};

// Only necessary information is actually saved
struct AttributeHeader {
  bool isNonResident;
  std::string name;
};
struct StandardInformationAttribute {
  AttributeHeader header;
  FilePermissons filePermissons;
};

struct FileNameAttribute {
  AttributeHeader header;
  std::string fileName;
};

struct DataAttribute {
  AttributeHeader header;

  // if it's resident
  std::vector<BYTE> contentInEntry;

  // if it's non resident
  BYTE size;
  WORD clusterCount;
  std::array<BYTE, 3> firstCluster;
};

struct MftEntryHeader {
  bool isInUse = false;
  bool isDirectory = false;
};

struct MftEntry {
  MftEntryHeader header;

  StandardInformationAttribute stdInfoAttr;
  std::vector<DataAttribute> dataAttr;
  FileNameAttribute fileNameAttr;
};

// This represent a file or directory
struct DirectoryNode {
  MftEntry cur;
  std::vector<DirectoryNode> children;
};

// BIOS Parameter Block
struct BPB {
  WORD bytesPerSector;
  BYTE sectorsPerCluster;
  WORD reservedSector;
  BYTE mediaDescriptor;
  WORD sectorsPerTrack;
  WORD numberOfHeads;
  DWORD hiddenSectors;
  QWORD totalSectors;
  QWORD MftClusterNum;
  QWORD MftMirrClusterNum;
  char clustersPerFileRecordSegment;
  char clustersPerIdexBlock;
  QWORD volumeSerialNumber;
  DWORD checksum;

  // in case the size of each record segment/index block is smaller than a
  // cluster, store calculated values in bytes here
  std::pair<bool, int> BytesPerFileRecordSegment = {false, -1};
  std::pair<bool, int> BytesPerIndexBlock = {false, -1};
};

// NTFS' Partition Boot Sector
struct PBS {
  std::vector<BYTE> jumpInstruction;
  std::string OemID;
  BPB bpb;
  std::vector<BYTE> bootstrapCode;
};

class Reader {
 private:
  Drive curDrive;
  PBS pbs;
  bool hasRead = false;

  MftEntry readMftEntry(Index id);

 public:
  Reader() = default;

  ~Reader() = default;

  PBS getPbs();

  void read(Drive drive);
  void refresh();
  DirectoryNode genDirTree();
};

}  // namespace Ntfs
