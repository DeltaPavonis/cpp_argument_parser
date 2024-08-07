cmake_minimum_required(VERSION 3.12)
project(cpp_argument_parser
    LANGUAGES CXX)

# Notify user if their build generator is multi-configuration, because this prevents
# us from setting a default `CMAKE_BUILD_TYPE` (as multi-config generators ignore
# the `CMAKE_BUILD_TYPE` variable).
if(CMAKE_CONFIGURATION_TYPES)
    message(STATUS "\
        NOTE: You are on a multi-configuration generator (VSCode, XCode, etc). This means\n\
        the build type (Debug, Release, etc) should be set from within the IDE itself, because\n\
        multi-configuration generators ignore the CMAKE_BUILD_TYPE variable)."
    )
endif()

# Set the default build type to Release if `CMAKE_BUILD_TYPE` has not been set previously,
# and if the build generator is single-configuration (because if it is multi-config, then
# it ignores `CMAKE_BUILD_TYPE`). From https://www.kitware.com/cmake-and-the-default-build-type/.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "NOTE: Setting build type to 'Release' as none was specified.")
    set(CMAKE_BUILD_TYPE "Release" CACHE
        STRING "Choose the type of build." FORCE)
    # Set the possible values of build type for cmakde-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
      "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# Set the source files
set(CPP_ARGUMENT_PARSER_SOURCES
    src/main.cpp
    src/argumentparser.cpp
)

# Add the executable
add_executable(cpp_argument_parser ${CPP_ARGUMENT_PARSER_SOURCES})

# Require C++20 for `cpp_argument_parser` (and because I use PUBLIC, also for all targets that link to
# `cpp_argument_parser`), and also avoid having extensions being added.
target_compile_features(cpp_argument_parser PUBLIC cxx_std_20)
set_target_properties(cpp_argument_parser PROPERTIES CXX_EXTENSIONS OFF)

# Use cpp_argument_parser/include as an include directory for building the `cpp_argument_parser`
# executable
target_include_directories(cpp_argument_parser PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Collect all needed preprocessor definitions
set(CPP_ARGUMENT_PARSER_DEFINITIONS)

# If the current operating system is Windows, add preprocessor definitions for
# `CPP_ARGUMENT_PARSER_IS_ON_WINDOWS` (allowing us to detect this case in-code), and also define
# `NOMINMAX`, which solves the issue where legacy macros for min and max inside "windows.h" cause
# correct code to emit warnings (from StackOverflow: https://tinyurl.com/zpz6wyzt).
if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    list(APPEND CPP_ARGUMENT_PARSER_DEFINITIONS "CPP_ARGUMENT_PARSER_IS_ON_WINDOWS" "NOMINMAX")
endif()

# Add all compile definitions (proprocessor options) to `cpp_argument_parser`.
target_compile_definitions(cpp_argument_parser PRIVATE ${CPP_ARGUMENT_PARSER_DEFINITIONS})
