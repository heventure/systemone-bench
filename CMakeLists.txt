cmake_minimum_required(VERSION 3.26)
project(systemone_bench VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Network)
set(ONNXRUNTIME_ROOT "" CACHE PATH "Path to the ONNX Runtime SDK")
find_path(ONNXRUNTIME_INCLUDE_DIR onnxruntime_cxx_api.h
  HINTS "${ONNXRUNTIME_ROOT}/include")
find_library(ONNXRUNTIME_LIBRARY NAMES onnxruntime
  HINTS "${ONNXRUNTIME_ROOT}/lib")
if(NOT ONNXRUNTIME_INCLUDE_DIR OR NOT ONNXRUNTIME_LIBRARY)
  message(FATAL_ERROR "ONNX Runtime SDK not found; set ONNXRUNTIME_ROOT")
endif()
add_library(onnxruntime::onnxruntime UNKNOWN IMPORTED)
set_target_properties(onnxruntime::onnxruntime PROPERTIES
  IMPORTED_LOCATION "${ONNXRUNTIME_LIBRARY}"
  INTERFACE_INCLUDE_DIRECTORIES "${ONNXRUNTIME_INCLUDE_DIR}")
qt_standard_project_setup()

set(S1B_SOURCES
  src/main.cpp
  src/MainWindow.cpp src/MainWindow.h
  src/Benchmark.cpp src/Benchmark.h
  src/RuntimeProbe.cpp src/RuntimeProbe.h
  src/LocalBackend.cpp src/LocalBackend.h
  src/ModelCatalog.cpp src/ModelCatalog.h
  src/ModelManagerDialog.cpp src/ModelManagerDialog.h
  src/OnnxRuntimeBackend.cpp src/OnnxRuntimeBackend.h
)

if(APPLE)
  enable_language(OBJCXX)
  list(APPEND S1B_SOURCES src/CoreMLBackend.mm src/CoreMLBackend.h)
endif()

if(WIN32)
  find_package(OpenVINO QUIET COMPONENTS Runtime)
  if(OpenVINO_FOUND)
    list(APPEND S1B_SOURCES src/OpenVINOBackend.cpp src/OpenVINOBackend.h)
  endif()
endif()

qt_add_executable(systemone-bench ${S1B_SOURCES})
target_link_libraries(systemone-bench PRIVATE Qt6::Core Qt6::Widgets Qt6::Network onnxruntime::onnxruntime)
target_compile_definitions(systemone-bench PRIVATE S1B_WITH_ONNXRUNTIME=1)
message(STATUS "SystemOne Bench: ONNX Runtime backend enabled")

if(APPLE)
  find_library(COREML_FRAMEWORK CoreML REQUIRED)
  find_library(COREVIDEO_FRAMEWORK CoreVideo REQUIRED)
  find_library(FOUNDATION_FRAMEWORK Foundation REQUIRED)
  target_link_libraries(systemone-bench PRIVATE
    ${COREML_FRAMEWORK} ${COREVIDEO_FRAMEWORK} ${FOUNDATION_FRAMEWORK})
  set_source_files_properties(src/CoreMLBackend.mm PROPERTIES COMPILE_FLAGS "-fobjc-arc")
endif()

if(WIN32 AND OpenVINO_FOUND)
  target_link_libraries(systemone-bench PRIVATE openvino::runtime)
  target_compile_definitions(systemone-bench PRIVATE S1B_WITH_OPENVINO=1)
  message(STATUS "SystemOne Bench: OpenVINO backend enabled")
elseif(WIN32)
  message(WARNING "SystemOne Bench: OpenVINO not found; Windows local backend disabled")
endif()

set_target_properties(systemone-bench PROPERTIES
  WIN32_EXECUTABLE ON
  MACOSX_BUNDLE ON
  MACOSX_BUNDLE_BUNDLE_NAME "SystemOne Bench"
  MACOSX_BUNDLE_GUI_IDENTIFIER "com.heventure.systemone-bench"
)

install(TARGETS systemone-bench BUNDLE DESTINATION . RUNTIME DESTINATION .)
