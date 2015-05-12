########################################################
# Files

SET(SPLASH_GEOGL_SRCS splash.cpp)

IF (WIN32)
  IF (MSVC)
    ADD_DEFINITIONS("-DSPLASH_EXPORT=__declspec(dllexport)")
  ELSE (MSVC)
    ADD_DEFINITIONS("-USPLASH_EXPORT \"-DSPLASH_EXPORT=__declspec(dllexport)\"")
  ENDIF (MSVC)
ENDIF (WIN32)

ADD_LIBRARY(qgis_splash_geogl SHARED ${SPLASH_GEOGL_SRCS})
SET_TARGET_PROPERTIES(qgis_splash_geogl PROPERTIES
                                     OUTPUT_NAME qgis_splash LIBRARY_OUTPUT_DIRECTORY ${QGIS_OUTPUT_DIRECTORY}/geogl )

INSTALL(TARGETS qgis_splash_geogl
  RUNTIME DESTINATION ${QGIS_BIN_DIR}/geogl
  LIBRARY DESTINATION ${QGIS_LIB_DIR}/geogl
  ARCHIVE DESTINATION ${QGIS_LIB_DIR}/geogl
  )