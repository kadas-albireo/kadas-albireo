########################################################
# Files

SET(SPLASH_GBD_SRCS splash.cpp)

IF (WIN32)
  IF (MSVC)
    ADD_DEFINITIONS("-DSPLASH_EXPORT=__declspec(dllexport)")
  ELSE (MSVC)
    ADD_DEFINITIONS("-USPLASH_EXPORT \"-DSPLASH_EXPORT=__declspec(dllexport)\"")
  ENDIF (MSVC)
ENDIF (WIN32)

ADD_LIBRARY(qgis_splash_gbd SHARED ${SPLASH_GBD_SRCS})
SET_TARGET_PROPERTIES(qgis_splash_gbd PROPERTIES
                                     OUTPUT_NAME qgis_splash LIBRARY_OUTPUT_DIRECTORY ${QGIS_OUTPUT_DIRECTORY}/gbd )

INSTALL(TARGETS qgis_splash_gbd
  RUNTIME DESTINATION ${QGIS_BIN_DIR}/gbd
  LIBRARY DESTINATION ${QGIS_LIB_DIR}/gbd
  ARCHIVE DESTINATION ${QGIS_LIB_DIR}/gbd
  )
