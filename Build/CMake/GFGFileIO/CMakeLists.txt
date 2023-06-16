
set(CURRENT_SOURCE_DIR ${GFG_SOURCE_DIRECTORY}/GFG)

set(SRC_HEADER_STRUCTS
    ${CURRENT_SOURCE_DIR}/GFGAnimationHeader.h
    ${CURRENT_SOURCE_DIR}/GFGHeader.cpp
    ${CURRENT_SOURCE_DIR}/GFGHeader.h
    ${CURRENT_SOURCE_DIR}/GFGMaterialHeader.h
    ${CURRENT_SOURCE_DIR}/GFGMeshHeader.h
    ${CURRENT_SOURCE_DIR}/GFGSkeletonHeader.h)

set(SRC_COMMON
    ${CURRENT_SOURCE_DIR}/GFGConversion.cpp
    ${CURRENT_SOURCE_DIR}/GFGConversion.h
    ${CURRENT_SOURCE_DIR}/GFGEnumerations.h
    ${CURRENT_SOURCE_DIR}/GFGFileExporter.cpp
    ${CURRENT_SOURCE_DIR}/GFGFileExporter.h
    ${CURRENT_SOURCE_DIR}/GFGFileLoader.cpp
    ${CURRENT_SOURCE_DIR}/GFGFileLoader.h
    ${CURRENT_SOURCE_DIR}/GFGMaterialTypes.h
    ${CURRENT_SOURCE_DIR}/GFGVertexElementTypes.cpp
    ${CURRENT_SOURCE_DIR}/GFGVertexElementTypes.h)

set(EXPORT_HEADERS
    ${CURRENT_SOURCE_DIR}/GFGAnimationHeader.h
    ${CURRENT_SOURCE_DIR}/GFGHeader.h
    ${CURRENT_SOURCE_DIR}/GFGMaterialHeader.h
    ${CURRENT_SOURCE_DIR}/GFGMeshHeader.h
    ${CURRENT_SOURCE_DIR}/GFGSkeletonHeader.h
    ${CURRENT_SOURCE_DIR}/GFGConversion.h
    ${CURRENT_SOURCE_DIR}/GFGEnumerations.h
    ${CURRENT_SOURCE_DIR}/GFGFileExporter.h
    ${CURRENT_SOURCE_DIR}/GFGFileLoader.h
    ${CURRENT_SOURCE_DIR}/GFGMaterialTypes.h
    ${CURRENT_SOURCE_DIR}/GFGVertexElementTypes.h)

set(SRC_ALL
    ${SRC_HEADER_STRUCTS}
    ${SRC_COMMON})

source_group("HeaderStructs" FILES ${SRC_HEADER_STRUCTS})

source_group("" FILES ${SRC_COMMON})

# TBB for std::execution (clang & GCC)
# if(MSVC)
#     set(PLATFORM_SPEC_LIBRARIES)
# elseif(UNIX)
#     set(PLATFORM_SPEC_LIBRARIES tbb)
# endif()

# DLL File
add_library(GFGFileIO STATIC)

target_sources(GFGFileIO PRIVATE ${SRC_ALL})

target_sources(GFGFileIO PUBLIC FILE_SET HEADERS BASE_DIRS ${CURRENT_SOURCE_DIR} FILES ${EXPORT_HEADERS})

target_include_directories(GFGFileIO PRIVATE
                           ${GFG_SOURCE_DIRECTORY})

target_include_directories(GFGFileIO SYSTEM PRIVATE

                           ${GFG_LIB_INCLUDE_DIRECTORY})

target_compile_definitions(GFGFileIO PRIVATE ${GFG_PREPROCESSOR_DEFS_GENERIC})

target_compile_options(GFGFileIO PRIVATE ${GFG_COMPILER_FLAGS_GENERIC})

set_target_properties(GFGFileIO PROPERTIES
                      POSITION_INDEPENDENT_CODE ON)

target_link_libraries(GFGFileIO
                      ${PLATFORM_SPEC_LIBRARIES})

# Installation
if(GFG_INSTALL)
    install(TARGETS GFGFileIO
            DESTINATION ${CMAKE_INSTALL_LIBDIR})
    install(TARGETS GFGFileIO FILE_SET HEADERS
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/gfg)
endif()