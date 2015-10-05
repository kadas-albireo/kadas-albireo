########################################################
# Files

SET (crashplugin_MOC_HDRS crash_plugin.h)

SET (crashplugin_RCCS
  crash.qrc
)

########################################################
# Build

QT4_WRAP_CPP (crashplugin_MOC_SRCS  ${crashplugin_MOC_HDRS})

QT4_ADD_RESOURCES(crashplugin_RCC_SRCS ${crashplugin_RCCS})

ADD_LIBRARY (crashplugin   MODULE ${crashplugin_MOC_HDRS} ${crashplugin_MOC_SRCS} ${crashplugin_RCC_SRCS})

INCLUDE_DIRECTORIES(
     ${CMAKE_CURRENT_BINARY_DIR}
	 ../../core
     ../../gui
)

TARGET_LINK_LIBRARIES(crashplugin
  qgis_core
)

########################################################
# Install

INSTALL(TARGETS crashplugin
  RUNTIME DESTINATION ${QGIS_PLUGIN_DIR}
  LIBRARY DESTINATION ${QGIS_PLUGIN_DIR})
