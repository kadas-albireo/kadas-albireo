FIND_PACKAGE(QJSON REQUIRED)

########################################################
# Files

SET (vbsfunctionality_SRCS
  qgsvbsfunctionality.cpp
  qgsvbscoordinatedisplayer.cpp
  qgsvbscoordinateconverter.cpp
  qgsvbscrashhandler.cpp
  qgsvbscrsselection.cpp
  pinannotation/qgsvbsmaptoolpinannotation.cpp
  pinannotation/qgsvbspinannotationitem.cpp
  search/qgsvbssearchbox.cpp
  search/qgsvbscoordinatesearchprovider.cpp
  search/qgsvbslocationsearchprovider.cpp
  search/qgsvbslocaldatasearchprovider.cpp
  search/qgsvbsremotedatasearchprovider.cpp
)

SET (vbsfunctionality_HDRS
  vbsfunctionality_plugin.h
  qgsvbscoordinatedisplayer.h
  qgsvbscoordinateconverter.h
  pinannotation/qgsvbsmaptoolpinannotation.h
)

SET (vbsfunctionality_MOC_HDRS
  qgsvbsfunctionality.h
  qgsvbscrashhandler.h
  qgsvbscoordinatedisplayer.h
  pinannotation/qgsvbspinannotationitem.h
  search/qgsvbssearchbox.h
  search/qgsvbssearchprovider.h
  search/qgsvbscoordinatesearchprovider.h
  search/qgsvbslocationsearchprovider.h
  search/qgsvbslocaldatasearchprovider.h
  search/qgsvbsremotedatasearchprovider.h
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
     ${GDAL_INCLUDE_DIR}
     ../../core
     ../../core/raster
     ../../core/geometry
     ../../gui
     ../
)

TARGET_LINK_LIBRARIES(vbsfunctionality
  qgis_core
  qgis_gui
  ${QJSON_LIBRARIES}
  SMTPEmail
)

########################################################
# Install

INSTALL(TARGETS vbsfunctionality
  RUNTIME DESTINATION ${QGIS_PLUGIN_DIR}
  LIBRARY DESTINATION ${QGIS_PLUGIN_DIR})
