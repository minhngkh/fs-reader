#include "NTFS.hpp"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <map>
#include <stdexcept>

#include "Drive.hpp"
#include "Global.hpp"
#include "Utils.hpp"

using namespace Ntfs;

void Reader::read(Drive drive) {
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

MftEntry Reader::readMftEntry(Index id) {
  if (!hasRead) {
    throw std::runtime_error("No drive has been read");
  }
  MftEntry entry;
  std::vector<BYTE> entryRaw;

  Index sectorNum;
  int entrySize;

  if (pbs.bpb.BytesPerFileRecordSegment.first = true) {
    entrySize = (pbs.bpb.BytesPerFileRecordSegment.second / 512);
  } else {
    entrySize =
        pbs.bpb.clustersPerFileRecordSegment * pbs.bpb.sectorsPerCluster;
  }

  entryRaw.reserve(entrySize);
  sectorNum = pbs.bpb.MftClusterNum + id * entrySize;

  for (int i = 0; i < entrySize; ++i) {
    Sector sector;
    curDrive.readSector(sectorNum + i, sector);

    entryRaw.insert(entryRaw.end(), sector.begin(), sector.end());
  }

  // --- Read header ---
  WORD flag = Utils::readLittleEndianVal<WORD>(entryRaw, 0x16);

  // check if bit 0 and 1 is set
  if (flag & 1)
    entry.header.isInUse = true;
  else
    return entry;  // No purpose of continue reading
  if (flag & (1 << 1)) entry.header.isDirectory = true;

  WORD attrOffset = Utils::readLittleEndianVal<WORD>(entryRaw, 0x14);

  // --- Read Attribute ---
  while (attrOffset < 512 - sizeof(DWORD)) {
    DWORD attrTypeID = Utils::readLittleEndianVal<DWORD>(entryRaw, attrOffset);

    if (attrTypeID == 0x10) {
      // $STANDARD_INFORMATION

      DWORD attrLength =
          Utils::readLittleEndianVal<DWORD>(entryRaw, attrOffset + 0x4);

      // --- header ---
      StandardInformationAttribute &attr = entry.stdInfoAttr;

      if (Utils::readLittleEndianVal<BYTE>(entryRaw, attrOffset + 0x8) != 0) {
        attr.header.isNonResident = true;
      }

      BYTE nameLength =
          Utils::readLittleEndianVal<BYTE>(entryRaw, attrOffset + 0x9);
      if (nameLength != 0) {
        WORD nameOffset =
            Utils::readLittleEndianVal<WORD>(entryRaw, attrOffset + 0x10);
        attr.header.name =
            Utils::readString(entryRaw, attrOffset + nameOffset, nameLength);
      }

      WORD dataOffset = Utils::readLittleEndianVal<WORD>(entryRaw, 0x14);

      // --- attribute ---
      DWORD filePermissons = Utils::readLittleEndianVal<DWORD>(entryRaw, 0x20);
      FilePermissons &fp = attr.filePermissons;

      // Check flag in file permissions
      if (filePermissons & 1) fp.readOnly = true;
      if (filePermissons & (1 << 1)) fp.hidden = true;
      if (filePermissons & (1 << 2)) fp.system = true;
      if (filePermissons & (1 << 5)) fp.archive = true;
      if (filePermissons & (1 << 6)) fp.device = true;
      if (filePermissons & (1 << 7)) fp.normal = true;
      if (filePermissons & (1 << 8)) fp.temporary = true;
      if (filePermissons & (1 << 9)) fp.sparseFile = true;
      if (filePermissons & (1 << 10)) fp.reparsePoint = true;
      if (filePermissons & (1 << 11)) fp.compressed = true;
      if (filePermissons & (1 << 12)) fp.offline = true;
      if (filePermissons & (1 << 13)) fp.notContentIndexed = true;
      if (filePermissons & (1 << 14)) fp.encrypted = true;

    } else if (attrTypeID == 0x30) {
      // $FILE_NAME

    } else if (attrTypeID == 0x80) {
      // $DATA

    } else if (attrTypeID == 0xFFFFFF) {
      break;
    }
  }
