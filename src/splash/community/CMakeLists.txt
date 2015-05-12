########################################################
# Files

SET(SPLASH_COMMUNITY_SRCS splash.cpp)

IF (WIN32)
  IF (MSVC)
    ADD_DEFINITIONS("-DSPLASH_EXPORT=__declspec(dllexport)")
  ELSE (MSVC)
    ADD_DEFINITIONS("-USPLASH_EXPORT \"-DSPLASH_EXPORT=__declspec(dllexport)\"")
  ENDIF (MSVC)
ENDIF (WIN32)

ADD_LIBRARY(qgis_splash_community SHARED ${SPLASH_COMMUNITY_SRCS})
SET_TARGET_PROPERTIES(qgis_splash_community PROPERTIES
                                     OUTPUT_NAME qgis_splash LIBRARY_OUTPUT_DIRECTORY ${QGIS_OUTPUT_DIRECTORY}/community )

INSTALL(TARGETS qgis_splash_community
  RUNTIME DESTINATION ${QGIS_BIN_DIR}/community
  LIBRARY DESTINATION ${QGIS_LIB_DIR}/community
  ARCHIVE DESTINATION ${QGIS_LIB_DIR}/community
  )