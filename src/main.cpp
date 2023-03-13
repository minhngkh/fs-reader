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
  Drive drive;
  drive.configure("D");

  Sector sector;
  drive.readSector(0, sector);

  /*for (int i = 0; i < 512; i++) {
    std::cout << std::hex << std::setw(2) << std::setfill('0')
              << sector[i] % 256 << " ";
    if (i % 16 == 15) {
      std::cout << std::endl;
    }
  }*/

  cout << "---------- 02" << endl;

  NtfsReader reader;
  reader.read(drive);

  NtfsPbs pbs = reader.getPbs();

  system("cls");

  displayInputScreen();

  return 0;
}
