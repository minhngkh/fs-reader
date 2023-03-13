add_executable(fs-reader 
  "main.cpp")

target_link_libraries(fs-reader
  PRIVATE ftxui::screen
  PRIVATE ftxui::dom
  PRIVATE ftxui::component # Not needed for this example.
)

set_target_properties(fs-reader PROPERTIES CXX_STANDARD 17)