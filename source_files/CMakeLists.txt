
if (NOT MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fwrapv")
endif()

add_subdirectory(coal)
add_subdirectory(ddf)
add_subdirectory(dehacked)
add_subdirectory(epi)
add_subdirectory(edge)
