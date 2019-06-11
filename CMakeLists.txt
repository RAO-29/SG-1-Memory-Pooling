cmake_minimum_required(VERSION 3.9.6)

project(libcarbon VERSION 1.0.0.0 DESCRIPTION "A library to operate with CARBON files"
        LANGUAGES C) # VERSION major.minor.patch.tweak

include(GNUInstallDirs)

set (CMAKE_C_STANDARD 11)

#if (${BUILD_TYPE} MATCHES Debug)
#        message("-- Debug build is enabled")
#        set(CMAKE_BUILD_TYPE Debug)
#else()
        message("-- Release build is enabled")
        set(CMAKE_BUILD_TYPE Release)
#endif()

file(MAKE_DIRECTORY ${CMAKE_HOME_DIRECTORY}/build)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/build)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/build)

include_directories("${CMAKE_SOURCE_DIR}/src/include")

include_directories("/usr/include")
include_directories("/usr/local/include")

set(LIBS ${LIBS} pthread m)

file(GLOB_RECURSE LIB_HEADER_FILES src/include/*.h )
file(GLOB_RECURSE LIB_SOURCE_FILES src/*.c )
set(LIB_SOURCES ${LIB_HEADER_FILES} ${LIB_SOURCE_FILES})

# message("header files: ${LIB_HEADER_FILES}")
# message("source files: ${LIB_SOURCE_FILES}")

if (${LOG_INFO} MATCHES "on")
    if (${BUILD_TYPE} MATCHES Debug)
        message("-- Info log is enabled")
        add_definitions(-DNG5_LOG_INFO)
    else()
        message("-- ** WARNING ** Info log is enabled but ignored")
        message("   use `cmake -DBUILD_TYPE=Debug` to turn on logging")
    endif()
endif()
if (${LOG_TRACE} MATCHES "on")
    if (${BUILD_TYPE} MATCHES Debug)
        message("-- Trace log is enabled")
        add_definitions(-DNG5_LOG_TRACE)
    else()
        message("-- ** WARNING ** Trace log is enabled but ignored")
        message("   use `cmake -DBUILD_TYPE=Debug` to turn on logging")
    endif()
endif()
if (${LOG_DEBUG} MATCHES "on")
    if (${BUILD_TYPE} MATCHES Debug)
        message("-- Debug log is enabled")
        add_definitions(-DNG5_LOG_DEBUG)
    else()
        message("-- ** WARNING ** Debug log is enabled but ignored")
        message("   use `cmake -DBUILD_TYPE=Debug` to turn on logging")
    endif()
endif()
if (${LOG_WARN} MATCHES "on")
    if (${BUILD_TYPE} MATCHES Debug)
        message("-- Warn log is enabled")
        add_definitions(-DNG5_LOG_DEBUG)
    else()
        message("-- ** WARNING ** Warn log is enabled but ignored")
        message("   use `cmake -DBUILD_TYPE=Debug` to turn on logging")
    endif()
endif()

#set (CMAKE_C_COMPILER             "/usr/bin/clang")
set (CMAKE_C_FLAGS                "-Wall -std=c11 -Wextra -Werror -Wno-implicit-fallthrough")
set (CMAKE_C_FLAGS_DEBUG          "-g")
set (CMAKE_C_FLAGS_MINSIZEREL     "-Os -DNDEBUG")
set (CMAKE_C_FLAGS_RELEASE        "-O3 -DNDEBUG")
set (CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -g")

#set (CMAKE_CXX_COMPILER             "/usr/bin/clang++")
set (CMAKE_CXX_FLAGS                "-std=c++11 -Wall")
set (CMAKE_CXX_FLAGS_DEBUG          "-g")
set (CMAKE_CXX_FLAGS_MINSIZEREL     "-Os -DNDEBUG")
set (CMAKE_CXX_FLAGS_RELEASE        "-O3 -DNDEBUG")
set (CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")

set (CMAKE_AR      "/usr/bin/llvm-ar")
set (CMAKE_LINKER  "/usr/bin/llvm-ld")
set (CMAKE_NM      "/usr/bin/llvm-nm")
set (CMAKE_OBJDUMP "/usr/bin/llvm-objdump")
set (CMAKE_RANLIB  "/usr/bin/llvm-ranlib")


add_library(carbon SHARED ${LIB_SOURCES})
set_target_properties(carbon PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(carbon PROPERTIES PUBLIC_HEADER include/carbon/carbon.h)

install(TARGETS carbon
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/carbon)
configure_file(carbon.pc.in carbon.pc @ONLY)
install(FILES carbon.pc DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/carbon DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/carbon
        FILES_MATCHING PATTERN "*.h*")

add_subdirectory("${CMAKE_SOURCE_DIR}/tools/carbon")
add_subdirectory("${CMAKE_SOURCE_DIR}/benches")
add_subdirectory("${CMAKE_SOURCE_DIR}/examples")

enable_testing()
add_subdirectory("${CMAKE_SOURCE_DIR}/tests")


