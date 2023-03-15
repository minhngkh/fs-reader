#pragma once

#include <chrono>
#include <codecvt>
#include <locale>
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

Index readLittleEndianVal(const std::vector<BYTE> &byteArr, int start,
                          int length);

std::string readString(const Sector &sector, int start, int length);
std::string readString(const std::vector<BYTE> &byteArr, int start, int length);

std::vector<BYTE> readRawString(const std::vector<BYTE> &byteArr, int start,
                                int length);

std::vector<BYTE> readRawWString(const std::vector<BYTE> &byteArr, int start,
                                 int length);

std::vector<BYTE> readByteArr(const Sector &sector, int start, int length);

template <typename T>
std::string toHexStr(T val);

std::string toHexStr(std::vector<BYTE> bytes);
std::string toHexStr(std::string str);

std::chrono::system_clock::time_point filetimeToSystemclock(
    std::uint64_t fileTime);

std::string filetimeToFormattedString(std::uint64_t fileTime);

int countSetBits(int N);

std::wstring StringToWString(std::string str) {
  std::wstring result;

  std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
  std::string narrow = converter.to_bytes(result);
  std::wstring wide = converter.from_bytes(str);

  return result;
}

}  // namespace Utils
