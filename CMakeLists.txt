# Copyright (c) 2010 Fizians SAS. <http://www.fizians.com>
# This file is part of Rozofs.
#
# Rozofs is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published
# by the Free Software Foundation, version 2.
#
# Rozofs is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see
# <http://www.gnu.org/licenses/>.

#
# Project settings.
#
cmake_minimum_required(VERSION 2.6)
project(rozofs C)
set(MAJOR "1")
set(MINOR "3")
set(REVISION "2")
set(VERSION "${MAJOR}.${MINOR}.${REVISION}")
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${PROJECT_SOURCE_DIR}/cmake")

#
# Find packages
#
find_package(UUID REQUIRED)
find_package(CRYPT REQUIRED)
find_package(CONFIG REQUIRED)
find_package(XATTR REQUIRED)
find_package(PTHREAD REQUIRED)
find_package(FUSE REQUIRED)
find_package(READLINE REQUIRED)
find_package(SWIG REQUIRED)
find_package(Doxygen QUIET)
find_program(PYTHON NAMES python2.7)
find_program(DEBUILD NAMES debuild)
find_program(DEBUILD NAMES debuild)
find_program(MAKEINFO NAMES makeinfo)
find_program(TEXI2DVI NAMES texi2dvi)
find_program(TEXI2PDF NAMES texi2pdf)

IF(CMAKE_COMPILER_IS_GNUCC)
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fmessage-length=0")
ENDIF(CMAKE_COMPILER_IS_GNUCC)
IF(CMAKE_COMPILER_IS_GNUCXX)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fmessage-length=0")
ENDIF(CMAKE_COMPILER_IS_GNUCXX)

#
# Project config.
#
if(NOT SYSCONFDIR)
    set(SYSCONFDIR "${CMAKE_INSTALL_PREFIX}/etc")
endif(NOT SYSCONFDIR)

set(ROZOFS_CONFIG_DIR "${SYSCONFDIR}/rozofs")

set(EXPORTD_DEFAULT_CONFIG "${ROZOFS_CONFIG_DIR}/export.conf")

set(STORAGED_DEFAULT_CONFIG "${ROZOFS_CONFIG_DIR}/storage.conf")

#if(NOT ROZOFS_SHELL_DIR)
#    set(ROZOFS_SHELL_DIR "/usr/local/bin")
#endif(NOT ROZOFS_SHELL_DIR)
#
#if(NOT ROZOFS_BIN_DIR)
#    set(ROZOFS_BIN_DIR "/usr/local/bin")
#endif(NOT ROZOFS_BIN_DIR)
#
#set(STORCLI_STARTER "${ROZOFS_SHELL_DIR}/storcli_starter.sh")
#set(STORCLI_KILLER "${ROZOFS_SHELL_DIR}/storcli_killer.sh")
#set(STORCLI_EXEC "${ROZOFS_BIN_DIR}/storcli")

if(NOT DAEMON_PID_DIRECTORY)
    set(DAEMON_PID_DIRECTORY "/var/run")
endif(NOT DAEMON_PID_DIRECTORY)

if(NOT ROZOFS_RPC_BUFFER_SIZE)
    set(ROZOFS_RPC_BUFFER_SIZE 0x0)
endif(NOT ROZOFS_RPC_BUFFER_SIZE)

string(REGEX MATCH "$\\\\/" TRAILING_SLASH "${DAEMON_PID_DIRECTORY}")
if(NOT TRAILING_SLASH)
    set(DAEMON_PID_DIRECTORY "${DAEMON_PID_DIRECTORY}/")
endif(NOT TRAILING_SLASH)

if(NOT DEBIAN_LAYOUT)
    set(DEBIAN_LAYOUT OFF)
endif(NOT DEBIAN_LAYOUT)

add_subdirectory(rozofs)
add_subdirectory(src)
add_subdirectory(doc)
add_subdirectory(tests)
add_subdirectory(tests/fs_ops/fileop)
add_subdirectory(tests/fs_ops/pjd-fstest)
add_subdirectory(manager)

set(ROZOFS_PACKAGE_FILE_NAME "rozofs-${VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_HOST_SYSTEM_PROCESSOR}")

set(ROZOFS_SOURCE_PACKAGE_FILE_NAME "rozofs-${VERSION}")

configure_file("${PROJECT_SOURCE_DIR}/cmake/config.h.in" "${PROJECT_BINARY_DIR}/config.h")
configure_file("${PROJECT_SOURCE_DIR}/cmake/uninstall.in" "${PROJECT_BINARY_DIR}/uninstall.cmake" @ONLY)
configure_file("${PROJECT_SOURCE_DIR}/cmake/debian.in" "${PROJECT_BINARY_DIR}/debian.cmake" @ONLY)
configure_file("${PROJECT_SOURCE_DIR}/cmake/debian.changelog.in" "${PROJECT_SOURCE_DIR}/debian/changelog" @ONLY)
configure_file("${PROJECT_SOURCE_DIR}/cmake/redhat.in" "${PROJECT_BINARY_DIR}/redhat.cmake" @ONLY)
configure_file("${PROJECT_SOURCE_DIR}/cmake/rozofs.spec.in" "${PROJECT_BINARY_DIR}/rozofs.spec" @ONLY)
configure_file("${PROJECT_SOURCE_DIR}/cmake/doxyfile.in" "${PROJECT_BINARY_DIR}/doc/doxyfile" IMMEDIATE @ONLY)
configure_file("${PROJECT_SOURCE_DIR}/cmake/setup.py.in" "${PROJECT_BINARY_DIR}/manager/setup.py" @ONLY)

#
# Build settings
#
if(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE Release CACHE STRING
  "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
  FORCE)
endif(NOT CMAKE_BUILD_TYPE)

set(CMAKE_C_FLAGS "-D_GNU_SOURCE" CACHE STRING
    "Flags used by the compiler during all build types."
    FORCE)

if(CMAKE_SIZEOF_VOID_P MATCHES 4) 
  message("-- 32 bits target")
  set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG -D_GNU_SOURCE  -g  -Wno-int-to-pointer-cast -Wno-pointer-to-int-cast" CACHE STRING
    "Flags used by the C compiler during maintainer builds."
    FORCE)
else()
  message("-- 64 bits target")    
  set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG -D_GNU_SOURCE  -g" CACHE STRING
    "Flags used by the C compiler during maintainer builds."
    FORCE)
endif()

set(CMAKE_C_FLAGS_DEBUG "-g -Wall -DNDEBUGFUNCTION -D_GNU_SOURCE " CACHE STRING
    "Flags used by the C compiler during maintainer builds."
    FORCE)

#
# uninstall target
#
add_custom_target(uninstall "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/uninstall.cmake")
add_custom_target(package_debian "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/debian.cmake")
add_custom_target(package_redhat "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/redhat.cmake")

#
# Packing
#
include(InstallRequiredSystemLibraries)

# Binaries
set(CPACK_GENERATOR "")
set(CPACK_STGZ_COMPONENT_INSTALL ON)
set(CPACK_TGZ_COMPONENT_INSTALL ON)
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Rozofs filesystem")
set(CPACK_PACKAGE_VENDOR "Fizians SAS")
set(CPACK_PACKAGE_CONTACT "Fizians SAS")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${PROJECT_SOURCE_DIR}/README.md")
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/COPYING")
set(CPACK_PACKAGE_VERSION "${VERSION}")
set(CPACK_PACKAGE_FILE_NAME ""${ROZOFS_PACKAGE_FILE_NAME}"")
set(CPACK_COMPONENTS_ALL client export storage)
set(CPACK_COMPONENT_CLIENT_DISPLAY_NAME "rozofsmount")
set(CPACK_COMPONENT_EXPORT_DISPLAY_NAME "exportd")
set(CPACK_COMPONENT_STORAGE_DISPLAY_NAME "storaged")
set(CPACK_COMPONENT_CLIENT_DESCRIPTION "rozofsmount")
set(CPACK_COMPONENT_EXPORT_DESCRIPTION "exportd")
set(CPACK_COMPONENT_HEADERS_DESCRIPTION "storaged")

# Sources
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${ROZOFS_SOURCE_PACKAGE_FILE_NAME}")
set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_IGNORE_FILES "/config.h$;.mk$;/build/;tags;/CMakeFiles/;CMakeCache.txt;Makefile$;\\\\.vim$;\\\\.swp$;CPackConfig.cmake;CPackSourceConfig.cmake;~$;/nbproject/;/\\\\.git;/\\\\.project;/\\\\.cproject")
include(CPack)
