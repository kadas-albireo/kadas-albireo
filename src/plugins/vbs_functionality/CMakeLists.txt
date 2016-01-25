FIND_PACKAGE(QuaZip REQUIRED)

IF(MSVC)
ADD_SUBDIRECTORY(milix/Server)
ENDIF(MSVC)

SET(RSVGRENDERER_INCLUDE_DIR "" CACHE PATH "Path to rsvgrenderer header")
SET(RSVGRENDERER_LIBRARY "rsvgrenderer" CACHE FILEPATH "Path to rsvgrenderer library")

########################################################
# Files

SET (vbsfunctionality_SRCS
  qgsvbsfunctionality.cpp
  milix/qgsvbsmaptoolmilix.cpp
  milix/qgsvbsmilixannotationitem.cpp
  milix/qgsvbsmilixlibrary.cpp
  milix/qgsvbsmilixmanager.cpp
  milix/qgsvbsmilixio.cpp
  milix/Client/VBSMilixClient.cpp
  ovl/qgsvbsovlimporter.cpp
)

SET (vbsfunctionality_HDRS
  vbsfunctionality_plugin.h
  milix/qgsvbsmaptoolmilix.h
  milix/qgsvbsmilixannotationitem.h
  milix/qgsvbsmilixio.h
  ovl/qgsvbsovlimporter.h
)

SET (vbsfunctionality_MOC_HDRS
  qgsvbsfunctionality.h
  milix/qgsvbsmilixlibrary.h
  milix/qgsvbsmilixmanager.h
  milix/Client/VBSMilixClient.hpp
)

SET (vbsfunctionality_UIS
)

SET (vbsfunctionality_RCCS
  qgsvbsfunctionality.qrc
)

########################################################
# Build

QT4_WRAP_UI (vbsfunctionality_UIS_H  ${vbsfunctionality_UIS})

QT4_WRAP_CPP (vbsfunctionality_MOC_SRCS  ${vbsfunctionality_MOC_HDRS})

QT4_ADD_RESOURCES(vbsfunctionality_RCC_SRCS ${vbsfunctionality_RCCS})

ADD_LIBRARY (vbsfunctionality   MODULE ${vbsfunctionality_SRCS} ${vbsfunctionality_HDRS} ${vbsfunctionality_MOC_SRCS} ${vbsfunctionality_RCC_SRCS} ${vbsfunctionality_UIS_H})

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

TARGET_LINK_LIBRARIES(vbsfunctionality
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

INSTALL(TARGETS vbsfunctionality
  RUNTIME DESTINATION ${QGIS_PLUGIN_DIR}
  LIBRARY DESTINATION ${QGIS_PLUGIN_DIR})
