#include "Utils.hpp"

#include <date.h>

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

#include "Global.hpp"

namespace Utils {

OS getOSName() {
#ifdef _WIN32
  return OS::Windows;
#elif __APPLE__ || __MACH__
  return OS::MacOS;
#elif __linux__
  return OS::Linux;
#elif __unix__ || __unix
  return OS::Unix;
#elif __FreeBSD__
  return OS::FreeB;
#else
  return OS::Other;
#endif
}

template <typename T>
T changeEndianess(T val) {
  T temp = val;
  std::reverse((BYTE *)&temp, (BYTE *)&temp + sizeof(T));
  return temp;
}

// Implementations of the above template
template BYTE changeEndianess(BYTE);
template WORD changeEndianess(WORD);
template DWORD changeEndianess(DWORD);
template QWORD changeEndianess(QWORD);

template <typename T>
T readLittleEndianVal(const Sector &sector, int start) {
  /*
  const int bitsPerByte = 8;

  T result = 0;
  for (int i = start; i < start + sizeof(T); i++) {
    result |= (sector[i] << ((i - start) * bitsPerByte));
  }
  */

  T result;
  std::copy(sector.begin() + start, sector.begin() + start + sizeof(T),
            (BYTE *)&result);

  return result;
}

// Implementations of the above template
template BYTE readLittleEndianVal(const Sector &, int);
template WORD readLittleEndianVal(const Sector &, int);
template DWORD readLittleEndianVal(const Sector &, int);
template QWORD readLittleEndianVal(const Sector &, int);

template <typename T>
T readLittleEndianVal(const std::vector<BYTE> &byteArr, int start) {
  T result;
  std::copy(byteArr.begin() + start, byteArr.begin() + start + sizeof(T),
            (BYTE *)&result);

  return result;
}

Index readLittleEndianVal(const std::vector<BYTE> &byteArr, int start,
                          int length) {
  /*Index result;
  std::copy(byteArr.begin() + start, byteArr.begin() + start + length,
            (BYTE *)&result);*/

  const int bitsPerByte = 8;

  Index result = 0;
  for (int i = start; i < start + length; i++) {
    result |= (byteArr[i] << ((i - start) * bitsPerByte));
  }

  return result;
}

template BYTE readLittleEndianVal(const std::vector<BYTE> &, int);
template WORD readLittleEndianVal(const std::vector<BYTE> &, int);
template DWORD readLittleEndianVal(const std::vector<BYTE> &, int);
template QWORD readLittleEndianVal(const std::vector<BYTE> &, int);

// Implementations of the above template
template BYTE readLittleEndianVal(const Sector &, int);
template WORD readLittleEndianVal(const Sector &, int);
template DWORD readLittleEndianVal(const Sector &, int);
template QWORD readLittleEndianVal(const Sector &, int);

std::string readString(const Sector &sector, int start, int length) {
  // std::string result;
  // std::copy_n(sector.begin() + start, length, std::back_inserter(result));

  return std::string(sector.begin() + start, sector.begin() + start + length);
}

std::string readString(const std::vector<BYTE> &byteArr, int start,
                       int length) {
  // std::string result;
  // std::copy_n(byteArr.begin() + start, length, std::back_inserter(result));

  return std::string(byteArr.begin() + start, byteArr.begin() + start + length);
}

std::vector<BYTE> readRawString(const std::vector<BYTE> &byteArr, int start,
                                int length) {
  return std::vector<BYTE>(byteArr.begin() + start,
                           byteArr.begin() + start + length);
}

std::vector<BYTE> readRawWString(const std::vector<BYTE> &byteArr, int start,
                                 int length) {
  return std::vector<BYTE>(byteArr.begin() + start,
                           byteArr.begin() + start + length);
}

std::vector<BYTE> readByteArr(const Sector &sector, int start, int length) {
  // std::vector<BYTE> result;
  // std::copy_n(sector.begin() + start, length, std::back_inserter(result));

  return std::vector<BYTE>(sector.begin() + start,
                           sector.begin() + start + length);
}

std::string formatHexStr(std::string str) {
  std::transform(str.begin(), str.end(), str.begin(), ::toupper);
  std::stringstream builder;

  for (int i = 0; i < str.length(); ++i) {
    if (i % 2 == 0 && i != 0) builder << " ";
    builder << str[i];
  }
  return builder.str();
}

template <typename T>
std::string toHexStr(T val) {
  T temp = changeEndianess(val);
  std::stringstream builder;
  // 1 byte = 2 hex
  builder << std::hex << std::setfill('0') << std::setw(sizeof(T) * 2)
          << (int)temp;

  return formatHexStr(builder.str());
}

template std::string toHexStr(BYTE);
template std::string toHexStr(WORD);
template std::string toHexStr(DWORD);
template std::string toHexStr(QWORD);

std::string toHexStr(std::vector<BYTE> bytes) {
  std::stringstream builder;
  for (BYTE byte : bytes) {
    builder << std::hex << std::setfill('0') << std::setw(2) << (int)byte;
  }

  return formatHexStr(builder.str());
}

std::string toHexStr(std::string str) {
  std::stringstream builder;
  for (char c : str) {
    builder << std::hex << std::setfill('0') << std::setw(2) << (int)c;
  }

  return formatHexStr(builder.str());
}

std::chrono::system_clock::time_point filetimeToSystemclock(
    std::uint64_t fileTime) {
  using namespace std;
  using namespace std::chrono;
  // filetime_duration has the same layout as FILETIME; 100ns intervals
  using filetime_duration = duration<int64_t, ratio<1, 10000000>>;
  // January 1, 1601 (NT epoch) - January 1, 1970 (Unix epoch):
  constexpr duration<int64_t> nt_to_unix_epoch{INT64_C(-11644473600)};

  const filetime_duration asDuration{static_cast<int64_t>(fileTime)};
  const auto withUnixEpoch = asDuration + nt_to_unix_epoch;
  return system_clock::time_point{
      duration_cast<system_clock::duration>(withUnixEpoch)};
}

std::string filetimeToFormattedString(std::uint64_t fileTime) {
  return date::format("%F %R", filetimeToSystemclock(fileTime));
}

int countSetBits(int N) {
  int count = 0;

  // (1 << i) = pow(2, i)
  for (int i = 0; i < sizeof(int) * 8; i++) {
    if (N & (1 << i)) count++;
  }
  return count;
}

}  // namespace Utils
