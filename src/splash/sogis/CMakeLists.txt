########################################################
# Files

SET(SPLASH_SOGIS_SRCS splash.cpp)

IF (WIN32)
 IF (MSVC)
  ADD_DEFINITIONS("-DSPLASH_EXPORT=__declspec(dllexport)")
 ELSE (MSVC)
  ADD_DEFINITIONS("-USPLASH_EXPORT \"-DSPLASH_EXPORT=__declspec(dllexport)\"")
 ENDIF (MSVC)
ENDIF (WIN32)

ADD_LIBRARY(qgis_splash_sogis SHARED ${SPLASH_SOGIS_SRCS})
SET_TARGET_PROPERTIES(qgis_splash_sogis PROPERTIES
                                     OUTPUT_NAME qgis_splash LIBRARY_OUTPUT_DIRECTORY ${QGIS_OUTPUT_DIRECTORY}/sogis )

INSTALL(TARGETS qgis_splash_sogis
  RUNTIME DESTINATION ${QGIS_BIN_DIR}/sogis
  LIBRARY DESTINATION ${QGIS_LIB_DIR}/sogis 
  ARCHIVE DESTINATION ${QGIS_LIB_DIR}/sogis
  )