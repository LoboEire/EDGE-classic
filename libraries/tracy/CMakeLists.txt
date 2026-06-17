##########################################
# edge_tracy
##########################################

add_library(edge_tracy INTERFACE)

if (EDGE_PROFILING)
  find_package(Threads REQUIRED)

  add_library(TracyClient STATIC ${CMAKE_CURRENT_SOURCE_DIR}/TracyClient.cpp)
  target_compile_features(TracyClient PUBLIC cxx_std_11)
  target_include_directories(TracyClient SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
  target_link_libraries(TracyClient PUBLIC Threads::Threads ${CMAKE_DL_LIBS})

  target_compile_definitions(TracyClient PUBLIC TRACY_ENABLE)

  option(TRACY_ON_DEMAND "" OFF)
  if (TRACY_ON_DEMAND)
    target_compile_definitions(TracyClient PUBLIC TRACY_ON_DEMAND)
  endif()

  option(TRACY_CALLSTACK "" ON)
  if (TRACY_CALLSTACK)
    target_compile_definitions(TracyClient PUBLIC TRACY_CALLSTACK)
  endif()

  if (WIN32 AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_link_libraries(TracyClient PUBLIC ws2_32 dbghelp)
  endif()
endif()

target_include_directories(edge_tracy INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR})
