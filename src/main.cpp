// fs-reader.cpp : Defines the entry point for the application.
//

#include <iomanip>
#include <iostream>

#include "Drive.hpp"
#include "Global.hpp"
#include "NTFS.hpp"
#include "UI.hpp"
#include "Utils.hpp"

using namespace std;

int main() {
  // displayInputScreen();

  Drive drive;
  drive.configure("D");

  Ntfs::Reader reader;
  reader.read(drive);

  /*Sector sector;
  drive.readSector(8000000, sector);

  for (int i = 0; i < 512; i++) {
    std::cout << std::hex << std::setw(2) << std::setfill('0')
              << sector[i] % 256 << " ";
    if (i % 16 == 15) {
      std::cout << std::endl;
    }
  }*/

  reader.generateDirectoryTree();

  return 0;
}
