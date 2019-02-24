# Name: C++ threading library
# Creation date: lundi 24 septembre 2018, 10:39:35 (UTC+0200)
# Description: C++ threading library that mimics the C++11 standard implementation.
#
cmake_minimum_required(VERSION 3.11)

project(
  cpp-pthread
  VERSION 1.8.0
  DESCRIPTION "Simple C++ wrapper to pthread functions (${GIT_LOG})")

option(BUILD_TESTS "enable/disable tests (default is enabled)" ON)
set(CMAKE_CXX_STANDARD 11)

configure_file(src/config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/src/config.h)

# this works because we've extended CMAKE_MODULE_PATH
# uncomment if you have a conanfile.txt and run 'conan install ...' include(conan_paths)
#find_package(Git CONFIG)
#find_package(Conan)

# main targets --------------------------------------------------
#
if( CMAKE_BUILD_TYPE MATCHES Release )
  if( GIT_LOG )
    add_definitions( -DCPP_PTHREAD_VERSION="${PROJECT_VERSION} - ${GIT_LOG}")
  else()
    add_definitions( -DCPP_PTHREAD_VERSION="${PROJECT_VERSION}")
  endif()
else()
  if( GIT_LOG )
    add_definitions( -DCPP_PTHREAD_VERSION="${PROJECT_VERSION}-${GIT_LOG}-SNAPSHOT")
  else()
    add_definitions( -DCPP_PTHREAD_VERSION="${PROJECT_VERSION}-SNAPSHOT")
  endif()
endif()

# project's public headers
include_directories(include src)

set(PTHREAD_SOURCE_CODE
        src/config.h
        src/condition_variable.cpp
        src/exceptions.cpp
        src/lock_guard.cpp
        src/pthread.cpp
        src/read_write_lock.cpp
        src/thread.cpp
        src/mutex.cpp
        )
add_library(cpp-pthread-static STATIC ${PTHREAD_SOURCE_CODE})
target_link_libraries(cpp-pthread-static pthread)
set_target_properties(cpp-pthread-static PROPERTIES OUTPUT_NAME cpp-pthread)

add_library(cpp-pthread-shared SHARED ${PTHREAD_SOURCE_CODE})

target_link_libraries(cpp-pthread-shared pthread)
set_target_properties(cpp-pthread-shared PROPERTIES OUTPUT_NAME cpp-pthread)

# Testing -------------------------------------------------------
#

# Load and compile GTest
# Aliases: GTest::GTest, GTest::gtest_main, GMock::GMock
find_package(GTest PATHS cmake)
if (GTest_FOUND)
  enable_testing()
  add_subdirectory(tests)
endif()

# doxygen -------------------------------------------------------
#
find_package(Doxygen REQUIRED dot OPTIONAL_COMPONENTS mscgen dia)
if (Doxygen_FOUND)
  # set(DOXYGEN_OUTPUT_DIRECTORY doxygen)
  # set(DOXYGEN_GENERATE_MAN YES)
  # set(DOXYGEN_GENERATE_HTML YES)
  set(DOXYGEN_EXAMPLE_PATH tests)
  set(DOXYGEN_PROJECT_BRIEF "Simple C++ wrapper to pthread functions.")
  set(DOXYGEN_USE_MDFILE_AS_MAINPAGE "${CMAKE_SOURCE_DIR}/README.md")
  doxygen_add_docs(doxygen README.md src include COMMENT "generate on-line documentation")

endif()

# install -------------------------------------------------------
#
install( TARGETS cpp-pthread-static cpp-pthread-shared DESTINATION lib )
install( DIRECTORY include DESTINATION include COMPONENT Devel)
# install( DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html DESTINATION share/doc)

# CPACK ---------------------------------------------------------
#

set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_NAME ${CMAKE_PROJECT_NAME})
set(CPACK_PACKAGE_RELEASE 1)
set(CPACK_PACKAGE_CONTACT "Herbert Koelman")
set(CPACK_PACKAGE_VENDOR "Urbix Software")
set(CPACK_PACKAGING_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})

include(CPack)

# misc ------------------------------------------------------------
#
find_program(RM rm)