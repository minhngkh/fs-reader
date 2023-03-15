#include "UI.hpp"

#include <chrono>
#include <ftxui/component/animation.hpp>
#include <ftxui/component/captured_mouse.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/mouse.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/table.hpp>
#include <ftxui/screen/color.hpp>
#include <ftxui/screen/screen.hpp>
#include <string>
#include <vector>
#include <functional>
#include "Scroller.hpp"

#include "Drive.hpp"
#include "IReader.hpp"
#include "NTFS.hpp"
#include "Utils.hpp"

using namespace ftxui;

Table generate


Table generateTable(Ntfs::PBS pbs, std::string perFileRecordSegment,
                    std::string perIndexBlock, Component &button) {
  auto table = Table(
        {{text("Field"), text("Raw hex"), text("Value")},
         {text("Jump Instruction"), text(Utils::toHexStr(pbs.jumpInstruction))},
         {text("OEM ID"), text(Utils::toHexStr(pbs.OemID)), text(pbs.OemID)},
         {text("- BIOS Parameter Block")},
         {text("  + Bytes per sector"),
          text(Utils::toHexStr<WORD>(pbs.bpb.bytesPerSector)),
          text(std::to_string(pbs.bpb.bytesPerSector))},
         {text("  + Sectors per cluster"),
          text(Utils::toHexStr<BYTE>(pbs.bpb.sectorsPerCluster)),
          text(std::to_string(pbs.bpb.sectorsPerCluster))},
         {text("  + Reserved sector"),
          text(Utils::toHexStr<WORD>(pbs.bpb.reservedSector))},
         {text("  + Media descriptor"),
          text(Utils::toHexStr<BYTE>(pbs.bpb.mediaDescriptor)),
          text(std::to_string(pbs.bpb.mediaDescriptor))},
         {text("  + Sectors per track"),
          text(Utils::toHexStr<WORD>(pbs.bpb.sectorsPerTrack)),
          text(std::to_string(pbs.bpb.sectorsPerTrack))},
         {text("  + Number of heads"),
          text(Utils::toHexStr<WORD>(pbs.bpb.numberOfHeads)),
          text(std::to_string(pbs.bpb.numberOfHeads))},
         {text("  + Hidden sectors"),
          text(Utils::toHexStr<DWORD>(pbs.bpb.hiddenSectors)),
          text(std::to_string(pbs.bpb.hiddenSectors))},
         {text("  + Total sectors"),
          text(Utils::toHexStr<QWORD>(pbs.bpb.totalSectors)),
          text(std::to_string(pbs.bpb.totalSectors))},
         {text("  + $MFT cluster number"),
          text(Utils::toHexStr<QWORD>(pbs.bpb.MftClusterNum)),
          text(std::to_string(pbs.bpb.MftClusterNum))},
         {text("  + $MFTMirr cluster number"),
          text(Utils::toHexStr<QWORD>(pbs.bpb.MftMirrClusterNum)),
          text(std::to_string(pbs.bpb.MftMirrClusterNum))},
         {text("  + Bytes/Clusters per file record segment"),
          text(Utils::toHexStr<BYTE>(pbs.bpb.clustersPerFileRecordSegment)),
          text(perFileRecordSegment)},
         {text("  + Bytes/Clusters per index block"),
          text(Utils::toHexStr<BYTE>(pbs.bpb.clustersPerIdexBlock)),
          text(perIndexBlock)},
         {text("  + Volume serial number"),
          text(Utils::toHexStr<QWORD>(pbs.bpb.volumeSerialNumber)),
          text(std::to_string(pbs.bpb.volumeSerialNumber))},
         {text("  + Checksum"), text(Utils::toHexStr<QWORD>(pbs.bpb.checksum)),
          text(std::to_string(pbs.bpb.checksum))},
         {text("Bootstrap code"), button->Render()}});

    table.SelectAll().Border(LIGHT);
    table.SelectRow(0).DecorateCells(center);
    // Add border around the first column.
    table.SelectAll().SeparatorVertical(LIGHT);
    table.SelectAll().SeparatorHorizontal(LIGHT);

    // Make first row bold with a double border.
    table.SelectRow(0).Decorate(bold);
    table.SelectRow(0).SeparatorVertical(DOUBLE);
    table.SelectRow(0).Border(DOUBLE);

    table.SelectRectangle(1, -1, 3, 3).SeparatorVertical(EMPTY);

    return table;
}

void generateDriveInfo(Drive &drive, Ntfs::PBS &pbs,
                       std::string &perFileRecordSegment,
                       std::string &perIndexBlock) {
  if (drive.getFileSystem() == FileSystem::NTFS) {
    Ntfs::Reader reader;
    reader.read(drive);

    pbs = reader.getPbs();

    if (pbs.bpb.BytesPerFileRecordSegment.first = true) {
      perFileRecordSegment =
          std::to_string(pbs.bpb.BytesPerFileRecordSegment.second) + " bytes";
    } else {
      int val = (int)pbs.bpb.clustersPerFileRecordSegment;
      perFileRecordSegment =
          std::to_string(val) + (val > 1 ? " clusters" : " cluster");
    }

    if (pbs.bpb.BytesPerIndexBlock.first = true) {
      perFileRecordSegment =
          std::to_string(pbs.bpb.BytesPerIndexBlock.second) + " bytes";
    } else {
      int val = (int)pbs.bpb.clustersPerIdexBlock;
      perFileRecordSegment =
          std::to_string(val) + (val > 1 ? " clusters" : " cluster");
    }

    return;
  }
  
  throw std::runtime_error("Unsupported Filesystem");
}



void displayDriveInfoScreen(Drive drive) {
  using namespace std::literals;

  auto screen = ScreenInteractive::Fullscreen();



  Component backButton =
      Button("Back", screen.ExitLoopClosure(), ButtonOption::Ascii());
  backButton |= color(Color::Red);
  
  // --- COMPONENTS ---
  int popupValShown = 0;

  auto viewPopupButton = Button(
      "View",
      [&] {
        popupValShown = 1;
        return false;
      },
      ButtonOption::Ascii());
  viewPopupButton |= color(Color::Yellow);

  // --- Prep data ----
  Ntfs::PBS pbs;
  std::string perFileRecordSegment, perIndexBlock;

  generateDriveInfo(drive, pbs, perFileRecordSegment, perIndexBlock);


  std::string popupContent = Utils::toHexStr(pbs.bootstrapCode);

  auto closePopupButton = Button(
      "Close",
      [&] {
        popupValShown = 0;
        return false;
      },
      ButtonOption::Ascii());
  auto popupValViewer = Renderer(closePopupButton, [&] {
    return vbox({paragraph(popupContent), closePopupButton->Render() | hcenter}) | border | size(WIDTH, EQUAL, 47);
  });

  // ---- ACTUAL DRIVE INFO TAB ---
  /*
  auto scroller = Scroller(Renderer(viewPopupButton, [&] {
    auto table = Table(
        {{text("Field"), text("Raw hex"), text("Value")},
         {text("Jump Instruction"), text(Utils::toHexStr(PBS.jumpInstruction))},
         {text("OEM ID"), text(Utils::toHexStr(PBS.OemID)), text(PBS.OemID)},
         {text("- BIOS Parameter Block")},
         {text("  + Bytes per sector"),
          text(Utils::toHexStr<WORD>(PBS.BPB.bytesPerSector)),
          text(std::to_string(PBS.BPB.bytesPerSector))},
         {text("  + Sectors per cluster"),
          text(Utils::toHexStr<BYTE>(PBS.BPB.sectorsPerCluster)),
          text(std::to_string(PBS.BPB.sectorsPerCluster))},
         {text("  + Reserved sector"),
          text(Utils::toHexStr<WORD>(PBS.BPB.reservedSector))},
         {text("  + Media descriptor"),
          text(Utils::toHexStr<BYTE>(PBS.BPB.mediaDescriptor)),
          text(std::to_string(PBS.BPB.mediaDescriptor))},
         {text("  + Sectors per track"),
          text(Utils::toHexStr<WORD>(PBS.BPB.sectorsPerTrack)),
          text(std::to_string(PBS.BPB.sectorsPerTrack))},
         {text("  + Number of heads"),
          text(Utils::toHexStr<WORD>(PBS.BPB.numberOfHeads)),
          text(std::to_string(PBS.BPB.numberOfHeads))},
         {text("  + Hidden sectors"),
          text(Utils::toHexStr<DWORD>(PBS.BPB.hiddenSectors)),
          text(std::to_string(PBS.BPB.hiddenSectors))},
         {text("  + Total sectors"),
          text(Utils::toHexStr<QWORD>(PBS.BPB.totalSectors)),
          text(std::to_string(PBS.BPB.totalSectors))},
         {text("  + $MFT cluster number"),
          text(Utils::toHexStr<QWORD>(PBS.BPB.MftClusterNum)),
          text(std::to_string(PBS.BPB.MftClusterNum))},
         {text("  + $MFTMirr cluster number"),
          text(Utils::toHexStr<QWORD>(PBS.BPB.MftMirrClusterNum)),
          text(std::to_string(PBS.BPB.MftMirrClusterNum))},
         {text("  + Bytes/Clusters per file record segment"),
          text(Utils::toHexStr<BYTE>(PBS.BPB.clustersPerFileRecordSegment)),
          text(perFileRecordSegment)},
         {text("  + Bytes/Clusters per index block"),
          text(Utils::toHexStr<BYTE>(PBS.BPB.clustersPerIdexBlock)),
          text(perIndexBlock)},
         {text("  + Volume serial number"),
          text(Utils::toHexStr<QWORD>(PBS.BPB.volumeSerialNumber)),
          text(std::to_string(PBS.BPB.volumeSerialNumber))},
         {text("  + Checksum"), text(Utils::toHexStr<QWORD>(PBS.BPB.checksum)),
          text(std::to_string(PBS.BPB.checksum))},
         {text("Bootstrap code"), viewPopupButton->Render()}});

    table.SelectAll().Border(LIGHT);
    table.SelectRow(0).DecorateCells(center);
    // Add border around the first column.
    table.SelectAll().SeparatorVertical(LIGHT);
    table.SelectAll().SeparatorHorizontal(LIGHT);

    // Make first row bold with a double border.
    table.SelectRow(0).Decorate(bold);
    table.SelectRow(0).SeparatorVertical(DOUBLE);
    table.SelectRow(0).Border(DOUBLE);

    table.SelectRectangle(1, 2, 3, 3).SeparatorVertical(EMPTY);

    return table.Render();
  }));
  
  auto infoTable =
      Renderer(scroller, [&] { return scroller->Render() | flex | hcenter; });

  */

  Component driveInfoRenderer = Renderer(viewPopupButton, [&] {
    return generateTable(pbs, perFileRecordSegment, perIndexBlock,
                         viewPopupButton)
               .Render() | hcenter ;
  });

  auto driveInfoContainer = Container::Tab({
          driveInfoRenderer,
          popupValViewer  
      },
      &popupValShown);
  
  auto driveInfo = Renderer(driveInfoContainer, [&] {
    Element document = driveInfoRenderer->Render();
 
    if (popupValShown == 1) {
      document = dbox({
          document,
          popupValViewer->Render() | clear_under | center,
      });
    }
    return document;
  });

  
  // --- DIRECTORY TAB ---
  
  Component test = Renderer([] {
    return vbox({
        text(L"Test"),
        text(L"Press any key to continue..."),
    });
  });


  // --- RENDER ---
  auto menuOption = MenuOption::HorizontalAnimated();
  menuOption.underline.SetAnimationDuration(0ms);
  menuOption.underline.color_active = Color::Cyan;

  int tab_index = 0;
  std::vector<std::string> tab_entries = {
      "Info",
      "Directory",
  };
  auto tab_selection =
      Menu(&tab_entries, &tab_index, menuOption);
  auto tab_content = Container::Tab(
      {
          driveInfo,
          test
      },
      &tab_index);
 
  auto main_container = Container::Vertical({
      backButton,
      tab_selection,
      tab_content,
  });
  
  auto main_renderer = Renderer(main_container, [&] {
    return vbox({
        hbox({
          backButton->Render() | size(WIDTH, EQUAL, 6), 
          text(drive.getName()) | bold | hcenter | flex,
          filler() | size(WIDTH, EQUAL, 6)
        }),
        separator(),
        tab_selection->Render() | hcenter,
        tab_content->Render() | flex
    });
  });

  screen.Loop(main_renderer);
}

void displayInputScreen() {
  auto screen = ScreenInteractive::Fullscreen();

  Drive drive;
  
  std::string inputDrive;
  std::string errorMsg;

  auto validDriveCheck = [&] {
    try {
      drive.configure(inputDrive);
    } catch (std::runtime_error &e) {
      errorMsg = e.what();
      return false;
    }

    errorMsg = "";
    displayDriveInfoScreen(drive);
    return true;
  };

  InputOption inputOption;
  inputOption.on_enter = validDriveCheck;

  Component input = Input(&inputDrive, "", inputOption);
  Component okButton = Button("OK", validDriveCheck, ButtonOption::Ascii());

  auto component = Container::Vertical({
      input,
      okButton,
  });

  auto renderer = Renderer(component, [&] {
    if (errorMsg.empty())
      return vbox({hbox({text("Enter drive letter: "),
                         input->Render() | size(WIDTH, EQUAL, 4)}),
                   hbox({filler(), okButton->Render() | size(WIDTH, EQUAL, 4),
                         filler()})}) |
             border | center;
    else
      return vbox({hbox({text("Enter drive letter: "),
                         input->Render() | size(WIDTH, EQUAL, 4)}),
                   hbox({filler(), okButton->Render() | size(WIDTH, EQUAL, 4),
                         filler()}),
                   text(errorMsg)}) |
             border | center;
  });

  screen.Loop(renderer);
}