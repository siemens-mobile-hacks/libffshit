cmake_minimum_required(VERSION 3.15)

project(ffshit VERSION 0.0.1 DESCRIPTION "Siemens filesystem library" LANGUAGES CXX)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    option(BUILD_SHARED_LIBS "Build shadred library" OFF)
else()
    option(BUILD_SHARED_LIBS "Build shadred library" ON)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(fmt REQUIRED)
find_package(Iconv REQUIRED)

include_directories(
    ${PROJECT_SOURCE_DIR}/inc
    ${Iconv_INCLUDE_DIRS}
)

set(SOURCES_LIST 
    src/system.cpp
    src/rawdata.cpp
    src/partition/block.cpp
    src/partition/partition.cpp
    src/partition/partitions.cpp
    src/filesystem/help.cpp
    src/filesystem/structure/file.cpp
    src/filesystem/structure/directory.cpp
    src/filesystem/platform/sgold.cpp
    src/filesystem/platform/newsgold.cpp
    src/filesystem/platform/newsgold_x85.cpp
    src/filesystem/platform/builder.cpp)

set(PUBLIC_HEADERS_LIST
    inc/ffshit/rawdata.h
    inc/ffshit/help.h
    inc/ffshit/patterns/ex.h
    inc/ffshit/patterns/raw.h
    inc/ffshit/patterns/pattern.h
    inc/ffshit/partition/ex.h
    inc/ffshit/partition/block.h
    inc/ffshit/partition/partition.h
    inc/ffshit/partition/partitions.h
    inc/ffshit/filesystem/help.h
    inc/ffshit/filesystem/structure/file.h
    inc/ffshit/filesystem/structure/directory.h
    inc/ffshit/filesystem/platform/types.h
    inc/ffshit/filesystem/platform/platform.h
    inc/ffshit/filesystem/platform/base.h
    inc/ffshit/filesystem/platform/newsgold.h
    inc/ffshit/filesystem/platform/sgold.h
    inc/ffshit/filesystem/platform/newsgold_x85.h
    inc/ffshit/filesystem/platform/builder.h
    inc/ffshit/filesystem/ex.h
    inc/ffshit/log/interface.h
    inc/ffshit/log/logger.h
    inc/ffshit/ex.h
    inc/ffshit/system.h
)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} /Z7 /D_DEBUG /Od")
    set(CMAKE_CXX_FLAGS_RELEASE "/O2")
else()
    set(CMAKE_CXX_FLAGS_DEBUG "-g")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
endif()

add_library(${PROJECT_NAME} ${SOURCES_LIST} ${PUBLIC_HEADERS_LIST})
#set_target_properties(${PROJECT_NAME} PROPERTIES FRAMEWORK TRUE)
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(${PROJECT_NAME} PROPERTIES SOVERSION ${PROJECT_VERSION_MAJOR})

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
