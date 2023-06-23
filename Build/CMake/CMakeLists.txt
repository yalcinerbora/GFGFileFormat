cmake_minimum_required(VERSION 3.25)

# All CMake Related files is in the current folder
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}")

# Only Debug and Release
set(CMAKE_CONFIGURATION_TYPES Debug;Release CACHE STRING "Selected by user")
set(CMAKE_BUILD_TYPE Release CACHE STRING "Selected by user")

option(GFG_BUILD_MAYA_PLUG_IN "Build maya import/export plug-in" ON)
option(GFG_INSTALL "install gfg (usefull for externalProject_add" OFF)

# Globals Common for All Projects
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Ask for Maya include directiories
if(GFG_BUILD_MAYA_PLUG_IN)
    set(GFG_MAYA_INCLUDE_DIRECTORY "C:/Program Files/Autodesk/Maya2024/include" CACHE STRING "Maya plug-in include directory")
    set(GFG_MAYA_LIB_DIRECTORY "C:/Program Files/Autodesk/Maya2024/lib" CACHE STRING "Maya plug-in lib directory")
endif()


project(GPUFriendlyGraphics LANGUAGES CXX)

include(GNUInstallDirs)

# CMAKE Project Definitions and Source, Library Folders are separate
# Directories
set(GFG_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../..)
cmake_path(NORMAL_PATH GFG_DIRECTORY)

set(GFG_SOURCE_DIRECTORY ${GFG_DIRECTORY}Source)
set(GFG_LIB_DIRECTORY ${GFG_DIRECTORY}Lib)
set(GFG_LIB_INCLUDE_DIRECTORY ${GFG_DIRECTORY}Lib/Include)
set(GFG_CONFIG_LIB_DOC_DIRECTORY ${GFG_DIRECTORY}Lib/Docs)
set(GFG_BIN_DIRECTORY ${GFG_DIRECTORY}Bin)
set(GFG_EXT_DIRECTORY ${GFG_DIRECTORY}Ext)
set(GFG_RESOURCE_DIRECTORY ${GFG_DIRECTORY}Resources)
# Build is probably not used but its here for convenience
set(GFG_BUILD_DIRECTORY ${GFG_DIRECTORY}Build)
# Working Dir is used for debugging (currently shaders are here so...)
# For Visual Studio Projects this is copied to WorkingDir property for executables
set(GFG_WORKING_DIRECTORY ${GFG_DIRECTORY}WorkingDir)

# Determine Platform and Config
if(MSVC)
    set(GFG_PLATFORM_NAME Win)
    set(GFG_PREPROCESSOR_DEFS_GENERIC ${GFG_PREPROCESSOR_DEFS_GENERIC}
        -D_UNICODE
        -DUNICODE
        -DNOMINMAX)
    # Filter ALL_BUILD and ZERO_CHECK projects
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
    set(PREDEFINED_TARGETS_FOLDER "CustomTargets")
elseif(APPLE)
    set(GFG_PLATFORM_NAME Mac)
elseif(UNIX)
    set(GFG_PLATFORM_NAME Linux)
    include_directories(${CMAKE_CUDA_TOOLKIT_INCLUDE_DIRECTORIES})
else()
    message(FATAL_ERROR "Unknown platform... Terminating CMake.")
endif()

# Force Colored Output on Ninja
if ((CMAKE_GENERATOR MATCHES "Ninja") AND UNIX)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        add_compile_options(-fcolor-diagnostics)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        add_compile_options(-fdiagnostics-color=always)
    endif()
endif()

# Generic Compile Options
if(MSVC)
    # Warnings
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:/W3>
                        $<$<COMPILE_LANGUAGE:CXX>:/Zi>
                        $<$<COMPILE_LANGUAGE:CXX>:/external:W0>)
    add_link_options($<$<COMPILE_LANGUAGE:CXX>:/DEBUG>)
else()
    # Clang / GCC both use same syntax
    # Warnings
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wall>
                        $<$<COMPILE_LANGUAGE:CXX>:-Wextra>
                        $<$<COMPILE_LANGUAGE:CXX>:-Wpedantic>
                        $<$<COMPILE_LANGUAGE:CXX>:-g3>)
endif()

# Platform Specific Lib Bin Ext Folders
set(GFG_PLATFORM_LIB_DIRECTORY ${GFG_LIB_DIRECTORY}/${GFG_PLATFORM_NAME})
set(GFG_PLATFORM_BIN_DIRECTORY ${GFG_BIN_DIRECTORY}/${GFG_PLATFORM_NAME})
set(GFG_PLATFORM_EXT_DIRECTORY ${GFG_EXT_DIRECTORY}/${GFG_PLATFORM_NAME})
# Platform & Configurations Related Lib Bin folders
set(GFG_CONFIG_LIB_DIRECTORY ${GFG_PLATFORM_LIB_DIRECTORY}/$<CONFIG>)
set(GFG_CONFIG_BIN_DIRECTORY ${GFG_PLATFORM_BIN_DIRECTORY}/$<CONFIG>)
# Set cmake vars for output
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${GFG_CONFIG_BIN_DIRECTORY})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${GFG_CONFIG_BIN_DIRECTORY})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${GFG_CONFIG_BIN_DIRECTORY})
# Meta library directory
# All precompiled libraries are resides here
link_directories(${GFG_CONFIG_LIB_DIRECTORY}
                 ${GFG_PLATFORM_LIB_DIRECTORY}
                 $<$<BOOL:GFG_BUILD_MAYA_PLUG_IN>:${GFG_MAYA_LIB_DIRECTORY}>)

# Generic Preprocessor Definitions
set(GFG_PREPROCESSOR_DEFS_GENERIC
    ${GFG_PREPROCESSOR_DEFS_GENERIC}
    $<$<CONFIG:Debug>:GFG_DEBUG>
    $<$<CONFIG:Release>:NDEBUG>)

# Enable LTO / Device-LTO
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_DEBUG FALSE)

# BUILDS
# Libs
add_subdirectory(GFGFileIO)

# Maya Plug-in
if(GFG_BUILD_MAYA_PLUG_IN)
    add_subdirectory(GFGMaya)
endif()


# Install Config
if(GFG_INSTALL)

    # From the cmake docs tutorial
    # TODO: Find out how to create config file for both targets
    # Currently it only does for GFG lib but not GFGMaya
    include(CMakePackageConfigHelpers)

    set(GFG_CONFIG_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/GFGFileIO)

    # generate the config file that includes the exports
    configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/GFGFileIOConfig.cmake.in
            "${CMAKE_CURRENT_BINARY_DIR}/GFGFileIOConfig.cmake"
            INSTALL_DESTINATION ${GFG_CONFIG_DIR}
            NO_CHECK_REQUIRED_COMPONENTS_MACRO
    )

    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/GFGFileIOConfig.cmake"
            DESTINATION ${GFG_CONFIG_DIR})

    install(EXPORT GFGFileIOTargets
            FILE GFGFileIOTargets.cmake
            DESTINATION ${GFG_CONFIG_DIR})

endif()