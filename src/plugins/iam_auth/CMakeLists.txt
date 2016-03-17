########################################################
# Files

SET (iamauth_SRCS
  qgsiamauth.cpp
)

SET (iamauth_HDRS
  iamauth_plugin.h
)

SET (iamauth_MOC_HDRS
  qgsiamauth.h
)

SET (iamauth_UIS
)

SET (iamauth_RCCS
  qgsiamauth.qrc
)

########################################################
# Build

QT4_WRAP_UI (iamauth_UIS_H  ${iamauth_UIS})

QT4_WRAP_CPP (iamauth_MOC_SRCS  ${iamauth_MOC_HDRS})

QT4_ADD_RESOURCES(iamauth_RCC_SRCS ${iamauth_RCCS})

ADD_LIBRARY (iamauth   SHARED ${iamauth_SRCS} ${iamauth_HDRS} ${iamauth_MOC_SRCS} ${iamauth_RCC_SRCS} ${iamauth_UIS_H})

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
    ADD_DEFINITIONS("\"-DVBSIAM_EXPORT=${DLLEXPORT}\"")
ELSE(WIN32)
    ADD_DEFINITIONS("-DVBSIAM_EXPORT=")
ENDIF(WIN32)

TARGET_LINK_LIBRARIES(iamauth
  qgis_core
  qgis_gui
  qgis_analysis
)

########################################################
# Install

INSTALL(TARGETS iamauth
  RUNTIME DESTINATION ${QGIS_PLUGIN_DIR}
  LIBRARY DESTINATION ${QGIS_PLUGIN_DIR})
