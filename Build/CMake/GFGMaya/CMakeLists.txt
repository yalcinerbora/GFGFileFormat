
set(CURRENT_SOURCE_DIR "${GFG_SOURCE_DIRECTORY}/Autodesk Maya")

# Add Mel Scripts as well for editing
set(SRC_MEL_SCRIPTS
    ${CURRENT_SOURCE_DIR}/GFGReDeploy.mel
    ${CURRENT_SOURCE_DIR}/GFGOptsExport.mel
    ${CURRENT_SOURCE_DIR}/GFGOptsImport.mel
    ${CURRENT_SOURCE_DIR}/GFGErrorWindow.mel)

set(SRC_COMMON
    ${CURRENT_SOURCE_DIR}/GFGMayaAnimation.cpp
    ${CURRENT_SOURCE_DIR}/GFGMayaAnimation.h
    ${CURRENT_SOURCE_DIR}/GFGMayaConversions.cpp
    ${CURRENT_SOURCE_DIR}/GFGMayaConversions.h
    ${CURRENT_SOURCE_DIR}/GFGMayaGraphIterator.cpp
    ${CURRENT_SOURCE_DIR}/GFGMayaGraphIterator.h
    ${CURRENT_SOURCE_DIR}/GFGMayaOptions.cpp
    ${CURRENT_SOURCE_DIR}/GFGMayaOptions.h
    ${CURRENT_SOURCE_DIR}/GFGMayaStructures.h
    ${CURRENT_SOURCE_DIR}/GFGTranslatorMaya.cpp
    ${CURRENT_SOURCE_DIR}/GFGTranslatorMaya.h)

set(SRC_ALL ${SRC_COMMON} ${SRC_MEL_SCRIPTS})

source_group("" FILES ${SRC_COMMON})
source_group("MelScripts" FILES ${SRC_MEL_SCRIPTS})

# TBB for std::execution (clang & GCC)
# if(MSVC)
#     set(PLATFORM_SPEC_LIBRARIES)
# elseif(UNIX)
#     set(PLATFORM_SPEC_LIBRARIES tbb)
# endif()

# DLL File
add_library(GFGMaya SHARED)

target_sources(GFGMaya PRIVATE ${SRC_ALL})

target_include_directories(GFGMaya PRIVATE
                           ${GFG_SOURCE_DIRECTORY})

target_include_directories(GFGMaya SYSTEM PRIVATE
                           ${GFG_LIB_INCLUDE_DIRECTORY}
                           ${GFG_MAYA_INCLUDE_DIRECTORY})

target_compile_definitions(GFGMaya PRIVATE ${GFG_PREPROCESSOR_DEFS_GENERIC})

target_compile_options(GFGMaya PRIVATE ${GFG_COMPILER_FLAGS_GENERIC})

set_target_properties(GFGMaya PROPERTIES
                      POSITION_INDEPENDENT_CODE ON)

target_link_libraries(GFGMaya
                        PRIVATE
                            OpenMayaAnim
                            OpenMaya
                            Foundation
                            GFGFileIO
                      ${PLATFORM_SPEC_LIBRARIES})

# Only expose MLL and scripts
# if(GFG_INSTALL)
#     install(TARGETS GFGMaya
#             DESTINATION ${CMAKE_INSTALL_BINDIR})
#     install(FILES ${SRC_MEL_SCRIPTS}
#             DESTINATION ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR})
# endif()