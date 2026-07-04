set(APPDIR "${BINARY_DIR}/edge-classic")

file(REMOVE_RECURSE "${APPDIR}")
file(MAKE_DIRECTORY
  "${APPDIR}/usr/bin"
  "${APPDIR}/usr/lib"
  "${APPDIR}/usr/share/edge-classic/edge_base"
)

file(COPY "${TARGET_FILE}" DESTINATION "${APPDIR}/usr/bin/")
file(CHMOD "${APPDIR}/usr/bin/edge-classic"
  PERMISSIONS
    OWNER_READ OWNER_WRITE OWNER_EXECUTE
    GROUP_READ GROUP_EXECUTE
    WORLD_READ WORLD_EXECUTE
)

execute_process(
  COMMAND ldd "${TARGET_FILE}"
  OUTPUT_VARIABLE LDD_OUTPUT
  ERROR_QUIET
)

set(EXCLUDED_LIB_PATTERNS
  "^libc\\.so"
  "^libm\\.so"
  "^libmvec\\.so"
  "^libdl\\.so"
  "^libpthread\\.so"
  "^librt\\.so"
  "^libutil\\.so"
  "^libresolv\\.so"
  "^libanl\\.so"
  "^libBrokenLocale\\.so"
  "^libcidn\\.so"
  "^libnss_"
  "^libthread_db\\.so"
  "^ld-linux"
  "^linux-vdso"
  "^libstdc\\+\\+\\.so"
  "^libgcc_s\\.so"
  "^libGL\\.so"
  "^libGLX\\.so"
  "^libEGL\\.so"
  "^libGLES"
  "^libGLdispatch\\.so"
  "^libOpenGL\\.so"
  "^libglapi\\.so"
  "^libgbm\\.so"
  "^libdrm\\.so"
  "^libvulkan\\.so"
  "^libX"
  "^libxcb"
  "^libwayland"
  "^libxkbcommon"
  "^libasound\\.so"
  "^libjack\\.so"
  "^libpipewire"
  "^libfontconfig\\.so"
  "^libfreetype\\.so"
  "^libharfbuzz\\.so"
  "^libfribidi\\.so"
  "^libexpat\\.so"
  "^libz\\.so"
  "^libgpg-error\\.so"
  "^libcom_err\\.so"
  "^libICE\\.so"
  "^libSM\\.so"
  "^libusb-1\\.0\\.so"
  "^libuuid\\.so"
  "^libgmp\\.so"
)

string(REGEX MATCHALL "[^\n]+" LDD_LINES "${LDD_OUTPUT}")
foreach(line IN LISTS LDD_LINES)
  if (line MATCHES "=> (/[^ ]+)")
    set(lib_path "${CMAKE_MATCH_1}")
    get_filename_component(lib_name "${lib_path}" NAME)

    set(excluded FALSE)
    foreach(pattern IN LISTS EXCLUDED_LIB_PATTERNS)
      if (lib_name MATCHES "${pattern}")
        set(excluded TRUE)
        break()
      endif()
    endforeach()

    if (NOT excluded AND EXISTS "${lib_path}")
      execute_process(
        COMMAND ${CMAKE_COMMAND} -E copy "${lib_path}" "${APPDIR}/usr/lib/${lib_name}"
      )
    endif()
  endif()
endforeach()

file(WRITE "${APPDIR}/AppRun" [=[#!/bin/sh
HERE="$(dirname "$(readlink -f "$0")")"
export LD_LIBRARY_PATH="$HERE/usr/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
exec "$HERE/usr/bin/edge-classic" "-game" "$HERE/usr/share/edge-classic" "$@"
]=])
file(CHMOD "${APPDIR}/AppRun"
  PERMISSIONS
    OWNER_READ OWNER_WRITE OWNER_EXECUTE
    GROUP_READ GROUP_EXECUTE
    WORLD_READ WORLD_EXECUTE
)

file(COPY "${SOURCE_DIR}/source_files/edge/edge-classic.png"
  DESTINATION "${APPDIR}/"
)

file(READ "${SOURCE_DIR}/source_files/edge/edge-classic.desktop" DESKTOP_CONTENT)
file(WRITE "${APPDIR}/edge-classic.desktop"
  "${DESKTOP_CONTENT}\nExec=edge-classic\nIcon=edge-classic\n"
)

foreach(dir autoload crosshairs docs edge_fixes overlays soundfont)
  file(COPY "${SOURCE_DIR}/${dir}"
    DESTINATION "${APPDIR}/usr/share/edge-classic/"
    PATTERN ".gitkeep" EXCLUDE
    PATTERN ".gitignore" EXCLUDE
  )
endforeach()

if (EXISTS "${SOURCE_DIR}/edge_defs.epk")
  file(COPY "${SOURCE_DIR}/edge_defs.epk"
    DESTINATION "${APPDIR}/usr/share/edge-classic/"
  )
  foreach(epk
    blasphemer chex1 chex3v chex3vm doom doom1 doom2
    freedoom1 freedoom2 hacx harmony harmonyc heretic
    plutonia rekkr tnt
  )
    set(epk_path "${SOURCE_DIR}/edge_base/${epk}.epk")
    if (EXISTS "${epk_path}")
      file(COPY "${epk_path}" DESTINATION "${APPDIR}/usr/share/edge-classic/edge_base/")
    endif()
  endforeach()
else()
  file(COPY "${SOURCE_DIR}/edge_base"
    DESTINATION "${APPDIR}/usr/share/edge-classic/"
    PATTERN ".gitkeep" EXCLUDE
    PATTERN ".gitignore" EXCLUDE
  )
  file(COPY "${SOURCE_DIR}/edge_defs"
    DESTINATION "${APPDIR}/usr/share/edge-classic/"
    PATTERN ".gitkeep" EXCLUDE
    PATTERN ".gitignore" EXCLUDE
  )
endif()

execute_process(
  COMMAND uname -m
  OUTPUT_VARIABLE HOST_ARCH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

set(APPIMAGETOOL "${BINARY_DIR}/appimagetool-${HOST_ARCH}.AppImage")
if (NOT EXISTS "${APPIMAGETOOL}")
  file(DOWNLOAD
    "https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-${HOST_ARCH}.AppImage"
    "${APPIMAGETOOL}"
    STATUS APPIMAGETOOL_DOWNLOAD_STATUS
  )
  list(GET APPIMAGETOOL_DOWNLOAD_STATUS 0 APPIMAGETOOL_DOWNLOAD_CODE)
  if (NOT APPIMAGETOOL_DOWNLOAD_CODE EQUAL 0)
    file(REMOVE "${APPIMAGETOOL}")
    message(FATAL_ERROR "Failed to download appimagetool: ${APPIMAGETOOL_DOWNLOAD_STATUS}")
  endif()
  file(CHMOD "${APPIMAGETOOL}"
    PERMISSIONS
      OWNER_READ OWNER_WRITE OWNER_EXECUTE
      GROUP_READ GROUP_EXECUTE
      WORLD_READ WORLD_EXECUTE
  )
endif()

execute_process(
  COMMAND ${CMAKE_COMMAND} -E env "ARCH=${HOST_ARCH}"
    "${APPIMAGETOOL}" --appimage-extract-and-run "${APPDIR}" "${BINARY_DIR}/edge-classic-${HOST_ARCH}.AppImage"
  RESULT_VARIABLE APPIMAGETOOL_RESULT
)
if (NOT APPIMAGETOOL_RESULT EQUAL 0)
  message(FATAL_ERROR "appimagetool failed with exit code ${APPIMAGETOOL_RESULT}")
endif()
