## Once run this will define: 
## 
## GDAL_FOUND       = system has GDAL lib
##
## GDAL_LIBRARY     = full path to the library
##
## GDAL_INCLUDE_DIR      = where to find headers 
##
## Magnus Homann


IF(WIN32)

  IF (MINGW)
    FIND_PATH(GDAL_INCLUDE_DIR gdal.h /usr/local/include /usr/include c:/msys/local/include)
    FIND_LIBRARY(GDAL_LIBRARY NAMES gdal PATHS /usr/local/lib /usr/lib c:/msys/local/lib)
  ENDIF (MINGW)

  IF (MSVC)
    SET (
       GDAL_INCLUDE_DIR 
       "$ENV{LIB_DIR}/include/gdal"
       CACHE STRING INTERNAL
       )
    FIND_LIBRARY(GDAL_LIBRARY NAMES gdal gdal_i PATHS 
      "$ENV{LIB_DIR}/lib" /usr/lib c:/msys/local/lib)
    IF (GDAL_LIBRARY)
      SET (
         GDAL_LIBRARY;odbc32;odbccp32 
         CACHE STRING INTERNAL)
    ENDIF (GDAL_LIBRARY)
  ENDIF (MSVC)
  
  
ELSE(WIN32)
  IF(UNIX) 

    # try to use framework on mac
    IF (APPLE)
      SET (GDAL_MAC_PATH /Library/Frameworks/GDAL.framework/unix/bin)
    ENDIF (APPLE)

    SET(GDAL_CONFIG_PREFER_PATH "$ENV{GDAL_HOME}/bin" CACHE STRING "preferred path to GDAL (gdal-config)")
    SET(GDAL_CONFIG_PREFER_FWTOOLS_PATH "$ENV{FWTOOLS_HOME}/bin_safe" CACHE STRING "preferred path to GDAL (gdal-config) from FWTools")
    FIND_PROGRAM(GDAL_CONFIG gdal-config
      ${GDAL_CONFIG_PREFER_PATH}
      ${GDAL_CONFIG_PREFER_FWTOOLS_PATH}
      ${GDAL_MAC_PATH}
      /usr/local/bin/
      /usr/bin/
      )
    # MESSAGE("DBG GDAL_CONFIG ${GDAL_CONFIG}")
    
    IF (GDAL_CONFIG) 

      ## extract gdal version 
      EXEC_PROGRAM(${GDAL_CONFIG}
        ARGS --version
        OUTPUT_VARIABLE GDAL_VERSION )
      STRING(REGEX REPLACE "([0-9]+)\\.([0-9]+)\\.([0-9]+)" "\\1" GDAL_VERSION_MAJOR "${GDAL_VERSION}")
      STRING(REGEX REPLACE "([0-9]+)\\.([0-9]+)\\.([0-9]+)" "\\2" GDAL_VERSION_MINOR "${GDAL_VERSION}")
  
      # MESSAGE("DBG GDAL_VERSION ${GDAL_VERSION}")
      # MESSAGE("DBG GDAL_VERSION_MAJOR ${GDAL_VERSION_MAJOR}")
      # MESSAGE("DBG GDAL_VERSION_MINOR ${GDAL_VERSION_MINOR}")
  
      # check for gdal version
      # version 1.2.5 is known NOT to be supported (missing CPL_STDCALL macro)
      # According to INSTALL, 1.4.0+ is required
      IF (GDAL_VERSION_MAJOR LESS 1 OR GDAL_VERSION_MINOR LESS 4)
          MESSAGE (FATAL_ERROR "GDAL version is too old (${GDAL_VERSION}). Use 1.4.0 or higher.")
      ENDIF (GDAL_VERSION_MAJOR LESS 1 OR GDAL_VERSION_MINOR LESS 4)

      # set INCLUDE_DIR to prefix+include
      EXEC_PROGRAM(${GDAL_CONFIG}
        ARGS --prefix
        OUTPUT_VARIABLE GDAL_PREFIX)
      #SET(GDAL_INCLUDE_DIR ${GDAL_PREFIX}/include CACHE STRING INTERNAL)
      FIND_PATH(GDAL_INCLUDE_DIR 
        gdal.h 
        ${GDAL_PREFIX}/include/gdal
        ${GDAL_PREFIX}/include
        /usr/local/include 
        /usr/include 
        )

      ## extract link dirs for rpath  
      EXEC_PROGRAM(${GDAL_CONFIG}
        ARGS --libs
        OUTPUT_VARIABLE GDAL_CONFIG_LIBS )

      ## split off the link dirs (for rpath)
      ## use regular expression to match wildcard equivalent "-L*<endchar>"
      ## with <endchar> is a space or a semicolon
      STRING(REGEX MATCHALL "[-][L]([^ ;])+" 
        GDAL_LINK_DIRECTORIES_WITH_PREFIX 
        "${GDAL_CONFIG_LIBS}" )
        #      MESSAGE("DBG  GDAL_LINK_DIRECTORIES_WITH_PREFIX=${GDAL_LINK_DIRECTORIES_WITH_PREFIX}")

      ## remove prefix -L because we need the pure directory for LINK_DIRECTORIES
      
      IF (GDAL_LINK_DIRECTORIES_WITH_PREFIX)
        STRING(REGEX REPLACE "[-][L]" "" GDAL_LINK_DIRECTORIES ${GDAL_LINK_DIRECTORIES_WITH_PREFIX} )
      ENDIF (GDAL_LINK_DIRECTORIES_WITH_PREFIX)

      ## split off the name
      ## use regular expression to match wildcard equivalent "-l*<endchar>"
      ## with <endchar> is a space or a semicolon
      STRING(REGEX MATCHALL "[-][l]([^ ;])+" 
        GDAL_LIB_NAME_WITH_PREFIX 
        "${GDAL_CONFIG_LIBS}" )
        #      MESSAGE("DBG  GDAL_LIB_NAME_WITH_PREFIX=${GDAL_LIB_NAME_WITH_PREFIX}")


      ## remove prefix -l because we need the pure name
      
      IF (GDAL_LIB_NAME_WITH_PREFIX)
        STRING(REGEX REPLACE "[-][l]" "" GDAL_LIB_NAME ${GDAL_LIB_NAME_WITH_PREFIX} )
      ENDIF (GDAL_LIB_NAME_WITH_PREFIX)

      IF (APPLE)
        SET(GDAL_LIBRARY ${GDAL_LINK_DIRECTORIES}/lib${GDAL_LIB_NAME}.dylib CACHE STRING INTERNAL)
      ELSE (APPLE)
       SET(GDAL_LIBRARY ${GDAL_LINK_DIRECTORIES}/lib${GDAL_LIB_NAME}.so CACHE STRING INTERNAL)
      ENDIF (APPLE)
      
    ELSE(GDAL_CONFIG)
      MESSAGE("FindGDAL.cmake: gdal-config not found. Please set it manually. GDAL_CONFIG=${GDAL_CONFIG}")
    ENDIF(GDAL_CONFIG)

  ENDIF(UNIX)
ENDIF(WIN32)


IF (GDAL_INCLUDE_DIR AND GDAL_LIBRARY)
   SET(GDAL_FOUND TRUE)
ENDIF (GDAL_INCLUDE_DIR AND GDAL_LIBRARY)

IF (GDAL_FOUND)

   IF (NOT GDAL_FIND_QUIETLY)
      MESSAGE(STATUS "Found GDAL: ${GDAL_LIBRARY}")
   ENDIF (NOT GDAL_FIND_QUIETLY)

ELSE (GDAL_FOUND)

   MESSAGE(GDAL_INCLUDE_DIR=${GDAL_INCLUDE_DIR})
   MESSAGE(GDAL_LIBRARY=${GDAL_LIBRARY})
   MESSAGE(FATAL_ERROR "Could not find GDAL")

ENDIF (GDAL_FOUND)
