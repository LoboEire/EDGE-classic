##########################################
# Edge Classic - CMake Script
##########################################

cmake_minimum_required(VERSION 3.20)

project(
  edge-classic
  LANGUAGES C CXX
  VERSION 0.1.0
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

option(EDGE_GL_ES2 "Enable GLES2 Rendering Backend" OFF)
option(EDGE_SANITIZE "Enable code sanitizing" OFF)
option(EDGE_PROFILING "Enable Profiling" OFF)
option(EDGE_COAL_SUPPORT "Enable support for COAL scripting" ON)
option(EDGE_DEHACKED_SUPPORT "Enable support for Dehacked patch conversion" ON)
option(EDGE_VWAD_SUPPORT "Enable support for k8vavoom VWAD archives" ON)

include("${CMAKE_SOURCE_DIR}/cmake/EDGEClassic.cmake")

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
  set(CLANG true)
else()
  set(CLANG false)
endif()

if (EMSCRIPTEN)
  include("${CMAKE_SOURCE_DIR}/cmake/Emscripten.cmake")  
endif()

if (MSVC)

  # Disable RTTI
  string(FIND "${CMAKE_CXX_FLAGS}" "/GR" MSVC_HAS_GR)
  if(MSVC_HAS_GR)
      string(REGEX REPLACE "/GR" "/GR-" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  else()
      add_compile_options(/GR-)
  endif()
  
  # Disable C++ Exceptions
  string(REGEX REPLACE "/EHsc" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")    
  add_compile_options(/D_HAS_EXCEPTIONS=0)
  
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /fp:fast")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fp:fast")

  if (NOT CLANG)
    # get the number of logical cores for parallel build
    cmake_host_system_information(RESULT LOGICAL_CORES QUERY NUMBER_OF_LOGICAL_CORES)
    math(EXPR COMPILE_CORES "${LOGICAL_CORES} - 1")  
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP${COMPILE_CORES}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP${COMPILE_CORES}")
  endif()

  # Disable some very noisy warnings from the MSVC build
  # CRT security and POSIX deprecation warnings
  add_definitions("-D_CRT_SECURE_NO_WARNINGS /wd4996")
  # Loss of precision/data on assignment, requires lots of explicit casting
  add_definitions("/wd4244 /wd4267")
  # Unreferenced formal parameter, and there are many of these
  add_definitions("/wd4100")

  # warning level for edge specific source files 
  set (EDGE_WARNINGS "/W4")

  # To use the sanitizer with MSVC, you will need to either have your Visual Studio
  # or Build Tools install in your PATH variable, or copy the appropriate DLL to the program
  # folder before launching. The paths and filenames can vary based on your setup,
  # but, as an example, for a 64-bit Debug build using MSVC 2022 Build Tools, the path would be
  # C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Tools\MSVC\<version number>\bin\Hostx64\x64
  # and the file would be clang_rt.asan_dbg_dynamic-x86_64.dll
  if (EDGE_SANITIZE AND MSVC_VERSION GREATER_EQUAL 1929)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /fsanitize=address /Oy-")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fsanitize=address /Oy-")
  endif()

  if (CLANG)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-c++98-compat -Wno-c++98-compat-pedantic")
  endif()

  set(CMAKE_EXE_LINKER_FLAGS "/SUBSYSTEM:WINDOWS")
else()

  if (WIN32 AND CLANG)
    add_definitions("-D_CRT_SECURE_NO_WARNINGS")
  endif()

  # warning level for edge specific source files 
  set (EDGE_WARNINGS -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -Wno-deprecated-declarations)

  if (NOT CLANG)
    set (EDGE_WARNINGS ${EDGE_WARNINGS} -Wno-stringop-truncation -Wno-stringop-overflow)
  endif()

  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-exceptions -fno-strict-aliasing")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions -fno-rtti -fno-strict-aliasing")

  if (EDGE_SANITIZE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
    set(CMAKE_EXE_LINKER_FLAGS "-fsanitize=address -static-libasan")
  endif()

  if (MSYS)
    set(CMAKE_EXE_LINKER_FLAGS "-static -mwindows")
  endif()

endif()

# set some directory values for various situations

if(${CMAKE_SYSTEM} MATCHES "BSD")
  include_directories("/usr/local/include")  
endif()

if (MSVC)
  set(SDL2_DIR "${CMAKE_SOURCE_DIR}/libraries/sdl2")
endif()

find_package(SDL2 REQUIRED)

# set certain definitions (if appropriate)

if (APPLE)
  include_directories(${SDL2_INCLUDE_DIR})  
  if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm64" AND APPLE)
    add_compile_definitions(APPLE_SILICON)
  elseif(${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86_64" AND APPLE)
    add_compile_definitions(NOT_APPLE_SILICON)
  endif()
endif()

if (NOT EDGE_GL_ES2)
  find_package(OpenGL REQUIRED)
else()
  add_definitions(-DEDGE_GL_ES2)
endif()

if (EDGE_COAL_SUPPORT)
  add_definitions(-DEDGE_COAL_SUPPORT)
endif()

if (EDGE_DEHACKED_SUPPORT)
  add_definitions(-DEDGE_DEHACKED_SUPPORT)
endif()

if (EDGE_VWAD_SUPPORT)
  add_definitions(-DEDGE_VWAD_SUPPORT)
endif()

add_subdirectory(libraries)
add_subdirectory(source_files)