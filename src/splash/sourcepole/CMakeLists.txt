########################################################
# Files

SET(SPLASH_SOURCEPOLE_SRCS splash.cpp)

IF (WIN32)
  IF (MSVC)
    ADD_DEFINITIONS("-DSPLASH_EXPORT=__declspec(dllexport)")
  ELSE (MSVC)
    ADD_DEFINITIONS("-USPLASH_EXPORT \"-DSPLASH_EXPORT=__declspec(dllexport)\"")
  ENDIF (MSVC)
ENDIF (WIN32)

ADD_LIBRARY(qgis_splash_sourcepole SHARED ${SPLASH_SOURCEPOLE_SRCS})
SET_TARGET_PROPERTIES(qgis_splash_sourcepole PROPERTIES
                                     OUTPUT_NAME qgis_splash LIBRARY_OUTPUT_DIRECTORY ${QGIS_OUTPUT_DIRECTORY}/sourcepole )

INSTALL(TARGETS qgis_splash_sourcepole
  RUNTIME DESTINATION ${QGIS_BIN_DIR}/sourcepole
  LIBRARY DESTINATION ${QGIS_LIB_DIR}/sourcepole
  ARCHIVE DESTINATION ${QGIS_LIB_DIR}/sourcepole
  )