#pragma once

#include <string>
#include <vector>

#include "Global.hpp"

namespace Utils {

enum class OS { Windows, MacOS, Linux, Unix, FreeBSD, Other };

OS getOSName();

template <typename T>
T changeEndianess(T val);

template <typename T>
T readLittleEndianVal(const Sector &sector, int start);

template <typename T>
T readLittleEndianVal(const std::vector<BYTE> &byteArr, int start);

std::string readString(const Sector &sector, int start, int length);
std::string readString(const std::vector<BYTE> &byteArr, int start, int length);
std::vector<BYTE> readByteArr(const const Sector &sector, int start,
                              int length);

template <typename T>
std::string toHexStr(T val);

std::string toHexStr(std::vector<BYTE> bytes);
std::string toHexStr(std::string str);

}  // namespace Utils
