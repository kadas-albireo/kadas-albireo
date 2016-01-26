########################################################
# Files

SET(SPLASH_TRIAL_SRCS splash.cpp)

IF (WIN32)
  IF (MSVC)
    ADD_DEFINITIONS("-DSPLASH_EXPORT=__declspec(dllexport)")
  ELSE (MSVC)
    ADD_DEFINITIONS("-USPLASH_EXPORT \"-DSPLASH_EXPORT=__declspec(dllexport)\"")
  ENDIF (MSVC)
ENDIF (WIN32)

ADD_LIBRARY(qgis_splash_trial SHARED ${SPLASH_TRIAL_SRCS})
SET_TARGET_PROPERTIES(qgis_splash_trial PROPERTIES
                                     OUTPUT_NAME qgis_splash LIBRARY_OUTPUT_DIRECTORY ${QGIS_OUTPUT_DIRECTORY}/trial )

INSTALL(TARGETS qgis_splash_trial
  RUNTIME DESTINATION ${QGIS_BIN_DIR}/trial
  LIBRARY DESTINATION ${QGIS_LIB_DIR}/trial
  ARCHIVE DESTINATION ${QGIS_LIB_DIR}/trial
  )
