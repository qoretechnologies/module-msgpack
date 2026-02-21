cmake_minimum_required(VERSION 3.5...3.28)

project(qore-msgpack-module)

set (VERSION_MAJOR 1)
set (VERSION_MINOR 0)
set (VERSION_PATCH 1)

set(PROJECT_VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

include_directories( ${CMAKE_SOURCE_DIR}/src )
include_directories( ${CMAKE_SOURCE_DIR}/extern_libs )

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "release")
endif (NOT CMAKE_BUILD_TYPE)
string(TOLOWER ${CMAKE_BUILD_TYPE} QORE_BUILD_TYPE_LWR)
if (${QORE_BUILD_TYPE_LWR} MATCHES "debug")
    add_definitions(-DDEBUG)
else ()
    add_definitions(-DNDEBUG)
endif ()

add_definitions(-DUNICODE)

FIND_PACKAGE (Qore 0.9 REQUIRED)

# Check for C++11.
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
else()
    message(FATAL_ERROR "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()

# Set C99 mode for C (otherwise could not be compiled for example in Centos 7).
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")

set(QPP_SRC
    src/ql_msgpack.qpp
    src/QC_MsgPack.qpp
    src/QC_MsgPackExtension.qpp
)

set(CPP_SRC
    #src/mpack/mpack.c
    extern_libs/mpack/mpack.c
    src/msgpack-module.cpp
    src/msgpack_enums.cpp
    src/msgpack_extensions.cpp
    src/msgpack_pack.cpp
    src/msgpack_unpack.cpp
    src/MsgPackException.cpp
)

qore_wrap_qpp_value(QPP_SOURCES METALIST QPP_META_SRC ${QPP_SRC})
add_custom_target(QPP_GENERATED_FILES DEPENDS ${QPP_SOURCES})

set(module_name "msgpack")

set(QORE_DOX_TMPL_SRC
  docs/mainpage.dox.tmpl
)

# disable Expect and Node API of MPack
add_definitions(-DMPACK_EXPECT=0)
add_definitions(-DMPACK_NODE=0)

# enable Extension handling in MPack
add_definitions(-DMPACK_EXTENSIONS=1)

# enable read and write tracking in MPack to catch erroneous code
add_definitions(-DMPACK_READ_TRACKING=1)
add_definitions(-DMPACK_WRITE_TRACKING=1)

# enable MPack compatibility features
add_definitions(-DMPACK_COMPATIBILITY=1)

add_library(${module_name} MODULE ${CPP_SRC} ${QPP_SOURCES})

if (WIN32 AND MINGW AND MSYS)
    target_compile_definitions(${module_name} PUBLIC BUILDING_DLL)
endif (WIN32 AND MINGW AND MSYS)

add_dependencies(${module_name} QPP_GENERATED_FILES)

if (DEFINED ENV{DOXYGEN_EXECUTABLE})
    set(DOXYGEN_EXECUTABLE $ENV{DOXYGEN_EXECUTABLE})
endif()

qore_external_binary_module(${module_name} ${PROJECT_VERSION})

qore_dist(${PROJECT_VERSION})

# install qpp metadata files
if (COMMAND qore_install_qpp_metadata)
    qore_install_qpp_metadata(${module_name} ${QPP_META_SRC})
else()
    install(FILES ${QPP_META_SRC}
            DESTINATION ${CMAKE_INSTALL_DATADIR}/qore/metadata)
endif()

qore_config_info()

if (DOXYGEN_FOUND)
  qore_wrap_dox(QORE_DOX_SRC ${QORE_DOX_TMPL_SRC})
  add_custom_target(QORE_MOD_DOX_FILES DEPENDS ${QORE_DOX_SRC})
  add_dependencies(docs-module QORE_MOD_DOX_FILES)
endif()
