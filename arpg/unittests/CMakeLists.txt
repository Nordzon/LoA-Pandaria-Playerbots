# ARPG unit tests: pure logic from modules/mod_arpg/src/Core (no game libraries). Run: ctest --test-dir build -C RelWithDebInfo -R arpg
# Lives outside modules/mod_arpg because upstream's modules/CMakeLists.txt compiles every .cpp under a module folder into
# the server, and a test main() there would clash.
# Also builds standalone (seconds, own build dir, safe while the server build runs):
#   cmake -S arpg/unittests -B <dir> && cmake --build <dir> --config RelWithDebInfo && <dir>\RelWithDebInfo\arpg_unittests.exe
if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  cmake_minimum_required(VERSION 3.20)
  project(arpg_unittests CXX)
  enable_testing()
endif()

set(ARPG_CORE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../modules/mod_arpg/src/Core)
file(GLOB ARPG_CORE_SOURCES CONFIGURE_DEPENDS ${ARPG_CORE_DIR}/*.cpp)
file(GLOB ARPG_TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_executable(arpg_unittests ${ARPG_CORE_SOURCES} ${ARPG_TEST_SOURCES})
target_include_directories(arpg_unittests PRIVATE ${ARPG_CORE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(arpg_unittests PRIVATE cxx_std_17)
set_target_properties(arpg_unittests PROPERTIES FOLDER "arpg")

add_test(NAME arpg_unittests COMMAND arpg_unittests)
