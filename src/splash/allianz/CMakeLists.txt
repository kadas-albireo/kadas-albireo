########################################################
# Files

SET(SPLASH_ALLIANZ_SRCS splash.cpp)

IF (WIN32)
 IF (MSVC)
  ADD_DEFINITIONS("-DSPLASH_EXPORT=__declspec(dllexport)")
 ELSE (MSVC)
  ADD_DEFINITIONS("-USPLASH_EXPORT \"-DSPLASH_EXPORT=__declspec(dllexport)\"")
 ENDIF (MSVC)
ENDIF (WIN32)

ADD_LIBRARY(qgis_splash_allianz SHARED ${SPLASH_ALLIANZ_SRCS})
SET_TARGET_PROPERTIES(qgis_splash_allianz PROPERTIES
                                     OUTPUT_NAME qgis_splash LIBRARY_OUTPUT_DIRECTORY ${QGIS_OUTPUT_DIRECTORY}/allianz )

INSTALL(TARGETS qgis_splash_allianz
  RUNTIME DESTINATION ${QGIS_BIN_DIR}/allianz
  LIBRARY DESTINATION ${QGIS_LIB_DIR}/allianz
  ARCHIVE DESTINATION ${QGIS_LIB_DIR}/allianz
  )