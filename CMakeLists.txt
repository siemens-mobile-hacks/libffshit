cmake_minimum_required(VERSION 3.5)
project(ffshit VERSION 0.0.1 DESCRIPTION "Siemens filesystem library" LANGUAGES CXX)

option(BUILD_SHARED_LIBS "Build shadred library" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(fmt REQUIRED)
find_package(Iconv REQUIRED)
find_package(spdlog REQUIRED)

include_directories(
    ${PROJECT_SOURCE_DIR}/inc
    ${Iconv_INCLUDE_DIRS}
)

set(SOURCES_LIST 
    src/system.cpp
    src/rawdata.cpp
    src/blocks.cpp 
    src/filesystem/extract.cpp
    src/filesystem/file.cpp
    src/filesystem/directory.cpp
    src/filesystem/sgold.cpp 
    src/filesystem/newsgold.cpp
    src/filesystem/newsgold_x85.cpp)

set(PUBLIC_HEADERS_LIST
    inc/ffshit/rawdata.h
    inc/ffshit/help.h
    inc/ffshit/filesystem/file.h
    inc/ffshit/filesystem/directory.h
    inc/ffshit/filesystem/base.h
    inc/ffshit/filesystem/newsgold.h
    inc/ffshit/filesystem/sgold.h
    inc/ffshit/filesystem/newsgold_x85.h
    inc/ffshit/filesystem/extract.h
    inc/ffshit/filesystem/ex.h
    inc/ffshit/blocks.h
    inc/ffshit/ex.h
    inc/ffshit/system.h
)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

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
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(DIRECTORY ${PROJECT_SOURCE_DIR}/inc/
    DESTINATION include)

install(EXPORT ${PROJECT_NAME}Targets
    FILE ${PROJECT_NAME}Targets.cmake
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

