cmake_minimum_required(VERSION 3.16)
project(copyright_notice VERSION 2.1.0 LANGUAGES CXX)

# Options
option(USE_LIBGIT2 "Use LibGit2 as git archive parser backend" OFF) # else use git command line tool

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(USE_LIBGIT2)
    set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
    set(BUILD_SHARED_LIBS OFF)
    add_subdirectory(3rdparty/libgit2)
    set(GIT_REPOSITORY_SRC libgit2_src)
else()
    set(GIT_REPOSITORY_SRC cmdgit_src)
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt5 COMPONENTS
             Core
             Test
             REQUIRED QUIET
)

if(MSVC)
    add_compile_definitions(CRT_SECURE_NO_WARNINGS SCL_SECURE_NO_WARNINGS UNICODE UNICODE)
endif()

set(target_name ${PROJECT_NAME})

set(headers
    src/constants.h
    src/configuration/StaticConfig.h
    src/configuration/RunConfig.h
    src/logger/log.h
    src/file_utils/file_utils.h
    src/file_processor/FileProcessor.h
    src/file_processor/Context.h
    src/file_processor/git/GitRepository.h
    src/file_processor/git/${GIT_REPOSITORY_SRC}/GitRepository.h
    src/file_processor/git/GitBlameLine.h
    src/file_processor/git/git_helpers.h
    src/file_processor/parser/Header.h
    src/file_processor/parser/header_fields.h
    src/file_processor/parser/header_utils.h
    src/file_processor/parser/header_helpers.h
)

set(sources
    src/constants.cpp
    src/configuration/RunConfig.cpp
    src/logger/log.cpp
    src/file_utils/file_utils.cpp
    src/file_processor/FileProcessor.cpp
    src/file_processor/git/${GIT_REPOSITORY_SRC}/GitRepository.cpp
    src/file_processor/git/git_helpers.cpp
    src/file_processor/parser/Header.cpp
    src/file_processor/parser/header_helpers.cpp
)

add_executable(${target_name} ${headers} ${sources} src/main.cpp)

target_compile_definitions(${target_name} PRIVATE
                           CN_APP_NAME="${CMAKE_PROJECT_NAME}"
                           CN_APP_VERSION="${CMAKE_PROJECT_VERSION}"
                           USE_LIBGIT2="${USE_LIBGIT2}"
)

target_include_directories(${target_name} PRIVATE
                           $<$<BOOL:${USE_LIBGIT2}>:${CMAKE_SOURCE_DIR}/3rdparty/libgit2/include>
)

target_link_libraries(${target_name} PRIVATE
                      $<$<BOOL:${USE_LIBGIT2}>:git2>
                      Qt5::Core
)

set_target_properties(${target_name} PROPERTIES
                      MACOSX_BUNDLE_GUI_IDENTIFIER copyright_notice.com
                      MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
                      MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
)

# Tests
set(tst_target_name test_${PROJECT_NAME})
enable_testing(true)
set(tst_sources
    tests/tst_RunConfigTest.cpp
)
add_executable(${tst_target_name} ${sources} ${tst_sources})
target_include_directories(${tst_target_name} PRIVATE
                           $<$<BOOL:${USE_LIBGIT2}>:${CMAKE_SOURCE_DIR}/3rdparty/libgit2/include>
)
target_link_libraries(${tst_target_name} PRIVATE
                      $<$<BOOL:${USE_LIBGIT2}>:git2>
                      Qt5::Core
                      Qt5::Test
)