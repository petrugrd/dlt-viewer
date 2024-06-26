if(NOT WIN32)
    return()
endif()
message(STATUS "windows/version.cmake ${CMAKE_CURRENT_SOURCE_DIR}")

project(GenerateVersion CXX)

find_package(Qt5 "5" REQUIRED COMPONENTS Core)
set(DLT_QT5_VERSION ${Qt5Core_VERSION} CACHE STRING "DLT_QT5_VERSION")
get_target_property(DLT_QT5_LIBRARY_PATH Qt5::Core LOCATION)
get_filename_component(DLT_QT5_LIB_DIR ${DLT_QT5_LIBRARY_PATH} DIRECTORY)

find_package(Git REQUIRED)
execute_process(
    COMMAND ${GIT_EXECUTABLE} rev-list --count --no-merges HEAD
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_PATCH_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
    COMMAND "${CMAKE_CURRENT_SOURCE_DIR}\\scripts\\windows\\parse_version.bat" "${CMAKE_CURRENT_SOURCE_DIR}\\src\\version.h" PACKAGE_MAJOR_VERSION
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/scripts/windows"
    OUTPUT_VARIABLE DLT_PROJECT_VERSION_MAJOR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
    COMMAND "${CMAKE_CURRENT_SOURCE_DIR}\\scripts\\windows\\parse_version.bat" "${CMAKE_CURRENT_SOURCE_DIR}\\src\\version.h" PACKAGE_MINOR_VERSION
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/scripts/windows"
    OUTPUT_VARIABLE DLT_PROJECT_VERSION_MINOR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
    COMMAND "${CMAKE_CURRENT_SOURCE_DIR}\\scripts\\windows\\parse_version.bat" "${CMAKE_CURRENT_SOURCE_DIR}\\src\\version.h" PACKAGE_PATCH_LEVEL
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/scripts/windows"
    OUTPUT_VARIABLE DLT_PROJECT_VERSION_PATCH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

set(VS_VERSION ${MSVC_TOOLSET_VERSION})
string(REPLACE "140" "msvc2015" VS_VERSION ${VS_VERSION})
string(REPLACE "141" "msvc2017" VS_VERSION ${VS_VERSION})
string(REPLACE "142" "msvc2019" VS_VERSION ${VS_VERSION})
string(REPLACE "143" "msvc2022" VS_VERSION ${VS_VERSION})

set(DLT_VERSION_SUFFIX "STABLE-qt${DLT_QT5_VERSION}-r${GIT_PATCH_VERSION}_${VS_VERSION}_${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}")
