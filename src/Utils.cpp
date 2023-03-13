#include "Utils.hpp"

#include <algorithm>
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
  std::string result;
  std::copy_n(sector.begin() + start, length, std::back_inserter(result));

  return result;
}

std::string readString(const std::vector<BYTE> &byteArr, int start,
                       int length) {
  std::string result;
  std::copy_n(byteArr.begin() + start, length, std::back_inserter(result));

  return result;
}

std::vector<BYTE> readByteArr(const const Sector &sector, int start,
                              int length) {
  std::vector<BYTE> result;
  std::copy_n(sector.begin() + start, length, std::back_inserter(result));

  return result;
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

}  // namespace Utils
