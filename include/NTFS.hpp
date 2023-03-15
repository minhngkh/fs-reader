#pragma once

#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "Drive.hpp"
#include "Global.hpp"
#include "IReader.hpp"

namespace Ntfs {

enum class MftEntryAvailability { InUse, NotInUse, Invalid };

struct DataRun {
  QWORD clusterCount;
  QWORD firstCluster;
};

struct FileAttr {
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
  bool directory = false;
  bool indexView = false;
};

// Only necessary information is actually saved
struct AttributeHeader {
  bool isNonResident = false;
  std::string name;
};
struct StandardInformationAttribute {
  AttributeHeader header;

  QWORD createdTime;
  QWORD modifiedTime;
};

struct FileNameAttribute {
  AttributeHeader header;

  Index parent;  // 6 first byte
  FileAttr fileAttr;
  std::vector<BYTE> fileName;
  bool containsUnicode = false;
};

struct DataAttribute {
  AttributeHeader header;

  // if it's resident
  DWORD residentDataSize;

  // if it's non resident
  QWORD realSize;
  std::vector<DataRun> dataRuns;
};

struct MftEntryHeader {
  DWORD id;
  bool isInUse = false;
  bool isDirectory = false;
};

struct MftEntry {
  MftEntryHeader header;

  StandardInformationAttribute stdInfoAttr;
  FileNameAttribute fileNameAttr;
  std::vector<DataAttribute> dataAttrs;
};

// This represent a file or directory
struct DirectoryNode {
  Index id;
  bool isDirectory = false;
  std::vector<Index> children;
};

typedef std::unordered_map<Index, DirectoryNode*> HashMap;

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
  int entrySize;

  MftEntryAvailability readMftEntry(Index id, MftEntry& entry);
  // DataRun readBitmap(std::ifstream& bitmapStream);
  std::vector<DataRun> Reader::getEntrySegments();
  MftEntryAvailability createNode(Index sectorNum, HashMap& map);

 public:
  Reader() = default;

  ~Reader() = default;

  PBS getPbs();

  void read(Drive drive);
  void refresh();
  void Reader::generateDirectoryTree(HashMap& map);
  std::string Reader::readFile(MftEntry entry);
};

}  // namespace Ntfs
