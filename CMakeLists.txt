cmake_minimum_required(VERSION 3.16)

set(kst_version_major 2)
if(kst_version_string)
    set(kst_version ${kst_version_string})
else()
    set(kst_version_minor 1)
    #set(kst_version_patch x)
    set(kst_version ${kst_version_major}.${kst_version_minor})
    #set(kst_version ${kst_version_major}.${kst_version_minor}.${kst_version_patch})
    set(kst_version_string ${kst_version})
endif()

project(Kst VERSION ${kst_version})

if(kst_clang)
    set(CMAKE_C_COMPILER    "clang")
    SET (CMAKE_CXX_COMPILER "/usr/bin/clang++")
    message(STATUS "Using clang  ${CMAKE_CXX_COMPILER}")
endif()

# figure out if we use MinGW with Ninja: call with -DCC=gcc
if(WIN32 AND CMAKE_GENERATOR MATCHES Ninja AND CC MATCHES gcc)
    find_file(mingw_make mingw32-make.exe)
    get_filename_component(mingw_bin_dir ${mingw_make} PATH)
    set(CMAKE_RC_COMPILER ${mingw_bin_dir}/windres.exe)
endif()

include(FeatureSummary)

find_package(ECM ${KF_MIN_VERSION} NO_MODULE)
set_package_properties(ECM PROPERTIES
    TYPE REQUIRED
    URL "https://api.kde.org/ecm/"
    DESCRIPTION "extra cmake modules")

set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules")

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)

include(KstMacros)
include(KDEInstallDirs)

get_filename_component(kst_dir ${CMAKE_SOURCE_DIR} ABSOLUTE)
get_filename_component(kst_build_dir ${CMAKE_BINARY_DIR}/build ABSOLUTE)

set(kst_binary_name kst2)

set(kst_install_plugins "${CMAKE_INSTALL_LIBDIR}/kst2" CACHE PATH "Kst plugin installation directory")

kst_option_init()
kst_option(string ""  all version_string    "Version string")
kst_option(bool   ON  all release           "Build release version: optimize for speed, don't embedded debug symbols")
kst_option(bool   OFF all merge_files       "Merge files to speedup build")
kst_option(bool   OFF all merge_rebuild     "Rebuild generated files from merged files build")
kst_option(bool   OFF all verbose           "Make verbose CMake run and Makefiles")
kst_option(bool   ON  all 3rdparty          "Build plugins depending on 3rd party libraries")
kst_option(bool   ON  all dataobjects       "Build dataobject plugins")
kst_option(bool   OFF all test              "Build unit tests")
kst_option(bool   ON  all pch               "Use precompiled headers")
kst_option(bool   ON  all svnversion        "Use svnversion's output for Kst's version information")
kst_option(bool   OFF gcc rpath             "Use rpath")
kst_option(bool   OFF all 3rdparty_build    "Download and build 3rd party libraries")
kst_option(bool   OFF win 3rdparty_download "Download precompiled 3rd party libraries")
kst_option(bool   OFF win console           "Open console on Windows")
kst_option(bool   OFF win edit_cont         "Enable \"Edit and Continue\" for Visual Studio")
kst_option(bool   OFF all python            "Install Python support")
#kst_option(bool   OFF all python_depend     "Download and build NumPy/SciPy then install Python support")
#kst_option(string ""  all python_prefix     "Path for installing python bindings")
kst_option(string OFF all cross             "Cross-compile on Linux for Windows")
#kst_option(bool   OFF  all qt5base_only      "Only use Qt5 features available in qtbase")
kst_option(bool   OFF all clang             "Use Clang compiler")
kst_option(bool   OFF all sanitize          "Use Clang's sanitizers")
kst_option(bool   OFF all dbgsym            "Build with debug symbols enabled")
kst_option(bool   OFF win noinstaller       "Don't build installer")

message(STATUS)

if(kst_cross)
    include(KstCrossCompile)
endif()


if(kst_release)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DQT_NO_WARNING_OUTPUT")
else()
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()


if(kst_sanitize)
    include(Clang-sanitize)
    set(clang_flags "${clang_flags}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${clang_flags}")
endif()


if(CMAKE_COMPILER_IS_GNUCC)
    execute_process(COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)
    message(STATUS "Using GCC version ${GCC_VERSION}")
endif()


if(kst_3rdparty_build)
    set(kst_3rdparty_dir ${CMAKE_BINARY_DIR}/3rdparty/installed)
    add_subdirectory(cmake/3rdparty)
elseif(kst_3rdparty_download)
    if(MINGW)
        if(GCC_VERSION VERSION_EQUAL 4.7.2 OR GCC_VERSION VERSION_GREATER 4.7.2)
            set(ver -dw2-4.7.2)
            set(md5 53edc4009d5c3ee5e11d1ba24cd8ae61)
        elseif(GCC_VERSION VERSION_EQUAL 4.7 OR GCC_VERSION VERSION_GREATER 4.7)
            set(ver 4.7)
            set(md5 de6e8dbab1bb17eee6057941fddc93e3)
        elseif(GCC_VERSION VERSION_EQUAL 4.6 OR GCC_VERSION VERSION_GREATER 4.6)
            set(ver 4.6)
            set(md5 70d8670af9c21eb8fb466654c95d8e4d)
        elseif(GCC_VERSION VERSION_GREATER 4.4)
            set(ver 4.4)
            set(md5 999248fb40a44543af4dd4cd1be0ceeb)
        else()
            message(FATA_ERROR "MinGW version ${GCC_VERSION} not supported")
        endif()
    endif()
    message(STATUS "Checking downloaded 3rd party binaries.")
    set(_deps kst-3rdparty-win32-gcc${ver})
    set(_file ${_deps}.zip)
    set(_downloaded _downloaded-NOTFOUND CACHE PATH "3rd party binaries already downloaded" FORCE)
    find_file(_downloaded ${_file} ${CMAKE_BINARY_DIR})
    if (NOT _downloaded)
        file(DOWNLOAD http://sourceforge.net/projects/kst/files/3rdparty/${_file}
            ${CMAKE_BINARY_DIR}/${_file}
            EXPECTED_MD5 ${md5}
            SHOW_PROGRESS)
        execute_process(COMMAND ${CMAKE_COMMAND} -E tar -xfj ${CMAKE_BINARY_DIR}/${_file})
    endif()
    set(kst_3rdparty_dir ${CMAKE_BINARY_DIR}/${_deps})
endif()


kst_revision_project_name(Revision)
add_definitions(-DKST_HAVE_REVISION_H)
if(kst_revision)
    file(WRITE ${CMAKE_BINARY_DIR}/kstrevision.h.tmp "#define KST_REVISION \"${kst_revision}\"\n")
    configure_file(${CMAKE_BINARY_DIR}/kstrevision.h.tmp ${CMAKE_BINARY_DIR}/kstrevision.h COPYONLY)
elseif(kst_svnversion)
    include(KstRevisionHeader)
    KstRevisionHeader(${kst_dir} ${kst_revision_project} ${CMAKE_BINARY_DIR}/kstrevision.h _modified)
else()
    file(WRITE ${CMAKE_BINARY_DIR}/kstrevision.h.tmp "#define KST_REVISION \"unknown\"\n")
    configure_file(${CMAKE_BINARY_DIR}/kstrevision.h.tmp ${CMAKE_BINARY_DIR}/kstrevision.h COPYONLY)
endif()


# Find 3rd party libraries
if(kst_3rdparty)
  message(STATUS "3rd party libs for plugins--------------------")
  find_package(Getdata)
  find_package(GSL)
  find_package(Netcdf)
  find_package(Matio)
  find_package(CFITSIO)
  find_package(TIFF)
  find_package(HDF5)
  message(STATUS "----------------------------------------------")
else()
  message(STATUS "Building plugins depending on 3rd party libraries suppressed")
endif()
message(STATUS)

#set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DQT_NO_DEBUG_OUTPUT -DQT_NO_WARNING_OUTPUT")


# Find 3rd party libraries
# http://doc-snapshot.qt-project.org/5.0/cmake-manual.htm
if(kst_cross)
    # Cross-compiled Qt(?)
    set(QT_MINGW_DIR ${kst_qt5} CACHE PATH "Qt for Mingw" FORCE)
    set(CMAKE_FIND_ROOT_PATH /usr/${TOOLNAME} ${QT_MINGW_DIR} CACHE PATH "Mingw find root path" FORCE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++11")
endif()
macro(use_qt5lib qt5lib)
    find_package(${qt5lib} REQUIRED)
    include_directories(${${qt5lib}_INCLUDE_DIRS})
endmacro()
use_qt5lib(Qt5Core)
get_target_property(qmakebin Qt5::qmake IMPORTED_LOCATION)
get_filename_component(QT_BINARY_DIR ${qmakebin} PATH)
message(STATUS "Using Qt5 from ${QT_BINARY_DIR}")
set(CMAKE_PREFIX_PATH ${QT_BINARY_DIR}/..)
use_qt5lib(Qt5Concurrent)
use_qt5lib(Qt5Widgets)
use_qt5lib(Qt5Network)
use_qt5lib(Qt5Xml)
use_qt5lib(Qt5PrintSupport)
use_qt5lib(Qt5Svg)

add_definitions(-DQT_DISABLE_DEPRECATED_BEFORE)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DQT_NO_DEBUG")
include_directories(${Qt5Gui_PRIVATE_INCLUDE_DIRS}) # Qt5 bug?

#linguist tools
find_package(Qt5LinguistTools)
if(Qt5LinguistTools_FOUND)
    find_package(Qt5LinguistForKst)
endif()

message(STATUS)


# React on options


if(kst_verbose)
    set(CMAKE_VERBOSE_MAKEFILE 1)
else()
    set(CMAKE_VERBOSE_MAKEFILE 0)
endif()

if(NOT kst_console)
    set(kst_win32 WIN32)
    if(WIN32)
        set(kst_qtmain_library Qt5::WinMain)
    endif()
endif()

if(APPLE AND NOT CMAKE_GENERATOR STREQUAL Xcode)
    set(kst_win32 MACOSX_BUNDLE)
endif()

if(APPLE OR kst_clang)
    set(kst_pch 0 CACHE BOOL "Disable pch on Mac, not supported yet" FORCE)
endif()

if (APPLE)
    set(CMAKE_MACOSX_RPATH 1)
endif()

if(kst_merge_files)
    set(kst_pch 0 CACHE BOOL "Disable pch because we build merged" FORCE)
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(x64 TRUE)
endif()


if(WIN32)
    add_definitions(-D_WIN32_WINNT=0x0501) # Windows XP
    # on 32 bit Windows we must explicitly enable >2GB support
    # maybe you have to ' 4-gigabyte tune' your Windows XP 32 bit system:
    # http://msdn.microsoft.com/en-us/library/windows/desktop/aa366778%28v=vs.85%29.aspx
    macro(large_address var flag)
        set(${var} "${${var}} ${flag}")
    endmacro()
    macro(link_large_address flag)
        large_address(CMAKE_EXE_LINKER_FLAGS    ${flag})
        large_address(CMAKE_SHARED_LINKER_FLAGS ${flag})
        large_address(CMAKE_MODULE_LINKER_FLAGS ${flag})
    endmacro()
    if(MSVC)
        link_large_address(/LARGEADDRESSAWARE)
    elseif(NOT x64)
        link_large_address(-Wl,--large-address-aware)
    endif()
endif()

if(MSVC)
    add_definitions(-D_USE_MATH_DEFINES -DNOMINMAX)
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
    if(kst_dbgsym)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
    endif()
endif()

if(MSVC_IDE AND kst_edit_cont)
    # Enable <Edit and Continue> in Visual Studio
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /ZI")
    if(kst_pch)
        # Studio bug: <Visual Studio 2005 "Force Includes" Breaks Edit and Continue with Pre-compiled Headers>
        set(kst_pch 0 CACHE BOOL "Disable pch because of Edit&Continue" FORCE)
    endif()
endif()



if(MSVC)
    set(kst_debug_postfix d)
endif()

configure_file(${kst_dir}/cmake/config.h.cmake ${CMAKE_BINARY_DIR}/config.h)

include_directories(${CMAKE_BINARY_DIR} ${QT_INCLUDES})

message(STATUS "Build summary----------------------------------")
message(STATUS)
message(STATUS "Version       : ${kst_version}")
message(STATUS "Path to source: ${kst_dir}")
message(STATUS "Used options:")
kst_option_list_all()
message(STATUS "-----------------------------------------------")
message(STATUS)


add_subdirectory(src)

if(kst_test)
    enable_testing()
    add_subdirectory(tests)
endif()

if (NOT APPLE)
    add_subdirectory(misc)
endif()

if(kst_python)
    add_subdirectory(pyKst)
endif()

#adapted from [trojita.git] / CMakeLists.txt
if(Qt5LinguistForKst_FOUND)
    file(GLOB_RECURSE kst_PO "${CMAKE_SOURCE_DIR}/po/kst_common_*.po")
    qt5_wrap_po(kst_QM ${kst_PO})
    set(language_summary "")
    foreach(po ${kst_PO})
        string(REGEX REPLACE "^(.*)/kst_common_(.*).po" "\\2" lang ${po})
        list(APPEND language_summary ${lang})
    endforeach()
    list(SORT language_summary)
    list(LENGTH language_summary num_languages)
    if(num_languages)
        message(STATUS "Available languages: ${language_summary}")
        add_custom_target(qm DEPENDS ${kst_QM})
        add_dependencies(${kst_binary_name} qm)
        install(DIRECTORY ${CMAKE_BINARY_DIR}/locale/ DESTINATION share/kst/locale FILES_MATCHING PATTERN *.qm)
    else()
        message(STATUS "No .po files found, will not install any languages")
    endif()
else()
    message(STATUS "Qt Linguist (lupdate/lrelease/lconvert) not found, disabling localization support")
endif()
