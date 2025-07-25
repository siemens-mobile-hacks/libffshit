cmake_minimum_required(VERSION 3.15)

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

project(ffshit VERSION 0.0.2 DESCRIPTION "Siemens filesystem library" LANGUAGES CXX)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    option(BUILD_SHARED_LIBS "Build shadred library" OFF)
else()
    option(BUILD_SHARED_LIBS "Build shadred library" ON)
endif()

option(BUILD_DEV "Development build" ON)
option(BUILD_DEB_PACKAGE "Build debian-like package" OFF)
option(USE_BUILTIN_LIBFMT "Use built-in libfmt instead of system library" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    if(MINGW)
        set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
    endif()
endif()

if (USE_BUILTIN_LIBFMT)
    add_subdirectory(third_party/fmt)
else()
    find_package(fmt REQUIRED)
endif()

find_package(Iconv REQUIRED)
find_package(Git QUIET)

include_directories(
    ${PROJECT_SOURCE_DIR}/inc
    ${Iconv_INCLUDE_DIRS}
)

set(SOURCES_LIST
    src/version.cpp
    src/system.cpp
    src/rawdata.cpp
    src/fullflash.cpp
    src/detector.cpp
    src/partition/block.cpp
    src/partition/partition.cpp
    src/partition/partitions.cpp
    src/filesystem/help.cpp
    src/filesystem/structure/attributes.cpp
    src/filesystem/structure/file.cpp
    src/filesystem/structure/directory.cpp
    src/filesystem/platform/egold_ce.cpp
    src/filesystem/platform/sgold.cpp
    src/filesystem/platform/sgold2.cpp
    src/filesystem/platform/sgold2_elka.cpp
    src/filesystem/platform/builder.cpp)

set(PUBLIC_HEADERS_LIST
    inc/thirdparty/ordered_hash.h
    inc/thirdparty/ordered_map.h
    inc/ffshit/version.h
    inc/ffshit/rawdata.h
    inc/ffshit/detector.h
    inc/ffshit/fullflash.h
    inc/ffshit/help.h
    inc/ffshit/patterns/ex.h
    inc/ffshit/patterns/raw.h
    inc/ffshit/patterns/pattern.h
    inc/ffshit/partition/ex.h
    inc/ffshit/partition/block.h
    inc/ffshit/partition/partition.h
    inc/ffshit/partition/partitions.h
    inc/ffshit/filesystem/help.h
    inc/ffshit/filesystem/structure/attributes.h
    inc/ffshit/filesystem/structure/file.h
    inc/ffshit/filesystem/structure/directory.h
    inc/ffshit/filesystem/platform/types.h
    inc/ffshit/filesystem/platform/platform.h
    inc/ffshit/filesystem/platform/base.h
    inc/ffshit/filesystem/platform/egold_ce.h
    inc/ffshit/filesystem/platform/sgold.h
    inc/ffshit/filesystem/platform/sgold2.h
    inc/ffshit/filesystem/platform/sgold2_elka.h
    inc/ffshit/filesystem/platform/builder.h
    inc/ffshit/filesystem/ex.h
    inc/ffshit/log/interface.h
    inc/ffshit/log/logger.h
    inc/ffshit/ex.h
    inc/ffshit/system.h
)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    if(MINGW)
        set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
        set(CMAKE_CXX_FLAGS_DEBUG "-g")
        set(CMAKE_CXX_FLAGS_RELEASE "-O3")
    else()
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} /Z7 /D_DEBUG /Od")
        set(CMAKE_CXX_FLAGS_RELEASE "/O2")
    endif()
else()
    set(CMAKE_CXX_FLAGS_DEBUG "-g")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
endif()

set(VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(VERSION_PATCH "${PROJECT_VERSION_PATCH}")

if(GIT_FOUND)
    execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
                    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                    OUTPUT_VARIABLE GIT_COMMIT_HASH
                    OUTPUT_STRIP_TRAILING_WHITESPACE)

    string(APPEND VERSION_PATCH "-${GIT_COMMIT_HASH}")
endif()

if(BUILD_DEV)
    string(APPEND VERSION_PATCH "-unstable")
endif()

set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

add_library(${PROJECT_NAME} ${SOURCES_LIST} ${PUBLIC_HEADERS_LIST})

set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(${PROJECT_NAME} PROPERTIES SOVERSION ${PROJECT_VERSION_MAJOR})

target_compile_definitions(${PROJECT_NAME} PRIVATE DEF_VERSION_STRING_MAJOR="${VERSION_MAJOR}")
target_compile_definitions(${PROJECT_NAME} PRIVATE DEF_VERSION_STRING_MINOR="${VERSION_MINOR}")
target_compile_definitions(${PROJECT_NAME} PRIVATE DEF_VERSION_STRING_PATCH="${VERSION_PATCH}")
target_compile_definitions(${PROJECT_NAME} PRIVATE DEF_VERSION_STRING="${VERSION_STRING}")

target_include_directories(${PROJECT_NAME} PRIVATE inc)
target_link_libraries(${PROJECT_NAME} PRIVATE fmt::fmt ${Iconv_LIBRARIES})

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}Targets
    INCLUDES DESTINATION include
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(DIRECTORY ${PROJECT_SOURCE_DIR}/inc/
    DESTINATION include)

install(EXPORT ${PROJECT_NAME}Targets
    FILE ${PROJECT_NAME}Targets.cmake
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION lib/cmake/${PROJECT_NAME})

configure_package_config_file(
    ${CMAKE_SOURCE_DIR}/${PROJECT_NAME}Config.cmake.in
    ${CMAKE_BINARY_DIR}/${PROJECT_NAME}Config.cmake
    INSTALL_DESTINATION lib/cmake/${PROJECT_NAME})

install(FILES ${CMAKE_BINARY_DIR}/${PROJECT_NAME}Config.cmake
    DESTINATION lib/cmake/${PROJECT_NAME})

write_basic_package_version_file(
    ${CMAKE_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion)

install(
    FILES ${CMAKE_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
    DESTINATION lib/cmake/${PROJECT_NAME})

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Emscripten")
    add_executable(libffshit_wasm wasm/main.cpp)
    target_link_libraries(libffshit_wasm PRIVATE fmt::fmt ${PROJECT_NAME})
    target_link_options(libffshit_wasm PUBLIC
        -sWASM=1
        -sEXPORT_ES6=1
        -sMODULARIZE
        -sINVOKE_RUN=0
        -sSINGLE_FILE=0
        -sASSERTIONS=1
        -sEXIT_RUNTIME=1
        -sFILESYSTEM=0
        -sSTACK_OVERFLOW_CHECK=0
        -sALLOW_MEMORY_GROWTH
        -sEXPORTED_FUNCTIONS=['_malloc','_free']
        -sEXPORTED_RUNTIME_METHODS=['ccall','wasmMemory','HEAPU8']
        -fwasm-exceptions
        --bind
    )
    target_link_options(libffshit_wasm PRIVATE --emit-tsd "$<TARGET_FILE_DIR:libffshit_wasm>/libffshit_wasm.d.ts")
    target_compile_options(libffshit_wasm PUBLIC -O3 -fwasm-exceptions)
endif()

if (BUILD_DEB_PACKAGE)
    if (NOT DEFINED DEB_DIST_ARCH)
        message( FATAL_ERROR "You must specify DEB_DIST_ARCH. For example -DDEB_DIST_ARCH=amd64" )
    endif()

    if (NOT DEFINED DEB_DIST_DEPS)
        message( FATAL_ERROR "You must specify DEB_DIST_DEPS. For example -DDEB_DIST_DEPS=\"libfmt9,libfmt-dev\"" )
    endif()

    if (NOT DEFINED DEB_DIST_NAME)
        message( FATAL_ERROR "You must specify DEB_DIST_NAME. For example -DDEB_DIST_NAME=debian-12" )
    endif()

    set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/packaging/deb")

    include(deb)
endif()
