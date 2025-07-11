cmake_minimum_required(VERSION 3.19)
project(OpenParEMg LANGUAGES C CXX)

set(CMAKE_VERBOSE_MAKEFILE 1)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 11)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets)
find_package(OpenGL REQUIRED) #except QOpenGLWidget, which is handled separately below (include and lib)

include_directories(${CMAKE_CURRENT_SOURCE_DIR} /opt/Qt/6.9.0/gcc_64/include/QtOpenGLWidgets /home/briany/OpenParEM/src/OpenParEM3D /home/briany/OpenParEM/include /home/briany/OpenParEM/include/opencascade /home/briany/OpenParEM/lib /home/briany/OpenParEM/src/OpenParEMCommon /usr/lib/gcc/x86_64-linux-gnu/9/include /usr/include/GL)


# Open CASCADE Technology
find_package(OpenCASCADE REQUIRED NO_DEFAULT_PATH)
if(OpenCASCADE_FOUND)
  message (STATUS "Using OpenCASCADE from \"${OpenCASCADE_DIR}\"" )
  INCLUDE_DIRECTORIES(${OpenCASCADE_INCLUDE_DIR})
  LINK_DIRECTORIES(${OpenCASCADE_LIBRARY_DIR})
else()
  message(WARNING "Could not find OpenCASCADE, please set OpenCASCADE_DIR variable." )
  set(OCCT_LIBRARY_DIR)
  set(OCCT_BIN_DIR)
endif()

set(OpenCASCADE_LIBS TKRWMesh TKDEGLTF TKDEOBJ TKDEPLY TKBinXCAF TKBin TKBinL TKOpenGl TKXCAF TKVCAF TKCAF TKV3d TKHLR TKMesh TKService TKShHealing TKPrim TKTopAlgo TKGeomAlgo TKBRep TKGeomBase TKG3d TKG2d TKMath TKLCAF TKCDF TKernel)


qt_standard_project_setup()

qt_add_executable(OpenParEMg
    WIN32 MACOSX_BUNDLE
    OpenParEMg.cpp
    OPEMg.cpp
    OPEMg.h
    OPEMg.ui
    ../OpenParEM3D/project.c
    ../OpenParEM3D/project.h
    ../OpenParEMCommon/prefix.c
    ../OpenParEMCommon/prefix.h

    MeshOptions.h MeshOptions.cpp MeshOptions.ui

    SimulateOptions.h SimulateOptions.cpp SimulateOptions.ui
    license.h license.cpp license.ui
    ../OpenParEMCommon/LICENSE
    CustomLineEdit.h
    FrequencyPlanG.h FrequencyPlanG.cpp FrequencyPlanG.ui
    FrequencyView.h FrequencyView.cpp FrequencyView.ui
    ../OpenParEMCommon/frequencyPlan.hpp
    ../OpenParEMCommon/frequencyPlan.cpp
    Refinement.h Refinement.cpp Refinement.ui
    Materials.h Materials.cpp Materials.ui
    ../OpenParEMCommon/OpenParEMmaterials.cpp ../OpenParEMCommon/OpenParEMmaterials.hpp
    ../OpenParEMCommon/misc.cpp ../OpenParEMCommon/misc.hpp
    ../OpenParEMCommon/keywordPair.cpp ../OpenParEMCommon/keywordPair.hpp
    ../OpenParEMCommon/fem.cpp ../OpenParEMCommon/fem.hpp
    CustomOpenGLWidget.h CustomOpenGLWidget.cpp
    #TestOpenGLWidget.h TestOpenGLWidget.cpp
    #BreptViewerWidget.h BreptViewerWidget.cpp
)

add_library(Qt6OpenGLWidgets SHARED IMPORTED)
set_target_properties(Qt6OpenGLWidgets PROPERTIES IMPORTED_LOCATION "/opt/Qt/6.9.0/gcc_64/lib/libQt6OpenGLWidgets.so")

target_link_libraries(OpenParEMg
    PRIVATE
        Qt::Core
        Qt::Widgets
        ${OpenCASCADE_LIBS}
        Qt6OpenGLWidgets
        #freetype
)

include(GNUInstallDirs)

install(TARGETS OpenParEMg
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET OpenParEMg
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
