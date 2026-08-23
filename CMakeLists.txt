cmake_minimum_required(VERSION 3.21)

project(
  Acus
  VERSION 0.1.0
  DESCRIPTION "A C++ library for generating Brainfuck programs"
  LANGUAGES CXX
)

include(GNUInstallDirs)

option(ACUS_BUILD_BFINT "Build the bundled Brainfuck interpreter" OFF)
option(ACUS_BUILD_EXAMPLES "Build the C++ examples" OFF)
option(ACUS_BUILD_TESTS "Build the Acus test suites" OFF)
option(ACUS_INSTALL_ACS "Install the acs Python frontend" ON)
option(ACUS_ENABLE_WARNINGS "Enable the project's warning flags" ON)
option(ACUS_BUILD_DEV_APP "Build the dev-app" OFF)


set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")

function(acus_enable_warnings target)
  if(NOT ACUS_ENABLE_WARNINGS)
    return()
  endif()

  if(MSVC)
    target_compile_options(${target} PRIVATE /W4)
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    target_compile_options(${target} PRIVATE -Wall)
  endif()
endfunction()

add_subdirectory(src)

if(ACUS_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

if(ACUS_BUILD_TESTS)
  enable_testing()
  add_subdirectory(testsuite)
endif()

if(ACUS_INSTALL_ACS)
  add_subdirectory(tools/acs)
endif()

if(ACUS_BUILD_BFINT)
  add_subdirectory(tools/bfint)
endif()

if(ACUS_BUILD_DEV_APP)
  add_executable(acus-dev devapp/dev.cc)
  target_link_libraries(acus-dev PRIVATE acus)
endif()

if(PROJECT_IS_TOP_LEVEL)
  configure_file(
    "${PROJECT_SOURCE_DIR}/cmake/Uninstall.cmake.in"
    "${PROJECT_BINARY_DIR}/Uninstall.cmake"
    @ONLY
  )

  add_custom_target(
    uninstall
    COMMAND "${CMAKE_COMMAND}" -P "${PROJECT_BINARY_DIR}/Uninstall.cmake"
    COMMENT "Removing files installed by this build tree"
  )
endif()
