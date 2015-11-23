FIND_PACKAGE(QJSON REQUIRED)
FIND_PACKAGE(QuaZip REQUIRED)

IF(MSVC)
SET(CRASHRPT_INCLUDE_DIR "" CACHE PATH "Path to CrashRpt headers")
SET(CRASHRPT_LIBRARY "CrashRpt1403.lib" CACHE FILEPATH "Path to CrashRpt library")
ENDIF(MSVC)

########################################################
# Files

SET (vbsfunctionality_SRCS
  qgsvbsfunctionality.cpp
  qgsvbscoordinatedisplayer.cpp
  qgsvbscoordinateconverter.cpp
  qgsvbscrashhandler.cpp
  qgsvbscrsselection.cpp
  analysistools/qgsvbsslopetool.cpp
  analysistools/qgsvbshillshadetool.cpp
  analysistools/qgsvbsviewshedtool.cpp
  multimap/qgsvbsmapwidget.cpp
  multimap/qgsvbsmultimapmanager.cpp
  ovl/qgsvbsovlimporter.cpp
  pinannotation/qgsvbsmaptoolpinannotation.cpp
  pinannotation/qgsvbspinannotationitem.cpp
  search/qgsvbssearchbox.cpp
  search/qgsvbscoordinatesearchprovider.cpp
  search/qgsvbslocationsearchprovider.cpp
  search/qgsvbslocaldatasearchprovider.cpp
  search/qgsvbsremotedatasearchprovider.cpp
  search/qgsvbsworldlocationsearchprovider.cpp
)

SET (vbsfunctionality_HDRS
  vbsfunctionality_plugin.h
  qgsvbscoordinatedisplayer.h
  qgsvbscoordinateconverter.h
  ovl/qgsvbsovlimporter.h
  pinannotation/qgsvbsmaptoolpinannotation.h
)

SET (vbsfunctionality_MOC_HDRS
  qgsvbsfunctionality.h
  qgsvbscrashhandler.h
  qgsvbscoordinatedisplayer.h
  analysistools/qgsvbsslopetool.h
  analysistools/qgsvbshillshadetool.h
  analysistools/qgsvbsviewshedtool.h
  pinannotation/qgsvbspinannotationitem.h
  multimap/qgsvbsmapwidget.h
  multimap/qgsvbsmultimapmanager.h
  search/qgsvbssearchbox.h
  search/qgsvbssearchprovider.h
  search/qgsvbscoordinatesearchprovider.h
  search/qgsvbslocationsearchprovider.h
  search/qgsvbslocaldatasearchprovider.h
  search/qgsvbsremotedatasearchprovider.h
  search/qgsvbsworldlocationsearchprovider.h
  qgsvbscrsselection.h
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
     ../../analysis
     ../../core
     ../../core/raster
     ../../core/geometry
     ../../gui
     ../
     ${CRASHRPT_INCLUDE_DIR}
)

TARGET_LINK_LIBRARIES(vbsfunctionality
  qgis_core
  qgis_gui
  qgis_analysis
  ${qjson_LIBRARIES}
  ${QUAZIP_LIBRARIES}
  ${CRASHRPT_LIBRARY}
)

########################################################
# Install

INSTALL(TARGETS vbsfunctionality
  RUNTIME DESTINATION ${QGIS_PLUGIN_DIR}
  LIBRARY DESTINATION ${QGIS_PLUGIN_DIR})
