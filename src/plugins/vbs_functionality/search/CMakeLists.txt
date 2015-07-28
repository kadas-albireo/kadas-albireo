
########################################################
# Files

SET (vbssearch_SRCS
  qgsvbssearch.cpp
  qgsvbssearchbox.cpp
  providers/qgsvbscoordinatesearchprovider.cpp
)

SET (vbssearch_HDRS
  vbssearch_plugin.h
  providers/qgsvbssearchprovider.h
  providers/qgsvbscoordinatesearchprovider.h
)

SET (vbssearch_MOC_HDRS
  qgsvbssearch.h
  qgsvbssearchbox.h
)

SET (vbssearch_UIS
)

SET (vbssearch_RCCS
  vbssearch.qrc)

########################################################
# Build

QT4_WRAP_UI (vbssearch_UIS_H  ${vbssearch_UIS})

QT4_WRAP_CPP (vbssearch_MOC_SRCS  ${vbssearch_MOC_HDRS})

QT4_ADD_RESOURCES(vbssearch_RCC_SRCS ${vbssearch_RCCS})

ADD_LIBRARY (vbssearchplugin   MODULE ${vbssearch_SRCS} ${vbssearch_HDRS} ${vbssearch_MOC_SRCS} ${vbssearch_RCC_SRCS} ${vbssearch_UIS_H})

INCLUDE_DIRECTORIES(
     ${CMAKE_CURRENT_BINARY_DIR}
     ${CMAKE_BINARY_DIR}/src/ui
     ${GEOS_INCLUDE_DIR}
     ${GDAL_INCLUDE_DIR}
     ../../core
     ../../core/symbology-ng
     ../../gui
     ..
)

TARGET_LINK_LIBRARIES(vbssearchplugin
  qgis_core
  qgis_gui
)

########################################################
# Install

INSTALL(TARGETS vbssearchplugin
  RUNTIME DESTINATION ${QGIS_PLUGIN_DIR}
  LIBRARY DESTINATION ${QGIS_PLUGIN_DIR})

