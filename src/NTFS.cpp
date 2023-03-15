#include "NTFS.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <stdexcept>
#include <unordered_map>

#include "Drive.hpp"
#include "Global.hpp"
#include "Utils.hpp"

using namespace Ntfs;

void Reader::read(Drive drive) {
  curDrive = drive;
  Sector sector;
  drive.readSector(0, sector);

  // read fields outside of BPB first
  pbs.jumpInstruction = Utils::readByteArr(sector, 0x00, 3);
  pbs.OemID = Utils::readString(sector, 0x03, 8);
  pbs.bootstrapCode = Utils::readByteArr(sector, 0x54, 426);

  // read other values that are in little-endian
  pbs.bpb.bytesPerSector = Utils::readLittleEndianVal<WORD>(sector, 0x0B);
  pbs.bpb.sectorsPerCluster = Utils::readLittleEndianVal<BYTE>(sector, 0x0D);
  pbs.bpb.reservedSector = Utils::readLittleEndianVal<WORD>(sector, 0x0E);
  pbs.bpb.mediaDescriptor = Utils::readLittleEndianVal<BYTE>(sector, 0x15);
  pbs.bpb.sectorsPerTrack = Utils::readLittleEndianVal<WORD>(sector, 0x18);
  pbs.bpb.numberOfHeads = Utils::readLittleEndianVal<WORD>(sector, 0x1A);
  pbs.bpb.hiddenSectors = Utils::readLittleEndianVal<DWORD>(sector, 0x1C);
  pbs.bpb.totalSectors = Utils::readLittleEndianVal<QWORD>(sector, 0x28);
  pbs.bpb.MftClusterNum = Utils::readLittleEndianVal<QWORD>(sector, 0x30);
  pbs.bpb.MftMirrClusterNum = Utils::readLittleEndianVal<QWORD>(sector, 0x38);
  pbs.bpb.clustersPerFileRecordSegment =
      Utils::readLittleEndianVal<BYTE>(sector, 0x40);
  pbs.bpb.clustersPerIdexBlock = Utils::readLittleEndianVal<BYTE>(sector, 0x44);
  pbs.bpb.volumeSerialNumber = Utils::readLittleEndianVal<QWORD>(sector, 0x48);
  pbs.bpb.checksum = Utils::readLittleEndianVal<DWORD>(sector, 0x50);

  // Perform some calculations for signed field
  if (int temp = (int)pbs.bpb.clustersPerFileRecordSegment; temp < 0) {
    pbs.bpb.BytesPerFileRecordSegment = {true, (int)std::pow(2, -temp)};
  }

  if (int temp = (int)pbs.bpb.clustersPerIdexBlock; temp < 0) {
    pbs.bpb.BytesPerIndexBlock = {true, (int)std::pow(2, -temp)};
  };

  // Calculate entry size
  if (pbs.bpb.BytesPerFileRecordSegment.first = true) {
    entrySize = (pbs.bpb.BytesPerFileRecordSegment.second / 512);
  } else {
    entrySize =
        pbs.bpb.clustersPerFileRecordSegment * pbs.bpb.sectorsPerCluster;
  }

  hasRead = true;
}

void Reader::refresh() {
  if (hasRead) {
    read(curDrive);
  } else {
    throw std::runtime_error("No drive has been read");
  }
}

PBS Reader::getPbs() {
  if (!hasRead) {
    throw std::runtime_error("No drive has been read");
  }

  return pbs;
}

MftEntryAvailability Reader::readMftEntry(Index sectorNum, MftEntry &entry) {
  if (!hasRead) {
    throw std::runtime_error("No drive has been read");
  }

  // --- Get the whole entry ---
  std::vector<BYTE> entryRaw;

  entryRaw.reserve(entrySize);

  bool isInvalidEntry = false;
  for (int i = 0; i < entrySize; ++i) {
    Sector sector;
    curDrive.readSector(sectorNum + i, sector);

    if (i == 0) {
      // Check if this is an actual entry/record
      std::string signature = Utils::readString(sector, 0, sizeof(DWORD));
      if (signature != "FILE") {
        isInvalidEntry = true;
        break;
      }
    }

    entryRaw.insert(entryRaw.end(), sector.begin(), sector.end());
  }

  if (isInvalidEntry) return MftEntryAvailability::Invalid;

  // --- Read Entry header ---
  entry.header.id = Utils::readLittleEndianVal<DWORD>(entryRaw, 0x2C);

  WORD flags = Utils::readLittleEndianVal<WORD>(entryRaw, 0x16);
  if (flags & 1)
    entry.header.isInUse = true;
  else
    return MftEntryAvailability::NotInUse;  // No purpose of continue reading
  if (flags & (1 << 1)) entry.header.isDirectory = true;

  WORD firstAttrOffset = Utils::readLittleEndianVal<WORD>(entryRaw, 0x14);

  // --- Read Entry's attributes ---
  auto readAttrHeader = [&](DWORD &attrLength, BYTE &nameLength,
                            WORD &dataOffset,
                            AttributeHeader &attrHeader) -> void {
    attrLength =
        Utils::readLittleEndianVal<DWORD>(entryRaw, firstAttrOffset + 0x4);

    nameLength =
        Utils::readLittleEndianVal<BYTE>(entryRaw, firstAttrOffset + 0x9);
    if (nameLength != 0) {
      WORD nameOffset =
          Utils::readLittleEndianVal<WORD>(entryRaw, firstAttrOffset + 0x10);
      attrHeader.name =
          Utils::readString(entryRaw, firstAttrOffset + nameOffset, nameLength);
    }

    if (Utils::readLittleEndianVal<BYTE>(entryRaw, firstAttrOffset + 0x8) !=
        0) {
      attrHeader.isNonResident = true;
      dataOffset =
          Utils::readLittleEndianVal<WORD>(entryRaw, firstAttrOffset + 0x20);
    } else {
      dataOffset =
          Utils::readLittleEndianVal<WORD>(entryRaw, firstAttrOffset + 0x14);
    }
  };

  while (firstAttrOffset < 512 - sizeof(DWORD)) {
    DWORD attrTypeID =
        Utils::readLittleEndianVal<DWORD>(entryRaw, firstAttrOffset);

    if (attrTypeID == 0x10) {  // $STANDARD_INFORMATION

      StandardInformationAttribute &attr = entry.stdInfoAttr;

      // --- header ---
      DWORD attrLength;
      BYTE nameLength;
      WORD dataOffset;
      readAttrHeader(attrLength, nameLength, dataOffset, attr.header);

      // --- attribute ---
      QWORD createdFiletime = Utils::readLittleEndianVal<QWORD>(
          entryRaw, firstAttrOffset + dataOffset);
      QWORD modifiedFiletime = Utils::readLittleEndianVal<QWORD>(
          entryRaw, firstAttrOffset + dataOffset + 0x8);

      // --- Finished reading
      firstAttrOffset += attrLength;

    } else if (attrTypeID == 0x30) {  // $FILE_NAME

      FileNameAttribute &attr = entry.fileNameAttr;

      // --- header ---
      DWORD attrLength;
      BYTE nameLength;
      WORD dataOffset;
      readAttrHeader(attrLength, nameLength, dataOffset, attr.header);

      // --- attribute ---
      attr.parent =
          Utils::readLittleEndianVal(entryRaw, firstAttrOffset + dataOffset, 6);

      DWORD filePermissons = Utils::readLittleEndianVal<DWORD>(
          entryRaw, firstAttrOffset + dataOffset + 0x38);

      // Check flags in file attribute
      FileAttr &fa = attr.fileAttr;
      if (filePermissons & 1) fa.readOnly = true;
      if (filePermissons & (1 << 1)) fa.hidden = true;
      if (filePermissons & (1 << 2)) fa.system = true;
      if (filePermissons & (1 << 5)) fa.archive = true;
      if (filePermissons & (1 << 6)) fa.device = true;
      if (filePermissons & (1 << 7)) fa.normal = true;
      if (filePermissons & (1 << 8)) fa.temporary = true;
      if (filePermissons & (1 << 9)) fa.sparseFile = true;
      if (filePermissons & (1 << 10)) fa.reparsePoint = true;
      if (filePermissons & (1 << 11)) fa.compressed = true;
      if (filePermissons & (1 << 12)) fa.offline = true;
      if (filePermissons & (1 << 13)) fa.notContentIndexed = true;
      if (filePermissons & (1 << 14)) fa.encrypted = true;
      if (filePermissons & (1 << 28)) fa.directory = true;
      if (filePermissons & (1 << 29)) fa.indexView = true;

      BYTE fileNameLength = Utils::readLittleEndianVal<BYTE>(
          entryRaw, firstAttrOffset + dataOffset + 0x40);
      BYTE fileNameNamespace = Utils::readLittleEndianVal<BYTE>(
          entryRaw, firstAttrOffset + dataOffset + 0x41);

      // If file name contain unicode
      if (fileNameNamespace == 0 || fileNameNamespace == 1) {
        fileNameLength *= 2;
        attr.containsUnicode = true;
      }

      attr.fileName = Utils::readRawString(
          entryRaw, firstAttrOffset + dataOffset + 0x42, fileNameLength);

      // --- Finished reading
      firstAttrOffset += attrLength;

    } else if (attrTypeID == 0x80) {  // $DATA

      DataAttribute dataAttr;

      // --- header ---
      DWORD attrLength;
      BYTE nameLength;
      WORD dataOffset;
      readAttrHeader(attrLength, nameLength, dataOffset, dataAttr.header);

      // --- attribute ---
      if (dataAttr.header.isNonResident) {
        dataAttr.realSize =
            Utils::readLittleEndianVal<QWORD>(entryRaw, firstAttrOffset + 0x30);

        int offsetWithinData = 0;
        BYTE allocatedBytes = Utils::readLittleEndianVal<BYTE>(
            entryRaw, firstAttrOffset + dataOffset + offsetWithinData);

        while (allocatedBytes != 0) {
          DataRun dataRun;

          int clusterCountInfoSize = allocatedBytes % 16;
          int firstClusterInfoSize = allocatedBytes / 16;

          dataRun.clusterCount = Utils::readLittleEndianVal(
              entryRaw, firstAttrOffset + dataOffset + offsetWithinData + 0x1,
              clusterCountInfoSize);
          dataRun.firstCluster = Utils::readLittleEndianVal(
              entryRaw,
              firstAttrOffset + dataOffset + offsetWithinData + 0x1 +
                  clusterCountInfoSize,
              firstClusterInfoSize);

          dataAttr.dataRuns.push_back(dataRun);

          offsetWithinData += clusterCountInfoSize + firstClusterInfoSize + 1;
          allocatedBytes = Utils::readLittleEndianVal<BYTE>(
              entryRaw, firstAttrOffset + dataOffset + offsetWithinData);
        }
      } else {
        dataAttr.residentDataSize =
            Utils::readLittleEndianVal<DWORD>(entryRaw, firstAttrOffset + 0x10);
      }

      entry.dataAttrs.push_back(dataAttr);

      // --- Finished reading
      firstAttrOffset += attrLength;

    } else if (attrTypeID == 0xFFFFFFFF) {
      break;
    } else {  // Other Attributes
      DWORD attrLength =
          Utils::readLittleEndianVal<DWORD>(entryRaw, firstAttrOffset + 0x4);

      firstAttrOffset += attrLength;
    }
  }

  return MftEntryAvailability::InUse;
}

// DirectoryNode Reader::generateDirectoryTree() {
//   DirectoryNode test;
//   const int NumberOfMetadataEntries = 30;
//
//   Index entryNum = 30;
//   std::ifstream bitmapStream;
//   DataRun bitmapDR = readBitmap(bitmapStream);
//
//   int totalEntries = 0;
//
//   for (int i = 0; i < bitmapDR.clusterCount; ++i) {
//     BYTE byte;
//     bitmapStream.read((char *)byte, 1);
//
//     int clusterEnd = pbs.bpb.sectorsPerCluster;
//
//     int j = 0;
//     while (true) {
//       if (!(byte & (1 << j))) {
//         entryNum = (j + 1) * clusterEnd;
//         continue;
//       }
//
//       for (; entryNum < (j + 1) * clusterEnd; ++entryNum) {
//         if (readMftEntry(entryNum)) {
//           std::cout << "entry num " << entryNum << std::endl;
//           ++totalEntries;
//         } else {
//           std::cout << "SKIP " << entryNum << std::endl;
//           entryNum = (j + 1) * clusterEnd;
//           break;
//         }
//       }
//
//       ++j;
//       if (j % 8 == 0) break;
//     }
//
//     if (entryNum == pbs.bpb.sectorsPerCluster) }
// }

std::vector<DataRun> Reader::getEntrySegments() {
  if (!hasRead) {
    throw std::runtime_error("No drive has been read");
  }

  QWORD sectorNum = pbs.bpb.MftClusterNum * pbs.bpb.sectorsPerCluster;

  // --- Get the whole entry ---
  std::vector<BYTE> entryRaw;
  std::vector<DataRun> result;

  entryRaw.reserve(entrySize);

  bool isInvalidEntry = false;
  for (int i = 0; i < entrySize; ++i) {
    Sector sector;
    curDrive.readSector(sectorNum + i, sector);
    entryRaw.insert(entryRaw.end(), sector.begin(), sector.end());
  }

  WORD firstAttrOffset = Utils::readLittleEndianVal<WORD>(entryRaw, 0x14);

  while (firstAttrOffset < 512 - sizeof(DWORD)) {
    DWORD attrTypeID =
        Utils::readLittleEndianVal<DWORD>(entryRaw, firstAttrOffset);

    if (attrTypeID == 0xFFFFFFFF) break;
    if (attrTypeID != 0x80) {
      DWORD attrLength =
          Utils::readLittleEndianVal<DWORD>(entryRaw, firstAttrOffset + 0x4);

      firstAttrOffset += attrLength;
      continue;
    }

    WORD dataOffset =
        Utils::readLittleEndianVal<WORD>(entryRaw, firstAttrOffset + 0x20);

    int offsetWithinData = 0;
    BYTE allocatedBytes = Utils::readLittleEndianVal<BYTE>(
        entryRaw, firstAttrOffset + dataOffset + offsetWithinData);

    while (allocatedBytes != 0) {
      DataRun dataRun;

      int clusterCountInfoSize = allocatedBytes % 16;
      int firstClusterInfoSize = allocatedBytes / 16;

      dataRun.clusterCount = Utils::readLittleEndianVal(
          entryRaw, firstAttrOffset + dataOffset + offsetWithinData + 0x1,
          clusterCountInfoSize);
      dataRun.firstCluster = Utils::readLittleEndianVal(
          entryRaw,
          firstAttrOffset + dataOffset + offsetWithinData + 0x1 +
              clusterCountInfoSize,
          firstClusterInfoSize);

      result.push_back(dataRun);

      offsetWithinData += clusterCountInfoSize + firstClusterInfoSize + 1;
      allocatedBytes = Utils::readLittleEndianVal<BYTE>(
          entryRaw, firstAttrOffset + dataOffset + offsetWithinData);
    }

    break;
  }

  return result;
}

MftEntryAvailability Reader::createNode(Index sectorNum, HashMap &map) {
  if (!hasRead) {
    throw std::runtime_error("No drive has been read");
  }

  // --- Get the whole entry ---
  std::vector<BYTE> entryRaw;

  entryRaw.reserve(entrySize);

  bool isInvalidEntry = false;
  for (int i = 0; i < entrySize; ++i) {
    Sector sector;
    curDrive.readSector(sectorNum + i, sector);

    if (i == 0) {
      // Check if this is an actual entry/record
      std::string signature = Utils::readString(sector, 0, sizeof(DWORD));
      if (signature != "FILE") {
        isInvalidEntry = true;
        break;
      }
    }

    entryRaw.insert(entryRaw.end(), sector.begin(), sector.end());
  }

  if (isInvalidEntry) return MftEntryAvailability::Invalid;

  WORD flags = Utils::readLittleEndianVal<WORD>(entryRaw, 0x16);
  if (!(flags & 1))
    return MftEntryAvailability::NotInUse;  // No purpose of continue reading

  // --- Create child node ---
  DWORD childTempId = Utils::readLittleEndianVal<DWORD>(entryRaw, 0x2C);

  DirectoryNode *child;
  // Child node hasn't exist
  if (map.find(childTempId) == map.end()) {
    child = new DirectoryNode;
    child->id = childTempId;
    if (flags & (1 << 1)) child->isDirectory = true;
    map[child->id] = child;
  } else {  // Child was add as an parent before
    child = map[childTempId];
  }

  WORD firstAttrOffset = Utils::readLittleEndianVal<WORD>(entryRaw, 0x14);

  // --- Read Parent data ---

  while (firstAttrOffset < 512 - sizeof(DWORD)) {
    DWORD attrTypeID =
        Utils::readLittleEndianVal<DWORD>(entryRaw, firstAttrOffset);

    if (attrTypeID == 0xFFFFFFFF) break;
    if (attrTypeID != 0x30) {
      DWORD attrLength =
          Utils::readLittleEndianVal<DWORD>(entryRaw, firstAttrOffset + 0x4);

      firstAttrOffset += attrLength;
    }

    // $FILE_NAME

    // --- header ---
    DWORD attrLength =
        Utils::readLittleEndianVal<DWORD>(entryRaw, firstAttrOffset + 0x4);
    WORD dataOffset =
        Utils::readLittleEndianVal<WORD>(entryRaw, firstAttrOffset + 0x14);

    // --- attribute ---

    // --- Create parent node ---
    Index parentTempID =
        Utils::readLittleEndianVal(entryRaw, firstAttrOffset + dataOffset, 6);

    // parent node exists
    if (map.find(parentTempID) != map.end()) {
      map[parentTempID]->children.push_back(child->id);
    } else {
      DirectoryNode *parent = new DirectoryNode;
      parent->id = parentTempID;
      parent->isDirectory = true;
      parent->children.push_back(child->id);

      map[parentTempID] = parent;
    }
    break;
  }

  return MftEntryAvailability::InUse;
}

void Reader::generateDirectoryTree(HashMap &map) {
  DirectoryNode test;

  DirectoryNode *root = new DirectoryNode;
  root->id = 5;
  map[5] = root;
  std::vector<DataRun> segments = getEntrySegments();

  QWORD relativeCluster = 0;
  for (DataRun &segment : segments) {
    relativeCluster += segment.firstCluster;
    int start = relativeCluster * pbs.bpb.sectorsPerCluster + 36;
    int end = start + segment.clusterCount * pbs.bpb.sectorsPerCluster;

    for (int i = start; i < end; i += 2) {
      createNode(i, map);
    }
  }
}

std::string Reader::readFile(MftEntry entry) {
  std::string result;

  for (DataAttribute &da : entry.dataAttrs) {
    // Choose the unnamed data stream
    if (!da.header.name.empty()) continue;

    QWORD relativeCluster = 0;

    for (DataRun &dr : da.dataRuns) {
      relativeCluster += dr.firstCluster;
    }
  }

  return result;
}

// DataRun Reader::readBitmap(std::ifstream &bitmapStream) {
//   if (!hasRead) {
//     throw std::runtime_error("No drive has been read");
//   }
//
//   std::vector<BYTE> entryRaw;
//
//   int id = 6;  // Default id of bitmap
//   Index sectorNum;
//   int entrySize;
//
//   if (pbs.bpb.BytesPerFileRecordSegment.first = true) {
//     entrySize = (pbs.bpb.BytesPerFileRecordSegment.second / 512);
//   } else {
//     entrySize =
//         pbs.bpb.clustersPerFileRecordSegment * pbs.bpb.sectorsPerCluster;
//   }
//
//   entryRaw.reserve(entrySize);
//   sectorNum =
//       pbs.bpb.MftClusterNum * pbs.bpb.sectorsPerCluster + id * entrySize;
//
//   for (int i = 0; i < entrySize; ++i) {
//     Sector sector;
//     curDrive.readSector(sectorNum + i * entrySize, sector);
//     entryRaw.insert(entryRaw.end(), sector.begin(), sector.end());
//   }
//
//   WORD attrOffset = Utils::readLittleEndianVal<WORD>(entryRaw, 0x14);
//   WORD dataOffset = Utils::readLittleEndianVal<WORD>(entryRaw, 0x14);
//
//   DataRun dr;
//
//   while (attrOffset < 512 - sizeof(DWORD)) {
//     BYTE allocatedBytes = Utils::readLittleEndianVal<BYTE>(
//         entryRaw, attrOffset + dataOffset + 0x0);
//
//     dr.clusterCount = Utils::readLittleEndianVal(
//         entryRaw, attrOffset + dataOffset + 0x1, allocatedBytes % 16);
//     dr.firstCluster = Utils::readLittleEndianVal(
//         entryRaw, attrOffset + dataOffset + 0x1 + allocatedBytes % 16,
//         allocatedBytes / 16);
//   }
//
//   dr.firstSector = dr.firstCluster * pbs.bpb.sectorsPerCluster;
//
//   curDrive.readSector(dr.firstSector, bitmapStream);
//
//   return dr;
// }
