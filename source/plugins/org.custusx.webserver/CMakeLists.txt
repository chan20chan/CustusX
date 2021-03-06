project(org_custusx_webserver)

find_package(Qt5Network REQUIRED)
find_package(qhttpserver REQUIRED)
#message(STATUS "qhttpserver: " ${qhttpserver_DIR} ", " ${qhttpserver_FOUND})

set(PLUGIN_export_directive "${PROJECT_NAME}_EXPORT")

set(PLUGIN_SRCS
  cxWebServerPluginActivator.cpp
  cxScreenVideoProvider.cpp
  cxHttpRequestHandler.cpp
  cxRemoteAPI.cpp
  cxLayoutVideoSource.cpp
  cxWebServerGUIExtenderService.h
  cxWebServerGUIExtenderService.cpp
  cxWebServerWidget.h
  cxWebServerWidget.cpp
)

# Files which should be processed by Qts moc
set(PLUGIN_MOC_SRCS
  cxWebServerPluginActivator.h
  cxScreenVideoProvider.h
  cxHttpRequestHandler.h
  cxRemoteAPI.h
  cxLayoutVideoSource.h
)

# Qt Designer files which should be processed by Qts uic
set(PLUGIN_UI_FORMS
)

# QRC Files which should be compiled into the plugin
set(PLUGIN_resources
)


#Compute the plugin dependencies
ctkFunctionGetTargetLibraries(PLUGIN_target_libraries)
set(PLUGIN_target_libraries 
    ${PLUGIN_target_libraries}
    cxGUIExtenderService
    cxPluginUtilities
	cxResource
	cxResourceVisualization
	cxResourceWidgets
	qhttpserver
    )

set(PLUGIN_OUTPUT_DIR "")
if(CX_WINDOWS)
    #on windows we want dlls to be placed with the executables
    set(PLUGIN_OUTPUT_DIR "../")
endif(CX_WINDOWS)

ctkMacroBuildPlugin(
  NAME ${PROJECT_NAME}
  EXPORT_DIRECTIVE ${PLUGIN_export_directive}
  SRCS ${PLUGIN_SRCS}
  MOC_SRCS ${PLUGIN_MOC_SRCS}
  UI_FORMS ${PLUGIN_UI_FORMS}
  RESOURCES ${PLUGIN_resources}
  TARGET_LIBRARIES ${PLUGIN_target_libraries}
  OUTPUT_DIR ${PLUGIN_OUTPUT_DIR}
  ${CX_CTK_PLUGIN_NO_INSTALL}
)

target_include_directories(org_custusx_webserver
    PUBLIC
    .
    ${CMAKE_CURRENT_BINARY_DIR}
)


add_subdirectory(testing)

