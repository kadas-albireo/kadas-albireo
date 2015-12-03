########################################################
# Files

SET (IMAGE_RCCS images.qrc)
INSTALL ( FILES qgskadasstyle.css
         DESTINATION ${QGIS_DATA_DIR}/resources)

########################################################
# Build

QT4_ADD_RESOURCES(IMAGE_RCC_SRCS ${IMAGE_RCCS})

########################################################
# Continue on to subdirs
ADD_SUBDIRECTORY(icons)
ADD_SUBDIRECTORY(themes)
ADD_SUBDIRECTORY(svg)
