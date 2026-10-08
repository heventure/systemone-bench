cmake_minimum_required(VERSION 3.24)
project(systemone_bench VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Network)
qt_standard_project_setup()
qt_add_executable(systemone-bench
  src/main.cpp src/MainWindow.cpp src/MainWindow.h
  src/Benchmark.cpp src/Benchmark.h src/RuntimeProbe.cpp src/RuntimeProbe.h)
target_link_libraries(systemone-bench PRIVATE Qt6::Core Qt6::Widgets Qt6::Network)
set_target_properties(systemone-bench PROPERTIES WIN32_EXECUTABLE ON MACOSX_BUNDLE ON
  MACOSX_BUNDLE_BUNDLE_NAME "SystemOne Bench")
install(TARGETS systemone-bench BUNDLE DESTINATION . RUNTIME DESTINATION .)
