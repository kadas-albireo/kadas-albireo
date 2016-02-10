SET (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${QGIS_OUTPUT_DIRECTORY}/${QGIS_BIN_SUBDIR})

set(milixserver_SRC VBSMilixServer.cpp)
set(milixserver_HDR VBSMilixCommands.hpp)
set(milixserver_MOC_HDRS VBSMilixServer.hpp)
QT4_WRAP_CPP(milixserver_MOC_SRCS  ${milixserver_MOC_HDRS})


add_executable(milixserver ${milixserver_SRC} ${milixserver_HDR} ${milixserver_MOC_HDRS} ${milixserver_MOC_SRCS})
set_target_properties(milixserver PROPERTIES COMPILE_FLAGS "/Zc:wchar_t- /MD")

install(TARGETS milixserver
  BUNDLE DESTINATION ${QGIS_BIN_DIR}
  RUNTIME DESTINATION ${QGIS_BIN_DIR}
)

target_link_libraries(milixserver
  ${QT_QTCORE_LIBRARY}
  ${QT_QTGUI_LIBRARY}
  ${QT_QTNETWORK_LIBRARY}
  ${QT_QTSVG_LIBRARY}
)
