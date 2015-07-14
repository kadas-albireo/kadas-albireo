
########################################################
# Files

SET (coordinatedisplay_SRCS
  qgscoordinatedisplay.cpp
  qgscoordinateconverter.cpp
)

SET (coordinatedisplay_HDRS
  coordinatedisplay_plugin.h
  qgscoordinateconverter.h
)

SET (coordinatedisplay_MOC_HDRS
  qgscoordinatedisplay.h
)

SET (coordinatedisplay_UIS
)

SET (coordinatedisplay_RCCS
  coordinatedisplay.qrc)

########################################################
# Build

QT4_WRAP_UI (coordinatedisplay_UIS_H  ${coordinatedisplay_UIS})

QT4_WRAP_CPP (coordinatedisplay_MOC_SRCS  ${coordinatedisplay_MOC_HDRS})

QT4_ADD_RESOURCES(coordinatedisplay_RCC_SRCS ${coordinatedisplay_RCCS})

ADD_LIBRARY (coordinatedisplay   MODULE ${coordinatedisplay_SRCS} ${coordinatedisplay_HDRS} ${coordinatedisplay_MOC_SRCS} ${coordinatedisplay_RCC_SRCS} ${coordinatedisplay_UIS_H})

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

TARGET_LINK_LIBRARIES(coordinatedisplay
  qgis_core
  qgis_gui
)

########################################################
# Install

INSTALL(TARGETS coordinatedisplay
  RUNTIME DESTINATION ${QGIS_PLUGIN_DIR}
  LIBRARY DESTINATION ${QGIS_PLUGIN_DIR})

