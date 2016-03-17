FIND_PACKAGE(QuaZip REQUIRED)

IF(MSVC)
ENDIF(MSVC)

SET(RSVGRENDERER_INCLUDE_DIR "" CACHE PATH "Path to rsvgrenderer header")
SET(RSVGRENDERER_LIBRARY "rsvgrenderer" CACHE FILEPATH "Path to rsvgrenderer library")

########################################################
# Files

SET (qgs_milx_SRCS
  qgsmilxplugin.cpp
  qgsmilxmaptools.cpp
  qgsmilxannotationitem.cpp
  qgsmilxlibrary.cpp
  qgsmilxio.cpp
  qgsmilxlayer.cpp
  MilXClient.cpp
)

SET (qgs_milx_HDRS
  milx_plugin.h
  qgsmilxio.h
)

SET (qgs_milx_MOC_HDRS
  qgsmilxplugin.h
  qgsmilxmaptools.h
  qgsmilxannotationitem.h
  qgsmilxlayer.h
  qgsmilxlibrary.h
  MilXClient.hpp
)

SET (qgs_milx_UIS
)

SET (qgs_milx_RCCS
  qgsmilxplugin.qrc
)

IF (WITH_BINDINGS)
    ADD_SUBDIRECTORY(python)
ENDIF (WITH_BINDINGS)

########################################################
# Build

QT4_WRAP_UI (qgs_milx_UIS_H  ${qgs_milx_UIS})

QT4_WRAP_CPP (qgs_milx_MOC_SRCS  ${qgs_milx_MOC_HDRS})

QT4_ADD_RESOURCES(qgs_milx_RCC_SRCS ${qgs_milx_RCCS})

ADD_LIBRARY (qgs_milx   SHARED ${qgs_milx_SRCS} ${qgs_milx_HDRS} ${qgs_milx_MOC_SRCS} ${qgs_milx_RCC_SRCS} ${qgs_milx_UIS_H})

INCLUDE_DIRECTORIES(
     ${CMAKE_CURRENT_BINARY_DIR}
     ${CMAKE_BINARY_DIR}/src/ui
     ${QJSON_INCLUDE_DIR}
     ${QUAZIP_INCLUDE_DIR}
     ${GDAL_INCLUDE_DIR}
     ${RSVGRENDERER_INCLUDE_DIR}
     ../../analysis
     ../../core
     ../../core/raster
     ../../core/geometry
     ../../gui
     ../
)

IF(WIN32)
    ADD_DEFINITIONS("\"-DQGS_MILX_EXPORT=${DLLEXPORT}\"")
ELSE(WIN32)
    ADD_DEFINITIONS("-DQGS_MILX_EXPORT=")
ENDIF(WIN32)

TARGET_LINK_LIBRARIES(qgs_milx
  qgis_core
  qgis_gui
  qgis_analysis
  ${qjson_LIBRARIES}
  ${QJSON_LIBRARIES}
  ${QUAZIP_LIBRARIES}
  ${RSVGRENDERER_LIBRARY}
)

########################################################
# Install

INSTALL(TARGETS qgs_milx
  RUNTIME DESTINATION ${QGIS_PLUGIN_DIR}
  LIBRARY DESTINATION ${QGIS_PLUGIN_DIR})
