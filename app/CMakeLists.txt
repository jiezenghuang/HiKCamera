cmake_minimum_required (VERSION 3.12)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} 
	PRIVATE_SOURCES)

add_executable(CameraPluginConsole
    ${PRIVATE_SOURCES})
    
target_include_directories(CameraPluginConsole
	PUBLIC
	"$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>"
	"$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
    PRIVATE    
    ${OpenCV_INCLUDE_DIRS}
    )

target_link_libraries(CameraPluginConsole
	PRIVATE
    fal::device
    Boost::system
    ${OpenCV_LIBS})